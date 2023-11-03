#include "cppapi.hpp"
#include "ctypes.h"

#include <gmock/gmock.h>

namespace {

const std::string SAMPLES_10 = "file://10_samples_default.vds";

const std::string CREDENTIALS = "";

Grid samples_10_grid = Grid(2, 0, 7.2111, 3.6056, 33.69);

class SubCubeTest : public ::testing::Test {
protected:
    void SetUp() override {
        datasource_A = make_ovds_datasource(
            SAMPLES_10.c_str(),
            CREDENTIALS.c_str());

        subvolume_A = make_subvolume(
            datasource_A->get_metadata(), primary_surface, top_surface, bottom_surface);

        cppapi::fetch_subvolume(*datasource_A, *subvolume_A, NEAREST, 0, size);
    }

    void TearDown() override {
        delete subvolume_A;
        delete datasource_A;
    }

    OvdsDataSource *datasource_A;
    SurfaceBoundedSubVolume *subvolume_A;
    static constexpr int nrows = 3;
    static constexpr int ncols = 2;
    static constexpr std::size_t size = nrows * ncols;

    std::array<float, size> top_surface_data = {16, 16, 16, 16, 16, 16};
    std::array<float, nrows *ncols> primary_surface_data = {20, 20, 20, 20, 20, 20};
    std::array<float, size> bottom_surface_data = {24, 24, 24, 24, 24, 24};

    static constexpr float fill = -999.25;

    RegularSurface primary_surface =
        RegularSurface(primary_surface_data.data(), nrows, ncols, samples_10_grid, fill);

    RegularSurface top_surface =
        RegularSurface(top_surface_data.data(), nrows, ncols, samples_10_grid, fill);

    RegularSurface bottom_surface =
        RegularSurface(bottom_surface_data.data(), nrows, ncols, samples_10_grid, fill);
};

TEST_F(SubCubeTest, SizeBaseCase) {
    // Arrange
    MetadataHandle metadata = datasource_A->get_metadata();

    SubCube subcube(metadata);

    int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
    int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};

    std::copy(lower, lower + sizeof(lower) / sizeof(lower[0]), subcube.bounds.lower);
    std::copy(upper, upper + sizeof(upper) / sizeof(upper[0]), subcube.bounds.upper);

    // Act
    std::size_t size = subcube.size();

    // Assert
    EXPECT_EQ(size, 1);
}

TEST_F(SubCubeTest, SizeAllNegative) {
    // Arrange
    MetadataHandle metadata = datasource_A->get_metadata();

    SubCube subcube(metadata);

    int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{-34, -34, -34, -34, -34, -34};
    int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{-36, -35, -35, -37, -35, -35};

    std::copy(lower, lower + sizeof(lower) / sizeof(lower[0]), subcube.bounds.lower);
    std::copy(upper, upper + sizeof(upper) / sizeof(upper[0]), subcube.bounds.upper);

    // Act
    std::size_t size = subcube.size();

    // Assert
    EXPECT_EQ(size, 6);
}

TEST_F(SubCubeTest, SizeAllDimensions) {
    // Arrange
    MetadataHandle metadata = datasource_A->get_metadata();

    SubCube subcube(metadata);

    int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
    int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{3, 3, 3, 3, 3, 3};

    std::copy(lower, lower + sizeof(lower) / sizeof(lower[0]), subcube.bounds.lower);
    std::copy(upper, upper + sizeof(upper) / sizeof(upper[0]), subcube.bounds.upper);

    // Act
    std::size_t size = subcube.size();

    // Assert
    EXPECT_EQ(size, 4 * 4 * 4);
}

} // namespace
