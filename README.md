# EZCache

A small, header-only cache library for C++20.  
Designed to be simple, fast enough for most uses, and dependency-free — just include it and go.

---

## Features

- **Header-only** — just `#include "EZCache.hpp"`.
- **Function memorization** with optional TTL (time-to-live).
- **LRU eviction** when the cache reaches its maximum size.
- **Type-safe** — detects hash collisions with different return types.
- **Optional statistics** (hits, misses, collisions, hit rate).
- **Thread-safe** — for reads and writes.

---

## Basic Usage

```cpp
#include "EZCache.hpp"
#include <iostream>

using namespace ezcache;

EZCache<128, true> cache; // max 128 entries, stats enabled

int double_value(int x) {
    return x * 2;
}

int main() {
    // First call → cache miss
    std::cout << cache(double_value, 21) << "\n"; // prints 42

    // Second call with same args → cache hit
    std::cout << cache(double_value, 21) << "\n"; // still 42

    // With TTL of 500 ms
    std::cout << cache(std::chrono::milliseconds(500), double_value, 10) << "\n";

    // Stats
    std::cout << "Hits: " << cache.get_hit_count() << "\n";
    std::cout << "Misses: " << cache.get_miss_count() << "\n";
    std::cout << "Hit rate: " << cache.get_hit_rate() * 100 << "%\n";
}
```

---

## Build & Run

### Benchmark
```bash
cmake -S . -B build -DEZCACHE_BUILD_BENCHMARK=ON
cmake --build build
./build/benchmark
```

### Unit Tests
```bash
cmake -S . -B build -DEZCACHE_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## Template Parameters

```cpp
EZCache<MaxSize, EnableStats>
```
- **MaxSize** — maximum number of entries in the cache (LRU eviction policy).
- **EnableStats** — `true` to track hits, misses, collisions; `false` to remove overhead.

---

## Contributing

Contributions are welcome.

- Open a pull request to the `main` branch.
- Keep your changes in a single commit (use `git rebase -i` if needed).
- Your PR will be reviewed and, if approved, merged via rebase.

---

## License


[MIT License](LICENSE)
