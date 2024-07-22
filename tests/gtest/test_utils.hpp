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

};
#endif // TEST_UTILS_H
