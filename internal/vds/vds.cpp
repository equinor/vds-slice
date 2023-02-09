#include "vds.h"

#include <stdio.h>
#include <stdlib.h>

#include <array>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <utility>
#include <cmath>

#include "nlohmann/json.hpp"

#include "axis.h"
#include "boundingbox.h"
#include "datahandle.h"
#include "metadatahandle.h"

using namespace std;

void response_delete(struct response* buf) {
    if (!buf)
        return;

    delete[] buf->data;
    delete[] buf->err;
    *buf = response {};
}

nlohmann::json json_axis(
    const Axis& axis
) {
    nlohmann::json doc;
    doc = {
        { "annotation", axis.get_vds_name()         },
        { "min",        axis.get_min()              },
        { "max",        axis.get_max()              },
        { "samples",    axis.get_number_of_points() },
        { "unit",       axis.get_unit()             },
    };
    return doc;
}

struct response fetch_slice(
    std::string url,
    std::string credentials,
    api_axis_name ax,
    int lineno
) {
    DataHandle handle(url, credentials);
    const SliceRequestParameters request_parameters{ax, lineno};
    return handle.slice(request_parameters);
}

struct response fetch_slice_metadata(
    std::string url,
    std::string credentials,
    api_axis_name ax
) {
    const DataHandle handle(url, credentials);
    const MetadataHandle& metadata = handle.get_metadata();

    nlohmann::json meta;
    meta["format"] = metadata.get_format();

    /*
     * SEGYImport always writes annotation 'Sample' for axis K. We, on the
     * other hand, decided that we base the valid input direction on the units
     * of said axis. E.g. ms/s -> Time, etc. This leads to an inconsistency
     * between what we require as input for axis K and what we return as
     * metadata. In the ms/s case we require the input to be asked for in axis
     * 'Time', but the return metadata can potentially say 'Sample'.
     *
     * TODO: Either revert the 'clever' unit validation, or patch the
     * K-annotation here. IMO the later is too clever for it's own good and
     * would be quite suprising for people that use this API in conjunction
     * with the OpenVDS library.
     */
    if (ax == api_axis_name::I or ax == api_axis_name::INLINE) {
        meta["x"] = json_axis(metadata.get_sample());
        meta["y"] = json_axis(metadata.get_crossline());
    }
    else if (ax == api_axis_name::J or ax == api_axis_name::CROSSLINE) {
        meta["x"] = json_axis(metadata.get_sample());
        meta["y"] = json_axis(metadata.get_inline());
    }
    else {
        meta["x"] = json_axis(metadata.get_crossline());
        meta["y"] = json_axis(metadata.get_inline());
    }

    auto str = meta.dump();
    auto *data = new char[str.size()];
    std::copy(str.begin(), str.end(), data);

    response buffer{};
    buffer.size = str.size();
    buffer.data = data;

    return buffer;
}

struct response fetch_fence(
    const std::string& url,
    const std::string& credentials,
    enum coordinate_system coordinate_system,
    const float* coordinates,
    size_t npoints,
    enum interpolation_method interpolation_method
) {
    const FenceRequestParameters requestParameters{
        coordinate_system,
        coordinates,
        npoints,
        interpolation_method
    };
    DataHandle handle(url, credentials);
    return handle.fence(requestParameters);
}

struct response fetch_fence_metadata(
    std::string url,
    std::string credentials,
    size_t npoints
) {
    const DataHandle handle(url, credentials);
    const MetadataHandle& metadata = handle.get_metadata();

    nlohmann::json meta;
    const Axis sample_axis = metadata.get_sample();
    meta["shape"] = nlohmann::json::array({npoints, sample_axis.get_number_of_points() });
    meta["format"] = metadata.get_format();

    auto str = meta.dump();
    auto *data = new char[str.size()];
    std::copy(str.begin(), str.end(), data);

    response buffer{};
    buffer.size = str.size();
    buffer.data = data;

    return buffer;
}

struct response metadata(
    const std::string& url,
    const std::string& credentials
) {
    const DataHandle handle(url, credentials);
    const MetadataHandle& metadata = handle.get_metadata();

    nlohmann::json meta;
    meta["format"] = metadata.get_format();

    meta["crs"] = metadata.get_crs();

    auto bbox = metadata.get_bounding_box();
    meta["boundingBox"]["ij"]   = bbox.index();
    meta["boundingBox"]["cdp"]  = bbox.world();
    meta["boundingBox"]["ilxl"] = bbox.annotation();


    const Axis& inline_axis = metadata.get_inline();
    meta["axis"].push_back(json_axis(inline_axis));

    const Axis& crossline_axis = metadata.get_crossline();
    meta["axis"].push_back(json_axis(crossline_axis));

    const Axis& sample_axis = metadata.get_sample();
    meta["axis"].push_back(json_axis(sample_axis));

    auto str = meta.dump();
    auto *data = new char[str.size()];
    std::copy(str.begin(), str.end(), data);

    response buffer{};
    buffer.size = str.size();
    buffer.data = data;

    return buffer;
}

struct response handle_error(
    const std::exception& e
) {
    response buf {};
    buf.err = new char[std::strlen(e.what()) + 1];
    std::strcpy(buf.err, e.what());
    return buf;
}

struct response slice(
    const char* vds,
    const char* credentials,
    int lineno,
    api_axis_name ax
) {
    std::string cube(vds);
    std::string cred(credentials);

    try {
        return fetch_slice(cube, cred, ax, lineno);
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct response slice_metadata(
    const char* vds,
    const char* credentials,
    api_axis_name ax
) {
    std::string cube(vds);
    std::string cred(credentials);

    try {
        return fetch_slice_metadata(cube, cred, ax);
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct response fence(
    const char* vds,
    const char* credentials,
    enum coordinate_system coordinate_system,
    const float* coordinates,
    size_t npoints,
    enum interpolation_method interpolation_method
) {
    std::string cube(vds);
    std::string cred(credentials);

    try {
        return fetch_fence(
            cube, cred, coordinate_system, coordinates, npoints,
            interpolation_method);
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct response fence_metadata(
    const char* vds,
    const char* credentials,
    size_t npoints
) {
    std::string cube(vds);
    std::string cred(credentials);

    try {
        return fetch_fence_metadata(cube, cred, npoints);
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct response metadata(
    const char* vds,
    const char* credentials
) {
    try {
        std::string cube(vds);
        std::string cred(credentials);
        return metadata(cube, cred);
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}
