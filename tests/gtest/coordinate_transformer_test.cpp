#include "coordinate_transformer.hpp"
#include "ctypes.h"
#include "datahandle.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <OpenVDS/OpenVDS.h>

namespace {

const std::string CREDENTIALS = "";

TEST(DoubleCoordinateTransformerTest, A_Before_B) {
    const std::string a = "file://regular_8x2_cube.vds";
    const std::string b = "file://shift_4_8x2_cube.vds";

    auto datahandle = make_double_datahandle(
        a.c_str(),
        CREDENTIALS.c_str(),
        b.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::DIVISION
    );

    OpenVDS::DoubleVector3 as_annotation = {21, 22, 20};
    OpenVDS::IntVector3 as_intersection_index = {2, 6, 0};

    CoordinateTransformer const& transformer = datahandle->get_metadata().coordinate_transformer();
    EXPECT_EQ(as_annotation, transformer.IJKIndexToAnnotation(as_intersection_index));
}

TEST(DoubleCoordinateTransformerTest, A_After_B) {
    const std::string a = "file://shift_4_8x2_cube.vds";
    const std::string b = "file://regular_8x2_cube.vds";

    auto datahandle = make_double_datahandle(
        a.c_str(),
        CREDENTIALS.c_str(),
        b.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::DIVISION
    );

    OpenVDS::DoubleVector3 as_annotation = {21, 22, 20};
    OpenVDS::DoubleVector3 as_intersection_position = {2, 6, 0};

    CoordinateTransformer const& transformer = datahandle->get_metadata().coordinate_transformer();
    EXPECT_EQ(as_annotation, transformer.IJKPositionToAnnotation(as_intersection_position));
}

TEST(DoubleCoordinateTransformerTest, A_Equals_B) {
    const std::string a = "file://regular_8x2_cube.vds";
    const std::string b = "file://regular_8x2_cube.vds";

    auto datahandle = make_double_datahandle(
        a.c_str(),
        CREDENTIALS.c_str(),
        b.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::DIVISION
    );

    OpenVDS::DoubleVector3 as_annotation = {12, 10, 8};
    OpenVDS::DoubleVector3 as_intersection_position = {3, 4, 1};

    CoordinateTransformer const& transformer = datahandle->get_metadata().coordinate_transformer();
    EXPECT_EQ(as_annotation, transformer.IJKPositionToAnnotation(as_intersection_position));
}

TEST(DoubleCoordinateTransformerTest, A_In_B) {
    const std::string a = "file://inner_4x2_cube.vds";
    const std::string b = "file://regular_8x2_cube.vds";

    auto datahandle = make_double_datahandle(
        a.c_str(),
        CREDENTIALS.c_str(),
        b.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::DIVISION
    );

    OpenVDS::DoubleVector3 as_annotation = {9, 12, 20};
    OpenVDS::IntVector3 as_intersection_index = {0, 2, 3};

    CoordinateTransformer const& transformer = datahandle->get_metadata().coordinate_transformer();
    EXPECT_EQ(as_annotation, transformer.IJKIndexToAnnotation(as_intersection_index));
}

TEST(DoubleCoordinateTransformerTest, A_Around_B) {
    const std::string a = "file://regular_8x2_cube.vds";
    const std::string b = "file://inner_4x2_cube.vds";

    auto datahandle = make_double_datahandle(
        a.c_str(),
        CREDENTIALS.c_str(),
        b.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::DIVISION
    );

    OpenVDS::DoubleVector3 as_annotation = {9, 12, 20};
    OpenVDS::IntVector3 as_intersection_index = {0, 2, 3};

    CoordinateTransformer const& transformer = datahandle->get_metadata().coordinate_transformer();
    EXPECT_EQ(as_annotation, transformer.IJKIndexToAnnotation(as_intersection_index));
}

} // namespace
