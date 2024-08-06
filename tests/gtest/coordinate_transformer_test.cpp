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

    CoordinateTransformer const& transformer = datahandle.get_metadata().coordinate_transformer();
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

    CoordinateTransformer const& transformer = datahandle.get_metadata().coordinate_transformer();
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

    CoordinateTransformer const& transformer = datahandle.get_metadata().coordinate_transformer();
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

    CoordinateTransformer const& transformer = datahandle.get_metadata().coordinate_transformer();
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

    CoordinateTransformer const& transformer = datahandle.get_metadata().coordinate_transformer();
    EXPECT_EQ(as_annotation, transformer.IJKIndexToAnnotation(as_intersection_index));
}

TEST(NegativeCoordinateTransformerTest, NegativeAnnotation) {
    const std::string url = "file://10_negative.vds";

    auto datahandle = make_single_datahandle(
        url.c_str(),
        CREDENTIALS.c_str()
    );

    OpenVDS::DoubleVector3 as_annotation = {3, -11, -8};
    OpenVDS::IntVector3 as_index = {1, 0, 3};
    // we always ignore the value of third cdp element
    OpenVDS::DoubleVector3 as_cdp = {8, 4, 0};

    CoordinateTransformer const& transformer = datahandle.get_metadata().coordinate_transformer();

    EXPECT_EQ(as_annotation, transformer.IJKIndexToAnnotation(as_index));

    auto index_to_world = transformer.IJKIndexToWorld(as_index);
    EXPECT_EQ(as_cdp.X, index_to_world.X);
    EXPECT_EQ(as_cdp.Y, index_to_world.Y);

    auto world_to_annotation = transformer.WorldToAnnotation(as_cdp);
    EXPECT_EQ(as_annotation.X, transformer.WorldToAnnotation(as_cdp).X);
    EXPECT_EQ(as_annotation.Y, transformer.WorldToAnnotation(as_cdp).Y);
}

} // namespace
