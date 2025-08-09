Header only simple cache library for C++

Benchmark Build: <br>
cmake -S . -B build -DEZCACHE_BUILD_BENCHMARK=ON <br>
cmake --build build <br>
./build/benchmark

Unit Test Build: <br>
cmake -S . -B build -DEZCACHE_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release <br>
cmake --build build <br>
ctest --test-dir build --output-on-failure