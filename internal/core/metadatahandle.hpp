#ifndef VDS_SLICE_METADATAHANDLE_HPP
#define VDS_SLICE_METADATAHANDLE_HPP

#include <string>

#include <OpenVDS/OpenVDS.h>

#include "axis.hpp"
#include "boundingbox.hpp"
#include "direction.hpp"

class MetadataHandle {
public:
    MetadataHandle(OpenVDS::VolumeDataLayout const * const layout);

    Axis iline()  const noexcept (true);
    Axis xline()  const noexcept (true);
    Axis sample() const noexcept (true);

    BoundingBox bounding_box()      const noexcept (true);
    std::string crs()               const noexcept (true);
    std::string input_filename()    const noexcept (true);
    std::string import_time_stamp() const noexcept (true);

    Axis get_axis(Direction const direction) const noexcept (false);

    OpenVDS::IJKCoordinateTransformer coordinate_transformer() const noexcept (true);
private:
    OpenVDS::VolumeDataLayout const * const m_layout;

    Axis m_iline;
    Axis m_xline;
    Axis m_sample;

    void dimension_validation() const;
    void axis_order_validation() const;
};

#endif /* VDS_SLICE_METADATAHANDLE_HPP */
