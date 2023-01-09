#include "seismichandle.h"

#include<iostream>
#include<stdexcept>

#include <OpenVDS/KnownMetadata.h>

AxisMetadata::AxisMetadata(
    const OpenVDS::VolumeDataLayout* layout,
    const int voxel_dimension
) noexcept (true)
    : axis_descriptor_(layout->GetAxisDescriptor(voxel_dimension))
{}

int AxisMetadata::min() const noexcept (true) {
    return this->axis_descriptor_.GetCoordinateMin();
}

int AxisMetadata::max() const noexcept (true) {
    return this->axis_descriptor_.GetCoordinateMax();
}

int AxisMetadata::number_of_samples() const noexcept (true) {
    return this->axis_descriptor_.GetNumSamples();
}

std::string AxisMetadata::name() const noexcept (true) {
    std::string name = this->axis_descriptor_.GetName();
    return name;
}

std::string AxisMetadata::unit() const noexcept (true) {
    std::string unit = this->axis_descriptor_.GetUnit();
    return unit;
}


AxisDescriptor::AxisDescriptor(
    const Axis axis,
    const OpenVDS::VolumeDataLayout* layout,
    const int voxel_dimension
) noexcept (true)
    : AxisMetadata( layout, voxel_dimension ),
    axis_(axis),
    voxel_dimension_(voxel_dimension)
{}

CoordinateSystem AxisDescriptor::system() const {
    switch (this->axis_) {
        case I:
        case J:
        case K:
            return CoordinateSystem::INDEX;
        case INLINE:
        case CROSSLINE:
        case DEPTH:
        case TIME:
        case SAMPLE:
            return CoordinateSystem::ANNOTATION;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}

Axis AxisDescriptor::value() const noexcept (true) {
    return this->axis_;
}

int AxisDescriptor::space_dimension() const {
    switch (this->axis_) {
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

int AxisDescriptor::voxel_dimension() const noexcept (true){
    return this->voxel_dimension_;
}

std::string AxisDescriptor::api_name() const {
    switch (this->axis_) {
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

void SeismicHandle::SeismicValidator::validate( const SeismicHandle& vds_handle ) {
    if (vds_handle.layout_->GetDimensionality() != expected_dimensionality_) {
        throw std::runtime_error(
            "Unsupported VDS, expected 3 dimensions, got " +
            std::to_string(vds_handle.layout_->GetDimensionality())
        );
    }
    // TODO: Check if CRS is non-empty?
}

SeismicHandle::SeismicHandle(
    const std::string    url,
    const std::string    connection,
    const Channel        default_channel,
    const LevelOfDetail  default_lod,
    std::unique_ptr<SeismicAxisMap> axis_map ) : axis_map_(std::move(axis_map)),
                                                 default_channel_(default_channel),
                                                 default_lod_(default_lod) {

    OpenVDS::Error error;
    handle_ = OpenVDS::Open( url, connection, error );
    if(error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }

    access_manager_ = OpenVDS::GetAccessManager(handle_);
    layout_ = access_manager_.GetVolumeDataLayout();

    SeismicValidator().validate(*this);
}

// Maps from our Axis to an axis descriptor.
AxisDescriptor SeismicHandle::get_axis(Axis axis) const {
    switch (axis) {
        case Axis::I:
        case Axis::INLINE: {
            return AxisDescriptor(
                axis, this->layout_, this->axis_map_->iline()
            );
        }
        case Axis::J:
        case Axis::CROSSLINE: {
            return AxisDescriptor(
                axis, this->layout_, this->axis_map_->xline()
            );
        }
        case Axis::K:
        case Axis::DEPTH:
        case Axis::TIME:
        case Axis::SAMPLE: {
            return AxisDescriptor(
                axis, this->layout_, this->axis_map_->sample()
            );
        }
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}

BoundingBox SeismicHandle::get_bounding_box() const noexcept (true) {
    return BoundingBox( this->layout_ );
}

std::string SeismicHandle::get_crs() const {
    const auto crs = OpenVDS::KnownMetadata::SurveyCoordinateSystemCRSWkt();
    return this->layout_->GetMetadataString(crs.GetCategory(), crs.GetName());
}

std::string SeismicHandle::get_format(Channel ch) const {
    const OpenVDS::VolumeDataFormat format =
        this->layout_->GetChannelFormat(static_cast<int>(ch)); //TODO: Check if we can avoid the casting

    switch (format) {
        case OpenVDS::VolumeDataFormat::Format_U8:
            return "<u1";
        case OpenVDS::VolumeDataFormat::Format_U16:
            return "<u2";
        case OpenVDS::VolumeDataFormat::Format_R32:
            return "<f4";
        case OpenVDS::VolumeDataFormat::Format_Any:
        case OpenVDS::VolumeDataFormat::Format_1Bit:
        case OpenVDS::VolumeDataFormat::Format_U32:
        case OpenVDS::VolumeDataFormat::Format_U64:
        case OpenVDS::VolumeDataFormat::Format_R64:
            throw std::runtime_error("unsupported VDS format type");
    }
}

std::vector<AxisMetadata> SeismicHandle::get_all_axes_metadata() const {
    return {
        AxisMetadata( this->layout_, Axis::I ),
        AxisMetadata( this->layout_, Axis::J ),
        AxisMetadata( this->layout_, Axis::K ),
    };
}

int SeismicHandle::to_voxel(
    const AxisDescriptor& axis_desc,
    const int lineno) const {

    /* Assume that annotation coordinates are integers
     * Line-numbers in IJK match Voxel - do bound checking and return
     */
    const bool is_annnotation_system = (axis_desc.system() == CoordinateSystem::ANNOTATION);
    const int nsamples = axis_desc.number_of_samples();

    const int min    = (is_annnotation_system) ? axis_desc.min() : 0;
    const int max    = (is_annnotation_system) ? axis_desc.max() : nsamples-1;
    const int stride = (is_annnotation_system) ? (max - min) / (nsamples - 1) : 1;

    if (lineno < min || lineno > max || (lineno - min) % stride ) {
        std::string msg = "Invalid lineno: " + std::to_string(lineno) +
                          ", valid range: [" + std::to_string(min) +
                          ":" + std::to_string(max);
        if  (axis_desc.system() == CoordinateSystem::ANNOTATION) {
            msg += ":" + std::to_string(stride) + "]";
        }
        else {
            msg += ":1]";
        }

        throw std::runtime_error(msg);
    }
    return (lineno - min) / stride;
}

OpenVDS::InterpolationMethod SeismicHandle::get_interpolation(
    InterpolationMethod interpolation
) {
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
