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
    ezcache::EZCache<4, true> cache;
    auto f = [](int x){ return x * 2; };

    cache(f, 1); // miss
    cache(f, 2); // miss (cache: {2,1})
    cache(f, 3); // miss
    // LRU: [(f,3), (f,2), (f,1)]

    EXPECT_EQ(cache.get_miss_count(), 3u);
    EXPECT_EQ(cache(f, 1), 2); // hit (LRU order update: {1,2})
    // LRU: [(f,1), (f,3), (f,2)]

    EXPECT_EQ(cache.get_hit_count(), 1u);
    cache(f, 4); // miss
    // LRU: [(f,4), (f,1), (f,3)]
    EXPECT_EQ(cache.get_miss_count(), 4u);

    EXPECT_EQ(cache(f, 2), 4); // miss again, 2 evicted
    // LRU: [(f,2), (f,4), (f,1)]
    EXPECT_EQ(cache.get_miss_count(), 5u);

    EXPECT_EQ(cache(f, 1), 2); // hit
    // LRU: [(f,1), (f,2), (f,4)]
    cache(f, 3); // miss
    EXPECT_EQ(cache.get_miss_count(), 6u);
    EXPECT_EQ(cache.get_hit_count(), 2u);
}

TEST(EZCache, EvictionLRUInLoop) {
    ezcache::EZCache<101, true> cache;
    auto f = [](int x){ return x * 2; };
    for (auto i = 0; i < 100; i++) {
        cache(f, i);
    }
    EXPECT_EQ(cache.get_miss_count(), 100u);

    for (auto i = 0; i < 40; i++) {
        cache(f, i);
    }
    EXPECT_EQ(cache.get_miss_count(), 100u);
    EXPECT_EQ(cache.get_hit_count(), 40u);

    for (auto i = 100; i < 120; i++) { //evicts 30 entry [40,70)
        cache(f, i);
    }
    EXPECT_EQ(cache.get_miss_count(), 120u);
    EXPECT_EQ(cache.get_hit_count(), 40u);
    // (120, 100] + (40, 0] + [100, 70)

    for (auto i = 0; i < 40; i++) {
        cache(f, i);
    }
    EXPECT_EQ(cache.get_miss_count(), 120u);
    EXPECT_EQ(cache.get_hit_count(), 80u);

    for (auto i = 70; i < 120; i++) {
        cache(f, i);
    }
    EXPECT_EQ(cache.get_miss_count(), 120u);
    EXPECT_EQ(cache.get_hit_count(), 130u);

    for (auto i = 40; i < 70; i++) {
        cache(f, i);
    }
    EXPECT_EQ(cache.get_miss_count(), 150u);
    EXPECT_EQ(cache.get_hit_count(), 130u);
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
