#include "cppapi.hpp"
#include "ctypes.h"
#include <iostream>
#include <sstream>

#include "attribute.hpp"
#include "metadatahandle.hpp"
#include "nlohmann/json.hpp"
#include "subvolume.hpp"
#include "utils.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {
const std::string REGULAR_DATA = "file://regular_8x3_cube.vds";
const std::string SHIFT_4_DATA = "file://shift_4_8x3_cube.vds";

const std::string CREDENTIALS = "";

double norm(const std::vector<double>& vec) {
    double sum = 0.0;
    for (double component : vec) {
        sum += std::pow(component, 2);
    }
    return std::sqrt(sum);
}

class DoubleVolumeDataLayoutTest : public ::testing::Test {

    void SetUp() override {
        expected_intersect_metadata["crs"] = "\"utmXX\"; \"utmXX\"";
        expected_intersect_metadata["inputFileName"] = "regular_8x3_cube.segy; shift_4_8x3_cube.segy";
        expected_intersect_metadata["boundingBox"]["cdp"] = {{22.0, 48.0}, {49.0, 66.0}, {37.0, 84.0}, {10.0, 66.0}};
        expected_intersect_metadata["boundingBox"]["ij"] = {{0.0, 0.0}, {3.0, 0.0}, {3.0, 3.0}, {0.0, 3.0}};
        expected_intersect_metadata["boundingBox"]["ilxl"] = {{15, 10}, {24, 10}, {24, 16}, {15, 16}};
        expected_intersect_metadata["axis"][0] = {{"annotation", "Inline"}, {"max", 24.0f}, {"min", 15.0f}, {"samples", 4}, {"stepsize", 3.0f}, {"unit", "unitless"}};
        expected_intersect_metadata["axis"][1] = {{"annotation", "Crossline"}, {"max", 16.0f}, {"min", 10.0f}, {"samples", 4}, {"stepsize", 2.0f}, {"unit", "unitless"}};
        expected_intersect_metadata["axis"][2] = {{"annotation", "Sample"}, {"max", 128.0f}, {"min", 20.0f}, {"samples", 28}, {"stepsize", 4.0f}, {"unit", "ms"}};

        iline_array = std::vector<int>(8);
        xline_array = std::vector<int>(8);
        sample_array = std::vector<int>(32);

        for (int i = 0; i < iline_array.size(); i++) {
            iline_array[i] = 3 + i * 3;
            xline_array[i] = 2 + i * 2;
        }

        for (int i = 0; i < sample_array.size(); i++) {
            sample_array[i] = 4 + i * 4;
        }

        single_datasource = make_single_datasource(
            REGULAR_DATA.c_str(),
            CREDENTIALS.c_str()
        );

        double_datasource = make_double_datasource(
            REGULAR_DATA.c_str(),
            CREDENTIALS.c_str(),
            SHIFT_4_DATA.c_str(),
            CREDENTIALS.c_str(),
            &inplace_addition
        );

        double_reverse_datasource = make_double_datasource(
            SHIFT_4_DATA.c_str(),
            CREDENTIALS.c_str(),
            REGULAR_DATA.c_str(),
            CREDENTIALS.c_str(),
            &inplace_addition
        );
    }

    void TearDown() override {
        delete single_datasource;
        delete double_datasource;
        delete double_reverse_datasource;
    }

public:
    nlohmann::json expected_intersect_metadata;

    std::vector<int> iline_array;
    std::vector<int> xline_array;
    std::vector<int> sample_array;

    std::vector<Bound> slice_bounds;

    SingleDataSource* single_datasource;
    DoubleDataSource* double_datasource;
    DoubleDataSource* double_reverse_datasource;

    static constexpr float fill = -999.25;

    void check_slice(struct response response_data, int low[], int high[], float factor) {
        std::size_t nr_of_values = (std::size_t)(response_data.size / sizeof(float));

        EXPECT_EQ(nr_of_values, (high[0] - low[0]) * (high[1] - low[1]) * (high[2] - low[2]));

        int counter = 0;
        for (int il = low[0]; il < high[0]; ++il) {
            for (int xl = low[1]; xl < high[1]; ++xl) {
                for (int s = low[2]; s < high[2]; s++) {
                    int value = int(*(float*)&response_data.data[counter * sizeof(float)] / factor) + 0.5f;
                    int sample = value & 0xFF;
                    int xline = value & 0xFF00;
                    int iline = value & 0xFF0000;
                    iline = iline >> 16;
                    xline = xline >> 8;

                    EXPECT_EQ(iline, iline_array[il]);
                    EXPECT_EQ(xline, xline_array[xl]);
                    EXPECT_EQ(sample, sample_array[s]);
                    counter += 1;
                }
            }
        }
    }

