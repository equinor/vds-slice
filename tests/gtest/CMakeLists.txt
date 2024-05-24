# obtain gtest as recommended
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(cppcoretests
  coordinate_transformer_test.cpp
  cppapi_test.cpp
  datahandle_attribute_test.cpp
  datahandle_fence_test.cpp
  datahandle_metadata_test.cpp
  datahandle_slice_test.cpp
  datahandle_test.cpp
  regularsurface_test.cpp
  subvolume_test.cpp
)

target_link_libraries(cppcoretests
  PRIVATE cppcore
  PRIVATE GTest::gtest_main
  PRIVATE GTest::gmock_main
)

configure_file(../../testdata/well_known/well_known_default.vds . COPYONLY)
configure_file(../../testdata/well_known/well_known_custom_axis_order.vds . COPYONLY)
configure_file(../../testdata/well_known/well_known_custom_inline_spacing.vds . COPYONLY)
configure_file(../../testdata/well_known/well_known_custom_origin.vds . COPYONLY)
configure_file(../../testdata/well_known/well_known_custom_xline_spacing.vds . COPYONLY)
configure_file(../../testdata/well_known/well_known_depth_axis.vds . COPYONLY)

configure_file(../../testdata/10_samples/10_default_crs.vds . COPYONLY)
configure_file(../../testdata/10_samples/10_double_value.vds . COPYONLY)
configure_file(../../testdata/10_samples/10_min_dimensions.vds . COPYONLY)
configure_file(../../testdata/10_samples/10_samples_default.vds . COPYONLY)
configure_file(../../testdata/10_samples/10_single_sample.vds . COPYONLY)
configure_file(../../testdata/10_samples/10_single_xline.vds . COPYONLY)

configure_file(../../testdata/cube_intersection/regular_8x2_cube.vds . COPYONLY)
configure_file(../../testdata/cube_intersection/shift_4_8x2_cube.vds . COPYONLY)
configure_file(../../testdata/cube_intersection/shift_6_8x2_cube.vds . COPYONLY)
configure_file(../../testdata/cube_intersection/shift_7_inLine_8x2_cube.vds . COPYONLY)
configure_file(../../testdata/cube_intersection/shift_7_xLine_8x2_cube.vds . COPYONLY)
configure_file(../../testdata/cube_intersection/shift_31_Sample_8x2_cube.vds . COPYONLY)
configure_file(../../testdata/cube_intersection/shift_8_32x3_cube.vds . COPYONLY)
configure_file(../../testdata/cube_intersection/inner_4x2_cube.vds . COPYONLY)

include(GoogleTest)
gtest_discover_tests(cppcoretests)
