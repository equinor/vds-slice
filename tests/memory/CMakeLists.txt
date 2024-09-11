# obtain gtest as recommended
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(memorytests
  memory.cpp
)
target_link_libraries(memorytests
  PRIVATE ccore
  PRIVATE GTest::gtest_main
  PRIVATE GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(memorytests)

set_target_properties(memorytests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/
)