    void check_fence(struct response response_data, std::vector<float> coordinates, int low[], int high[], float factor) {
        std::size_t nr_of_values = (std::size_t)(response_data.size / sizeof(float));
        std::size_t nr_of_traces = (std::size_t)(coordinates.size() / 2);

        EXPECT_EQ(nr_of_values, nr_of_traces * (high[2] - low[2]));

        int counter = 0;
        for (int t = 0; t < nr_of_traces; t++) {
            int ic = coordinates[2 * t];
            int xc = coordinates[(2 * t) + 1];
            for (int s = low[2]; s < high[2]; ++s) {
                int value = int((*(float*)&response_data.data[counter * sizeof(float)] / factor) + 0.5f);
                int sample = value & 0xFF;
                int xline = (value & 0xFF00) >> 8;
                int iline = (value & 0xFF0000) >> 16;

                EXPECT_EQ(iline, iline_array[ic]);
                EXPECT_EQ(xline, xline_array[xc]);
                EXPECT_EQ(sample, sample_array[s]);

                counter += 1;
            }
        }
    }

    Grid get_grid(DataSource* datasource) {

        const MetadataHandle* metadata = &(datasource->get_metadata());
        OpenVDS::VolumeDataLayout const* const layout = metadata->get_layout();
        OpenVDS::VDSIJKGridDefinition grid_def = layout->GetVDSIJKGridDefinitionFromMetadata();
        std::vector<double> iUnitStep = {grid_def.iUnitStep.X, grid_def.iUnitStep.Y};
        std::vector<double> jUnitStep = {grid_def.jUnitStep.X, grid_def.jUnitStep.Y};
        return Grid(grid_def.origin.X, grid_def.origin.Y, norm(iUnitStep), norm(jUnitStep), 33.69);
    }

    void check_attribute(SurfaceBoundedSubVolume& subvolume, int low[], int high[], float factor) {

        std::size_t nr_of_values = subvolume.nsamples(0, (high[0] - low[0]) * (high[1] - low[1]));
        EXPECT_EQ(nr_of_values, (high[0] - low[0]) * (high[1] - low[1]) * (high[2] - low[2]));

        int counter = 0;
        for (int il = low[0]; il < high[0]; ++il) {
            for (int xl = low[1]; xl < high[1]; ++xl) {
                RawSegment rs = subvolume.vertical_segment(counter);
                int s = low[2];
                for (auto it = rs.begin(); it != rs.end(); ++it) {
                    int value = int(*it / factor + 0.5f);
                    int sample = value & 0xFF;
                    int xline = (value & 0xFF00) >> 8;
                    int iline = (value & 0xFF0000) >> 16;

                    EXPECT_EQ(iline, iline_array[il]);
                    EXPECT_EQ(xline, xline_array[xl]);
                    EXPECT_EQ(sample, sample_array[s]);
                    s += 1;
                }
                EXPECT_EQ(s, high[2]);
                counter += 1;
            }
        }
    }
};

