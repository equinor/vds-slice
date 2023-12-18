#include "metadatahandle.hpp"

#include <stdexcept>
#include <list>
#include <utility>
#include <boost/algorithm/string/join.hpp>
#include <OpenVDS/KnownMetadata.h>

#include "axis.hpp"
#include "boundingbox.hpp"
#include "direction.hpp"

SingleMetadataHandle::SingleMetadataHandle(OpenVDS::VolumeDataLayout const* const layout)
    : m_layout(layout),
      m_iline(Axis(layout, get_dimension({std::string(OpenVDS::KnownAxisNames::Inline())}))),
      m_xline(Axis(layout, get_dimension({std::string(OpenVDS::KnownAxisNames::Crossline())}))),
      m_sample(Axis(layout, get_dimension({std::string(OpenVDS::KnownAxisNames::Sample()), std::string(OpenVDS::KnownAxisNames::Depth()), std::string(OpenVDS::KnownAxisNames::Time())}))) {
    this->dimension_validation();
}

Axis SingleMetadataHandle::iline() const noexcept(true) {
    return this->m_iline;
}

Axis SingleMetadataHandle::xline() const noexcept(true) {
    return this->m_xline;
}

Axis SingleMetadataHandle::sample() const noexcept(true) {
    return this->m_sample;
}

Axis SingleMetadataHandle::get_axis(
    Direction const direction
) const noexcept(false) {
    if (direction.is_iline())
        return this->iline();
    else if (direction.is_xline())
        return this->xline();
    else if (direction.is_sample())
        return this->sample();

    throw std::runtime_error("Unhandled axis");
}

BoundingBox SingleMetadataHandle::bounding_box() const noexcept(false) {
    return BoundingBox(
        this->iline().nsamples(),
        this->xline().nsamples(),
        this->coordinate_transformer()
    );
}

std::string SingleMetadataHandle::crs() const noexcept(false) {
    auto const crs = OpenVDS::KnownMetadata::SurveyCoordinateSystemCRSWkt();
    return this->m_layout->GetMetadataString(crs.GetCategory(), crs.GetName());
}

std::string SingleMetadataHandle::input_filename() const noexcept(false) {
    auto const disp_name = OpenVDS::KnownMetadata::ImportInformationInputFileName();
    return this->m_layout->GetMetadataString(disp_name.GetCategory(), disp_name.GetName());
}

std::string SingleMetadataHandle::import_time_stamp() const noexcept(false) {
    auto const time_stamp = OpenVDS::KnownMetadata::ImportInformationImportTimeStamp();
    return this->m_layout->GetMetadataString(time_stamp.GetCategory(), time_stamp.GetName());
}

OpenVDS::IJKCoordinateTransformer SingleMetadataHandle::coordinate_transformer() const noexcept(false) {
    return OpenVDS::IJKCoordinateTransformer(this->m_layout);
}

void SingleMetadataHandle::dimension_validation() const {
    if (this->m_layout->GetDimensionality() != 3) {
        throw std::runtime_error(
            "Unsupported VDS, expected 3 dimensions, got " +
            std::to_string(this->m_layout->GetDimensionality())
        );
    }
}

int SingleMetadataHandle::get_dimension(std::vector<std::string> const& names) const {
    for (auto i = 0; i < this->m_layout->GetDimensionality(); i++) {
        std::string dimension_name = this->m_layout->GetDimensionName(i);
        if (std::find(names.begin(), names.end(), dimension_name) != names.end()) {
            return i;
        }
    }
    throw std::runtime_error(
        "Requested axis not found under names " + boost::algorithm::join(names, ", ") +
        " in vds file "
    );
}

DoubleMetadataHandle::DoubleMetadataHandle(
    MetadataHandle const& handle_A,
    MetadataHandle const& handle_B
)
    : m_handle_A(&handle_A),
      m_handle_B(&handle_B) {
    this->validate_metadata();
}

Axis DoubleMetadataHandle::iline() const noexcept(true) {
    // Axis iline in handle A and B are identical by validate_metadata()
    return this->m_handle_A->iline();
}

Axis DoubleMetadataHandle::xline() const noexcept(true) {
    // Axis xline in handle A and B are identical by validate_metadata()
    return this->m_handle_A->xline();
}

Axis DoubleMetadataHandle::sample() const noexcept(true) {
    // Axis sample in handle A and B are identical by validate_metadata()
    return this->m_handle_A->sample();
}

Axis DoubleMetadataHandle::get_axis(
    Direction const direction
) const noexcept(false) {
    if (direction.is_iline())
        return this->iline();
    else if (direction.is_xline())
        return this->xline();
    else if (direction.is_sample())
        return this->sample();

    throw std::runtime_error("Unhandled axis");
}

BoundingBox DoubleMetadataHandle::bounding_box() const noexcept(false) {
    throw std::runtime_error("Not implemented");
}

std::string DoubleMetadataHandle::crs() const noexcept(false) {
    throw std::runtime_error("Not implemented");
}

std::string DoubleMetadataHandle::input_filename() const noexcept(false) {
    throw std::runtime_error("Not implemented");
}

std::string DoubleMetadataHandle::import_time_stamp() const noexcept(false) {
    throw std::runtime_error("Not implemented");
}

OpenVDS::IJKCoordinateTransformer DoubleMetadataHandle::coordinate_transformer() const noexcept(false) {
    throw std::runtime_error("Not implemented");
}

void DoubleMetadataHandle::dimension_validation() const {
    this->m_handle_A->dimension_validation();
    this->m_handle_B->dimension_validation();
}

void DoubleMetadataHandle::validate_metadata() const noexcept(false) {
    this->dimension_validation();
    this->m_handle_A->iline().assert_equal(this->m_handle_B->iline());
    this->m_handle_A->xline().assert_equal(this->m_handle_B->xline());
    this->m_handle_A->sample().assert_equal(this->m_handle_B->sample());
}
