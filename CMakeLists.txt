cmake_minimum_required(VERSION 3.16)
project(EZCache LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(EZCache INTERFACE)
add_library(EZCache::EZCache ALIAS EZCache)
target_include_directories(EZCache INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(EZCache INTERFACE cxx_std_20)


### BENCHMARK ###
option(EZCACHE_BUILD_BENCHMARK "Build EZCache benchmark" OFF)
if (EZCACHE_BUILD_BENCHMARK AND (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR))
    add_executable(benchmark benchmark/benchmark.cpp)
    target_link_libraries(benchmark PRIVATE EZCache)
endif()
