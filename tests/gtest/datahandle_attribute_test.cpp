#include "cppapi.hpp"
#include "ctypes.h"
#include <iostream>
#include <sstream>

#include "test_utils.hpp"

#include "attribute.hpp"
#include "metadatahandle.hpp"
#include "subvolume.hpp"
#include "utils.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

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

Grid get_grid(DataHandle& datahandle) {
    return get_grid(datahandle, 0, 0);
}

TEST_F(DatahandleCubeIntersectionTest, Attribute_Single) {

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

    int low[3] = {3, 2, 28};
    int high[3] = {24, 16, 52};
    check_attribute(*subvolume, low, high, 1);

    delete subvolume;
}

TEST_F(DatahandleCubeIntersectionTest, Attribute_Double) {

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

    int low[3] = {15, 10, 28};
    int high[3] = {24, 16, 52};
    check_attribute(*subvolume, low, high, 2);

    delete subvolume;
}

TEST_F(DatahandleCubeIntersectionTest, Attribute_Reverse_Double) {

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

    int low[3] = {15, 10, 28};
    int high[3] = {24, 16, 52};
    check_attribute(*subvolume, low, high, 2);

    delete subvolume;
}

TEST_F(DatahandleCubeIntersectionTest, Attribute_Different_Size_Double) {

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

    int low[3] = {27, 18, 44};
    int high[3] = {36, 24, 72};
    check_attribute(*subvolume, low, high, 2);

    delete subvolume;
}

TEST_F(DatahandleCubeIntersectionTest, Attribute_Double_Data_Outside_Intersection_Horizontal_Plane) {
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

    int expected_low[3] = {15, 10, 28};
    int expected_high[3] = {24, 16, 52};

    int requested_low[3] = {
        expected_low[0] - preceding_ilines * ILINE_STEP,
        expected_low[1] - preceding_xlines * XLINE_STEP,
        expected_low[2]
    };
    int requested_high[3] = {
        expected_high[0] + succeeding_ilines * ILINE_STEP,
        expected_high[1] + succeeding_xlines * XLINE_STEP,
        expected_high[2]
    };

    check_attribute(*subvolume, requested_low, requested_high, expected_low, expected_high, 2);

    delete subvolume;
}

TEST_F(DatahandleCubeIntersectionTest, Attribute_Double_Data_Outside_Intersection_Vertical_Plane) {

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

TEST_F(DatahandleCubeIntersectionTest, Attribute_Double_Different_Number_Of_Samples_To_Border_Per_Dimension) {
    const std::string INNER_CUBE = "file://inner_4x2_cube.vds";

    DoubleDataHandle datahandle = make_double_datahandle(
        INNER_CUBE.c_str(),
        CREDENTIALS.c_str(),
        REGULAR_DATA.c_str(),
        CREDENTIALS.c_str(),
        binary_operator::ADDITION
    );

    Grid grid = get_grid(datahandle);

    const MetadataHandle* metadata = &(datahandle.get_metadata());

    std::size_t nrows = metadata->iline().nsamples();
    std::size_t ncols = metadata->xline().nsamples();
    static std::vector<float> top_surface_data(nrows * ncols, 16.0f);
    static std::vector<float> pri_surface_data(nrows * ncols, 20.0f);
    static std::vector<float> bot_surface_data(nrows * ncols, 28.0f);
    RegularSurface pri_surface = RegularSurface(pri_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface top_surface = RegularSurface(top_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface bot_surface = RegularSurface(bot_surface_data.data(), nrows, ncols, grid, fill);
    SurfaceBoundedSubVolume* subvolume = make_subvolume(datahandle.get_metadata(), pri_surface, top_surface, bot_surface);

    cppapi::fetch_subvolume(datahandle, *subvolume, NEAREST, 0, nrows * ncols);

    /* For regular files "low" in other double tests is something like {4, 4, 4}
     * in index coordinate system; The files which create this test intersection
     * are crafted such a way that in every dimension "low" is different, thus
     * checking for the possible mess up in values order.
     */
    int expected_low[3] = {9, 8, 16};
    int expected_high[3] = {18, 14, 28};

    check_attribute(*subvolume, expected_low, expected_high, 2);

    delete subvolume;
}

} // namespace
