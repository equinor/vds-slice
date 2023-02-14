#include "metadatahandle.hpp"

#include <stdexcept>
#include <list>
#include <utility>

#include <OpenVDS/KnownMetadata.h>

#include "axis.hpp"
#include "boundingbox.h"

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

std::string MetadataHandle::format() const noexcept (false) {
    auto format = this->m_layout->GetChannelFormat(0);
    switch (format) {
        case OpenVDS::VolumeDataFormat::Format_U8:  return "<u1";
        case OpenVDS::VolumeDataFormat::Format_U16: return "<u2";
        case OpenVDS::VolumeDataFormat::Format_R32: return "<f4";
        default: {
            throw std::runtime_error("unsupported VDS format type");
        }
    }
}
