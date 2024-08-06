#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <iostream>

#include "test_utils.hpp"

#include "cppapi.hpp"
#include "datahandle.hpp"

namespace {

TEST_F(DatahandleCubeIntersectionTest, Fence_INDEX_Single) {

    struct response response_data;
    const std::vector<float> coordinates{0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};
    const std::vector<float> check_coordinates{3, 2, 6, 4, 9, 6, 12, 8, 15, 10, 18, 12, 21, 14, 24, 16};

    cppapi::fence(
        single_datahandle,
        coordinate_system::INDEX,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        nullptr,
        &response_data
    );

    int low = 4;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 1, false);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_INDEX_Double) {

    struct response response_data;
    const std::vector<float> coordinates{0, 0, 1, 1, 2, 2, 3, 3};
    const std::vector<float> check_coordinates{15, 10, 18, 12, 21, 14, 24, 16};

    cppapi::fence(
        double_datahandle,
        coordinate_system::INDEX,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        nullptr,
        &response_data
    );

    int low = 20;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 2, false);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_INDEX_Fill_Single) {

    struct response response_data;
    const std::vector<float> coordinates{-1, -1, 8, 8};
    const std::vector<float> check_coordinates{0, 0, 27, 18};

    cppapi::fence(
        single_datahandle,
        coordinate_system::INDEX,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        &fill,
        &response_data
    );

    int low = 4;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 1, true);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_INDEX_Fill_Double) {

    struct response response_data;
    const std::vector<float> coordinates{-1, -1, 4, 4};
    const std::vector<float> check_coordinates{12, 8, 27, 18};

    cppapi::fence(
        double_datahandle,
        coordinate_system::INDEX,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        &fill,
        &response_data
    );

    int low = 20;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 1, true);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_INDEX_out_Of_Bounds_Single) {

    struct response response_data;
    const std::vector<float> coordinates{8, 8};

    EXPECT_THAT([&]() {
        cppapi::fence(
            single_datahandle,
            coordinate_system::INDEX,
            coordinates.data(),
            int(coordinates.size() / 2),
            NEAREST,
            nullptr,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Coordinate (8.000000,8.000000) is out of boundaries in dimension 0.")));
}

TEST_F(DatahandleCubeIntersectionTest, Fence_INDEX_out_Of_Bounds_Double) {

    struct response response_data;
    const std::vector<float> coordinates{4, 4};

    EXPECT_THAT([&]() {
        cppapi::fence(
            double_datahandle,
            coordinate_system::INDEX,
            coordinates.data(),
            int(coordinates.size() / 2),
            NEAREST,
            nullptr,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Coordinate (4.000000,4.000000) is out of boundaries in dimension 0.")));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(DatahandleCubeIntersectionTest, Fence_ANNOTATION_Single) {

    struct response response_data;
    const std::vector<float> coordinates{3, 2, 6, 4, 9, 6, 12, 8, 15, 10, 18, 12, 21, 14, 24, 16};
    const std::vector<float> check_coordinates{3, 2, 6, 4, 9, 6, 12, 8, 15, 10, 18, 12, 21, 14, 24, 16};

    cppapi::fence(
        single_datahandle,
        coordinate_system::ANNOTATION,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        nullptr,
        &response_data
    );

    int low = 4;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 1, false);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_ANNOTATION_Double) {

    struct response response_data;
    const std::vector<float> coordinates{15, 10, 18, 12, 21, 14, 24, 16};
    const std::vector<float> check_coordinates{15, 10, 18, 12, 21, 14, 24, 16};

    cppapi::fence(
        double_datahandle,
        coordinate_system::ANNOTATION,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        nullptr,
        &response_data
    );

    int low = 20;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 2, false);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_ANNOTATION_Fill_Single) {

    struct response response_data;
    const std::vector<float> coordinates{0, 0, 27, 18};
    const std::vector<float> check_coordinates{0, 0, 27, 18};

    cppapi::fence(
        single_datahandle,
        coordinate_system::ANNOTATION,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        &fill,
        &response_data
    );

    int low = 4;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 1, true);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_ANNOTATION_Fill_Double) {

    struct response response_data;
    const std::vector<float> coordinates{12, 8, 27, 18};
    const std::vector<float> check_coordinates{12, 8, 27, 18};

    cppapi::fence(
        double_datahandle,
        coordinate_system::ANNOTATION,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        &fill,
        &response_data
    );

    int low = 20;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 1, true);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_ANNOTATION_out_Of_Bounds_Single) {

    struct response response_data;
    const std::vector<float> coordinates{0, 0};

    EXPECT_THAT([&]() {
        cppapi::fence(
            single_datahandle,
            coordinate_system::ANNOTATION,
            coordinates.data(),
            int(coordinates.size() / 2),
            NEAREST,
            nullptr,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Coordinate (0.000000,0.000000) is out of boundaries in dimension 0.")));
}

TEST_F(DatahandleCubeIntersectionTest, Fence_ANNOTATION_out_Of_Bounds_Double) {

    struct response response_data;
    const std::vector<float> coordinates{4, 4};

    EXPECT_THAT([&]() {
        cppapi::fence(
            double_datahandle,
            coordinate_system::ANNOTATION,
            coordinates.data(),
            int(coordinates.size() / 2),
            NEAREST,
            nullptr,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Coordinate (4.000000,4.000000) is out of boundaries in dimension 0.")));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(DatahandleCubeIntersectionTest, Fence_CDP_Single) {

    struct response response_data;
    const std::vector<float> coordinates{2, 0, 7, 12, 12, 24, 17, 36, 22, 48, 27, 60, 32, 72, 37, 84};
    const std::vector<float> check_coordinates{3, 2, 6, 4, 9, 6, 12, 8, 15, 10, 18, 12, 21, 14, 24, 16};

    cppapi::fence(
        single_datahandle,
        coordinate_system::CDP,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        nullptr,
        &response_data
    );

    int low = 4;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 1, false);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_CDP_Double) {

    struct response response_data;
    const std::vector<float> coordinates{22, 48, 27, 60, 32, 72, 37, 84};
    const std::vector<float> check_coordinates{15, 10, 18, 12, 21, 14, 24, 16};

    cppapi::fence(
        double_datahandle,
        coordinate_system::CDP,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        nullptr,
        &response_data
    );

    int low = 20;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 2, false);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_CDP_Fill_Single) {

    struct response response_data;
    const std::vector<float> coordinates{-3, -12, 42, 96};
    const std::vector<float> check_coordinates{0, 0, 27, 18};

    cppapi::fence(
        single_datahandle,
        coordinate_system::CDP,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        &fill,
        &response_data
    );

    int low = 4;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 1, true);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_CDP_Fill_Double) {

    struct response response_data;
    const std::vector<float> coordinates{17, 36, 42, 96};
    const std::vector<float> check_coordinates{12, 8, 27, 18};

    cppapi::fence(
        double_datahandle,
        coordinate_system::CDP,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        &fill,
        &response_data
    );

    int low = 20;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 1, true);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_CDP_out_Of_Bounds_Single) {

    struct response response_data;
    const std::vector<float> coordinates{-3, -12};

    EXPECT_THAT([&]() {
        cppapi::fence(
            single_datahandle,
            coordinate_system::CDP,
            coordinates.data(),
            int(coordinates.size() / 2),
            NEAREST,
            nullptr,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Coordinate (-3.000000,-12.000000) is out of boundaries in dimension 0.")));
}

TEST_F(DatahandleCubeIntersectionTest, Fence_CDP_out_Of_Bounds_Double) {

    struct response response_data;
    const std::vector<float> coordinates{17, 36};

    EXPECT_THAT([&]() {
        cppapi::fence(
            double_datahandle,
            coordinate_system::CDP,
            coordinates.data(),
            int(coordinates.size() / 2),
            NEAREST,
            nullptr,
            &response_data
        );
    },
                testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Coordinate (17.000000,36.000000) is out of boundaries in dimension 0.")));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Miscellaneous tests

TEST_F(DatahandleCubeIntersectionTest, Fence_INDEX_Reverse_Double) {

    struct response response_data;
    const std::vector<float> coordinates{0, 0, 1, 1, 2, 2, 3, 3};
    const std::vector<float> check_coordinates{15, 10, 18, 12, 21, 14, 24, 16};

    cppapi::fence(
        double_reverse_datahandle,
        coordinate_system::INDEX,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        nullptr,
        &response_data
    );

    int low = 20;
    int high = 128;
    check_fence(response_data, check_coordinates, low, high, 2, false);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_INDEX_Different_Size_Double) {

    struct response response_data;
    const std::vector<float> coordinates{0, 0, 1, 1, 2, 2, 3, 3};
    const std::vector<float> check_coordinates{27, 18, 30, 20, 33, 22, 36, 24};

    cppapi::fence(
        double_different_size,
        coordinate_system::INDEX,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        nullptr,
        &response_data
    );

    int low = 36;
    int high = 144;
    check_fence(response_data, check_coordinates, low, high, 2, false);
}

TEST_F(DatahandleCubeIntersectionTest, Fence_Different_Number_Of_Samples_To_Border_Per_Dimension_Double) {

    struct response response_data;
    struct response response_data_reverse;
    const std::vector<float> coordinates{0, 0, 1, 1, 2, 2, 3, 3};
    const std::vector<float> check_coordinates{9, 8, 12, 10, 15, 12, 18, 14};
    int low_sample = 8;
    int high_sample = 36;

    const std::string INNER_CUBE = "file://inner_4x2_cube.vds";

    DoubleDataHandle double_overlap_handle = make_double_datahandle(
        REGULAR_DATA.c_str(),
        CREDENTIALS.c_str(),
        INNER_CUBE.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::ADDITION
    );

    cppapi::fence(
        double_overlap_handle,
        coordinate_system::INDEX,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        nullptr,
        &response_data
    );

    check_fence(response_data, check_coordinates, low_sample, high_sample, 2, false);

    DoubleDataHandle double_overlap_handle_reverse = make_double_datahandle(
        INNER_CUBE.c_str(),
        CREDENTIALS.c_str(),
        REGULAR_DATA.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::ADDITION
    );

    cppapi::fence(
        double_overlap_handle_reverse,
        coordinate_system::INDEX,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        nullptr,
        &response_data_reverse
    );

    check_fence(response_data_reverse, check_coordinates, low_sample, high_sample, 2, false);
}

TEST_F(Datahandle10SamplesTest, Fence_Single_Negative) {
    const std::string NEGATIVE = "file://10_negative.vds";

    SingleDataHandle datahandle = make_single_datahandle(
        NEGATIVE.c_str(),
        CREDENTIALS.c_str()
    );
    const auto metadata = datahandle.get_metadata();

    const std::vector<float> coordinates{1, -10, 3, -11};

    int low = -20;
    int high = 16;

    struct response response_data;

    cppapi::fence(
        datahandle,
        coordinate_system::ANNOTATION,
        coordinates.data(),
        int(coordinates.size() / 2),
        NEAREST,
        nullptr,
        &response_data
    );

    check_fence(response_data, metadata.coordinate_transformer(), coordinates, low, high);
}

} // namespace
