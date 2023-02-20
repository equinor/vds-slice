#include "vds.h"

#include <array>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <memory>
#include <cmath>

#include "nlohmann/json.hpp"

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

#include "axis.hpp"
#include "boundingbox.hpp"
#include "datahandle.hpp"
#include "direction.hpp"
#include "metadatahandle.hpp"
#include "subvolume.hpp"

void response_delete(struct response* buf) {
    if (!buf)
        return;

    delete[] buf->data;
    delete[] buf->err;
    *buf = response {};
}

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

/*
 * Unit validation of Z-slices
 *
 * Verify that the units of the VDS' Z axis matches the requested slice axis.
 * E.g. a Time slice is only valid if the units of the Z-axis in the VDS is
 * "Seconds" or "Milliseconds"
 */
bool unit_validation(axis_name ax, std::string const& zunit) {
    /* Define some convenient lookup tables for units */
    static const std::array< const char*, 3 > depthunits = {
        OpenVDS::KnownUnitNames::Meter(),
        OpenVDS::KnownUnitNames::Foot(),
        OpenVDS::KnownUnitNames::USSurveyFoot()
    };

    static const std::array< const char*, 2 > timeunits = {
        OpenVDS::KnownUnitNames::Millisecond(),
        OpenVDS::KnownUnitNames::Second()
    };

    static const std::array< const char*, 1 > sampleunits = {
        OpenVDS::KnownUnitNames::Unitless(),
    };

    auto isoneof = [zunit](const char* x) {
        return !std::strcmp(x, zunit.c_str());
    };

    switch (ax) {
        case I:
        case J:
        case K:
        case INLINE:
        case CROSSLINE:
            return true;
        case DEPTH:
            return std::any_of(depthunits.begin(), depthunits.end(), isoneof);
        case TIME:
            return std::any_of(timeunits.begin(), timeunits.end(), isoneof);
        case SAMPLE:
            return std::any_of(sampleunits.begin(), sampleunits.end(), isoneof);
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
};

static response to_response(nlohmann::json const& metadata) {
    auto const dump = metadata.dump();
    std::unique_ptr< char[] > tmp(new char[dump.size()]);
    std::copy(dump.begin(), dump.end(), tmp.get());
    return response{tmp.release(), nullptr, dump.size()};
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

struct response fetch_slice(
    std::string url,
    std::string credentials,
    Direction const direction,
    int lineno
) {
    DataHandle handle(url, credentials);
    MetadataHandle const& metadata = handle.get_metadata();

    Axis const& axis = metadata.get_axis(direction);
    std::string zunit = metadata.sample().unit();
    if (not unit_validation(direction.name(), zunit)) {
        std::string msg = "Unable to use " + direction.to_string();
        msg += " on cube with depth units: " + zunit;
        throw std::runtime_error(msg);
    }

    SubVolume bounds(metadata);
    bounds.set_slice(axis, lineno, direction.coordinate_system());

    std::int64_t const size = handle.subvolume_buffer_size(bounds);

    std::unique_ptr< char[] > data(new char[size]);
    handle.read_subvolume(data.get(), size, bounds);

    /* The data should *not* be free'd on success, as it's returned to CGO */
    return response{data.release(), nullptr, static_cast<unsigned long>(size)};
}

struct response fetch_slice_metadata(
    std::string url,
    std::string credentials,
    Direction const direction
) {
    DataHandle handle(url, credentials);
    MetadataHandle const& metadata = handle.get_metadata();

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

    if (direction.is_iline()) {
        meta["x"] = json_axis(sample_axis);
        meta["y"] = json_axis(crossline_axis);
    } else if (direction.is_xline()) {
        meta["x"] = json_axis(sample_axis);
        meta["y"] = json_axis(inline_axis);
    } else if (direction.is_sample()) {
        meta["x"] = json_axis(crossline_axis);
        meta["y"] = json_axis(inline_axis);
    } else {
        throw std::runtime_error("Unhandled direction");
    }

    return to_response(meta);
}

struct response fetch_fence(
    const std::string& url,
    const std::string& credentials,
    enum coordinate_system coordinate_system,
    const float* coordinates,
    size_t npoints,
    enum interpolation_method interpolation_method
) {
    DataHandle handle(url, credentials);
    MetadataHandle const& metadata = handle.get_metadata();

    std::unique_ptr< trace[] > coords(new trace[npoints]{{0}});

    auto coordinate_transformer = metadata.coordinate_transformer();
    auto transform_coordinate = [&] (const float x, const float y) {
        switch (coordinate_system) {
            case INDEX:
                return OpenVDS::Vector<double, 3> {x, y, 0};
            case ANNOTATION:
                return coordinate_transformer.AnnotationToIJKPosition({x, y, 0});
            case CDP:
                return coordinate_transformer.WorldToIJKPosition({x, y, 0});
            default: {
                throw std::runtime_error("Unhandled coordinate system");
            }
        }
    };

    Axis const& inline_axis = metadata.iline();
    Axis const& crossline_axis = metadata.xline();

    for (size_t i = 0; i < npoints; i++) {
        const float x = *(coordinates++);
        const float y = *(coordinates++);

        auto coordinate = transform_coordinate(x, y);

        auto validate_boundary = [&] (const int voxel, Axis const& axis) {
            const auto min = -0.5;
            const auto max = axis.nsamples() - 0.5;
            if(coordinate[voxel] < min || coordinate[voxel] >= max) {
                const std::string coordinate_str =
                    "(" +std::to_string(x) + "," + std::to_string(y) + ")";
                throw std::runtime_error(
                    "Coordinate " + coordinate_str + " is out of boundaries "+
                    "in dimension "+ std::to_string(voxel)+ "."
                );
            }
        };

        validate_boundary(0, inline_axis);
        validate_boundary(1, crossline_axis);

        /* openvds uses rounding down for Nearest interpolation.
         * As it is counterintuitive, we fix it by snapping to nearest index
         * and rounding half-up.
         */
        if (interpolation_method == NEAREST) {
            coordinate[0] = std::round(coordinate[0] + 1) - 1;
            coordinate[1] = std::round(coordinate[1] + 1) - 1;
        }

        coords[i][   inline_axis.dimension()] = coordinate[0];
        coords[i][crossline_axis.dimension()] = coordinate[1];
    }

    std::int64_t const size = handle.traces_buffer_size(npoints);

    std::unique_ptr< char[] > data(new char[size]);

    handle.read_traces(
        data.get(),
        size,
        coords.get(),
        npoints,
        interpolation_method
    );

    /* The data should *not* be free'd on success, as it's returned to CGO */
    return response{data.release(), nullptr, static_cast<unsigned long>(size)};
}

struct response fetch_fence_metadata(
    std::string url,
    std::string credentials,
    size_t npoints
) {
    DataHandle handle(url, credentials);
    MetadataHandle const& metadata = handle.get_metadata();

    nlohmann::json meta;
    Axis const& sample_axis = metadata.sample();
    meta["shape"] = nlohmann::json::array({npoints, sample_axis.nsamples() });
    meta["format"] = fmtstr(DataHandle::format());

    return to_response(meta);
}

struct response metadata(
    const std::string& url,
    const std::string& credentials
) {
    DataHandle handle(url, credentials);
    MetadataHandle const& metadata = handle.get_metadata();

    nlohmann::json meta;

    meta["crs"] = metadata.crs();

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

    return to_response(meta);
}

struct response to_response(std::exception const& e) {
    std::size_t size = std::char_traits<char>::length(e.what()) + 1;

    std::unique_ptr<char[]> msg(new char[size]);
    std::copy(e.what(), e.what() + size, msg.get());
    return response{nullptr, msg.release(), 0};
}

struct response slice(
    const char* vds,
    const char* credentials,
    int lineno,
    axis_name ax
) {
    std::string cube(vds);
    std::string cred(credentials);
    Direction const direction(ax);

    try {
        return fetch_slice(cube, cred, direction, lineno);
    } catch (const std::exception& e) {
        return to_response(e);
    }
}

struct response slice_metadata(
    const char* vds,
    const char* credentials,
    axis_name ax
) {
    std::string cube(vds);
    std::string cred(credentials);
    Direction const direction(ax);

    try {
        return fetch_slice_metadata(cube, cred, direction);
    } catch (const std::exception& e) {
        return to_response(e);
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
        return to_response(e);
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
        return to_response(e);
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
        return to_response(e);
    }
}
