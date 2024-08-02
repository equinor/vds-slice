#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <iostream>

#include "test_utils.hpp"

#include "cppapi.hpp"
#include "datahandle.hpp"

namespace {

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_I_Single) {

    int line_index = 2;
    int line_annotation = 9;

    struct response response_data;
    cppapi::slice(
        single_datahandle,
        Direction(axis_name::I),
        line_index,
        slice_bounds,
        &response_data
    );

    int low[3] = {line_annotation, 2, 4};
    int high[3] = {line_annotation, 16, 128};
    check_slice(response_data, low, high, 1);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_I_Double) {

    int line_index = 2;
    int line_annotation = 21;

    struct response response_data;
    cppapi::slice(
        double_datahandle,
        Direction(axis_name::I),
        line_index,
        slice_bounds,
        &response_data
    );

    int low[3] = {line_annotation, 10, 20};
    int high[3] = {line_annotation, 16, 128};
    check_slice(response_data, low, high, 2);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_J_Single) {

    int line_index = 2;
    int line_annotation = 6;

    struct response response_data;
    cppapi::slice(
        single_datahandle,
        Direction(axis_name::J),
        line_index,
        slice_bounds,
        &response_data
    );

    int low[3] = {3, line_annotation, 4};
    int high[3] = {24, line_annotation, 128};
    check_slice(response_data, low, high, 1);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_J_Double) {

    int line_index = 2;
    int line_annotation = 14;

    struct response response_data;
    cppapi::slice(
        double_datahandle,
        Direction(axis_name::J),
        line_index,
        slice_bounds,
        &response_data
    );

    int low[3] = {15, line_annotation, 20};
    int high[3] = {24, line_annotation, 128};
    check_slice(response_data, low, high, 2);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_K_Single) {

    int line_index = 2;
    int line_annotation = 12;

    struct response response_data;
    cppapi::slice(
        single_datahandle,
        Direction(axis_name::K),
        line_index,
        slice_bounds,
        &response_data
    );

    int low[3] = {3, 2, line_annotation};
    int high[3] = {24, 16, line_annotation};
    check_slice(response_data, low, high, 1);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_K_Double) {

    int line_index = 2;
    int line_annotation = 28;

    struct response response_data;
    cppapi::slice(
        double_datahandle,
        Direction(axis_name::K),
        line_index,
        slice_bounds,
        &response_data
    );

    int low[3] = {15, 10, line_annotation};
    int high[3] = {24, 16, line_annotation};
    check_slice(response_data, low, high, 2);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_INLINE_Single) {

    struct response response_data;
    cppapi::slice(
        single_datahandle,
        Direction(axis_name::INLINE),
        21,
        slice_bounds,
        &response_data
    );

    int low[3] = {21, 2, 4};
    int high[3] = {21, 16, 128};
    check_slice(response_data, low, high, 1);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_INLINE_Double) {

    struct response response_data;
    cppapi::slice(
        double_datahandle,
        Direction(axis_name::INLINE),
        21,
        slice_bounds,
        &response_data
    );

    int low[3] = {21, 10, 20};
    int high[3] = {21, 16, 128};
    check_slice(response_data, low, high, 2);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_CROSSLINE_Single) {

    struct response response_data;
    cppapi::slice(
        single_datahandle,
        Direction(axis_name::CROSSLINE),
        14,
        slice_bounds,
        &response_data
    );

    int low[3] = {3, 14, 4};
    int high[3] = {24, 14, 128};
    check_slice(response_data, low, high, 1);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_CROSSLINE_Double) {

    struct response response_data;
    cppapi::slice(
        double_datahandle,
        Direction(axis_name::CROSSLINE),
        14,
        slice_bounds,
        &response_data
    );

    int low[3] = {15, 14, 20};
    int high[3] = {24, 14, 128};
    check_slice(response_data, low, high, 2);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_SAMPLE_Single) {

    struct response response_data;
    cppapi::slice(
        single_datahandle,
        Direction(axis_name::SAMPLE),
        40,
        slice_bounds,
        &response_data
    );

    int low[3] = {3, 2, 40};
    int high[3] = {24, 16, 40};
    check_slice(response_data, low, high, 1);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_SAMPLE_Double) {

    struct response response_data;
    cppapi::slice(
        double_datahandle,
        Direction(axis_name::SAMPLE),
        40,
        slice_bounds,
        &response_data
    );

    int low[3] = {15, 10, 40};
    int high[3] = {24, 16, 40};
    check_slice(response_data, low, high, 2);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_TIME_Single) {

    struct response response_data;
    cppapi::slice(
        single_datahandle,
        Direction(axis_name::TIME),
        40,
        slice_bounds,
        &response_data
    );

    int low[3] = {3, 2, 40};
    int high[3] = {24, 16, 40};
    check_slice(response_data, low, high, 1);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_TIME_Double) {

    struct response response_data;
    cppapi::slice(
        double_datahandle,
        Direction(axis_name::TIME),
        40,
        slice_bounds,
        &response_data
    );

    int low[3] = {15, 10, 40};
    int high[3] = {24, 16, 40};
    check_slice(response_data, low, high, 2);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_DEPTH_Single) {

    struct response response_data;

    EXPECT_THAT([&]() {
        cppapi::slice(
            single_datahandle,
            Direction(axis_name::DEPTH),
            40,
            slice_bounds,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Cannot fetch depth slice for VDS file with vertical axis unit: ms")));
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_DEPTH_Double) {

    struct response response_data;

    EXPECT_THAT([&]() {
        cppapi::slice(
            double_datahandle,
            Direction(axis_name::DEPTH),
            40,
            slice_bounds,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Cannot fetch depth slice for VDS file with vertical axis unit: ms")));
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_TIME_Invalid_Single) {

    Direction const direction(axis_name::TIME);
    struct response response_data;

    EXPECT_THAT([&]() {
        cppapi::slice(
            single_datahandle,
            direction,
            0,
            slice_bounds,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Invalid lineno: 0, valid range: [4.00:128.00:4.00]")));

    EXPECT_THAT([&]() {
        cppapi::slice(
            single_datahandle,
            direction,
            132,
            slice_bounds,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Invalid lineno: 132, valid range: [4.00:128.00:4.00]")));

    EXPECT_THAT([&]() {
        cppapi::slice(
            single_datahandle,
            direction,
            21,
            slice_bounds,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Invalid lineno: 21, valid range: [4.00:128.00:4.00]")));
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_TIME_Invalid_Double) {

    Direction const direction(axis_name::TIME);
    struct response response_data;

    EXPECT_THAT([&]() {
        cppapi::slice(
            double_datahandle,
            direction,
            16,
            slice_bounds,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Invalid lineno: 16, valid range: [20.00:128.00:4.00]")));

    EXPECT_THAT([&]() {
        cppapi::slice(
            double_datahandle,
            direction,
            132,
            slice_bounds,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Invalid lineno: 132, valid range: [20.00:128.00:4.00]")));

    EXPECT_THAT([&]() {
        cppapi::slice(
            double_datahandle,
            direction,
            21,
            slice_bounds,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Invalid lineno: 21, valid range: [20.00:128.00:4.00]")));
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Axis_TIME_Reverse_Double) {

    struct response response_data;
    cppapi::slice(
        double_reverse_datahandle,
        Direction(axis_name::TIME),
        40,
        slice_bounds,
        &response_data
    );

    int low[3] = {15, 10, 40};
    int high[3] = {24, 16, 40};
    check_slice(response_data, low, high, 2);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Different_Size_Double) {

    struct response response_data;
    cppapi::slice(
        double_different_size,
        Direction(axis_name::INLINE),
        30,
        slice_bounds,
        &response_data
    );

    int low[3] = {30, 18, 36};
    int high[3] = {30, 24, 144};
    check_slice(response_data, low, high, 2);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Different_Number_Of_Samples_To_Border_Per_Dimension) {
    const std::string INNER_CUBE = "file://inner_4x2_cube.vds";

    DoubleDataHandle datahandle = make_double_datahandle(
        REGULAR_DATA.c_str(),
        CREDENTIALS.c_str(),
        INNER_CUBE.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::ADDITION
    );

    struct response response_data;
    cppapi::slice(
        datahandle,
        Direction(axis_name::CROSSLINE),
        14,
        slice_bounds,
        &response_data
    );

    int low[3] = {9, 14, 8};
    int high[3] = {18, 14, 36};

    check_slice(response_data, low, high, 2);
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Minimum_Single) {

    const std::string MINIMUM_CUBE = "file://10_min_dimensions.vds";

    SingleDataHandle single_minimum_handle = make_single_datahandle(
        MINIMUM_CUBE.c_str(),
        CREDENTIALS.c_str()
    );

    struct response response_data;
    cppapi::slice(
        single_minimum_handle,
        Direction(axis_name::TIME),
        8,
        slice_bounds,
        &response_data
    );

    std::size_t nr_of_values = (std::size_t)(response_data.size / sizeof(float));
    EXPECT_EQ(nr_of_values, 2 * 2);

    std::vector<float> expected = {-3.5, 3.5, -14.5, 0.5};
    for (int i = 0; i < nr_of_values; i++) {
        float value = *(float*)&response_data.data[i * sizeof(float)];
        EXPECT_EQ(value, expected[i]);
    }
}

TEST_F(DatahandleCubeIntersectionTest, Slice_Minimum_Double) {

    const std::string SHIFT_6_DATA = "file://shift_6_8x2_cube.vds";

    DoubleDataHandle double_minimum_handle = make_double_datahandle(
        REGULAR_DATA.c_str(),
        CREDENTIALS.c_str(),
        SHIFT_6_DATA.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::ADDITION
    );

    struct response response_data;
    cppapi::slice(
        double_minimum_handle,
        Direction(axis_name::TIME),
        124,
        slice_bounds,
        &response_data
    );

    std::size_t nr_of_values = (std::size_t)(response_data.size / sizeof(float));
    EXPECT_EQ(nr_of_values, 2 * 2);

    std::vector<int> expected_iline = {21, 21, 24, 24};
    std::vector<int> expected_xline = {14, 16, 14, 16};

    for (int i = 0; i < nr_of_values; i++) {
        int value = std::lround(*(float*)&response_data.data[i * sizeof(float)] / 2);
        int sample = value & 0xFF;
        int xline = (value & 0xFF00) >> 8;
        int iline = (value & 0xFF0000) >> 16;

        EXPECT_EQ(iline, expected_iline[i]);
        EXPECT_EQ(xline, expected_xline[i]);
        EXPECT_EQ(sample, 124);
    }
}

} // namespace
