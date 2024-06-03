#include "cppapi.hpp"
#include "ctypes.h"
#include <iostream>
#include <sstream>

#include "attribute.hpp"
#include "metadatahandle.hpp"
#include "subvolume.hpp"
#include "utils.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

/// Each value in the test files has the positional information encoded in little-endian.
/// Byte 0: Sample position
/// Byte 1: CrossLine position
/// Byte 2: InLine position
const std::string REGULAR_DATA = "file://regular_8x2_cube.vds";
const std::string SHIFT_4_DATA = "file://shift_4_8x2_cube.vds";
const std::string SHIFT_8_32_DATA = "file://shift_8_32x3_cube.vds";

const std::string CREDENTIALS = "";


class DatahandleAttributeTest : public ::testing::Test {
protected:
    DatahandleAttributeTest() : single_datahandle(make_single_datahandle(REGULAR_DATA.c_str(), CREDENTIALS.c_str())),
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

    void SetUp() override {

        iline_array = std::vector<int>(32);
        xline_array = std::vector<int>(32);
        sample_array = std::vector<int>(32);

        for (int i = 0; i < iline_array.size(); i++) {
            iline_array[i] = 3 + i * 3;
            xline_array[i] = 2 + i * 2;
        }

        for (int i = 0; i < sample_array.size(); i++) {
            sample_array[i] = 4 + i * 4;
        }
    }

public:
    std::vector<int> iline_array;
    std::vector<int> xline_array;
    std::vector<int> sample_array;

    std::vector<Bound> slice_bounds;

    SingleDataHandle single_datahandle;
    DoubleDataHandle double_datahandle;
    DoubleDataHandle double_reverse_datahandle;
    DoubleDataHandle double_different_size;

    static constexpr float fill = -999.25;

    Grid get_grid(DataHandle& datahandle) {
        return get_grid(datahandle, 0, 0);
    }

    /**
     * @brief Get the grid object
     *
     * @param datahandle
     * @param origin_steps_shift_iline Integer number of steps in iline direction to shift origin by
     * @param origin_steps_shift_xline Integer number of steps in xline direction to shift origin by
     * @return Grid
     */
    Grid get_grid(DataHandle& datahandle, int origin_steps_shift_iline, int origin_steps_shift_xline) {
        const MetadataHandle& metadata = datahandle.get_metadata();

        auto cdp = metadata.bounding_box().world();

        auto nsteps_iline = metadata.iline().nsamples() - 1;
        auto nsteps_xline = metadata.xline().nsamples() - 1;

        auto iline_distance_x = cdp[1].first - cdp[0].first;
        auto iline_distance_y = cdp[1].second - cdp[0].second;
        auto xline_distance_x = cdp[3].first - cdp[0].first;
        auto xline_distance_y = cdp[3].second - cdp[0].second;

        double xori = cdp[0].first;
        double yori = cdp[0].second;
        const double xinc = std::hypot(iline_distance_x, iline_distance_y) / nsteps_iline;
        const double yinc = std::hypot(xline_distance_x, xline_distance_y) / nsteps_xline;
        const double rotation = std::atan2(iline_distance_y, iline_distance_x) * 180 / M_PI;

        xori += iline_distance_x * origin_steps_shift_iline / nsteps_iline +
                xline_distance_x * origin_steps_shift_xline / nsteps_xline;
        yori += iline_distance_y * origin_steps_shift_iline / nsteps_iline +
                xline_distance_y * origin_steps_shift_xline / nsteps_xline;

        return Grid(xori, yori, xinc, yinc, rotation);
    }

    void check_attribute(SurfaceBoundedSubVolume& subvolume, int low[], int high[], float factor) {
        return check_attribute(subvolume, low, high, low, high, factor);
    }

