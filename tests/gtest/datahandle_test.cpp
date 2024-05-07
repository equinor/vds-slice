#include "cppapi.hpp"
#include "ctypes.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {
const std::string DEFAULT_DATA = "file://10_samples_default.vds";
const std::string DOUBLE_VALUE_DATA = "file://10_double_value.vds";

const std::string CREDENTIALS = "";

const float DELTA = 0.00001;
const int EXPECTED_TRACE_LENGTH = 7;

const float x_inc = std::sqrt((8 - 2) * (8 - 2) + 4 * 4);                                            // 7.2111
const float y_inc = std::sqrt((-2) * (-2) + 3 * 3);                                                  // 3.6056
const float angle = std::asin(4 / std::sqrt((8 - 2) * (8 - 2) + 4 * 4)) / (2 * std::acos(-1)) * 360; // 33.69
Grid default_grid = Grid(2, 0, x_inc, y_inc, angle);

class DataHandleTest : public ::testing::Test {
protected:
    DoubleDataHandle* datahandle;
    SingleDataHandle* datahandle_reference;
    SurfaceBoundedSubVolume* subvolume_reference;
    static constexpr int nrows = 3;
    static constexpr int ncols = 2;
    static constexpr std::size_t size = nrows * ncols;

    std::array<float, size> top_surface_data;
    std::array<float, nrows * ncols> primary_surface_data;
    std::array<float, size> bottom_surface_data;

    static constexpr float fill = -999.25;

    RegularSurface primary_surface =
        RegularSurface(primary_surface_data.data(), nrows, ncols, default_grid, fill);

    RegularSurface top_surface =
        RegularSurface(top_surface_data.data(), nrows, ncols, default_grid, fill);

    RegularSurface bottom_surface =
        RegularSurface(bottom_surface_data.data(), nrows, ncols, default_grid, fill);

    void SetUp() override {
        datahandle_reference = make_single_datahandle(
            DEFAULT_DATA.c_str(),
            CREDENTIALS.c_str()
        );

        for (int i = 0; i < size; ++i) {
            top_surface_data[i] = 19.0;
            primary_surface_data[i] = 20.0;
            bottom_surface_data[i] = 29.0;
        };

        subvolume_reference = make_subvolume(
            datahandle_reference->get_metadata(), primary_surface, top_surface, bottom_surface
        );

        cppapi::fetch_subvolume(*datahandle_reference, *subvolume_reference, NEAREST, 0, size);
    }

    void TearDown() override {
        delete subvolume_reference;
        delete datahandle_reference;
    }
};

TEST_F(DataHandleTest, Addition) {

    DoubleDataHandle* datahandle = make_double_datahandle(
        DEFAULT_DATA.c_str(),
        CREDENTIALS.c_str(),
        DOUBLE_VALUE_DATA.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::ADDITION
    );

    SurfaceBoundedSubVolume* subvolume = make_subvolume(
        datahandle->get_metadata(), primary_surface, top_surface, bottom_surface
    );

    cppapi::fetch_subvolume(*datahandle, *subvolume, NEAREST, 0, size);
    int compared_values = 0;
    for (int i = 0; i < size; ++i) {
        RawSegment rs = subvolume->vertical_segment(i);
        RawSegment rs_ref = subvolume_reference->vertical_segment(i);

        for (auto it = rs.begin(), a_it = rs_ref.begin();
             it != rs.end() && a_it != rs_ref.end();
             ++it, ++a_it) {
            compared_values++;
            EXPECT_NEAR(*it, *a_it * 3, DELTA) << "at segment " << i << " at position in data " << std::distance(rs.begin(), it);
        }
    }
    EXPECT_EQ(compared_values, size * EXPECTED_TRACE_LENGTH);

    delete subvolume;
    delete datahandle;
}

