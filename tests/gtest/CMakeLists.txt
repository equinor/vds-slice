# obtain gtest as recommended
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(cppcoretests
  cppapi_test.cpp
)

target_link_libraries(cppcoretests
  PRIVATE cppcore
  PRIVATE GTest::gtest_main
  PRIVATE GTest::gmock_main
)

configure_file(../../testdata/10_samples/10_samples_default.vds 10_samples_default.vds COPYONLY)

include(GoogleTest)
gtest_discover_tests(cppcoretests)