TEST_F(DoubleVolumeDataLayoutTest, Single_Metadata) {

    nlohmann::json expected;
    expected["crs"] = "\"utmXX\"";
    expected["inputFileName"] = "regular_8x3_cube.segy";
    expected["importTimeStamp"] = "2024-03-01T11:58:35.521Z";
    expected["boundingBox"]["cdp"] = {{2.0, 0.0}, {65.0, 42.0}, {37.0, 84.0}, {-26.0, 42.0}};
    expected["boundingBox"]["ij"] = {{0.0, 0.0}, {7.0, 0.0}, {7.0, 7.0}, {0.0, 7.0}};
    expected["boundingBox"]["ilxl"] = {{3, 2}, {24, 2}, {24, 16}, {3, 16}};
    expected["axis"][0] = {{"annotation", "Inline"}, {"max", 24.0f}, {"min", 3.0f}, {"samples", 8}, {"stepsize", 3.0f}, {"unit", "unitless"}};
    expected["axis"][1] = {{"annotation", "Crossline"}, {"max", 16.0f}, {"min", 2.0f}, {"samples", 8}, {"stepsize", 2.0f}, {"unit", "unitless"}};
    expected["axis"][2] = {{"annotation", "Sample"}, {"max", 128.0f}, {"min", 4.0f}, {"samples", 32}, {"stepsize", 4.0f}, {"unit", "ms"}};

    struct response response_data;
    cppapi::metadata(*single_datasource, &response_data);
    nlohmann::json metadata = nlohmann::json::parse(response_data.data);

    EXPECT_EQ(metadata["crs"], expected["crs"]);
    EXPECT_EQ(metadata["inputFileName"], expected["inputFileName"]);
    EXPECT_EQ(metadata["boundingBox"], expected["boundingBox"]);
    EXPECT_EQ(metadata["axis"], expected["axis"]);
}

TEST_F(DoubleVolumeDataLayoutTest, Addition_Metadata) {

    struct response response_data;
    cppapi::metadata(*double_datasource, &response_data);
    nlohmann::json metadata = nlohmann::json::parse(response_data.data);

    EXPECT_EQ(metadata["crs"], this->expected_intersect_metadata["crs"]);
    EXPECT_EQ(metadata["inputFileName"], expected_intersect_metadata["inputFileName"]);
    EXPECT_EQ(metadata["boundingBox"], expected_intersect_metadata["boundingBox"]);
    EXPECT_EQ(metadata["axis"], expected_intersect_metadata["axis"]);
}

TEST_F(DoubleVolumeDataLayoutTest, Reverse_Addition_Metadata) {

    struct response response_data;
    cppapi::metadata(*double_reverse_datasource, &response_data);
    nlohmann::json metadata = nlohmann::json::parse(response_data.data);

    EXPECT_EQ(metadata["crs"], this->expected_intersect_metadata["crs"]);
    EXPECT_EQ(metadata["inputFileName"], "shift_4_8x3_cube.segy; regular_8x3_cube.segy");
    EXPECT_EQ(metadata["boundingBox"], expected_intersect_metadata["boundingBox"]);
    EXPECT_EQ(metadata["axis"], expected_intersect_metadata["axis"]);
}

TEST_F(DoubleVolumeDataLayoutTest, Single_Slice) {

    Direction const direction(axis_name::TIME);
    int lineno = 24;

    struct response response_data;
    cppapi::slice(
        *single_datasource,
        direction,
        lineno,
        slice_bounds,
        &response_data
    );

    int low[3] = {0, 0, 5};
    int high[3] = {8, 8, 6};
    check_slice(response_data, low, high, 1);
}

TEST_F(DoubleVolumeDataLayoutTest, Single_Slice_Time) {

    Direction const direction(axis_name::TIME);
    int lineno = 24;

    struct response response_data;
    cppapi::slice(
        *single_datasource,
        direction,
        lineno,
        slice_bounds,
        &response_data
    );

    int low[3] = {0, 0, 5};
    int high[3] = {8, 8, 6};
    check_slice(response_data, low, high, 1);
}

TEST_F(DoubleVolumeDataLayoutTest, Single_Slice_K) {

    Direction const direction(axis_name::K);
    int lineno = 5;

    struct response response_data;
    cppapi::slice(
        *single_datasource,
        direction,
        lineno,
        slice_bounds,
        &response_data
    );

    int low[3] = {0, 0, 5};
    int high[3] = {8, 8, 6};
    check_slice(response_data, low, high, 1);
}