TEST_F(DataHandleTest, Multiplication) {

    DoubleDataHandle* datahandle = make_double_datahandle(
        DEFAULT_DATA.c_str(),
        CREDENTIALS.c_str(),
        DOUBLE_VALUE_DATA.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::MULTIPLICATION
    );

    SurfaceBoundedSubVolume* subvolume = make_subvolume(
        datahandle->get_metadata(), primary_surface, top_surface, bottom_surface
    );

    cppapi::fetch_subvolume(*datahandle, *subvolume, NEAREST, 0, size);

    int compared_values = 0;
    for (int i = 0; i < size; ++i) {
        RawSegment rs = subvolume->vertical_segment(i);
        RawSegment rs_ref = subvolume_reference->vertical_segment(i);

        for (auto it = rs.begin(), a_it = rs_ref.begin();
             it != rs.end() && a_it != rs_ref.end();
             ++it, ++a_it) {
            compared_values++;
            EXPECT_NEAR(*it, 2 * (*a_it) * (*a_it), DELTA) << "at segment " << i << " at position in data " << std::distance(rs.begin(), it);
        }
    }
    EXPECT_EQ(compared_values, size * EXPECTED_TRACE_LENGTH);
    delete subvolume;
    delete datahandle;
}

TEST_F(DataHandleTest, Division) {

    DoubleDataHandle* datahandle = make_double_datahandle(
        DEFAULT_DATA.c_str(),
        CREDENTIALS.c_str(),
        DOUBLE_VALUE_DATA.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::DIVISION
    );

    SurfaceBoundedSubVolume* subvolume = make_subvolume(
        datahandle->get_metadata(), primary_surface, top_surface, bottom_surface
    );

    cppapi::fetch_subvolume(*datahandle, *subvolume, NEAREST, 0, size);
    int compared_values = 0;
    for (int i = 0; i < size; ++i) {
        RawSegment rs = subvolume->vertical_segment(i);
        RawSegment rs_ref = subvolume_reference->vertical_segment(i);

        for (auto it = rs.begin(), a_it = rs_ref.begin();
             it != rs.end() && a_it != rs_ref.end();
             ++it, ++a_it) {
            compared_values++;
            EXPECT_NEAR(*it, 0.5, DELTA) << "at segment " << i << " at position in data " << std::distance(rs.begin(), it);
        }
    }
    EXPECT_EQ(compared_values, size * EXPECTED_TRACE_LENGTH);
    delete subvolume;
    delete datahandle;
}

TEST_F(DataHandleTest, Subtraction) {

    DoubleDataHandle* datahandle = make_double_datahandle(
        DEFAULT_DATA.c_str(),
        CREDENTIALS.c_str(),
        DOUBLE_VALUE_DATA.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::SUBTRACTION
    );

    SurfaceBoundedSubVolume* subvolume = make_subvolume(
        datahandle->get_metadata(), primary_surface, top_surface, bottom_surface
    );

    cppapi::fetch_subvolume(*datahandle, *subvolume, NEAREST, 0, size);
    int compared_values = 0;
    for (int i = 0; i < size; ++i) {
        RawSegment rs = subvolume->vertical_segment(i);
        RawSegment rs_ref = subvolume_reference->vertical_segment(i);

        for (auto it = rs.begin(), a_it = rs_ref.begin();
             it != rs.end() && a_it != rs_ref.end();
             ++it, ++a_it) {
            compared_values++;
            EXPECT_NEAR(*it, -*a_it, DELTA) << "at segment " << i << " at position in data " << std::distance(rs.begin(), it);
        }
    }
    EXPECT_EQ(compared_values, size * EXPECTED_TRACE_LENGTH);
    delete subvolume;
    delete datahandle;
}

TEST_F(DataHandleTest, SubtractionReverse) {

    DoubleDataHandle* datahandle = make_double_datahandle(
        DOUBLE_VALUE_DATA.c_str(),
        CREDENTIALS.c_str(),
        DEFAULT_DATA.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::SUBTRACTION
    );

    SurfaceBoundedSubVolume* subvolume = make_subvolume(
        datahandle->get_metadata(), primary_surface, top_surface, bottom_surface
    );

    cppapi::fetch_subvolume(*datahandle, *subvolume, NEAREST, 0, size);
    int compared_values = 0;
    for (int i = 0; i < size; ++i) {
        RawSegment rs = subvolume->vertical_segment(i);
        RawSegment rs_ref = subvolume_reference->vertical_segment(i);

        for (auto it = rs.begin(), a_it = rs_ref.begin();
             it != rs.end() && a_it != rs_ref.end();
             ++it, ++a_it) {
            compared_values++;
            EXPECT_NEAR(*it, *a_it, DELTA) << "at segment " << i << " at position in data " << std::distance(rs.begin(), it);
        }
    }
    EXPECT_EQ(compared_values, size * EXPECTED_TRACE_LENGTH);
    delete subvolume;
    delete datahandle;
}

} // namespace
