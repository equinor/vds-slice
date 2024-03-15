# obtain gtest as recommended
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(cppcoretests
  cppapi_test.cpp
  datasource_test.cpp
  regularsurface_test.cpp
  subvolume_test.cpp
  doublevolumedatalayout_test.cpp
)

target_link_libraries(cppcoretests
  PRIVATE cppcore
  PRIVATE GTest::gtest_main
  PRIVATE GTest::gmock_main
)

configure_file(../../testdata/10_samples/10_samples_default.vds . COPYONLY)
configure_file(../../testdata/10_samples/10_missing_iline.vds . COPYONLY)
configure_file(../../testdata/10_samples/10_missing_samples.vds . COPYONLY)
configure_file(../../testdata/10_samples/10_missing_xline.vds . COPYONLY)
configure_file(../../testdata/10_samples/10_double_value.vds . COPYONLY)
configure_file(../../testdata/10_samples/10_miss_offset.vds . COPYONLY)
configure_file(../../testdata/10_samples/10_miss_stepsize.vds . COPYONLY)

configure_file(../../testdata/cube_intersection/regular_8x3_cube.vds . COPYONLY)
configure_file(../../testdata/cube_intersection/shift_4_8x3_cube.vds . COPYONLY)

include(GoogleTest)
gtest_discover_tests(cppcoretests)
