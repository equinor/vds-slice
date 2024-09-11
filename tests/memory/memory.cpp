#include "capi.h"
#include "ctypes.h"
#include "direction.hpp"
#include <cstdlib>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

/**
 * It would be desirable to perform memory tests through go.
 * However, go doesn't support valgrind. It supports asan, but
 * - it doesn't discover "still reachable" issues
 * - is inconvenient to set up as checking for leaks go -> c is not fully
 *   supported
 *
 * Thus tests are implemented on c library level as closest one to go. Even now
 * setup can be inconvenient to work with, so tests may be removed if
 * inconvenience outweighs benefit.
 *
 * Note that tests themselves are not really testing anything, they just create
 * various situations so that memory layouts can be tested with external tools
 * such as valgrind.
 */

class BaseTest : public ::testing::Test {
protected:
    BaseTest() {
        context = context_new();
        const char* storage_account_url = std::getenv("STORAGE_ACCOUNT_URL");
        const char* sas = std::getenv("SAS");
        url =
            "azure://testdata/well_known/well_known_default";
        url2 =
            "azure://testdata/10_samples/10_samples_default";
        connection =
            "BlobEndpoint=" + std::string(storage_account_url) + ";SharedAccessSignature=" + std::string(sas);
    }
    ~BaseTest() {
        context_free(context);
    }
    Context* context;
    std::string url;
    std::string url2;
    std::string connection;
};

TEST_F(BaseTest, SingleDatahandle) {
    DataHandle* dataHandle;
    int cerr = single_datahandle_new(
        context,
        url.c_str(),
        connection.c_str(),
        &dataHandle
    );
    EXPECT_EQ(cerr, STATUS_OK);
    datahandle_free(context, dataHandle);
}

TEST_F(BaseTest, SingleDatahandleWrongVds) {
    std::string fake_url = "azure://testdata/UFO/non_existent";
    DataHandle* dataHandle;
    int cerr = single_datahandle_new(
        context,
        fake_url.c_str(),
        connection.c_str(),
        &dataHandle
    );
    EXPECT_NE(cerr, STATUS_OK);

    std::string expected_msg = "404 The specified blob does not exist";
    const char* msg = errmsg(context);
    EXPECT_THAT(msg, testing::HasSubstr(expected_msg));
}

TEST_F(BaseTest, SingleDatahandleWrongConnection) {
    const char* storage_account_url = std::getenv("STORAGE_ACCOUNT_URL");
    std::string fake_sas = "sp=r&se=2012-12-21T00:00:00Z&spr=https&sv=2022-11-02&sr=c&sig=SIGN";
    std::string bad_connection =
        "BlobEndpoint=" + std::string(storage_account_url) + ";SharedAccessSignature=" + fake_sas;
    DataHandle* dataHandle;
    int cerr = single_datahandle_new(
        context,
        url.c_str(),
        bad_connection.c_str(),
        &dataHandle
    );
    EXPECT_NE(cerr, STATUS_OK);

    std::string expected_msg = "403 Server failed to authenticate the request";
    const char* msg = errmsg(context);
    EXPECT_THAT(msg, testing::HasSubstr(expected_msg));
}

TEST_F(BaseTest, DoubleDatahandle) {
    DataHandle* dataHandle;
    int cerr = double_datahandle_new(
        context,
        url.c_str(),
        connection.c_str(),
        url2.c_str(),
        connection.c_str(),
        binary_operator::MULTIPLICATION,
        &dataHandle
    );
    EXPECT_EQ(cerr, STATUS_OK);
    datahandle_free(context, dataHandle);
}

class EndpointTest : public BaseTest {
protected:
    EndpointTest() : BaseTest() {
        single_datahandle_new(
            context,
            url.c_str(),
            connection.c_str(),
            &dataHandle
        );
    }
    ~EndpointTest() {
        response_delete(&result);
        datahandle_free(context, dataHandle);
    }

