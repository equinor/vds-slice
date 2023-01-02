#include "seismichandle.h"

#include<iostream>
#include<stdexcept>

#include <OpenVDS/KnownMetadata.h>

AxisMetadata::AxisMetadata(
    const OpenVDS::VolumeDataLayout* layout,
    const int voxel_dimension
) noexcept
    : axis_descriptor_(layout->GetAxisDescriptor(voxel_dimension))
{}

int AxisMetadata::min() const {
    return this->axis_descriptor_.GetCoordinateMin();
}

int AxisMetadata::max() const {
    return this->axis_descriptor_.GetCoordinateMax();
}

int AxisMetadata::number_of_samples() const {
    return this->axis_descriptor_.GetNumSamples();
}

std::string AxisMetadata::name() const {
    std::string name = this->axis_descriptor_.GetName();
    return name;
}

std::string AxisMetadata::unit() const {
    std::string unit = this->axis_descriptor_.GetUnit();
    return unit;
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

// Maps from our Axis to a VDS axisDescriptor.
OpenVDS::VolumeDataAxisDescriptor SeismicHandle::get_axis(Axis axis) const {
    throw std::runtime_error("Not implemented");
}

BoundingBox SeismicHandle::get_bounding_box() const {
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
