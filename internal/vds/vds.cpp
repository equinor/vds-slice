#include "vds.h"

#include <stdio.h>
#include <stdlib.h>

#include <array>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <iostream>
#include <memory>

#include "nlohmann/json.hpp"

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

using namespace std;

void vdsbuffer_delete(struct vdsbuffer* buf) {
    if (!buf)
        return;

    delete[] buf->data;
    delete[] buf->err;
    *buf = vdsbuffer {};
}

enum coord_system {
    INDEX      = 0,
    ANNOTATION = 1,
};

int axis_todim(axis ax) {
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

coord_system axis_tosystem(axis ax) {
    switch (ax) {
        case I:
        case J:
        case K:
            return INDEX;
        case INLINE:
        case CROSSLINE:
        case DEPTH:
        case TIME:
        case SAMPLE:
            return ANNOTATION;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}

const std::string axis_tostring(axis ax) {
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

OpenVDS::InterpolationMethod to_interpolation(interpolation_method interpolation) {
    switch (interpolation)
    {
        case NEAREST: return OpenVDS::InterpolationMethod::Nearest;
        case LINEAR: return OpenVDS::InterpolationMethod::Linear;
        case CUBIC: return OpenVDS::InterpolationMethod::Cubic;
        case ANGULAR: return OpenVDS::InterpolationMethod::Angular;
        case TRIANGULAR: return OpenVDS::InterpolationMethod::Triangular;
        default: {
            throw std::runtime_error("Unhandled interpolation method");
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
bool unit_validation(axis ax, const char* zunit) {
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
bool axis_order_validation(axis ax, const OpenVDS::VolumeDataLayout *layout) {
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


void axis_validation(axis ax, const OpenVDS::VolumeDataLayout* layout) {
    if (not axis_order_validation(ax, layout)) {
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

int dim_tovoxel(int dimension) {
    /*
     * For now assume that the axis order is always depth/time/sample,
     * crossline, inline. This should be checked elsewhere.
     */
    switch (dimension) {
        case 0: return 2;
        case 1: return 1;
        case 2: return 0;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}

int lineno_annotation_to_voxel(
    int lineno,
    int vdim,
    const OpenVDS::VolumeDataLayout *layout
) {
    /* Assume that annotation coordinates are integers */
    int min      = layout->GetDimensionMin(vdim);
    int max      = layout->GetDimensionMax(vdim);
    int nsamples = layout->GetDimensionNumSamples(vdim);

    auto stride = (max - min) / (nsamples - 1);

    if (lineno < min || lineno > max || (lineno - min) % stride) {
        throw std::runtime_error(
            "Invalid lineno: " + std::to_string(lineno) +
            ", valid range: [" + std::to_string(min) +
            ":" + std::to_string(max) +
            ":" + std::to_string(stride) + "]"
        );
    }

    int voxelline = (lineno - min) / stride;
    return voxelline;
}

int lineno_index_to_voxel(
    int lineno,
    int vdim,
    const OpenVDS::VolumeDataLayout *layout
) {
    /* Line-numbers in IJK match Voxel - do bound checking and return*/
    int min = 0;
    int max = layout->GetDimensionNumSamples(vdim) - 1;

    if (lineno < min || lineno > max) {
        throw std::runtime_error(
            "Invalid lineno: " + std::to_string(lineno) +
            ", valid range: [" + std::to_string(min) +
            ":" + std::to_string(max) +
            ":1]"
        );
    }

    return lineno;
}

/*
 * Convert target dimension/axis + lineno to VDS voxel coordinates.
 */
void set_voxels(
    axis ax,
    int dimension,
    int lineno,
    const OpenVDS::VolumeDataLayout *layout,
    int (&voxelmin)[OpenVDS::VolumeDataLayout::Dimensionality_Max],
    int (&voxelmax)[OpenVDS::VolumeDataLayout::Dimensionality_Max]
) {
    auto vmin = OpenVDS::IntVector3 { 0, 0, 0 };
    auto vmax = OpenVDS::IntVector3 {
        layout->GetDimensionNumSamples(0),
        layout->GetDimensionNumSamples(1),
        layout->GetDimensionNumSamples(2)
    };

    int voxelline;
    auto vdim   = dim_tovoxel(dimension);
    auto system = axis_tosystem(ax);
    switch (system) {
        case ANNOTATION: {
            auto transformer = OpenVDS::IJKCoordinateTransformer(layout);
            if (not transformer.AnnotationsDefined()) {
                throw std::runtime_error("VDS doesn't define annotations");
            }
            voxelline = lineno_annotation_to_voxel(lineno, vdim, layout);
            break;
        }
        case INDEX: {
            voxelline = lineno_index_to_voxel(lineno, vdim, layout);
            break;
        }
    }

    vmin[vdim] = voxelline;
    vmax[vdim] = voxelline + 1;

    /* Commit */
    for (int i = 0; i < 3; i++) {
        voxelmin[i] = vmin[i];
        voxelmax[i] = vmax[i];
    }
}

void json_write_axis(
    nlohmann::json& json,
    int voxel_dim,
    const OpenVDS::VolumeDataLayout *layout
) {
    json.push_back({
        { "annotation", layout->GetDimensionName(voxel_dim)       },
        { "min",        layout->GetDimensionMin(voxel_dim)        },
        { "max",        layout->GetDimensionMax(voxel_dim)        },
        { "samples",    layout->GetDimensionNumSamples(voxel_dim) },
        { "unit",       layout->GetDimensionUnit(voxel_dim)       },
    });
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

struct vdsbuffer fetch_slice(
    std::string url,
    std::string credentials,
    axis ax,
    int lineno
) {
    auto handle = open_vds(url, credentials);

    auto access = OpenVDS::GetAccessManager(handle);
    auto const *layout = access.GetVolumeDataLayout();

    if (layout->GetDimensionality() != 3) {
        throw std::runtime_error(
            "Unsupported VDS, expected 3 dimensions, got " +
            std::to_string(layout->GetDimensionality())
        );
    }

    axis_validation(ax, layout);

    int vmin[OpenVDS::Dimensionality_Max] = { 0, 0, 0, 0, 0, 0};
    int vmax[OpenVDS::Dimensionality_Max] = { 1, 1, 1, 1, 1, 1};
    auto dimension = axis_todim(ax);
    set_voxels(ax, dimension, lineno, layout, vmin, vmax);

    auto format = layout->GetChannelFormat(0);
    auto size = access.GetVolumeSubsetBufferSize(vmin, vmax, format, 0, 0);

    std::unique_ptr< char[] > data(new char[size]());
    auto request = access.RequestVolumeSubset(
        data.get(),
        size,
        OpenVDS::Dimensions_012,
        0,
        0,
        vmin,
        vmax,
        format
    );

    request.get()->WaitForCompletion();

    vdsbuffer buffer{};
    buffer.size = size;
    buffer.data = data.get();

    /* The buffer should *not* be free'd on success, as it's returned to CGO */
    data.release();
    return buffer;
}

struct vdsbuffer fetch_slice_metadata(
    std::string url,
    std::string credentials,
    axis ax,
    int lineno
) {
    auto handle = open_vds(url, credentials);

    auto access = OpenVDS::GetAccessManager(handle);
    auto const *layout = access.GetVolumeDataLayout();

    if (layout->GetDimensionality() != 3) {
        throw std::runtime_error(
            "Unsupported VDS, expected 3 dimensions, got " +
            std::to_string(layout->GetDimensionality())
        );
    }

    axis_validation(ax, layout);

    auto dimension = axis_todim(ax);
    auto vdim = dim_tovoxel(dimension);

    nlohmann::json meta;
    meta["format"] = layout->GetChannelFormat(0); //TODO turn into numpy-style format?
    meta["axis"]   = nlohmann::json::array();

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
    for (int i = 2; i >= 0; i--) {
        if (i == vdim) continue;
        json_write_axis(meta["axis"], i, layout);
    }

    auto str = meta.dump();
    auto *data = new char[str.size()];
    std::copy(str.begin(), str.end(), data);

    vdsbuffer buffer{};
    buffer.size = str.size();
    buffer.data = data;

    return buffer;
}

struct vdsbuffer fetch_fence(
    const std::string& url,
    const std::string& credentials,
    const std::string& coordinate_system,
    const float* coordinates,
    size_t npoints,
    enum interpolation_method interpolation_method
) {
    auto handle = open_vds(url, credentials);

    auto accessManager = OpenVDS::GetAccessManager(handle);
    auto const *layout = accessManager.GetVolumeDataLayout();
    const auto dimension_map =
            layout->GetVDSIJKGridDefinitionFromMetadata().dimensionMap;

    unique_ptr< float[][OpenVDS::Dimensionality_Max] > coords(
        new float[npoints][OpenVDS::Dimensionality_Max]{{0}}
    );

    auto coordinate_transformer = OpenVDS::IJKCoordinateTransformer(layout);
    auto transform_coordinate = [&] (const float x, const float y) {
        if (coordinate_system == "ij") {
            return OpenVDS::Vector<double, 3> {x, y, 0};
        }
        else if (coordinate_system == "ilxl") {
            return coordinate_transformer.AnnotationToIJKPosition({x, y, 0});
        }
        else if (coordinate_system == "cdp") {
            return coordinate_transformer.WorldToIJKPosition({x, y, 0});
        }
        else {
            const auto msg =
                    "Coordinate system not recognized: " + coordinate_system;
            throw std::runtime_error(msg);
        }
    };

    for (size_t i = 0; i < npoints; i++) {
        const float x = *(coordinates++);
        const float y = *(coordinates++);

        const auto coordinate = transform_coordinate(x, y);

        coords[i][dimension_map[0]] = coordinate[0];
        coords[i][dimension_map[1]] = coordinate[1];
    }

    // TODO: Verify that trace dimension is always 0
    auto size = accessManager.GetVolumeTracesBufferSize(npoints, 0);

    std::unique_ptr< char[] > data(new char[size]());

    auto request = accessManager.RequestVolumeTraces(
            (float*)data.get(),
            size,
            OpenVDS::Dimensions_012, 0, 0,
            coords.get(),
            npoints,
            to_interpolation(interpolation_method),
            0
    );
    bool success = request.get()->WaitForCompletion();

    if(!success) {
        const auto msg = "Failed to fetch fence from VDS";
        throw std::runtime_error(msg);
    }

    vdsbuffer buffer{};
    buffer.size = size;
    buffer.data = data.get();

    data.release();

    return buffer;
}

struct vdsbuffer fetch_fence_metadata(
    std::string url,
    std::string credentials,
    size_t npoints
) {
    auto handle = open_vds(url, credentials);

    auto access = OpenVDS::GetAccessManager(handle);
    auto const *layout = access.GetVolumeDataLayout();

    if (layout->GetDimensionality() != 3) {
        throw std::runtime_error(
            "Unsupported VDS, expected 3 dimensions, got " +
            std::to_string(layout->GetDimensionality())
        );
    }

    nlohmann::json meta;
    meta["shape"] = nlohmann::json::array({npoints, layout->GetDimensionNumSamples(0)});

    auto str = meta.dump();
    auto *data = new char[str.size()];
    std::copy(str.begin(), str.end(), data);

    vdsbuffer buffer{};
    buffer.size = str.size();
    buffer.data = data;

    return buffer;
}

struct vdsbuffer metadata(
    const std::string& url,
    const std::string& credentials
) {
    auto handle = open_vds(url, credentials);

    auto access = OpenVDS::GetAccessManager(handle);
    const auto *layout = access.GetVolumeDataLayout();

    nlohmann::json meta;
    meta["format"] = layout->GetChannelFormat(0); //TODO turn into numpy-style format?
    for (int i = 2; i >= 0 ; i--) {
        json_write_axis(meta["axis"], i, layout);
    }
    auto str = meta.dump();
    auto *data = new char[str.size()];
    std::copy(str.begin(), str.end(), data);

    vdsbuffer buffer{};
    buffer.size = str.size();
    buffer.data = data;

    return buffer;
}

struct vdsbuffer slice(
    const char* vds,
    const char* credentials,
    int lineno,
    axis ax
) {
    std::string cube(vds);
    std::string cred(credentials);

    try {
        return fetch_slice(cube, cred, ax, lineno);
    } catch (const std::exception& e) {
        vdsbuffer buf {};
        buf.err = new char[std::strlen(e.what()) + 1];
        std::strcpy(buf.err, e.what());
        return buf;
    }
}

struct vdsbuffer slice_metadata(
    const char* vds,
    const char* credentials,
    int lineno,
    axis ax
) {
    std::string cube(vds);
    std::string cred(credentials);

    try {
        return fetch_slice_metadata(cube, cred, ax, lineno);
    } catch (const std::exception& e) {
        vdsbuffer buf{};
        buf.err = new char[std::strlen(e.what()) + 1];
        std::strcpy(buf.err, e.what());
        return buf;
    }
}

struct vdsbuffer fence(
    const char* vds,
    const char* credentials,
    const char* coordinate_system,
    const float* coordinates,
    size_t npoints,
    enum interpolation_method interpolation_method
) {
    try {
        std::string cube(vds);
        std::string cred(credentials);
        std::string coord_system(coordinate_system);

        return fetch_fence(cube, cred, coord_system, coordinates, npoints, interpolation_method);
    } catch (const std::exception& e) {
        vdsbuffer buf {};
        buf.err = new char[std::strlen(e.what()) + 1];
        std::strcpy(buf.err, e.what());
        return buf;
    }
}

struct vdsbuffer fence_metadata(
    const char* vds,
    const char* credentials,
    size_t npoints
) {
    std::string cube(vds);
    std::string cred(credentials);

    try {
        return fetch_fence_metadata(cube, cred, npoints);
    } catch (const std::exception& e) {
        vdsbuffer buf{};
        buf.err = new char[std::strlen(e.what()) + 1];
        std::strcpy(buf.err, e.what());
        return buf;
    }
}

struct vdsbuffer metadata(
    const char* vds,
    const char* credentials
) {
    try {
        std::string cube(vds);
        std::string cred(credentials);
        return metadata(cube, cred);
    } catch (const std::exception& e) {
        vdsbuffer buf {};
        buf.err = new char[std::strlen(e.what()) + 1];
        std::strcpy(buf.err, e.what());
        return buf;
    }
}
