//
// Created by Yunus Emre Ozturk on 3.08.2025.
//

#ifndef EZCACHE_HPP
#define EZCACHE_HPP
#include <any>
#include <list>
#include <map>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <iostream>
#include <optional>
#include <shared_mutex>
#include <type_traits>
#include <atomic>
#include <chrono>
#include <mutex>

#include "internal.hpp"

namespace ezcache::stats{
    struct Empty {};

    template<bool Enable>
    struct CounterBase : std::conditional_t<Enable, std::atomic<uint64_t>, Empty> { //EBO
        void inc() {
            if constexpr (Enable) this->fetch_add(1, std::memory_order_relaxed);
        }
        uint64_t get() const {
            if constexpr (Enable) return this->load(std::memory_order_relaxed);
            else return 0;
        }
    };
}

namespace ezcache {

template <std::size_t MaxSize = 2048, bool EnableStats = false>
class EZCache {

public:
    EZCache() {
        _lru.reserve(MaxSize);
    }

    template<typename Func, typename... Args>
    auto operator()(Func&& func, Args&&... args)
            -> std::decay_t<std::invoke_result_t<Func, Args...>> {
        return _memorize<double, std::ratio<1>>(std::nullopt, std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template<typename Rep, typename Period, typename Func, typename... Args>
    auto operator()(std::chrono::duration<Rep, Period> ttl, Func&& func, Args&&... args)
            -> std::decay_t<std::invoke_result_t<Func, Args...>> {
        return _memorize<Rep, Period>(ttl, std::forward<Func>(func), std::forward<Args>(args)...);
    }

    uint64_t get_hit_count() const {
        return _hit_counter.get();
    }

    uint64_t get_miss_count() const {
        return _miss_counter.get();
    }

    uint64_t get_collision_count() const {
        return _collision_counter.get();
    }

    double get_hit_rate() const {
        uint64_t hits = get_hit_count();
        uint64_t total = hits + get_miss_count();
        return total ? static_cast<double>(hits) / total : 0.0;
    }

private:
    using Hash = std::size_t;
    using Value = std::any;
    using ReturnType = std::type_index;
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    static constexpr double LRU_CLEAR_RATE = 0.3;

    struct Entry {
        Value value{};
        ReturnType return_type = typeid(void);
        Hash hash{};
        std::atomic<uint64_t> last_use{};
        std::optional<TimePoint> expire_time{};
        std::size_t lru_index{};
        std::optional<std::multimap<TimePoint, Hash>::iterator> expire_time_iter{};
    };

    template<typename Rep = double, typename Period = std::ratio<1>, typename Func, typename... Args>
    auto _memorize(std::optional<std::chrono::duration<Rep, Period>> ttl, Func&& func, Args&&... args)
            -> std::decay_t<std::invoke_result_t<Func, Args...>> {
        using ReturnT = std::invoke_result_t<Func, Args...>;
        const Hash key = ::ezcache::internal::make_cache_key(func, args...);
        auto return_val_opt = _get_from_cache<ReturnT>(key);
        if (return_val_opt.has_value()) {
            _hit_counter.inc();
            return std::move(*return_val_opt);
        }

        _miss_counter.inc();
        ReturnT result = std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        _put_in_cache<ReturnT, std::chrono::duration<Rep, Period>>(key, result, ttl);
        return result;
    }

    template<typename ReturnType, typename Duration>
    void _put_in_cache(const Hash key, const Value& value, std::optional<Duration> ttl = std::nullopt) {
        std::unique_lock<std::shared_mutex> lock(_mutex);
        _clear_entry(key);
        auto [it, inserted] = _cache.try_emplace(key);
        auto& entry = it->second;
        entry.value = value;
        entry.hash = key;
        entry.lru_index = _lru.size();
        entry.return_type = std::type_index(typeid(ReturnType));
        _lru.push_back(key);
        if (ttl.has_value()) {
            auto ttl_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(*ttl);
            entry.expire_time = Clock::now() + ttl_ns;
            entry.expire_time_iter = _expire_times.insert({*entry.expire_time, key});
        }
        entry.last_use.store(_epoch.fetch_add(1, std::memory_order_relaxed), std::memory_order_relaxed);
        _clear_if_needed();
    }

    template<typename ReturnT>
    std::optional<ReturnT> _get_from_cache(const Hash key) {
        std::shared_lock<std::shared_mutex> lock(_mutex);
        const auto entry_it = _cache.find(key);
        if (entry_it == _cache.end()) {
            return std::nullopt;
        }
        auto& entry = entry_it->second;
        if (entry.return_type != std::type_index(typeid(ReturnT))) {
            /* HASH COLLISION */
            _collision_counter.inc();
            return std::nullopt;
        }

        entry.last_use.store(_epoch.fetch_add(1, std::memory_order_relaxed), std::memory_order_relaxed);
        return std::any_cast<ReturnT>(entry.value);
    }

    void _clear_entry(const Hash key) {
        auto entry_it = _cache.find(key);
        if (entry_it == _cache.end()) {
            return;
        }
        const auto& entry = entry_it->second;
        if (!_lru.empty()) {
            if (entry.hash != _lru.back()) {
                _cache.at(_lru.back()).lru_index = entry.lru_index;
                std::swap(_lru[entry.lru_index], _lru.back());
            }
            _lru.pop_back();
        }

        if (entry.expire_time_iter.has_value()) {
            _expire_times.erase(entry.expire_time_iter.value());
        }
        _cache.erase(entry_it);
    }

    void _clear_using_lru() {
        if (_lru.empty()) {
            return;
        }
        std::vector<Hash> temp_lru{_lru};
        size_t element_count_to_drop = temp_lru.size() * LRU_CLEAR_RATE;
        if (element_count_to_drop == 0) {
            return;
        }
        const size_t element_count_to_preserve = temp_lru.size() - element_count_to_drop;
        std::nth_element(temp_lru.begin(), temp_lru.begin() + element_count_to_preserve, temp_lru.end(),
                [this](const auto& lhs, const auto& rhs) {
                    return (this->_cache.at(lhs).last_use.load(std::memory_order_relaxed) >
                            this->_cache.at(rhs).last_use.load(std::memory_order_relaxed) );
                });
        constexpr size_t MAX_BATCH = 1024;
        const size_t todo = std::min(element_count_to_drop, MAX_BATCH);
        for (size_t idx = 0; idx < todo; idx++) {
            const auto key = temp_lru.back();
            _clear_entry(key);
            temp_lru.pop_back();
        }
    }

    void _clear_using_expire_times() {
        const TimePoint now = Clock::now();
        while (!_expire_times.empty() && _expire_times.begin()->first <= now) {
            auto time_it = _expire_times.begin();
            const auto key = time_it->second;
            auto entry_it = _cache.find(key);
            if (entry_it == _cache.end()) {
                _expire_times.erase(time_it);
                continue;
            }

            const auto& entry = entry_it->second;
            if (entry.expire_time_iter.has_value()
                    && entry.expire_time_iter.value() == time_it) {
                _clear_entry(key);
            } else {
                _expire_times.erase(time_it);
            }
        }
    }

    void _clear_if_needed() {
        if (!_expire_times.empty()) {
            _clear_using_expire_times();
        }

        if (_cache.size() >= MaxSize) {
            _clear_using_lru();
        }
    }

    std::unordered_map<Hash, Entry> _cache;
    std::vector<Hash> _lru;
    std::multimap<TimePoint, Hash> _expire_times;
    std::shared_mutex _mutex;
    std::atomic<uint64_t> _epoch{0};
    [[no_unique_address]] stats::CounterBase<EnableStats> _collision_counter{};
    [[no_unique_address]] stats::CounterBase<EnableStats> _hit_counter{};
    [[no_unique_address]] stats::CounterBase<EnableStats> _miss_counter{};
};

}

#endif //EZCACHE_HPP