    DataHandle* dataHandle;
    response result = response_create();
};

TEST_F(EndpointTest, SliceEndpoint) {
    Bound bounds[1] = {Bound{4, 8, axis_name::TIME}};
    int cerr = slice(context, dataHandle, 3, axis_name::INLINE, &bounds[0], 1, &result);
    EXPECT_EQ(cerr, STATUS_OK);
    EXPECT_NE(result.size, 0);
}

TEST_F(EndpointTest, SliceEndpointInvalidRequest) {
    Bound bounds[1] = {Bound{4, 8, axis_name::TIME}};
    int cerr = slice(context, dataHandle, 30, axis_name::INLINE, &bounds[0], 0, &result);
    EXPECT_NE(cerr, STATUS_OK);

    std::string expected_msg = "Invalid lineno: 30";
    const char* msg = errmsg(context);
    EXPECT_THAT(msg, testing::HasSubstr(expected_msg));
}

TEST_F(EndpointTest, FenceEndpoint) {
    float coordinates[4] = {3, 10, 3, 11};
    int cerr = fence(
        context,
        dataHandle,
        coordinate_system::ANNOTATION,
        &coordinates[0],
        2,
        interpolation_method::LINEAR,
        nullptr,
        &result
    );
    EXPECT_EQ(cerr, STATUS_OK);
    EXPECT_NE(result.size, 0);
}

TEST_F(EndpointTest, FenceEndpointInvalidRequest) {
    float coordinates[4] = {3, 10, 3, 14};
    int cerr = fence(
        context,
        dataHandle,
        coordinate_system::ANNOTATION,
        &coordinates[0],
        2,
        interpolation_method::LINEAR,
        nullptr,
        &result
    );
    EXPECT_NE(cerr, STATUS_OK);

    std::string expected_msg = "is out of boundaries in dimension 1.";
    const char* msg = errmsg(context);
    EXPECT_THAT(msg, testing::HasSubstr(expected_msg));
}

class AttributeTest : public EndpointTest {
protected:
    AttributeTest() : EndpointTest() {
    }

    ~AttributeTest() {
        regular_surface_free(context, reference_surface);
        regular_surface_free(context, top_surface);
        regular_surface_free(context, bottom_surface);

        regular_surface_free(context, primary_surface);
        regular_surface_free(context, secondary_surface);
        regular_surface_free(context, aligned_surface);

        subvolume_free(context, subvolume);
    }

    const int xori = 2;
    const int yori = 0;
    const int xinc = 100;
    const int yinc = 1000;
    const int rotation = 0;
    const int fillvalue = 666;

    RegularSurface* reference_surface = nullptr;
    RegularSurface* top_surface = nullptr;
    RegularSurface* bottom_surface = nullptr;

    RegularSurface* primary_surface = nullptr;
    RegularSurface* secondary_surface = nullptr;
    RegularSurface* aligned_surface = nullptr;

    SurfaceBoundedSubVolume* subvolume = nullptr;
};

TEST_F(AttributeTest, AttributeEndpoint) {
    const int nvalues = 1;
    float reference_values[nvalues] = {11};
    float top_values[nvalues] = {10};
    float bottom_values[nvalues] = {12};

    const int nrows = 1;
    const int ncols = 1;

    regular_surface_new(
        context, &reference_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &reference_surface
    );
    regular_surface_new(
        context, &top_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &top_surface
    );
    regular_surface_new(
        context, &bottom_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &bottom_surface
    );

    int cerr = subvolume_new(context, dataHandle, reference_surface, top_surface, bottom_surface, &subvolume);
    EXPECT_EQ(cerr, 0);

    const int nattributes = 1;
    enum attribute attr[nattributes] = {RMS};

    float attr_res = 0;

    cerr = attribute(
        context,
        dataHandle,
        subvolume,
        interpolation_method::LINEAR,
        &attr[0],
        nattributes,
        0.1,
        0,
        nvalues,
        &attr_res
    );
    EXPECT_EQ(cerr, 0);
    EXPECT_NE(attr_res, 0);
}

