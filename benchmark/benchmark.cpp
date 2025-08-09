#include "EZCache.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <vector>
#include <random>

using namespace std;
using namespace ezcache;

EZCache<4, true> cache;

auto common_lambda = [](int x) {this_thread::sleep_for(chrono::microseconds(500)); return x * 2; };
auto double_common_lambda = [](int x) { this_thread::sleep_for(chrono::microseconds(500)); return static_cast<double>(x * 2); };

void benchmark_cold() {
    int expected = 84;
    auto start = chrono::high_resolution_clock::now();
    int result = cache(common_lambda, 42);
    auto end = chrono::high_resolution_clock::now();
    assert(result == expected);
    cout << "Cold (miss) time: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << " ns\n";
}

void benchmark_hot() {
    int expected = 84;
    auto start = chrono::high_resolution_clock::now();
    int result = cache(common_lambda, 42);
    auto end = chrono::high_resolution_clock::now();
    assert(result == expected);
    cout << "Hot  (hit)  time: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << " ns\n";
}

void benchmark_expire() {
    int expected = 14;
    cache(100ms, double_common_lambda, 7);
    this_thread::sleep_for(chrono::milliseconds(150));
    auto start = chrono::high_resolution_clock::now();
    int result = cache(100ms, double_common_lambda, 7);
    auto end = chrono::high_resolution_clock::now();
    assert(result == expected);
    cout << "Expire + miss time: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << " ns\n";
}

void benchmark_lru() {
    int expected = 2;
    cache(common_lambda, 1);
    cache(common_lambda, 2);
    cache(common_lambda, 3);
    cache(common_lambda, 4);
    cache(common_lambda, 5);  // evicts the oldest
    auto start = chrono::high_resolution_clock::now();
    int result = cache(common_lambda, 1); // should re-compute
    auto end = chrono::high_resolution_clock::now();
    assert(result == expected);
    cout << "LRU reinsert time: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << " ns\n";
}

void benchmark_type_mismatch() {
    double expected = 198.0;
    cache(common_lambda, 99);
    auto start = chrono::high_resolution_clock::now();
    double result = cache(double_common_lambda, 99);
    auto end = chrono::high_resolution_clock::now();
    assert(result == expected);
    cout << "Type mismatch time: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << " ns\n";
}

void benchmark_hash_collision() {
    int result1 = cache(common_lambda, 10);
    double result2 = cache(double_common_lambda, 10);
    assert(result1 == 20);
    assert(result2 == 20.0);
    cout << "Hash collision + type safety test passed.\n";
}

void benchmark_hot_loop(std::size_t N) {
    using clock = std::chrono::high_resolution_clock;
    (void)cache(common_lambda, 42);

    uint64_t total = 0, minv = std::numeric_limits<uint64_t>::max();
    volatile int sink = 0;

    for (std::size_t i = 0; i < N; ++i) {
        auto t0 = clock::now();
        int r = cache(common_lambda, 42); // hit
        auto t1 = clock::now();
        sink ^= r;

        uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        total += ns;
        if (ns < minv) minv = ns;
    }

    std::cout << "Hot  loop: N=" << N
              << ", avg=" << (total / N) << " ns"
              << ", min=" << minv << " ns\n";
}

void benchmark_cold_loop(std::size_t N) {
    using clock = std::chrono::high_resolution_clock;

    uint64_t total = 0, minv = std::numeric_limits<uint64_t>::max();
    volatile int sink = 0;

    for (std::size_t i = 0; i < N; ++i) {
        // her seferinde farklı key -> zorunlu miss
        int key = static_cast<int>(1000000 + i);

        auto t0 = clock::now();
        int r = cache(common_lambda, key); // miss + insert (ve gerekirse eviction)
        auto t1 = clock::now();
        sink ^= r;

        uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        total += ns;
        if (ns < minv) minv = ns;
    }

    std::cout << "Cold loop: N=" << N
              << ", avg=" << (total / N) << " ns"
              << ", min=" << minv << " ns\n";
}

int main() {
    benchmark_hot();
    benchmark_cold();
    benchmark_expire();
    benchmark_lru();
    benchmark_type_mismatch();
    benchmark_hash_collision();
    benchmark_hot_loop(100);
    benchmark_cold_loop(100);
    cout << "All correctness tests passed.\n";

    std::cout << "Hits: " << cache.get_hit_count() << "\n";
    std::cout << "Misses: " << cache.get_miss_count() << "\n";
    std::cout << "Collisions: " << cache.get_collision_count() << "\n";
    std::cout << "Hit Rate: " << cache.get_hit_rate() * 100 << "%\n";
    return 0;
}