TEST_F(DoubleVolumeDataLayoutTest, Addition_Offset_Slice_TIME) {

    Direction const direction(axis_name::TIME);
    int lineno = 24;

    struct response response_data;
    cppapi::slice(
        *double_datasource,
        direction,
        lineno,
        slice_bounds,
        &response_data
    );

    int low[3] = {4, 4, 5};
    int high[3] = {8, 8, 6};
    check_slice(response_data, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Addition_Offset_Slice_K) {

    Direction const direction(axis_name::K);
    int lineno = 5;

    struct response response_data;

    cppapi::slice(
        *double_datasource,
        direction,
        lineno,
        slice_bounds,
        &response_data
    );
    int low[3] = {4, 4, 9};
    int high[3] = {8, 8, 10};
    check_slice(response_data, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Addition_Offset_Reverse_Slice_TIME) {

    Direction const direction(axis_name::TIME);
    int lineno = 24;

    struct response response_data;
    cppapi::slice(
        *double_reverse_datasource,
        direction,
        lineno,
        slice_bounds,
        &response_data
    );
    int low[3] = {4, 4, 5};
    int high[3] = {8, 8, 6};
    check_slice(response_data, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Addition_Offset_Reverse_Slice_K) {

    Direction const direction(axis_name::K);
    int lineno = 5;

    struct response response_data;
    cppapi::slice(
        *double_reverse_datasource,
        direction,
        lineno,
        slice_bounds,
        &response_data
    );

    int low[3] = {4, 4, 9};
    int high[3] = {8, 8, 10};
    check_slice(response_data, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Addition_Offset_Slice_INLINE) {

    Direction const direction(axis_name::INLINE);
    int lineno = 15;

    struct response response_data;
    cppapi::slice(
        *double_datasource,
        direction,
        lineno,
        slice_bounds,
        &response_data
    );

    int low[3] = {4, 4, 4};
    int high[3] = {5, 8, 32};
    check_slice(response_data, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Addition_Offset_Slice_CROSSLINE) {

    Direction const direction(axis_name::CROSSLINE);
    int lineno = 14;

    struct response response_data;
    cppapi::slice(
        *double_datasource,
        direction,
        lineno,
        slice_bounds,
        &response_data
    );

    int low[3] = {4, 6, 4};
    int high[3] = {8, 7, 32};
    check_slice(response_data, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Single_Fence_INDEX) {

    struct response response_data;
    const std::vector<float> coordinates{0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};
    const coordinate_system c_system = coordinate_system::INDEX;
    const int coordinate_size = int(coordinates.size() / 2);
    const enum interpolation_method interpolation = NEAREST;
    const float fill = -999.25;

    cppapi::fence(
        *single_datasource,
        c_system,
        coordinates.data(),
        coordinate_size,
        interpolation,
        &fill,
        &response_data
    );

    int low[3] = {0, 0, 0};
    int high[3] = {8, 8, 32};
    check_fence(response_data, coordinates, low, high, 1);
}

TEST_F(DoubleVolumeDataLayoutTest, Single_Fence_ANNOTATION) {

    struct response response_data;
    const std::vector<float> coordinates{3, 2, 6, 4, 9, 6, 12, 8, 15, 10, 18, 12, 21, 14, 24, 16};
    const std::vector<float> check_coordinates{0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};

    const coordinate_system c_system = coordinate_system::ANNOTATION;
    const int coordinate_size = int(coordinates.size() / 2);
    const enum interpolation_method interpolation = NEAREST;
    const float fill = -999.25;

    cppapi::fence(
        *single_datasource,
        c_system,
        coordinates.data(),
        coordinate_size,
        interpolation,
        &fill,
        &response_data
    );

    int low[3] = {0, 0, 0};
    int high[3] = {8, 8, 32};
    check_fence(response_data, check_coordinates, low, high, 1);
}

TEST_F(DoubleVolumeDataLayoutTest, Single_Fence_CDP) {

    struct response response_data;
    const std::vector<float> coordinates{2, 0, 7, 12, 12, 24, 17, 36, 22, 48, 27, 60, 32, 72, 37, 84};
    const std::vector<float> check_coordinates{0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};

    const coordinate_system c_system = coordinate_system::CDP;
    const int coordinate_size = int(coordinates.size() / 2);
    const enum interpolation_method interpolation = NEAREST;
    const float fill = -999.25;

    cppapi::fence(
        *single_datasource,
        c_system,
        coordinates.data(),
        coordinate_size,
        interpolation,
        &fill,
        &response_data
    );

    int low[3] = {0, 0, 0};
    int high[3] = {8, 8, 32};
    check_fence(response_data, check_coordinates, low, high, 1);
}

TEST_F(DoubleVolumeDataLayoutTest, Double_Fence_INDEX) {

    struct response response_data;
    const std::vector<float> coordinates{0, 0, 1, 1, 2, 2, 3, 3};
    const std::vector<float> check_coordinates{4, 4, 5, 5, 6, 6, 7, 7};

    const coordinate_system c_system = coordinate_system::INDEX;
    const int coordinate_size = int(coordinates.size() / 2);
    const enum interpolation_method interpolation = NEAREST;
    const float fill = -999.25;

    cppapi::fence(
        *double_datasource,
        c_system,
        coordinates.data(),
        coordinate_size,
        interpolation,
        &fill,
        &response_data
    );

    int low[3] = {4, 4, 4};
    int high[3] = {8, 8, 32};
    check_fence(response_data, check_coordinates, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Double_Fence_ANNOTATION) {

    struct response response_data;

    const std::vector<float> coordinates{15, 10, 18, 12, 21, 14, 24, 16};
    const std::vector<float> check_coordinates{4, 4, 5, 5, 6, 6, 7, 7};

    const coordinate_system c_system = coordinate_system::ANNOTATION;
    const int coordinate_size = int(coordinates.size() / 2);
    const enum interpolation_method interpolation = NEAREST;
    const float fill = -999.25;

    cppapi::fence(
        *double_datasource,
        c_system,
        coordinates.data(),
        coordinate_size,
        interpolation,
        &fill,
        &response_data
    );

    int low[3] = {4, 4, 4};
    int high[3] = {8, 8, 32};
    check_fence(response_data, check_coordinates, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Double_Fence_CDP) {

    struct response response_data;
    const std::vector<float> coordinates{22, 48, 27, 60, 32, 72, 37, 84};
    const std::vector<float> check_coordinates{4, 4, 5, 5, 6, 6, 7, 7};

    const coordinate_system c_system = coordinate_system::CDP;
    const int coordinate_size = int(coordinates.size() / 2);
    const enum interpolation_method interpolation = NEAREST;
    const float fill = -999.25;

    cppapi::fence(
        *double_datasource,
        c_system,
        coordinates.data(),
        coordinate_size,
        interpolation,
        &fill,
        &response_data
    );

    int low[3] = {4, 4, 4};
    int high[3] = {8, 8, 32};
    check_fence(response_data, check_coordinates, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Double_Reverse_Fence_INDEX) {

    struct response response_data;
    const std::vector<float> coordinates{0, 0, 1, 1, 2, 2, 3, 3};
    const std::vector<float> check_coordinates{4, 4, 5, 5, 6, 6, 7, 7};

    const coordinate_system c_system = coordinate_system::INDEX;
    const int coordinate_size = int(coordinates.size() / 2);
    const enum interpolation_method interpolation = NEAREST;
    const float fill = -999.25;

    cppapi::fence(
        *double_reverse_datasource,
        c_system,
        coordinates.data(),
        coordinate_size,
        interpolation,
        &fill,
        &response_data
    );

    int low[3] = {4, 4, 4};
    int high[3] = {8, 8, 32};
    check_fence(response_data, check_coordinates, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Double_Reverse_Fence_ANNOTATION) {

    struct response response_data;

    const std::vector<float> coordinates{15, 10, 18, 12, 21, 14, 24, 16};
    const std::vector<float> check_coordinates{4, 4, 5, 5, 6, 6, 7, 7};

    const coordinate_system c_system = coordinate_system::ANNOTATION;
    const int coordinate_size = int(coordinates.size() / 2);
    const enum interpolation_method interpolation = NEAREST;
    const float fill = -999.25;

    cppapi::fence(
        *double_reverse_datasource,
        c_system,
        coordinates.data(),
        coordinate_size,
        interpolation,
        &fill,
        &response_data
    );

    int low[3] = {4, 4, 4};
    int high[3] = {8, 8, 32};
    check_fence(response_data, check_coordinates, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Double_Reverse_Fence_CDP) {

    struct response response_data;
    const std::vector<float> coordinates{22, 48, 27, 60, 32, 72, 37, 84};
    const std::vector<float> check_coordinates{4, 4, 5, 5, 6, 6, 7, 7};

    const coordinate_system c_system = coordinate_system::CDP;
    const int coordinate_size = int(coordinates.size() / 2);
    const enum interpolation_method interpolation = NEAREST;
    const float fill = -999.25;

    cppapi::fence(
        *double_reverse_datasource,
        c_system,
        coordinates.data(),
        coordinate_size,
        interpolation,
        &fill,
        &response_data
    );

    int low[3] = {4, 4, 4};
    int high[3] = {8, 8, 32};
    check_fence(response_data, check_coordinates, low, high, 2);
}

TEST_F(DoubleVolumeDataLayoutTest, Single_Attribute) {

    DataSource* datasource = single_datasource;
    Grid grid = get_grid(datasource);
    const MetadataHandle* metadata = &(datasource->get_metadata());

    std::size_t nrows = metadata->iline().nsamples();
    std::size_t ncols = metadata->xline().nsamples();
    static std::vector<float> top_surface_data(nrows * ncols, 28.0f);
    static std::vector<float> pri_surface_data(nrows * ncols, 36.0f);
    static std::vector<float> bot_surface_data(nrows * ncols, 52.0f);
    RegularSurface pri_surface = RegularSurface(pri_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface top_surface = RegularSurface(top_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface bot_surface = RegularSurface(bot_surface_data.data(), nrows, ncols, grid, fill);
    SurfaceBoundedSubVolume* subvolume = make_subvolume(datasource->get_metadata(), pri_surface, top_surface, bot_surface);

    cppapi::fetch_subvolume(*single_datasource, *subvolume, NEAREST, 0, nrows * ncols);

    int low[3] = {0, 0, 4};
    int high[3] = {8, 8, 15};
    check_attribute(*subvolume, low, high, 1);

    delete subvolume;
}

TEST_F(DoubleVolumeDataLayoutTest, Double_Attribute) {

    DataSource* datasource = double_datasource;
    Grid grid = get_grid(datasource);
    const MetadataHandle* metadata = &(datasource->get_metadata());

    std::size_t nrows = metadata->iline().nsamples();
    std::size_t ncols = metadata->xline().nsamples();
    static std::vector<float> top_surface_data(nrows * ncols, 28.0f);
    static std::vector<float> pri_surface_data(nrows * ncols, 36.0f);
    static std::vector<float> bot_surface_data(nrows * ncols, 52.0f);
    RegularSurface pri_surface = RegularSurface(pri_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface top_surface = RegularSurface(top_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface bot_surface = RegularSurface(bot_surface_data.data(), nrows, ncols, grid, fill);
    SurfaceBoundedSubVolume* subvolume = make_subvolume(datasource->get_metadata(), pri_surface, top_surface, bot_surface);

    cppapi::fetch_subvolume(*datasource, *subvolume, NEAREST, 0, nrows * ncols);

    int low[3] = {4, 4, 4};
    int high[3] = {8, 8, 15};
    check_attribute(*subvolume, low, high, 2);

    delete subvolume;
}

TEST_F(DoubleVolumeDataLayoutTest, Double_Reverse_Attribute) {

    DataSource* datasource = double_reverse_datasource;
    Grid grid = get_grid(datasource);
    const MetadataHandle* metadata = &(datasource->get_metadata());

    std::size_t nrows = metadata->iline().nsamples();
    std::size_t ncols = metadata->xline().nsamples();
    static std::vector<float> top_surface_data(nrows * ncols, 28.0f);
    static std::vector<float> pri_surface_data(nrows * ncols, 36.0f);
    static std::vector<float> bot_surface_data(nrows * ncols, 52.0f);
    RegularSurface pri_surface = RegularSurface(pri_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface top_surface = RegularSurface(top_surface_data.data(), nrows, ncols, grid, fill);
    RegularSurface bot_surface = RegularSurface(bot_surface_data.data(), nrows, ncols, grid, fill);
    SurfaceBoundedSubVolume* subvolume = make_subvolume(datasource->get_metadata(), pri_surface, top_surface, bot_surface);

    cppapi::fetch_subvolume(*datasource, *subvolume, NEAREST, 0, nrows * ncols);

    int low[3] = {4, 4, 4};
    int high[3] = {8, 8, 15};
    check_attribute(*subvolume, low, high, 2);

    delete subvolume;
}

} // namespace
