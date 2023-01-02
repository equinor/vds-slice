#include "poststack.h"

#include <array>
#include <iostream>
#include <stdexcept>

#include <OpenVDS/KnownMetadata.h>

#include "vds.h"

namespace internal {

CoordinateSystem axis_tosystem(Axis ax) {
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

/*
 * Convert target dimension/axis + lineno to VDS voxel coordinates.
 */
void set_voxels(
    Axis ax,
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
        case CDP:
        default: {
            throw std::runtime_error("Unhandled coordinate system");
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

} /* namespace internal */

PostStackHandle::PostStackHandle(std::string url, std::string conn)
    : SeismicHandle(url,
                    conn,
                    Channel::Sample,
                    LevelOfDetail::Default,
                    std::make_unique<PostStackAxisMap>( 2, 1, 0 ))  // hardcode axis map, this assumption will be validated
{
    // TODO hardcode defaults for lod and channel and send to SeismicVDSHandle
    //if not PostStackValidator().validate(this) throw std::runtime_error("");
}

requestdata PostStackHandle::get_slice(const Axis          axis,
                                       const int           line_number,
                                       const LevelOfDetail level_of_detail,
                                       const Channel       channel) {
    using namespace internal;
    axis_validation(axis, this->layout_);

    int vmin[OpenVDS::Dimensionality_Max] = { 0, 0, 0, 0, 0, 0};
    int vmax[OpenVDS::Dimensionality_Max] = { 1, 1, 1, 1, 1, 1};
    auto dimension = axis_todim(axis);
    set_voxels(axis, dimension, line_number, this->layout_, vmin, vmax);

    auto format = this->layout_->GetChannelFormat(static_cast<int>(channel));
    auto size = this->access_manager_.GetVolumeSubsetBufferSize(
        vmin,
        vmax,
        format,
        static_cast<int>(level_of_detail),
        static_cast<int>(channel) );

    std::unique_ptr< char[] > data(new char[size]());
    auto request = this->access_manager_.RequestVolumeSubset(
        data.get(),
        size,
        OpenVDS::Dimensions_012,
        static_cast<int>(level_of_detail),
        static_cast<int>(channel),
        vmin,
        vmax,
        format );

    request.get()->WaitForCompletion();

    requestdata buffer{};
    buffer.size = size;
    buffer.data = data.get();

    /* The buffer should *not* be free'd on success, as it's returned to CGO */
    data.release();
    return buffer;
}

std::array<AxisMetadata, 2> PostStackHandle::get_axis_metadata(const Axis axis) const {
    using namespace internal;
    auto dimension = axis_todim(axis);
    auto vdim = dim_tovoxel(dimension);
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
    std::vector< int > dims;
    for (int i = 0; i < 3; ++i) {
        if (i == vdim) continue;
        dims.push_back(i);
    }

    std::array<AxisMetadata, 2> axis_metadata{
        AxisMetadata( this->layout_, dims[AxisDirection::X] ),
        AxisMetadata( this->layout_, dims[AxisDirection::Y] )
    };
    return axis_metadata;
}