    /**
     * @brief Check attribute values against expected values
     *
     * @param subvolume
     * @param requested_low Low limit on axis for requested data. Array contains
     * 3 values corresponding to indicies in iline_array, xline_array,
     * sample_array.
     * @param requested_high High limit on axis for requested data. Exclusive
     * @param expected_low Low limit on axis for expected returned data.
     * @param expected_high High limit on axis for expected returned data. Exclusive
     * @param factor multiplicative factor (expected value * factor = actual
     * value). Expected value comes from rules used in file creation
     */
    void check_attribute(
        SurfaceBoundedSubVolume& subvolume, int requested_low[], int requested_high[],
        int expected_low[], int expected_high[], float factor
    ) {

        std::size_t nr_of_values = subvolume.nsamples(
            0, (requested_high[0] - requested_low[0]) * (requested_high[1] - requested_low[1])
        );
        EXPECT_EQ(
            nr_of_values,
            (expected_high[0] - expected_low[0]) *
                (expected_high[1] - expected_low[1]) *
                (expected_high[2] - expected_low[2])
        );

        int counter = 0;
        for (int il = requested_low[0]; il < requested_high[0]; ++il) {
            for (int xl = requested_low[1]; xl < requested_high[1]; ++xl) {
                RawSegment rs = subvolume.vertical_segment(counter);
                counter += 1;
                if (il < expected_low[0] || il >= expected_high[0] ||
                    xl < expected_low[1] || xl >= expected_high[1]) {
                    EXPECT_EQ(rs.size(), 0);
                    continue;
                }
                int s = expected_low[2];
                for (auto it = rs.begin(); it != rs.end(); ++it) {
                    int value = int(*it / factor + 0.5f);
                    int sample = value & 0xFF;
                    int xline = (value & 0xFF00) >> 8;
                    int iline = (value & 0xFF0000) >> 16;

                    EXPECT_EQ(iline, iline_array[il]) << "at iline " << il << " xline " << xl << " sample " << s;
                    EXPECT_EQ(xline, xline_array[xl]) << "at iline " << il << " xline " << xl << " sample " << s;
                    EXPECT_EQ(sample, sample_array[s]) << "at iline " << il << " xline " << xl << " sample " << s;
                    s += 1;
                }
                EXPECT_EQ(s, expected_high[2]);
            }
        }
    }
};

