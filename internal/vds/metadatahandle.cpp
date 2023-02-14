#include "metadatahandle.hpp"

#include <stdexcept>
#include <list>
#include <utility>

#include <OpenVDS/KnownMetadata.h>

Axis const& MetadataHandle::iline() const noexcept (true) {
    return this->m_iline;
}

Axis const& MetadataHandle::xline() const noexcept (true) {
    return this->m_xline;
}

Axis const& MetadataHandle::sample() const noexcept (true) {
    return this->m_sample;
}

BoundingBox MetadataHandle::bounding_box() const noexcept (true) {
    return BoundingBox(this->m_layout);
}

std::string MetadataHandle::crs() const noexcept (true) {
    auto const crs = OpenVDS::KnownMetadata::SurveyCoordinateSystemCRSWkt();
    return this->m_layout->GetMetadataString(crs.GetCategory(), crs.GetName());
}
