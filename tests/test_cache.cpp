#include <gtest/gtest.h>
#include "EZCache.hpp"
#include <chrono>
#include <thread>

TEST(EZCache, HitMissCounters) {
    ezcache::EZCache<4, true> cache;
    auto f = [](int x){ return x * 2; };

    EXPECT_EQ(cache(f, 21), 42); // miss
    EXPECT_EQ(cache(f, 21), 42); // hit

    EXPECT_EQ(cache.get_miss_count(), 1u);
    EXPECT_EQ(cache.get_hit_count(), 1u);
}

TEST(EZCache, EvictionLRU) {
    ezcache::EZCache<3, true> cache;
    auto f = [](int x){ return x * 2; };

    cache(f, 1); // miss
    cache(f, 2); // miss (cache: {2,1})
    // LRU: [(f,2), (f,1)]

    EXPECT_EQ(cache.get_miss_count(), 2u);
    EXPECT_EQ(cache(f, 1), 2); // hit (LRU order update: {1,2})
    // LRU: [(f,1), (f,2)]

    EXPECT_EQ(cache.get_hit_count(), 1u);
    cache(f, 3); // miss + evict 2 (cache: {3,1})
    // LRU: [(f,3), (f,1)]
    EXPECT_EQ(cache.get_miss_count(), 3u);

    EXPECT_EQ(cache(f, 2), 4); // miss again, 2 evicted
    // LRU: [(f,2), (f,3)]
    EXPECT_EQ(cache.get_miss_count(), 4u);

    EXPECT_EQ(cache(f, 1), 2); // miss again, 1 evicted
    // LRU: [(f,1), (f,2)]
    EXPECT_EQ(cache.get_miss_count(), 5u);
    EXPECT_EQ(cache.get_hit_count(), 1u);
}


TEST(EZCache, Expiration) {
    using namespace std;
    ezcache::EZCache<3, true> cache;
    auto f = [](int x){ return x * 2; };

    cache(f, 2);
    cache(100ms, f, 1);
    // LRU: [(f,1), (f,2)]
    this_thread::sleep_for(chrono::milliseconds(110));
    cache(f, 3); // miss + evict 1 from expiration
    // LRU: [(f,3), (f,2)]
    EXPECT_EQ(cache.get_miss_count(), 3u);

    cache(f,2);
    EXPECT_EQ(cache.get_hit_count(), 1u);
}
