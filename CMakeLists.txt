cmake_minimum_required(VERSION 3.31)
project(EZCache)

set(CMAKE_CXX_STANDARD 20)

add_executable(EZCache
        include/EZCache.hpp
        include/internal.hpp)
