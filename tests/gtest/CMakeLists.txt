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
  subcube_test.cpp
)

target_link_libraries(cppcoretests
  PRIVATE cppcore
  PRIVATE GTest::gtest_main
  PRIVATE GTest::gmock_main
)

configure_file(../../testdata/10_samples/10_samples_default.vds 10_samples_default.vds COPYONLY)

configure_file(../../testdata/dimensions_data/dimensions_data_il_10_xl_20_s_30.vds . COPYONLY)
configure_file(../../testdata/dimensions_data/dimensions_data_il_11_xl_20_s_30.vds . COPYONLY)
configure_file(../../testdata/dimensions_data/dimensions_data_il_10_xl_21_s_30.vds . COPYONLY)
configure_file(../../testdata/dimensions_data/dimensions_data_il_10_xl_20_s_31.vds . COPYONLY)
configure_file(../../testdata/dimensions_data/dimensions_data_il_10_xl_20_s_30_offset.vds . COPYONLY)
configure_file(../../testdata/dimensions_data/dimensions_data_il_10_xl_20_s_30_stepsize.vds . COPYONLY)
configure_file(../../testdata/dimensions_data/dimensions_data_il_10_xl_20_s_30_double.vds . COPYONLY)

include(GoogleTest)
gtest_discover_tests(cppcoretests)
