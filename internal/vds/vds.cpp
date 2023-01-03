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

int axis_todim(Axis ax) {
    switch (ax) {
        case I:
        case INLINE:
            return 0;
        case J:
        case CROSSLINE:
            return 1;
        case K:
        case DEPTH:
        case TIME:
        case SAMPLE:
            return 2;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}

const std::string axis_tostring(Axis ax) {
    switch (ax) {
        case I:         return std::string( OpenVDS::KnownAxisNames::I()         );
        case J:         return std::string( OpenVDS::KnownAxisNames::J()         );
        case K:         return std::string( OpenVDS::KnownAxisNames::K()         );
        case INLINE:    return std::string( OpenVDS::KnownAxisNames::Inline()    );
        case CROSSLINE: return std::string( OpenVDS::KnownAxisNames::Crossline() );
        case DEPTH:     return std::string( OpenVDS::KnownAxisNames::Depth()     );
        case TIME:      return std::string( OpenVDS::KnownAxisNames::Time()      );
        case SAMPLE:    return std::string( OpenVDS::KnownAxisNames::Sample()    );
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}


std::string vdsformat_tostring(OpenVDS::VolumeDataFormat format) {
    switch (format) {
        case OpenVDS::VolumeDataFormat::Format_U8:  return "<u1";
        case OpenVDS::VolumeDataFormat::Format_U16: return "<u2";
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
bool unit_validation(Axis ax, const char* zunit) {
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
        return !std::strcmp(x, zunit);
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

/*
 * Until we know more about how VDS' are constructed w.r.t. to axis ordering
 * we're gonna assume that all VDS' are ordered like so:
 *
 *     voxel[0] -> depth/time/sample
 *     voxel[1] -> crossline
 *     voxel[2] -> inline
 *
 * This function will return 0 if that's not the case
 */
bool axis_order_validation(const OpenVDS::VolumeDataLayout *layout) {
    if (std::strcmp(layout->GetDimensionName(2), OpenVDS::KnownAxisNames::Inline())) {
        return false;
    }

    if (std::strcmp(layout->GetDimensionName(1), OpenVDS::KnownAxisNames::Crossline())) {
        return false;
    }

    auto z = layout->GetDimensionName(0);

    /* Define some convenient lookup tables for units */
    static const std::array< const char*, 3 > depth = {
        OpenVDS::KnownAxisNames::Depth(),
        OpenVDS::KnownAxisNames::Time(),
        OpenVDS::KnownAxisNames::Sample()
    };

    auto isoneof = [z](const char* x) {
        return !std::strcmp(x, z);
    };

    return std::any_of(depth.begin(), depth.end(), isoneof);
}

void axis_validation(Axis ax, const OpenVDS::VolumeDataLayout* layout) {
    if (not axis_order_validation(layout)) {
        std::string msg = "Unsupported axis ordering in VDS, expected ";
        msg += "Depth/Time/Sample, Crossline, Inline";
        throw std::runtime_error(msg);
    }

    auto zaxis = layout->GetAxisDescriptor(0);
    const char* zunit = zaxis.GetUnit();
    if (not unit_validation(ax, zunit)) {
        std::string msg = "Unable to use " + axis_tostring(ax);
        msg += " on cube with depth units: " + std::string(zunit);
        throw std::runtime_error(msg);
    }
}

nlohmann::json json_axis(
    int voxel_dim,
    const OpenVDS::VolumeDataLayout *layout
) {
    nlohmann::json doc;
    doc = {
        { "annotation", layout->GetDimensionName(voxel_dim)       },
        { "min",        layout->GetDimensionMin(voxel_dim)        },
        { "max",        layout->GetDimensionMax(voxel_dim)        },
        { "samples",    layout->GetDimensionNumSamples(voxel_dim) },
        { "unit",       layout->GetDimensionUnit(voxel_dim)       },
    };
    return doc;
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

OpenVDS::ScopedVDSHandle open_vds(
    std::string url,
    std::string credentials
){
    OpenVDS::Error error;
    OpenVDS::ScopedVDSHandle handle = OpenVDS::Open(url, credentials, error);

    if(error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }
    return handle;
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

        auto str = meta.dump();
        auto *data = new char[str.size()];
        std::copy(str.begin(), str.end(), data);

        requestdata buffer{};
        buffer.size = str.size();
        buffer.data = data;

        return buffer;
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

        auto str = meta.dump();
        auto *data = new char[str.size()];
        std::copy(str.begin(), str.end(), data);

        requestdata buffer{};
        buffer.size = str.size();
        buffer.data = data;

        return buffer;
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

        auto str = meta.dump();
        auto *data = new char[str.size()];
        std::copy(str.begin(), str.end(), data);

        requestdata buffer{};
        buffer.size = str.size();
        buffer.data = data;

        return buffer;
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}
