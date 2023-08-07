#include <map>

#include "cppapi.hpp"
#include "ctypes.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace
{
const std::string SAMPLES_10 = "file://10_samples_default.vds";
const std::string CREDENTIALS = "";

Plane other_plane = Plane(2, 3, 4.472, 2.236, 333.43);

class HorizonTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        handle = make_datahandle(SAMPLES_10.c_str(), CREDENTIALS.c_str());
    }

    void TearDown() override
    {
        delete handle;
    }

    DataHandle *handle;

    static constexpr float fill = -999.25;
    std::map<float, std::vector<float>> points;
};

enum Samples10Points
{
    i1_x10,
    i1_x11,
    i3_x10,
    i3_x11,
    i5_x10,
    i5_x11,
    nodata
};

std::map<float, std::vector<float>> samples_10_data(float fill)
{
    std::map<float, std::vector<float>> points;
    points[i1_x10] = {-4.5, -3.5, -2.5, -1.5, -0.5, 0.5, 1.5, 2.5, 3.5, 4.5};
    points[i1_x11] = {4.5, 3.5, 2.5, 1.5, 0.5, -0.5, -1.5, -2.5, -3.5, -4.5};
    points[i3_x10] = {25.5, -14.5, -12.5, -10.5, -8.5, -6.5, -4.5, -2.5, -0.5, 25.5};
    points[i3_x11] = {25.5, 0.5, 2.5, 4.5, 6.5, 8.5, 10.5, 12.5, 14.5, 25.5};
    points[i5_x10] = {25.5, 4.5, 8.5, 12.5, 16.5, 20.5, 24.5, 20.5, 16.5, 8.5};
    points[i5_x11] = {25.5, -4.5, -8.5, -12.5, -16.5, -20.5, -24.5, -20.5, -16.5, -8.5};
    points[nodata] = {fill};
    return points;
}

TEST_F(HorizonTest, DataForUnalignedSurface)
{
    const float above = 2;
    const float below = 2;

    static constexpr int nrows = 4;
    static constexpr int ncols = 6;
    static constexpr std::size_t size = nrows * ncols;

    std::array<float, size> surface_data = {
        24, 20, 24, 24, 24, 20,
        20, 20, 20, 24, 20, 24,
        20, 24, 20, 20, 24, 20,
        24, 24, 24, 24, 20, 24
    };

    const std::array<float, size> expected = {
        i1_x11, i3_x11, i3_x11, nodata, nodata, nodata,
        i3_x10, i3_x10, i3_x10, i3_x11, i5_x11, i5_x11,
        nodata, nodata, nodata, nodata, i5_x10, i5_x10,
        nodata, nodata, nodata, nodata, nodata, nodata
    };

    RegularSurface surface =
        RegularSurface(surface_data.data(), nrows, ncols, other_plane, fill);

    std::size_t horizon_size;
    cppapi::horizon_size(*handle, surface, above, below, &horizon_size);

    std::vector< float> res(horizon_size);
    cppapi::horizon(*handle, surface, above, below, NEAREST, 0, size, res.data());

    std::size_t vsize = horizon_size / (surface.size() * sizeof(float));
    Horizon horizon(res.data(), size, vsize, surface.fillvalue());

    /* We are checking here points unordered. Meaning that if all points in a
     * row appear somewhere in the horizon, we assume we are good. Alternative
     * is to check all points explicitly, but for this we need to know margin
     * logic, which is too private to the code. Current solution seems to check
     * good enough that for unaligned surface data from correct trace is
     * returned.
     */
    auto points = samples_10_data(fill);
    for (int i = 0; i < nrows * ncols; ++i)
    {
        auto expected_position = expected[i];
        for (auto it = horizon.at(i).begin(); it != horizon.at(i).end(); ++it)
        {
            ASSERT_THAT(points.at(expected_position), ::testing::Contains(*it))
                << "Retrieved value " << *it << " is not expected at position " << i;
        }
    }
}
} // namespace
