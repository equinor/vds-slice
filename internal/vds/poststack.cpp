#include "poststack.h"

#include <algorithm>
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

OpenVDS::InterpolationMethod to_interpolation(InterpolationMethod interpolation) {
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

int voxel_todim(int voxel) {
    /*
     * For now assume that the axis order is always depth/time/sample,
     * crossline, inline. This should be checked elsewhere.
     */
    switch (voxel) {
        case 0: return 2;
        case 1: return 1;
        case 2: return 0;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}

/*
 * Convert target dimension/axis + lineno to VDS voxel coordinates.
 */
void set_voxels(
    const AxisDescriptor& axis_desc,
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
    const int vdim = axis_desc.voxel_dimension();
    switch (axis_desc.system()) {
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

const std::unordered_map<std::string, std::list<std::string>>
PostStackHandle::PostStackValidator::valid_z_axis_combinations_ = {
  { std::string( OpenVDS::KnownAxisNames::Depth() ),
    {
        std::string(OpenVDS::KnownUnitNames::Meter()),
        std::string(OpenVDS::KnownUnitNames::Foot()),
        std::string(OpenVDS::KnownUnitNames::USSurveyFoot())
    }
  },
  { std::string( OpenVDS::KnownAxisNames::Time() ),
    {
        std::string(OpenVDS::KnownUnitNames::Millisecond()),
        std::string(OpenVDS::KnownUnitNames::Second())
    }
  },
  {
    std::string( OpenVDS::KnownAxisNames::Sample() ),
    {
        std::string(OpenVDS::KnownUnitNames::Unitless())
    }
  }
};

PostStackHandle::PostStackValidator::PostStackValidator( const PostStackHandle& handle ) :
    handle_(handle) {
}

void PostStackHandle::PostStackValidator::validate_axes_order() {
    const std::string msg = "Unsupported axis ordering in VDS, expected "
                            "Depth/Time/Sample, Crossline, Inline";

    const AxisDescriptor inline_desc = handle_.get_axis(Axis::INLINE);
    if ( inline_desc.name() != std::string(OpenVDS::KnownAxisNames::Inline()) ) {
        throw std::runtime_error(msg);
    }

    const AxisDescriptor crossline_desc = handle_.get_axis(Axis::CROSSLINE);
    if ( crossline_desc.name() != std::string(OpenVDS::KnownAxisNames::Crossline()) ) {
        throw std::runtime_error(msg);
    }

    const AxisDescriptor sample_desc = handle_.get_axis(Axis::SAMPLE);
    const std::string z_axis_name = sample_desc.api_name();

    auto legal_names_contain = [] (const std::string name) {
        return valid_z_axis_combinations_.find( name ) != valid_z_axis_combinations_.end();
    };

    if ( not legal_names_contain( z_axis_name ) )
        throw std::runtime_error(msg);
}



void PostStackHandle::PostStackValidator::validate() {
    validate_axes_order();
}


PostStackHandle::SliceRequestValidator::SliceRequestValidator() {}

void PostStackHandle::SliceRequestValidator::validate(const AxisDescriptor& axis_desc) {
    switch (axis_desc.value()) {
        case I:
        case J:
        case K:
        case INLINE:
        case CROSSLINE:
            return;
        case DEPTH:
        case TIME:
        case SAMPLE: {
            const std::string axis_name = axis_desc.api_name();
            const std::string axis_unit = axis_desc.unit();

            const std::string msg = "Unable to use " + axis_name +
                                    " on cube with depth units: " + axis_unit;

            const std::list<std::string>& units =
                PostStackValidator::get_valid_z_axis_combinations().find(axis_name)->second;

            auto legal_units_contain = [&units] (const std::string name) {
                return std::find(units.begin(), units.end(), name) != units.end() ;
            };

            if ( not legal_units_contain(axis_unit) ) {
                throw std::runtime_error(msg);
            }
            break;
        }
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}

PostStackHandle::PostStackHandle(std::string url, std::string conn)
    : SeismicHandle(url,
                    conn,
                    Channel::Sample,
                    LevelOfDetail::Default,
                    std::make_unique<PostStackAxisMap>( 2, 1, 0 ))  // hardcode axis map, this assumption will be validated
{
    PostStackValidator(*this).validate();
}

requestdata PostStackHandle::get_slice(const Axis          axis,
                                       const int           line_number,
                                       const LevelOfDetail level_of_detail,
                                       const Channel       channel) {
    using namespace internal;

    const AxisDescriptor axis_desc = this->get_axis(axis);
    SliceRequestValidator().validate(axis_desc);

    int vmin[OpenVDS::Dimensionality_Max] = { 0, 0, 0, 0, 0, 0};
    int vmax[OpenVDS::Dimensionality_Max] = { 1, 1, 1, 1, 1, 1};
    set_voxels( axis_desc, line_number, this->layout_, vmin, vmax);

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

requestdata PostStackHandle::get_fence(
    const CoordinateSystem    coordinate_system,
    float const *             coordinates,
    const size_t              npoints,
    const InterpolationMethod interpolation_method,
    const LevelOfDetail       level_of_detail,
    const Channel             channel) {

    using namespace internal;

    const auto dimension_map =
            this->layout_->GetVDSIJKGridDefinitionFromMetadata().dimensionMap;

    std::unique_ptr< float[][OpenVDS::Dimensionality_Max] > coords(
        new float[npoints][OpenVDS::Dimensionality_Max]{{0}}
    );

    auto coordinate_transformer = OpenVDS::IJKCoordinateTransformer(this->layout_);
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

    for (size_t i = 0; i < npoints; i++) {
        const float x = *(coordinates++);
        const float y = *(coordinates++);

        auto coordinate = transform_coordinate(x, y);

        auto validate_boundary = [&] (const int voxel) {
            const auto min = -0.5;
            const auto max = this->layout_->GetDimensionNumSamples(voxel_todim(voxel))
                            - 0.5;
            if(coordinate[voxel] < min || coordinate[voxel] >= max) {
                const std::string coordinate_str =
                    "(" +std::to_string(x) + "," + std::to_string(y) + ")";
                throw std::runtime_error(
                    "Coordinate " + coordinate_str + " is out of boundaries "+
                    "in dimension "+ std::to_string(voxel)+ "."
                );
            }
        };

        validate_boundary(0);
        validate_boundary(1);

        /* openvds uses rounding down for Nearest interpolation.
         * As it is counterintuitive, we fix it by snapping to nearest index
         * and rounding half-up.
         */
        if (interpolation_method == NEAREST) {
            coordinate[0] = std::round(coordinate[0] + 1) - 1;
            coordinate[1] = std::round(coordinate[1] + 1) - 1;
        }

        coords[i][dimension_map[0]] = coordinate[0];
        coords[i][dimension_map[1]] = coordinate[1];
    }

    // TODO: Verify that trace dimension is always 0
    auto size = this->access_manager_.GetVolumeTracesBufferSize(
        npoints,
        0, //Trace dimension
        static_cast<int>(level_of_detail),
        static_cast<int>(channel)
    );

    std::unique_ptr< char[] > data(new char[size]());

    auto request = this->access_manager_.RequestVolumeTraces(
            (float*)data.get(),
            size,
            OpenVDS::Dimensions_012,
            static_cast<int>(level_of_detail),
            static_cast<int>(channel),
            coords.get(),
            npoints,
            to_interpolation(interpolation_method),
            0 // Replacement value
    );
    bool success = request.get()->WaitForCompletion();

    if(!success) {
        const auto msg = "Failed to fetch fence from VDS";
        throw std::runtime_error(msg);
    }

    requestdata buffer{};
    buffer.size = size;
    buffer.data = data.get();

    data.release();

    return buffer;
}

std::array<AxisMetadata, 2> PostStackHandle::get_slice_axis_metadata(const Axis axis) const {

    const AxisDescriptor axis_desc = this->get_axis(axis);
    auto vdim = axis_desc.voxel_dimension();
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

    return {
        AxisMetadata( this->layout_, dims[AxisDirection::X] ),
        AxisMetadata( this->layout_, dims[AxisDirection::Y] )
    };
}

std::array<AxisMetadata, 3> PostStackHandle::get_all_axes_metadata() const {
    return {
        AxisMetadata( this->layout_, AxisDirection::X ),
        AxisMetadata( this->layout_, AxisDirection::Y ),
        AxisMetadata( this->layout_, AxisDirection::Z )
    };
}
