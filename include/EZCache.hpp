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
#include "internal.hpp"

namespace ezcache {


class EZCache {
public:
    template<typename Func, typename... Args>
    decltype(auto) operator()(Func&& func, Args&&... args) {
        using ReturnT = std::invoke_result_t<Func, Args...>;
        const Hash key = ::ezcache::internal::make_cache_key(func, args...);

        auto cache_it = _cache.find(key);
        if (cache_it == _cache.end()) {
            ReturnT result = std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
            _put_in_cache<ReturnT>(key, result);
            return result;
        } else if (cache_it->second.return_type != std::type_index(typeid(ReturnT))) {
            return std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        }
        else {
            return _get_from_cache<ReturnT>(key);
        }
    }


private:
    using Hash = std::size_t;
    using Value = std::any;
    using ReturnType = std::type_index;
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    struct Entry {
        Value value;
        ReturnType return_type;
        std::optional<TimePoint> expire_time;
        std::list<Hash>::iterator lru_iter;
        std::optional<std::multimap<TimePoint, Hash>::iterator> expire_time_iter;
    };

    template<typename ReturnType, typename Duration>
    void _put_in_cache(const Hash key, const Value& value, std::optional<Duration> ttl = std::nullopt) {
        Entry entry{.value = value, .return_type = std::type_index(typeid(ReturnType))};
        _lru.push_front(key);
        entry.lru_iter = _lru.begin();
        if (ttl.has_value()) {
            auto ttl_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(*ttl);
            entry.expire_time = Clock::now() + ttl_ns;
            entry.expire_time_iter = _expire_times.insert({*entry.expire_time, key});
        }
        _cache[key] = entry;
    }

    template<typename ReturnT>
    ReturnT _get_from_cache(const Hash key) {
        const auto entry_it = _cache.find(key);
        _lru.splice(_lru.begin(), _lru, entry_it->second.lru_iter);
        entry_it->second.lru_iter = _lru.begin();
        return std::any_cast<ReturnT>(entry_it->second.value);
    }

    std::unordered_map<Hash, Entry> _cache;
    std::list<Hash> _lru;
    std::multimap<TimePoint, Hash> _expire_times;
};

}

#endif //EZCACHE_HPP
