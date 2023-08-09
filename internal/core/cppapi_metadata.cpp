#include "ctypes.h"

#include "nlohmann/json.hpp"

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

#include "axis.hpp"
#include "boundingbox.hpp"
#include "datahandle.hpp"
#include "direction.hpp"
#include "exceptions.hpp"
#include "metadatahandle.hpp"

namespace {

std::string fmtstr(OpenVDS::VolumeDataFormat format) {
    /*
     * We always request data in OpenVDS::VolumeDataFormat::Format_R32 format
     * as this seems to be intended way when working with openvds [1].
     * Thus users will always get data returned as f4.
     *
     * We also assume that server code is run on a little-endian machine.
     *
     * [1] https://community.opengroup.org/osdu/platform/domain-data-mgmt-services/seismic/open-vds/-/issues/156#note_165511
     */
    switch (format) {
        case OpenVDS::VolumeDataFormat::Format_R32: return "<f4";
        default: {
            throw std::runtime_error("unsupported VDS format type");
        }
    }
}

void to_response(nlohmann::json const& metadata, response* response) {
    auto const dump = metadata.dump();
    std::unique_ptr< char[] > tmp(new char[dump.size()]);
    std::copy(dump.begin(), dump.end(), tmp.get());

    response->data = tmp.release();
    response->size = dump.size();
}

nlohmann::json json_axis(
    Axis const& axis
) {
    nlohmann::json doc;
    doc = {
        { "annotation", axis.name()     },
        { "min",        axis.min()      },
        { "max",        axis.max()      },
        { "samples",    axis.nsamples() },
        { "unit",       axis.unit()     },
    };
    return doc;
}

nlohmann::json json_slice_geospatial(
    MetadataHandle const& metadata,
    Direction const direction,
    Axis const& axis,
    int lineno
) {
    auto const& transformer = metadata.coordinate_transformer();

    SubVolume bounds(metadata);
    bounds.set_slice(axis, lineno, direction.coordinate_system());

    auto const lower = transformer.VoxelIndexToIJKIndex({
        bounds.bounds.lower[0],
        bounds.bounds.lower[1],
        bounds.bounds.lower[2]
    });

    // The upper bound is exclusive, while we need it to be inclusive
    auto const upper = transformer.VoxelIndexToIJKIndex({
        bounds.bounds.upper[0] - 1,
        bounds.bounds.upper[1] - 1,
        bounds.bounds.upper[2] - 1
    });

    /** The slice bounds are given by the lower- and upper-coordinates only:
     *
     *
     *       Depth / Time slice   Inline slice   Crossline slice
     *       ------------------   -------------  ---------------
     *
     *         3         upper       upper
     *         +-----------+           +
     *         |           |           |
     *         |           |           |        lower       upper
     *         |           |           |          +-----------+
     *         |           |           |
     *         |           |           |
     *   J     +-----------+           +
     *   ^   lower         1         lower
     *   |
     *   +--> I
     *
     * For inline- and crossline-slices the horizontal bounding box is given by
     * a linestring from (lower.I, lower.J) to (upper.I, upper.J). However, for
     * time- and depth-slices we need to construct 4 corners. The first corner
     * is lower, then go in a counter-clockwise direction around the box.
     * Corner 1 is given by (upper.I, lower.J) and corner 3 is
     * (lower.I, upper.J).
     */
    const std::array< OpenVDS::DoubleVector3, 4 > corners {{
        transformer.IJKIndexToWorld({ lower[0], lower[1], 0 }),
        transformer.IJKIndexToWorld({ upper[0], lower[1], 0 }),
        transformer.IJKIndexToWorld({ upper[0], upper[1], 0 }),
        transformer.IJKIndexToWorld({ lower[0], upper[1], 0 }),
    }};


    if (direction.is_sample()) {
        return {
            { corners[0][0], corners[0][1] },
            { corners[1][0], corners[1][1] },
            { corners[2][0], corners[2][1] },
            { corners[3][0], corners[3][1] },
        };
    } else {
        return {
            { corners[0][0], corners[0][1] },
            { corners[2][0], corners[2][1] }
        };
    }
}

} // namespace

namespace cppapi {

void slice_metadata(
    DataHandle& handle,
    Direction const direction,
    int lineno,
    response* out
) {
    MetadataHandle const& metadata = handle.get_metadata();
    auto const& axis = metadata.get_axis(direction);

    nlohmann::json meta;
    meta["format"] = fmtstr(DataHandle::format());

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
    Axis const& inline_axis = metadata.iline();
    Axis const& crossline_axis = metadata.xline();
    Axis const& sample_axis = metadata.sample();

    auto json_shape = [&](Axis const &x, Axis const &y) {
        meta["x"] = json_axis(x);
        meta["y"] = json_axis(y);
        meta["shape"] = nlohmann::json::array({y.nsamples(), x.nsamples()});
    };

    if (direction.is_iline()) {
        json_shape(sample_axis, crossline_axis);
    } else if (direction.is_xline()) {
        json_shape(sample_axis, inline_axis);
    } else if (direction.is_sample()) {
        json_shape(crossline_axis, inline_axis);
    } else {
        throw std::runtime_error("Unhandled direction");
    }

    meta["geospatial"] = json_slice_geospatial(metadata, direction,axis, lineno);
    return to_response(meta, out);
}

void fence_metadata(
    DataHandle& handle,
    size_t npoints,
    response* out
) {
    MetadataHandle const& metadata = handle.get_metadata();

    nlohmann::json meta;
    Axis const& sample_axis = metadata.sample();
    meta["shape"] = nlohmann::json::array({npoints, sample_axis.nsamples() });
    meta["format"] = fmtstr(DataHandle::format());

    return to_response(meta, out);
}

void metadata(DataHandle& handle, response* out) {
    MetadataHandle const& metadata = handle.get_metadata();

    nlohmann::json meta;

    meta["crs"]             = metadata.crs();
    meta["inputFileName"]   = metadata.input_filename();
    meta["importTimeStamp"] = metadata.import_time_stamp();

    auto bbox = metadata.bounding_box();
    meta["boundingBox"]["ij"]   = bbox.index();
    meta["boundingBox"]["cdp"]  = bbox.world();
    meta["boundingBox"]["ilxl"] = bbox.annotation();


    Axis const& inline_axis = metadata.iline();
    meta["axis"].push_back(json_axis(inline_axis));

    Axis const& crossline_axis = metadata.xline();
    meta["axis"].push_back(json_axis(crossline_axis));

    Axis const& sample_axis = metadata.sample();
    meta["axis"].push_back(json_axis(sample_axis));

    return to_response(meta, out);
}

void attributes_metadata(
    DataHandle& handle,
    std::size_t nrows,
    std::size_t ncols,
    response* out
) {
    MetadataHandle const& metadata = handle.get_metadata();

    nlohmann::json meta;
    meta["shape"] = nlohmann::json::array({nrows, ncols});
    meta["format"] = fmtstr(DataHandle::format());

    return to_response(meta, out);
}

} // namespace cppapi
