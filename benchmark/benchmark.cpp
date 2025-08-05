#include "EZCache.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <vector>
#include <random>

using namespace std;
using namespace ezcache;

EZCache<4> cache;

void benchmark_cold() {
    int expected = 84;
    auto start = chrono::high_resolution_clock::now();
    int result = cache([](int x) { return x * 2; }, 42);
    auto end = chrono::high_resolution_clock::now();
    assert(result == expected);
    cout << "Cold (miss) time: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << " ns\n";
}

void benchmark_hot() {
    int expected = 84;
    auto start = chrono::high_resolution_clock::now();
    int result = cache([](int x) { return x * 2; }, 42);
    auto end = chrono::high_resolution_clock::now();
    assert(result == expected);
    cout << "Hot  (hit)  time: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << " ns\n";
}

void benchmark_expire() {
    int expected = 21;
    cache(chrono::milliseconds(100), [](int x) { return x * 3; }, 7);
    this_thread::sleep_for(chrono::milliseconds(150));
    auto start = chrono::high_resolution_clock::now();
    int result = cache(chrono::milliseconds(100), [](int x) { return x * 3; }, 7);
    auto end = chrono::high_resolution_clock::now();
    assert(result == expected);
    cout << "Expire + miss time: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << " ns\n";
}

void benchmark_lru() {
    int expected = 2;
    cache([](int x) { return x + 1; }, 1);
    cache([](int x) { return x + 2; }, 2);
    cache([](int x) { return x + 3; }, 3);
    cache([](int x) { return x + 4; }, 4);
    cache([](int x) { return x + 5; }, 5);  // evicts the oldest
    auto start = chrono::high_resolution_clock::now();
    int result = cache([](int x) { return x + 1; }, 1); // should re-compute
    auto end = chrono::high_resolution_clock::now();
    assert(result == expected);
    cout << "LRU reinsert time: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << " ns\n";
}

void benchmark_type_mismatch() {
    double expected = 198.0;
    cache([](int x) { return x * 2; }, 99);
    auto start = chrono::high_resolution_clock::now();
    double result = cache([](int x) { return static_cast<double>(x * 2); }, 99);
    auto end = chrono::high_resolution_clock::now();
    assert(result == expected);
    cout << "Type mismatch time: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << " ns\n";
}

void benchmark_hash_collision() {
    int result1 = cache([](int x) { return x + 1; }, 10);
    double result2 = cache([](int x) { return static_cast<double>(x + 1); }, 10);
    assert(result1 == 11);
    assert(result2 == 11.0);
    cout << "Hash collision + type safety test passed.\n";
}

int main() {
    benchmark_cold();
    benchmark_hot();
    benchmark_expire();
    benchmark_lru();
    benchmark_type_mismatch();
    benchmark_hash_collision();
    cout << "All correctness tests passed.\n";
    return 0;
}
