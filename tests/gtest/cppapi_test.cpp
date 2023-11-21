#include <map>

#include "cppapi.hpp"
#include "ctypes.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace
{
const std::string SAMPLES_10 = "file://10_samples_default.vds";
const std::string CREDENTIALS = "";

Grid samples_10_grid = Grid(2, 0, 7.2111, 3.6056, 33.69);
Grid larger_grid = Grid(-8, -11, 7.2111, 3.6056, 33.69);
Grid other_grid = Grid(2, 3, 4.472, 2.236, 333.43);

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

std::map<Samples10Points, std::vector<float>> samples_10_data(float fill)
{
    std::map<Samples10Points, std::vector<float>> points;
    points[i1_x10] = {-4.5, -3.5, -2.5, -1.5, -0.5, 0.5, 1.5, 2.5, 3.5, 4.5};
    points[i1_x11] = {4.5, 3.5, 2.5, 1.5, 0.5, -0.5, -1.5, -2.5, -3.5, -4.5};
    points[i3_x10] = {25.5, -14.5, -12.5, -10.5, -8.5, -6.5, -4.5, -2.5, -0.5, 25.5};
    points[i3_x11] = {25.5, 0.5, 2.5, 4.5, 6.5, 8.5, 10.5, 12.5, 14.5, 25.5};
    points[i5_x10] = {25.5, 4.5, 8.5, 12.5, 16.5, 20.5, 24.5, 20.5, 16.5, 8.5};
    points[i5_x11] = {25.5, -4.5, -8.5, -12.5, -16.5, -20.5, -24.5, -20.5, -16.5, -8.5};
    points[nodata] = {fill};
    return points;
}

class SubvolumeTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        datasource = make_single_datasource(SAMPLES_10.c_str(), CREDENTIALS.c_str());
    }

    void TearDown() override
    {
        delete datasource;
    }

    SingleDataSource *datasource;

    static constexpr float fill = -999.25;
    std::map<float, std::vector<float>> points;

    void test_successful_subvolume_call(
        RegularSurface const &primary_surface,
        RegularSurface const &top_surface,
        RegularSurface const &bottom_surface,
        const Samples10Points *expected_data
    ) {
        auto size = primary_surface.grid().size();

        SurfaceBoundedSubVolume* subvolume = make_subvolume(
            datasource->get_metadata(), primary_surface, top_surface, bottom_surface
        );

        cppapi::fetch_subvolume(*datasource, *subvolume, NEAREST, 0, size);

        /* We are checking here points unordered. Meaning that if all points in a
        * row appear somewhere in the horizon, we assume we are good. Alternative
        * is to check all points explicitly, but for this we need to know margin
        * logic, which is too private to the code. Current solution seems to check
        * good enough that for unaligned surface data from correct trace is
        * returned.
        */
        auto points = samples_10_data(primary_surface.fillvalue());
        for (int i = 0; i < size; ++i)
        {
            auto expected_position = expected_data[i];
            if (expected_position == nodata) {
                ASSERT_TRUE(subvolume->is_empty(i))
                    << "Expected empty subvolume at position " << i;
                continue;
            }
            for (auto it = subvolume->vertical_segment(i).begin(); it != subvolume->vertical_segment(i).end(); ++it)
            {
                ASSERT_THAT(points.at(expected_position), ::testing::Contains(*it))
                    << "Retrieved value " << *it << " is not expected at position " << i;
            }
        }
        delete subvolume;
    }
};