TEST_F(AttributeTest, AttributesEndpointInvalidRequest) {
    const int nvalues = 1;
    float reference_values[nvalues] = {4};
    float top_values[nvalues] = {3};
    float bottom_values[nvalues] = {5};

    const int nrows = 1;
    const int ncols = 1;

    regular_surface_new(
        context, &reference_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &reference_surface
    );
    regular_surface_new(
        context, &top_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &top_surface
    );
    regular_surface_new(
        context, &bottom_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &bottom_surface
    );

    int cerr = subvolume_new(context, dataHandle, reference_surface, top_surface, bottom_surface, &subvolume);
    EXPECT_NE(cerr, STATUS_OK);

    std::string expected_msg = "Vertical window is out of vertical bounds";
    const char* msg = errmsg(context);
    EXPECT_THAT(msg, testing::HasSubstr(expected_msg));
}

TEST_F(AttributeTest, SurfaceAlignment) {
    const int nvalues = 6;
    float primary_values[nvalues] = {3, 13, 14, 12, 5, 6};
    float secondary_values[nvalues] = {5, 16, 16, 14, 16, 10};
    float aligned_values[nvalues];

    const int nrows = 2;
    const int ncols = 3;

    int primary_is_top = 0;

    regular_surface_new(
        context, &primary_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &primary_surface
    );
    regular_surface_new(
        context, &aligned_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &aligned_surface
    );
    regular_surface_new(
        context, &secondary_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &secondary_surface
    );

    int cerr = align_surfaces(context, primary_surface, secondary_surface, aligned_surface, &primary_is_top);
    EXPECT_EQ(cerr, STATUS_OK);
}

TEST_F(AttributeTest, SurfaceAlignmentFailure) {
    const int nvalues = 6;
    float primary_values[nvalues] = {3, 13, 14, 12, 5, 6};
    float secondary_values[nvalues] = {5, 15, 16, 4, 16, 2};
    float aligned_values[nvalues];

    const int nrows = 2;
    const int ncols = 3;

    int primary_is_top = 0;

    regular_surface_new(
        context, &primary_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &primary_surface
    );
    regular_surface_new(
        context, &aligned_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &aligned_surface
    );
    regular_surface_new(
        context, &secondary_values[0], nrows, ncols, xori, yori, xinc, yinc, rotation, fillvalue, &secondary_surface
    );

    int cerr = align_surfaces(context, primary_surface, secondary_surface, aligned_surface, &primary_is_top);
    EXPECT_NE(cerr, STATUS_OK);

    std::string expected_msg = "Surfaces intersect";
    const char* msg = errmsg(context);
    EXPECT_THAT(msg, testing::HasSubstr(expected_msg));
}

TEST_F(EndpointTest, MetadataEndpoint) {
    int cerr = metadata(context, dataHandle, &result);
    EXPECT_EQ(cerr, STATUS_OK);
    EXPECT_NE(result.size, 0);
}

TEST_F(EndpointTest, SliceMetadataEndpoint) {
    int cerr = slice_metadata(context, dataHandle, 3, axis_name::INLINE, nullptr, 0, &result);
    EXPECT_EQ(cerr, STATUS_OK);
    EXPECT_NE(result.size, 0);
}

TEST_F(EndpointTest, FenceMetadataEndpoint) {
    int cerr = fence_metadata(context, dataHandle, 4, &result);
    EXPECT_EQ(cerr, STATUS_OK);
    EXPECT_NE(result.size, 0);
}

TEST_F(EndpointTest, AttributesMetadataEndpoint) {
    int cerr = attribute_metadata(context, dataHandle, 2, 3, &result);
    EXPECT_EQ(cerr, STATUS_OK);
    EXPECT_NE(result.size, 0);
}
