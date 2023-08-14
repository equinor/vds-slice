#include <map>

#include "cppapi.hpp"
#include "ctypes.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace
{
const std::string SAMPLES_10 = "file://10_samples_default.vds";
const std::string CREDENTIALS = "";

Plane samples_10_plane = Plane(2, 0, 7.2111, 3.6056, 33.69);
Plane larger_plane = Plane(-8, -11, 7.2111, 3.6056, 33.69);
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

    RegularSurface primary_surface =
        RegularSurface(surface_data.data(), nrows, ncols, other_plane, fill);

    std::array<float, size> above_data = surface_data;
    std::array<float, size> below_data = surface_data;

    std::transform(above_data.cbegin(), above_data.cend(), above_data.begin(),
                  [above](float value) { return value - above; });
    std::transform(below_data.cbegin(), below_data.cend(), below_data.begin(),
                   [below](float value) { return value + below; });

    RegularSurface top_surface =
        RegularSurface(above_data.data(), nrows, ncols, other_plane, fill);

    RegularSurface bottom_surface =
        RegularSurface(below_data.data(), nrows, ncols, other_plane, fill);

    constexpr std::size_t offset_size = size+1;
    std::array<std::size_t, offset_size> offsets;
    cppapi::horizon_buffer_offsets(*handle, primary_surface, top_surface, bottom_surface,
                           offsets.data(), offset_size);

    std::size_t horizon_size = offsets[size];
    std::vector< float> res(horizon_size);
    cppapi::horizon(*handle, primary_surface, top_surface, bottom_surface,
                           offsets.data(), NEAREST, 0, size, res.data());

    Horizon horizon(res.data(), size, offsets.data(), primary_surface.fillvalue());

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

class SurfaceAlignmentTest : public ::testing::Test
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
};

void test_successful_align_call(
    RegularSurface const &primary,
    RegularSurface const &secondary,
    RegularSurface &aligned,
    const float *expected_data,
    bool expected_primary_is_top
) {
    bool primary_is_top;
    cppapi::align_surfaces(primary, secondary, aligned, &primary_is_top);

    EXPECT_EQ(primary_is_top, expected_primary_is_top) << "Wrong column number";

    for (int i = 0; i < primary.size(); ++i)
    {
        EXPECT_EQ(aligned.value(i), expected_data[i]) << "Wrong surface at index " << i;
    }
}

void test_successful_align_call(
    RegularSurface const &primary,
    RegularSurface const &secondary,
    const float *expected_data,
    bool expected_primary_is_top
) {
    std::vector< float> data(primary.size());
    RegularSurface aligned = RegularSurface(
        data.data(), primary.nrows(), primary.ncols(), primary.plane(), secondary.fillvalue());

    test_successful_align_call(primary, secondary, aligned, expected_data, expected_primary_is_top);
}

TEST_F(SurfaceAlignmentTest, AlignedSurfaces)
{
    static constexpr std::size_t nrows = 3;
    static constexpr std::size_t ncols = 2;
    std::array<float, nrows * ncols> primary_surface_data = {
        20, 20,
        20, 20,
        20, 20
    };
    std::array<float, nrows * ncols> secondary_surface_data = {
        24, 25,
        26, 27,
        28, 29
    };

    RegularSurface primary = RegularSurface(
        primary_surface_data.data(), nrows, ncols, samples_10_plane, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), nrows, ncols, samples_10_plane, fill);

    test_successful_align_call(primary, secondary, secondary_surface_data.data(), true);
}

TEST_F(SurfaceAlignmentTest, PrimaryIsBottom)
{
    static constexpr std::size_t nrows = 3;
    static constexpr std::size_t ncols = 2;
    std::array<float, nrows * ncols> primary_surface_data = {
        30, 30,
        30, 30,
        30, 30
    };
    std::array<float, nrows * ncols> secondary_surface_data = {
        24, 25,
        26, 27,
        28, 29
    };

    RegularSurface primary = RegularSurface(
        primary_surface_data.data(), nrows, ncols, samples_10_plane, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), nrows, ncols, samples_10_plane, fill);

    test_successful_align_call(primary, secondary, secondary_surface_data.data(), false);
}

TEST_F(SurfaceAlignmentTest, SecondaryLarger)
{
    static constexpr std::size_t pnrows = 2;
    static constexpr std::size_t pncols = 1;

    std::array<float, pnrows * pncols> primary_surface_data = {
        20,
        20
    };

    static constexpr std::size_t snrows = 3;
    static constexpr std::size_t sncols = 2;

    std::array<float, snrows * sncols> secondary_surface_data = {
        24, 25,
        26, 27,
        28, 29
    };

    const std::array<float, pnrows * pncols> expected = {
        24,
        26
    };

    RegularSurface primary = RegularSurface(
        primary_surface_data.data(), pnrows, pncols, samples_10_plane, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_plane, fill);

    test_successful_align_call(primary, secondary, expected.data(), true);
}

TEST_F(SurfaceAlignmentTest, PrimaryLarger)
{
    static constexpr std::size_t pnrows = 3;
    static constexpr std::size_t pncols = 2;

    std::array<float, pnrows * pncols> primary_surface_data = {
        20, 20,
        20, 20,
        20, 20
    };

    static constexpr std::size_t snrows = 2;
    static constexpr std::size_t sncols = 1;

    std::array<float, snrows * sncols> secondary_surface_data = {
        24,
        26,
    };

    const std::array<float, pnrows * pncols> expected = {
        24, fill,
        26, fill,
        fill, fill
    };

    RegularSurface primary = RegularSurface(
        primary_surface_data.data(), pnrows, pncols, samples_10_plane, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_plane, fill);

    test_successful_align_call(primary, secondary, expected.data(), true);
}

