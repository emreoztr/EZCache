cmake_minimum_required(VERSION 3.16)
project(EZCache LANGUAGES CXX)

# Global standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Lib (header-only)
add_library(EZCache INTERFACE)
add_library(EZCache::EZCache ALIAS EZCache)
target_include_directories(EZCache INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(EZCache INTERFACE cxx_std_20)

# (İsteğe bağlı ama IDE için faydalı) Header'ı hedefe iliştir
# target_sources(EZCache INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/EZCache.hpp)

### BENCHMARK ###
option(EZCACHE_BUILD_BENCHMARK "Build EZCache benchmark" OFF)
if (EZCACHE_BUILD_BENCHMARK AND (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR))
    add_executable(benchmark benchmark/benchmark.cpp)
    target_link_libraries(benchmark PRIVATE EZCache)
    # Release’ta optimize
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(benchmark PRIVATE $<$<CONFIG:Release>:-O3 -DNDEBUG -march=native>)
        target_link_options(benchmark PRIVATE $<$<CONFIG:Release>:-flto>)
    elseif (MSVC)
        target_compile_options(benchmark PRIVATE $<$<CONFIG:Release>:/O2 /DNDEBUG>)
        target_link_options(benchmark PRIVATE $<$<CONFIG:Release>:/LTCG>)
    endif()
endif()

### TESTS ###
option(EZCACHE_BUILD_TESTS "Build EZCache unit tests" ON)

if (EZCACHE_BUILD_TESTS AND (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR))
    include(CTest)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(unit_tests
            tests/test_cache.cpp
    )
    target_link_libraries(unit_tests PRIVATE EZCache gtest gtest_main)
    target_compile_features(unit_tests PRIVATE cxx_std_20)

    add_test(NAME EZCache.All COMMAND unit_tests)
endif()
