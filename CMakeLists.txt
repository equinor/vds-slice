# Code is not used in production. It is built only to be linked against c++
# testing suite. In production everything is built through go.

cmake_minimum_required(VERSION 3.14)
project(vdsslice LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(internal/core)

enable_testing()
add_subdirectory(tests/gtest)
