#include "metadatahandle.hpp"

#include <stdexcept>
#include <list>
#include <utility>

#include <OpenVDS/KnownMetadata.h>

#include "axis.hpp"
#include "boundingbox.h"
#include "direction.hpp"

MetadataHandle::MetadataHandle(OpenVDS::VolumeDataLayout const * const layout)
    : m_layout(layout),
      m_iline( Axis(layout, 2)),
      m_xline( Axis(layout, 1)),
      m_sample(Axis(layout, 0))
{
    this->dimension_validation();
    this->axis_order_validation();
}

void MetadataHandle::dimension_validation() const {
    if (this->m_layout->GetDimensionality() != 3) {
        throw std::runtime_error(
            "Unsupported VDS, expected 3 dimensions, got " +
            std::to_string(this->m_layout->GetDimensionality())
        );
    }
}

/*
 * We're checking our axis order requirement.
 *
 *     voxel[0] -> sample
 *     voxel[1] -> crossline
 *     voxel[2] -> inline
 */
void MetadataHandle::axis_order_validation() const {
    std::string const msg = "Unsupported axis ordering in VDS, expected "
        "Sample, Crossline, Inline";

    using Names = OpenVDS::KnownAxisNames;

    if (this->sample().name() != std::string(Names::Sample())) {
        throw std::runtime_error(msg);
    }

    if (this->xline().name() != std::string(Names::Crossline())) {
        throw std::runtime_error(msg);
    }

    if (this->iline().name() != std::string(Names::Inline())) {
        throw std::runtime_error(msg);
    }
}

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

Axis const& MetadataHandle::get_axis(
    Direction const direction
) const noexcept (false) {
    if      (direction.is_iline())  return this->iline();
    else if (direction.is_xline())  return this->xline();
    else if (direction.is_sample()) return this->sample();

    throw std::runtime_error("Unhandled axis");
}