TEST_F(SurfaceAlignmentTest, HolesInData)
{
    static constexpr std::size_t pnrows = 3;
    static constexpr std::size_t pncols = 2;

    float fill1 = 111.11;
    float fill2 = 222.22;
    float fill3 = 333.33;

    std::array<float, pnrows * pncols> primary_surface_data = {
        20, 20,
        fill1, fill1,
        20, 20
    };

    static constexpr std::size_t snrows = 3;
    static constexpr std::size_t sncols = 2;

    std::array<float, snrows * sncols> secondary_surface_data = {
        fill2, 25,
        26, fill2,
        fill2, 29
    };

    const std::array<float, pnrows * pncols> expected = {
        fill3, 25,
        fill3, fill3,
        fill3, 29
    };

    RegularSurface primary = RegularSurface(
        primary_surface_data.data(), pnrows, pncols, samples_10_plane, fill1);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_plane, fill2);

    std::vector< float> data(primary.size());
    RegularSurface aligned = RegularSurface(
        data.data(), primary.nrows(), primary.ncols(), primary.plane(), fill3);

    test_successful_align_call(primary, secondary, aligned, expected.data(), true);
}

TEST_F(SurfaceAlignmentTest, SurfaceDifferentOrigin)
{
    static constexpr std::size_t pnrows = 6;
    static constexpr std::size_t pncols = 4;

    std::array<float, pnrows * pncols> primary_surface_data = {
        10, 10, 10, 10,
        10, 10, 10, 10,
        10, 10, 10, 10,
        10, 10, 10, 10,
        10, 10, 10, 10,
        10, 10, 10, 10
    };

    static constexpr std::size_t snrows = 3;
    static constexpr std::size_t sncols = 2;

    std::array<float, snrows * sncols> secondary_surface_data = {
        24, 25,
        26, 27,
        28, 29,
    };

    const std::array<float, pnrows * pncols> expected = {
        fill, fill, fill, fill,
        fill, fill, fill, fill,
        fill, 24,   25,   fill,
        fill, 26,   27,   fill,
        fill, 28,   29,   fill,
        fill, fill, fill, fill,
    };

    RegularSurface primary = RegularSurface(
        primary_surface_data.data(), pnrows, pncols, larger_plane, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_plane, fill);

    test_successful_align_call(primary, secondary, expected.data(), true);
}

TEST_F(SurfaceAlignmentTest, UnalignedSurfacesPrimaryAligned)
{
    static constexpr std::size_t pnrows = 3;
    static constexpr std::size_t pncols = 2;

    std::array<float, pnrows * pncols> primary_surface_data = {
        5, 5,
        5, 5,
        5, 5
    };

    static constexpr std::size_t snrows = 4;
    static constexpr std::size_t sncols = 6;

    std::array<float, snrows * sncols> secondary_surface_data = {
        10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21,
        22, 23, 24, 25, 26, 27,
        28, 29, 30, 31, 32, 33
    };

    const std::array<float, pnrows * pncols> expected = {
        fill, 10,
        18, 12,
        26, 21
    };

    RegularSurface primary = RegularSurface(
        primary_surface_data.data(), pnrows, pncols, samples_10_plane, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, other_plane, fill);

    test_successful_align_call(primary, secondary, expected.data(), true);
}

TEST_F(SurfaceAlignmentTest, UnalignedSurfacesPrimaryUnaligned)
{
    static constexpr std::size_t pnrows = 4;
    static constexpr std::size_t pncols = 6;

    std::array<float, pnrows * pncols> primary_surface_data = {
        20, 20, 20, 20, 20, 20,
        20, 20, 20, 20, 20, 20,
        20, 20, 20, 20, 20, 20,
        20, 20, 20, 20, 20, 20,
    };

    static constexpr std::size_t snrows = 3;
    static constexpr std::size_t sncols = 2;

    std::array<float, snrows * sncols> secondary_surface_data = {
        24, 25,
        26, 27,
        28, 29
    };

    const std::array<float, pnrows * pncols> expected = {
        25, 27, 27, fill, fill, fill,
        26, 26, 26, 27, 29, 29,
        fill, fill, fill, fill, 28, 28,
        fill, fill, fill, fill, fill, fill
    };

    RegularSurface primary = RegularSurface(
        primary_surface_data.data(), pnrows, pncols, other_plane, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_plane, fill);

    test_successful_align_call(primary, secondary, expected.data(), true);
}

TEST_F(SurfaceAlignmentTest, IntersectingSurfaces)
{
    static constexpr std::size_t pnrows = 3;
    static constexpr std::size_t pncols = 2;

    std::array<float, pnrows * pncols> primary_surface_data = {
        35, 5, // fill, less
        18, 8, // equals, less
        27, 20 // greater, less
    };

    static constexpr std::size_t snrows = 4;
    static constexpr std::size_t sncols = 6;

    std::array<float, snrows * sncols> secondary_surface_data = {
        10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21,
        22, 23, 24, 25, 26, 27,
        28, 29, 30, 31, 32, 33
    };

    RegularSurface primary = RegularSurface(
        primary_surface_data.data(), pnrows, pncols, samples_10_plane, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, other_plane, fill);

    std::vector< float> data(primary.size());
    RegularSurface aligned = RegularSurface(
        data.data(), pnrows, pncols, samples_10_plane, fill);
    bool primary_is_top;

    EXPECT_THAT(
        [&]() { cppapi::align_surfaces(primary, secondary, aligned, &primary_is_top); },
        testing::ThrowsMessage<std::runtime_error>(
            testing::HasSubstr("Surfaces intersect at primary surface point (2, 0)")));
}

} // namespace
