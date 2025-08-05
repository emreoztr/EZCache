//
// Created by Yunus Emre Ozturk on 3.08.2025.
//

#ifndef INTERNAL_HPP
#include <cstddef>
#include <functional>
#include <typeindex>

namespace ezcache::internal {

constexpr inline std::uint32_t golden_ratio_32 = 0x9e3779b9;

template <typename T>
void hash_combine(std::size_t& seed, const T& val) {
    seed ^= std::hash<T>{}(val) + golden_ratio_32 + (seed << 6) + (seed >> 2);
}

template<typename... Args>
std::size_t hash_args(const Args&... args) {
    std::size_t seed = 0;
    (hash_combine(seed, args), ...);
    return seed;
}

template<typename Func>
std::size_t hash_func_identity(const Func& f) {
    return typeid(f).hash_code();
}

template<typename Func, typename... Args>
std::size_t make_cache_key(const Func& func, const Args&... args) {
    using ReturnT = std::invoke_result_t<Func, Args...>;

    std::size_t seed = hash_func_identity(func);
    const std::size_t args_hash = hash_args(args...);
    const std::size_t type_hash = std::type_index(typeid(ReturnT)).hash_code();

    // Hash combine
    seed ^= args_hash + golden_ratio_32 + (seed << 6) + (seed >> 2);
    seed ^= type_hash + golden_ratio_32 + (seed << 6) + (seed >> 2);
    return seed;
}
}

#define INTERNAL_HPP

#endif //INTERNAL_HPP
