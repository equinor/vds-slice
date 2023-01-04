#include "poststack.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

#include <OpenVDS/KnownMetadata.h>

#include "vds.h"

namespace internal {

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

    if (not legal_names_contain(z_axis_name))
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

            if (not legal_units_contain(axis_unit)) {
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
    const AxisDescriptor axis_desc = this->get_axis(axis);
    SliceRequestValidator().validate(axis_desc);

    const SubVolume subvolume = slice_as_subvolume( axis_desc, line_number );

    return get_subvolume(subvolume, level_of_detail, channel);
}

requestdata PostStackHandle::get_fence(
    const CoordinateSystem    coordinate_system,
    float const *             coordinates,
    const size_t              npoints,
    const InterpolationMethod interpolation_method,
    const LevelOfDetail       level_of_detail,
    const Channel             channel) {

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
            const int voxel_dim = this->axis_map_->dimension_from(voxel);
            const AxisMetadata axis_meta( this->layout_, voxel_dim);
            const auto max = axis_meta.number_of_samples() - 0.5;

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

        coords[i][this->axis_map_->dimension_from(0)] = coordinate[0];
        coords[i][this->axis_map_->dimension_from(1)] = coordinate[1];
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
            get_interpolation(interpolation_method),
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

/*
 * Convert target dimension/axis + lineno to VDS voxel coordinates.
 */
SubVolume PostStackHandle::slice_as_subvolume(
    const AxisDescriptor& axis_desc,
    const int lineno
) const {
    SubVolume subvolume;
    for (int i = 0; i < 3; ++i) {
        subvolume.bounds.upper[i] = this->layout_->GetDimensionNumSamples(i);
    }

    switch (axis_desc.system()) {
        case ANNOTATION: {
            auto transformer = OpenVDS::IJKCoordinateTransformer(this->layout_);
            if (not transformer.AnnotationsDefined()) {
                throw std::runtime_error("VDS doesn't define annotations");
            }
            break;
        }
        case INDEX: {
            break;
        }
        case CDP:
        default: {
            throw std::runtime_error("Unhandled coordinate system");
        }
    }

    const int voxelline = to_voxel( axis_desc, lineno );
    const int vdim = axis_desc.voxel_dimension();
    subvolume.bounds.lower[vdim] = voxelline;
    subvolume.bounds.upper[vdim] = voxelline + 1;

    return subvolume;
}

requestdata PostStackHandle::get_subvolume(
    const SubVolume subvolume,
    const LevelOfDetail level_of_detail,
    const Channel channel ) {

    const auto format = this->layout_->GetChannelFormat(static_cast<int>(channel));
    const auto size = this->access_manager_.GetVolumeSubsetBufferSize(
        subvolume.bounds.lower,
        subvolume.bounds.upper,
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
        subvolume.bounds.lower,
        subvolume.bounds.upper,
        format );

    request.get()->WaitForCompletion();

    requestdata buffer{};
    buffer.size = size;
    buffer.data = data.get();

    /* The buffer should *not* be free'd on success, as it's returned to CGO */
    data.release();
    return buffer;
}