TEST_F(SubvolumeTest, SubvolumeSize)
{
    static constexpr int nrows = 3;
    static constexpr int ncols = 2;
    static constexpr std::size_t size = nrows * ncols;

    // 16, 20 and 24 fall on samples

    std::array<float, size> primary_surface_data = {
        20, 20,
        20, 20,
        20, 19,
    };

    std::array<float, size> top_surface_data = {
        20, 17,
        16, 17,
        16, 16,
    };

    std::array<float, size> bottom_surface_data = {
        20, 23,
        23, 24,
        24, 24,
    };

    /**
     * Note that in theory these sizes are not optimal:
     * - depending on the interpolation algorithm we might need several
     *   additional samples to interpolate points on the borders the best way
     *   possible. Current belief is that interpolation algorithm needs 2
     *   additional samples from one side to interpolate properly. That means
     *   that if border already falls on the sample, we might be fetching too
     *   much. However it seems like in practice implementing such behavior
     *   gives no performance benefit and only complicates code and creates
     *   problems when use case requires 1 or 3 samples as interpolation
     *   algorithm wants at least 4.
     */
    std::array<float, size> expected_fetched_size = {
        5, 5,
        6, 6,
        7, 7,
    };

    RegularSurface primary_surface =
        RegularSurface(primary_surface_data.data(), nrows, ncols, samples_10_grid, fill);

    RegularSurface top_surface =
        RegularSurface(top_surface_data.data(), nrows, ncols, samples_10_grid, fill);

    RegularSurface bottom_surface =
        RegularSurface(bottom_surface_data.data(), nrows, ncols, samples_10_grid, fill);

    SurfaceBoundedSubVolume* subvolume = make_subvolume(
        datasource->get_metadata(), primary_surface, top_surface, bottom_surface
    );

    cppapi::fetch_subvolume(*datasource, *subvolume, NEAREST, 0, size);
    for (int i = 0; i < size; ++i)
    {
        EXPECT_EQ(expected_fetched_size[i], subvolume->vertical_segment(i).size())
            << "Retrieved segment size not as expected at position " << i;
    }

    delete subvolume;
}

TEST_F(SubvolumeTest, DataForUnalignedSurface)
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

    const std::array<Samples10Points, size> expected = {
        i1_x11, i3_x11, i3_x11, nodata, nodata, nodata,
        i3_x10, i3_x10, i3_x10, i3_x11, i5_x11, i5_x11,
        nodata, nodata, nodata, nodata, i5_x10, i5_x10,
        nodata, nodata, nodata, nodata, nodata, nodata
    };

    RegularSurface primary_surface =
        RegularSurface(surface_data.data(), nrows, ncols, other_grid, fill);

    std::array<float, size> above_data = surface_data;
    std::array<float, size> below_data = surface_data;

    std::transform(above_data.cbegin(), above_data.cend(), above_data.begin(),
                  [above](float value) { return value - above; });
    std::transform(below_data.cbegin(), below_data.cend(), below_data.begin(),
                   [below](float value) { return value + below; });

    RegularSurface top_surface =
        RegularSurface(above_data.data(), nrows, ncols, other_grid, fill);

    RegularSurface bottom_surface =
        RegularSurface(below_data.data(), nrows, ncols, other_grid, fill);

    test_successful_subvolume_call(primary_surface, top_surface, bottom_surface, expected.data());
}

TEST_F(SubvolumeTest, ManyFills)
{
    static constexpr int nrows = 6;
    static constexpr int ncols = 4;
    static constexpr std::size_t size = nrows * ncols;

    std::array<float, size> primary_surface_data = {
        fill, 20,   20,   fill,
        24,   24,   24,   24,
        fill, 16,   16,   16,
        fill, fill, fill, fill,
        16,   20,   24,   28,
        60,   40,   80,   100
    };

    std::array<float, size> top_surface_data = {
        fill, 20,   20,   fill,
        24,   24,   24,   24,
        fill, 12,   fill, 16,
        fill, fill, fill, fill,
        16,   20,   16,   28,
        60,   fill, 80,   100
    };

    std::array<float, size> bottom_surface_data = {
        fill, 20,   fill, fill,
        24,   24,   24,   24,
        fill, 32,   16,   16,
        fill, fill, fill, fill,
        16,   fill, 24,   28,
        60,   40,   80,   100
    };

    std::array<Samples10Points, size> expected = {
        nodata, nodata, nodata, nodata,
        nodata, nodata, nodata, nodata,
        nodata, i1_x10, nodata, nodata,
        nodata, nodata, nodata, nodata,
        nodata, nodata, i5_x11, nodata,
        nodata, nodata, nodata, nodata
    };

    RegularSurface primary_surface =
        RegularSurface(primary_surface_data.data(), nrows, ncols, larger_grid, fill);

    RegularSurface top_surface =
        RegularSurface(top_surface_data.data(), nrows, ncols, larger_grid, fill);

    RegularSurface bottom_surface =
        RegularSurface(bottom_surface_data.data(), nrows, ncols, larger_grid, fill);

    test_successful_subvolume_call(primary_surface, top_surface, bottom_surface, expected.data());
}


class SurfaceAlignmentTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        datasource = make_single_datasource(SAMPLES_10.c_str(), CREDENTIALS.c_str());
    }

    void TearDown() override
    {
        delete datasource;
    }

    SingleDataSource *datasource;

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
        EXPECT_EQ(aligned[i], expected_data[i]) << "Wrong surface at index " << i;
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
        data.data(), primary.grid(), secondary.fillvalue());

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
        primary_surface_data.data(), nrows, ncols, samples_10_grid, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), nrows, ncols, samples_10_grid, fill);

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
        primary_surface_data.data(), nrows, ncols, samples_10_grid, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), nrows, ncols, samples_10_grid, fill);

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
        primary_surface_data.data(), pnrows, pncols, samples_10_grid, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_grid, fill);

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
        primary_surface_data.data(), pnrows, pncols, samples_10_grid, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_grid, fill);

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
        primary_surface_data.data(), pnrows, pncols, samples_10_grid, fill1);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_grid, fill2);

    std::vector< float> data(primary.size());
    RegularSurface aligned = RegularSurface(
        data.data(), primary.grid(), fill3);

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
        primary_surface_data.data(), pnrows, pncols, larger_grid, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_grid, fill);

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
        primary_surface_data.data(), pnrows, pncols, samples_10_grid, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, other_grid, fill);

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
        primary_surface_data.data(), pnrows, pncols, other_grid, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_grid, fill);

    test_successful_align_call(primary, secondary, expected.data(), true);
}

TEST_F(SurfaceAlignmentTest, IdenticalSurfaces)
{
    static constexpr std::size_t pnrows = 3;
    static constexpr std::size_t pncols = 2;

    std::array<float, pnrows * pncols> primary_surface_data = {
        20, 20,
        20, 20,
        20, 20
    };

    static constexpr std::size_t snrows = 3;
    static constexpr std::size_t sncols = 2;

    std::array<float, snrows * sncols> secondary_surface_data = {
        20, 20,
        20, 20,
        20, 20
    };

    const std::array<float, pnrows * pncols> expected = {
        20, 20,
        20, 20,
        20, 20
    };
    // current behavior, but value shouldn't matter
    const bool expected_primary_is_top = false;

    RegularSurface primary = RegularSurface(
        primary_surface_data.data(), pnrows, pncols, samples_10_grid, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_grid, fill);

    test_successful_align_call(primary, secondary, expected.data(), expected_primary_is_top);
}

TEST_F(SurfaceAlignmentTest, SameStartingValue)
{
    static constexpr std::size_t pnrows = 2;
    static constexpr std::size_t pncols = 2;

    std::array<float, pnrows * pncols> primary_surface_data = {
        20, 20,
        21, 20,
    };

    static constexpr std::size_t snrows = 2;
    static constexpr std::size_t sncols = 2;

    std::array<float, snrows * sncols> secondary_surface_data = {
        20, 20,
        20, 20,
    };

    const std::array<float, pnrows * pncols> expected = {
        20, 20,
        20, 20,
    };

    RegularSurface primary = RegularSurface(
        primary_surface_data.data(), pnrows, pncols, samples_10_grid, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, samples_10_grid, fill);

    test_successful_align_call(primary, secondary, expected.data(), false);
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
        primary_surface_data.data(), pnrows, pncols, samples_10_grid, fill);
    RegularSurface secondary = RegularSurface(
        secondary_surface_data.data(), snrows, sncols, other_grid, fill);

    std::vector< float> data(primary.size());
    RegularSurface aligned = RegularSurface(
        data.data(), pnrows, pncols, samples_10_grid, fill);
    bool primary_is_top;

    EXPECT_THAT(
        [&]() { cppapi::align_surfaces(primary, secondary, aligned, &primary_is_top); },
        testing::ThrowsMessage<std::runtime_error>(
            testing::HasSubstr("Surfaces intersect at primary surface point (2, 0)")));
}

} // namespace
