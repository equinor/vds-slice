#ifndef VDS_SLICE_METADATAHANDLE_HPP
#define VDS_SLICE_METADATAHANDLE_HPP

#include <string>

#include <OpenVDS/OpenVDS.h>

#include "axis.hpp"
#include "boundingbox.h"

class MetadataHandle {
public:
    MetadataHandle(OpenVDS::VolumeDataLayout const * const layout)
        : m_layout(layout),
          m_iline( Axis(layout, 2)),
          m_xline( Axis(layout, 1)),
          m_sample(Axis(layout, 0))
    {}

    Axis const& iline()  const noexcept (true);
    Axis const& xline()  const noexcept (true);
    Axis const& sample() const noexcept (true);

    BoundingBox bounding_box() const noexcept (true);
    std::string crs()          const noexcept (true);
private:
    OpenVDS::VolumeDataLayout const * const m_layout;

    Axis const m_iline;
    Axis const m_xline;
    Axis const m_sample;
};

#endif /* VDS_SLICE_METADATAHANDLE_HPP */
