#ifndef VDS_SLICE_METADATAHANDLE_HPP
#define VDS_SLICE_METADATAHANDLE_HPP

#include <string>

#include <OpenVDS/OpenVDS.h>

#include "axis.hpp"
#include "boundingbox.h"
#include "direction.hpp"

class MetadataHandle {
public:
    MetadataHandle(OpenVDS::VolumeDataLayout const * const layout);

    Axis const& iline()  const noexcept (true);
    Axis const& xline()  const noexcept (true);
    Axis const& sample() const noexcept (true);

    BoundingBox bounding_box() const noexcept (true);
    std::string crs()          const noexcept (true);
    std::string format()       const noexcept (false);

    Axis const& get_axis(Direction const direction) const noexcept (false);
private:
    OpenVDS::VolumeDataLayout const * const m_layout;

    Axis const m_iline;
    Axis const m_xline;
    Axis const m_sample;

    void dimension_validation() const;
    void axis_order_validation() const;
};

#endif /* VDS_SLICE_METADATAHANDLE_HPP */
