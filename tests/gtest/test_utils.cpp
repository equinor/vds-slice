#include "test_utils.hpp"

void DatahandleCubeIntersectionTest::check_value(
    int value, int expected_iline, int expected_xline, int expected_sample
) {
    int sample = value & 0xFF;            // Bits 0-7
    int xline = (value & 0xFF00) >> 8;    // Bits 8-15
    int iline = (value & 0xFF0000) >> 16; // Bits 15-23

    std::string error_message =
        "at iline " + std::to_string(expected_iline) +
        " xline " + std::to_string(expected_xline) +
        " sample " + std::to_string(expected_sample);
    EXPECT_EQ(iline, expected_iline) << error_message;
    EXPECT_EQ(xline, expected_xline) << error_message;
    EXPECT_EQ(sample, expected_sample) << error_message;
}

