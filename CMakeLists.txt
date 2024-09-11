# Code is not used in production. It is built only to be linked against c++
# testing suite. In production everything is built through go.

cmake_minimum_required(VERSION 3.14)
project(vdsslice LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GTEST "Include tests/gtest subdirectory" ON)
option(MEMORYTEST "Include tests/memory subdirectory" OFF)
option(BUILD_CCORE "Build the c core library" OFF)

add_subdirectory(internal/core)

if(GTEST)
    enable_testing()
    add_subdirectory(tests/gtest)
endif()

if(MEMORYTEST)
    enable_testing()
    add_subdirectory(tests/memory)
endif()