TEST_F(DatahandleAttributeTest, Attribute_Single) {

    DataHandle& datahandle = single_datahandle;
    Grid grid = get_grid(datahandle);
    const MetadataHandle* metadata = &(datahandle.get_metadata());

    std::size_t nrows = metadata->iline().nsamples();
    std::size_t ncols = metadata->xline().nsamples();
    static std::vector<float> top_surface_data(nrows * ncols, 28.0f);
    static std::vector<float> pri_surface_data(nrows * ncols, 36.0f);
    static std::vector<float> bot_surface_data(nrows * ncols, 52.0f);
    RegularSurface pri_surface = RegularSurface(pri_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface top_surface = RegularSurface(top_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface bot_surface = RegularSurface(bot_surface_data.data(), nrows, ncols, grid, fill);
    SurfaceBoundedSubVolume* subvolume = make_subvolume(datahandle.get_metadata(), pri_surface, top_surface, bot_surface);

    cppapi::fetch_subvolume(single_datahandle, *subvolume, NEAREST, 0, nrows * ncols);

    int low[3] = {0, 0, 4};
    int high[3] = {8, 8, 15};
    check_attribute(*subvolume, low, high, 1);

    delete subvolume;
}

TEST_F(DatahandleAttributeTest, Attribute_Double) {

    DoubleDataHandle& datahandle = double_datahandle;
    Grid grid = get_grid(datahandle);
    const MetadataHandle* metadata = &(datahandle.get_metadata());

    std::size_t nrows = metadata->iline().nsamples();
    std::size_t ncols = metadata->xline().nsamples();
    static std::vector<float> top_surface_data(nrows * ncols, 28.0f);
    static std::vector<float> pri_surface_data(nrows * ncols, 36.0f);
    static std::vector<float> bot_surface_data(nrows * ncols, 52.0f);
    RegularSurface pri_surface = RegularSurface(pri_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface top_surface = RegularSurface(top_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface bot_surface = RegularSurface(bot_surface_data.data(), nrows, ncols, grid, fill);
    SurfaceBoundedSubVolume* subvolume = make_subvolume(datahandle.get_metadata(), pri_surface, top_surface, bot_surface);

    cppapi::fetch_subvolume(datahandle, *subvolume, NEAREST, 0, nrows * ncols);

    int low[3] = {4, 4, 4};
    int high[3] = {8, 8, 15};
    check_attribute(*subvolume, low, high, 2);

    delete subvolume;
}

TEST_F(DatahandleAttributeTest, Attribute_Reverse_Double) {

    DataHandle& datahandle = double_reverse_datahandle;
    Grid grid = get_grid(datahandle);
    const MetadataHandle* metadata = &(datahandle.get_metadata());

    std::size_t nrows = metadata->iline().nsamples();
    std::size_t ncols = metadata->xline().nsamples();
    static std::vector<float> top_surface_data(nrows * ncols, 28.0f);
    static std::vector<float> pri_surface_data(nrows * ncols, 36.0f);
    static std::vector<float> bot_surface_data(nrows * ncols, 52.0f);
    RegularSurface pri_surface = RegularSurface(pri_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface top_surface = RegularSurface(top_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface bot_surface = RegularSurface(bot_surface_data.data(), nrows, ncols, grid, fill);
    SurfaceBoundedSubVolume* subvolume = make_subvolume(datahandle.get_metadata(), pri_surface, top_surface, bot_surface);

    cppapi::fetch_subvolume(datahandle, *subvolume, NEAREST, 0, nrows * ncols);

    int low[3] = {4, 4, 4};
    int high[3] = {8, 8, 15};
    check_attribute(*subvolume, low, high, 2);

    delete subvolume;
}

TEST_F(DatahandleAttributeTest, Attribute_Different_Size_Double) {

    DataHandle& datahandle = double_different_size;
    Grid grid = get_grid(datahandle);
    const MetadataHandle* metadata = &(datahandle.get_metadata());

    std::size_t nrows = metadata->iline().nsamples();
    std::size_t ncols = metadata->xline().nsamples();
    static std::vector<float> top_surface_data(nrows * ncols, 44.0f);
    static std::vector<float> pri_surface_data(nrows * ncols, 52.0f);
    static std::vector<float> bot_surface_data(nrows * ncols, 72.0f);
    RegularSurface pri_surface = RegularSurface(pri_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface top_surface = RegularSurface(top_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface bot_surface = RegularSurface(bot_surface_data.data(), nrows, ncols, grid, fill);
    SurfaceBoundedSubVolume* subvolume = make_subvolume(datahandle.get_metadata(), pri_surface, top_surface, bot_surface);

    cppapi::fetch_subvolume(datahandle, *subvolume, NEAREST, 0, nrows * ncols);

    int low[3] = {8, 8, 8};
    int high[3] = {12, 12, 20};
    check_attribute(*subvolume, low, high, 2);

    delete subvolume;
}


TEST_F(DatahandleAttributeTest, Attribute_Double_Data_Outside_Intersection_Horizontal_Plane) {
    DoubleDataHandle& datahandle = double_datahandle;

    /**
     * Usually in tests standard regular surface grid is perfectly aligned with
     * data, such that grid points fall on data points. We want to assure that
     * when user requests data which are outside of the actual data points, they
     * are ignored. The easiest way to do that is to extend our grid in all
     * directions by having some preceding and succeeding ilines/xlines.
     *
     */
    int preceding_ilines = 3;
    int succeeding_ilines = 1;
    int preceding_xlines = 1;
    int succeeding_xlines = 2;

    Grid grid = get_grid(datahandle, -preceding_ilines, -preceding_xlines);

    const MetadataHandle* metadata = &(datahandle.get_metadata());

    std::size_t nrows = metadata->iline().nsamples() + preceding_ilines + succeeding_ilines;
    std::size_t ncols = metadata->xline().nsamples() + preceding_xlines + succeeding_xlines;
    static std::vector<float> top_surface_data(nrows * ncols, 28.0f);
    static std::vector<float> pri_surface_data(nrows * ncols, 36.0f);
    static std::vector<float> bot_surface_data(nrows * ncols, 52.0f);
    RegularSurface pri_surface = RegularSurface(pri_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface top_surface = RegularSurface(top_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface bot_surface = RegularSurface(bot_surface_data.data(), nrows, ncols, grid, fill);
    SurfaceBoundedSubVolume* subvolume = make_subvolume(datahandle.get_metadata(), pri_surface, top_surface, bot_surface);

    cppapi::fetch_subvolume(datahandle, *subvolume, NEAREST, 0, nrows * ncols);

    int expected_low[3] = {4, 4, 4};
    int expected_high[3] = {8, 8, 15};

    int requested_low[3] = {expected_low[0] - preceding_ilines, expected_low[1] - preceding_xlines, expected_low[2]};
    int requested_high[3] = {expected_high[0] + succeeding_ilines, expected_high[1] + succeeding_xlines, expected_high[2]};

    check_attribute(*subvolume, requested_low, requested_high, expected_low, expected_high, 2);

    delete subvolume;
}

TEST_F(DatahandleAttributeTest, Attribute_Double_Data_Outside_Intersection_Vertical_Plane) {

    DoubleDataHandle& datahandle = double_datahandle;
    Grid grid = get_grid(datahandle);
    const MetadataHandle* metadata = &(datahandle.get_metadata());

    std::size_t nrows = metadata->iline().nsamples();
    std::size_t ncols = metadata->xline().nsamples();
    // lowest sample belonging to intersection is 20
    static std::vector<float> top_surface_data(nrows * ncols, 16.0f);
    static std::vector<float> pri_surface_data(nrows * ncols, 36.0f);
    static std::vector<float> bot_surface_data(nrows * ncols, 52.0f);
    RegularSurface pri_surface = RegularSurface(pri_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface top_surface = RegularSurface(top_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface bot_surface = RegularSurface(bot_surface_data.data(), nrows, ncols, grid, fill);

    EXPECT_THAT(
        [&]() {
            make_subvolume(datahandle.get_metadata(), pri_surface, top_surface, bot_surface);
        },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Vertical window is out of vertical bounds"))
    );
}

} // namespace
