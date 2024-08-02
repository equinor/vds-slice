#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "cppapi.hpp"
#include "ctypes.h"
#include <iostream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

const std::string CREDENTIALS = "";
static constexpr float fill = -999.25;

class DatahandleCubeIntersectionTest : public ::testing::Test {
protected:
    const std::string REGULAR_DATA = "file://regular_8x2_cube.vds";
    const std::string SHIFT_4_DATA = "file://shift_4_8x2_cube.vds";
    const std::string SHIFT_8_32_DATA = "file://shift_8_32x3_cube.vds";

    const int ILINE_STEP = 3;
    const int XLINE_STEP = 2;
    const int SAMPLE_STEP = 4;

    DatahandleCubeIntersectionTest()
        : single_datahandle(make_single_datahandle(REGULAR_DATA.c_str(), CREDENTIALS.c_str())),
          double_datahandle(make_double_datahandle(
              REGULAR_DATA.c_str(),
              CREDENTIALS.c_str(),
              SHIFT_4_DATA.c_str(),
              CREDENTIALS.c_str(),
              binary_operator::ADDITION
          )),
          double_reverse_datahandle(make_double_datahandle(
              SHIFT_4_DATA.c_str(),
              CREDENTIALS.c_str(),
              REGULAR_DATA.c_str(),
              CREDENTIALS.c_str(),
              binary_operator::ADDITION
          )),
          double_different_size(make_double_datahandle(
              SHIFT_4_DATA.c_str(),
              CREDENTIALS.c_str(),
              SHIFT_8_32_DATA.c_str(),
              CREDENTIALS.c_str(),
              binary_operator::ADDITION
          )) {}

public:
    std::vector<Bound> slice_bounds;

    SingleDataHandle single_datahandle;
    DoubleDataHandle double_datahandle;
    DoubleDataHandle double_reverse_datahandle;
    DoubleDataHandle double_different_size;

    /**
     * @brief Checks that retrieved value and expected iline, xline and sample match as:
     *
     * Each value in the test files has the positional information encoded in little-endian.
     * Byte 0: Sample position
     * Byte 1: CrossLine position
     * Byte 2: InLine position
     */
    void check_value(int value, int expected_iline, int expected_xline, int expected_sample);

    /**
     * @brief Check response slice towards expected slice
     *
     * @param response_data Data from request
     * @param low Low limit on axis for expected data. Array contains
     * 3 values corresponding to inline/xline/sample annotation coordinate. Inclusive
     * @param high High limit on axis for expected data. Array contains
     * 3 values corresponding to inline/xline/sample annotation coordinate. Inclusive
     * @param factor multiplicative factor (expected value * factor = actual
     * value). Expected value comes from rules used in file creation
     */
    void check_slice(struct response response_data, int low_annotation[], int high_annotation[], float factor);

    /**
     * @brief Check result from fence request
     *
     * @param response_data Result from request
     * @param annotated_coordinates Provided coordinates (in annotation
     * coordinate system)
     * @param lowest_sample Low limit on samples axis for expected data as a
     * sample annotation coordinate. Inclusive
     * @param highest_sample High limit on samples axis for expected data as a
     * sample annotation coordinate. Inclusive
     * @param factor multiplicative factor (expected value * factor = actual
     * value). Expected value comes from rules used in file creation
     * @param fill_flag True if fill value is expected in the whole cube
     */
    void check_fence(
        struct response response_data,
        std::vector<float> annotated_coordinates,
        int lowest_sample,
        int highest_sample,
        float factor,
        bool fill_flag
    );

    /**
     * @brief Check attribute values against expected values
     *
     * @param subvolume
     * @param requested_low Low limit on axis for requested data. Array contains
     * 3 values corresponding to inline/xline/sample annotation coordinate.
     * Inclusive
     * @param requested_high High limit on axis for requested data. Inclusive
     * @param expected_low Low limit on axis for expected returned data. Array
     * contains 3 values corresponding to inline/xline/sample annotation
     * coordinate. Default margin would be applied automatically. Inclusive
     * @param expected_high High limit on axis for expected returned data.
     * Inclusive
     * @param factor multiplicative factor (expected value * factor = actual
     * value). Expected value comes from rules used in file creation
     */
    void check_attribute(
        SurfaceBoundedSubVolume& subvolume, int requested_low_annotation[], int requested_high_annotation[],
        int expected_low_annotation[], int expected_high_annotation[], float factor
    );

    /**
     * @brief Check attribute values against expected values
     *
     * @param subvolume
     * @param expected_low Low limit on axis for expected returned data. Array
     * contains 3 values corresponding to inline/xline/sample annotation
     * coordinate. Default margin would be applied automatically. Inclusive
     * @param expected_high High limit on axis for expected returned data.
     * Inclusive
     * @param factor multiplicative factor (expected value * factor = actual
     * value). Expected value comes from rules used in file creation
     */
    void check_attribute(SurfaceBoundedSubVolume& subvolume, int low[], int high[], float factor);
};
#endif // TEST_UTILS_H
