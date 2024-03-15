#include "metadatahandle.hpp"

#include <OpenVDS/KnownMetadata.h>
#include <boost/algorithm/string/join.hpp>
#include <list>
#include <stdexcept>
#include <utility>

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

OpenVDS::VolumeDataLayout const* const SingleMetadataHandle::get_layout() const noexcept(false) {
    return this->m_layout;
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
    DoubleVolumeDataLayout const* const layout,
    SingleMetadataHandle const* const m_metadata_a,
    SingleMetadataHandle const* const m_metadata_b)
    : m_layout(layout),
      m_metadata_a(m_metadata_a),
      m_metadata_b(m_metadata_b),
      m_iline(Axis(layout, get_dimension({std::string(OpenVDS::KnownAxisNames::Inline())}))),
      m_xline(Axis(layout, get_dimension({std::string(OpenVDS::KnownAxisNames::Crossline())}))),
      m_sample(Axis(layout, get_dimension({std::string(OpenVDS::KnownAxisNames::Sample()), std::string(OpenVDS::KnownAxisNames::Depth()), std::string(OpenVDS::KnownAxisNames::Time())}))) {
    this->dimension_validation();
}

Axis DoubleMetadataHandle::iline() const noexcept(true) {
    return this->m_iline;
}

Axis DoubleMetadataHandle::xline() const noexcept(true) {
    return this->m_xline;
}

Axis DoubleMetadataHandle::sample() const noexcept(true) {
    return this->m_sample;
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
    return BoundingBox(
        this->iline().nsamples(),
        this->xline().nsamples(),
        this->coordinate_transformer()
    );
}

std::string DoubleMetadataHandle::crs() const noexcept(false) {
    return this->m_metadata_a->crs() + "; " + this->m_metadata_b->crs();
}

std::string DoubleMetadataHandle::input_filename() const noexcept(false) {
    return this->m_metadata_a->input_filename() + "; " + this->m_metadata_b->input_filename();
}

std::string DoubleMetadataHandle::import_time_stamp() const noexcept(false) {
    return this->m_metadata_a->import_time_stamp() + "; " + this->m_metadata_b->import_time_stamp();
}


OpenVDS::VolumeDataLayout const* const DoubleMetadataHandle::get_layout() const noexcept(false) {
    return this->m_layout;
}

OpenVDS::IJKCoordinateTransformer DoubleMetadataHandle::coordinate_transformer() const noexcept(false) {
    return OpenVDS::IJKCoordinateTransformer(this->m_layout);
}

void DoubleMetadataHandle::dimension_validation() const {
    if (this->m_layout->GetDimensionality() != 3) {
        throw std::runtime_error(
            "Unsupported VDS, expected 3 dimensions, got " +
            std::to_string(this->m_layout->GetDimensionality())
        );
    }
}

int DoubleMetadataHandle::get_dimension(std::vector<std::string> const& names) const {
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

