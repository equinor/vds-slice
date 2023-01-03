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

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

#include "boundingbox.h"
#include "poststack.h"

using namespace std;

void requestdata_delete(struct requestdata* buf) {
    if (!buf)
        return;

    delete[] buf->data;
    delete[] buf->err;
    *buf = requestdata {};
}

static requestdata wrap_as_requestdata( const nlohmann::json::string_t& dump ) {
    requestdata tmp{ new char[dump.size()], nullptr, dump.size() };
    std::copy(dump.begin(), dump.end(), tmp.data);
    return tmp;
}

nlohmann::json convert_axis_descriptor_to_json(
    const AxisMetadata& axis_metadata
) {
    nlohmann::json doc;
    doc = {
        { "annotation", axis_metadata.name()              },
        { "min",        axis_metadata.min()               },
        { "max",        axis_metadata.max()               },
        { "samples",    axis_metadata.number_of_samples() },
        { "unit",       axis_metadata.unit()              },
    };
    return doc;
}

struct requestdata handle_error(
    const std::exception& e
) {
    requestdata buf {};
    buf.err = new char[std::strlen(e.what()) + 1];
    std::strcpy(buf.err, e.what());
    return buf;
}

struct requestdata metadata(
    char const * const vds,
    char const * const credentials
) {
    try {
        PostStackHandle poststackdata( vds, credentials );

        nlohmann::json meta;
        meta["format"] = poststackdata.get_format(Channel::Sample);

        meta["crs"] = poststackdata.get_crs();

        auto bbox = poststackdata.get_bounding_box();
        meta["boundingBox"]["ij"]   = bbox.index();
        meta["boundingBox"]["cdp"]  = bbox.world();
        meta["boundingBox"]["ilxl"] = bbox.annotation();

        {
            const auto axis_descriptor = poststackdata.get_all_axes_metadata();
            for (auto it = axis_descriptor.rbegin(); it != axis_descriptor.rend(); ++it) {
                meta["axis"].push_back(
                    convert_axis_descriptor_to_json( *it )
                );
            }
        }

        return wrap_as_requestdata( meta.dump() );
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct requestdata slice(
    char const * const vds,
    char const * const credentials,
    const int lineno,
    const Axis ax
) {
    try {
        PostStackHandle poststackdata( vds, credentials );
        return poststackdata.get_slice(ax, lineno);
    } catch (const std::exception& e) {
        std::cerr << "Fetching error " << e.what() << std::endl;
        return handle_error(e);
    }
}

struct requestdata slice_metadata(
    char const * const vds,
    char const * const credentials,
    const Axis ax
) {
    try {
        PostStackHandle poststackdata( vds, credentials );

        nlohmann::json meta;
        meta["format"] = poststackdata.get_format(Channel::Sample);

        const auto axis_metadata = poststackdata.get_slice_axis_metadata( ax );

        meta["x"] = convert_axis_descriptor_to_json( axis_metadata[AxisDirection::X] );
        meta["y"] = convert_axis_descriptor_to_json( axis_metadata[AxisDirection::Y] );

        auto bbox = poststackdata.get_bounding_box();
        meta["boundingBox"]["ij"]   = bbox.index();
        meta["boundingBox"]["cdp"]  = bbox.world();
        meta["boundingBox"]["ilxl"] = bbox.annotation();

        return wrap_as_requestdata( meta.dump() );
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct requestdata fence(
    char const * const vds,
    char const * const credentials,
    const enum CoordinateSystem coordinate_system,
    float const * const coordinates,
    const size_t npoints,
    const enum InterpolationMethod interpolation_method
) {
    try {
        PostStackHandle poststackdata( vds, credentials );
        return poststackdata.get_fence(
            coordinate_system,
            coordinates,
            npoints,
            interpolation_method );
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct requestdata fence_metadata(
    char const * const vds,
    char const * const credentials,
    const size_t npoints
) {
    try {
        PostStackHandle poststackdata( vds, credentials );

        nlohmann::json meta;
        {
            const auto axis_metadata = poststackdata.get_all_axes_metadata();
            meta["shape"] = nlohmann::json::array({npoints, axis_metadata[0].number_of_samples() });
        }
        meta["format"] = poststackdata.get_format(Channel::Sample);

        return wrap_as_requestdata( meta.dump() );
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}
