#ifndef METADATAHANDLE_H
#define METADATAHANDLE_H

#include <string>

#include <OpenVDS/OpenVDS.h>

#include "axis.h"
#include "boundingbox.h"

class MetadataHandle {
public:
    MetadataHandle(OpenVDS::VolumeDataLayout const * const layout) :
        layout(layout),
        inline_axis(Axis(api_axis_name::INLINE, layout)),
        crossline_axis(Axis(api_axis_name::CROSSLINE, layout)),
        sample_axis(Axis(api_axis_name::SAMPLE, layout)),
        bounding_box(BoundingBox(layout, inline_axis, crossline_axis))
    {
        this->validate_dimensionality();
        this->validate_axes_order();
    }

    const Axis& get_inline()    const noexcept (true);
    const Axis& get_crossline() const noexcept (true);
    const Axis& get_sample()    const noexcept (true);

    const BoundingBox& get_bounding_box() const noexcept (true);
    std::string        get_format()       const noexcept (true);
    std::string        get_crs()          const noexcept (true);
private:
    OpenVDS::VolumeDataLayout const * const layout;

    const Axis inline_axis;
    const Axis crossline_axis;
    const Axis sample_axis;

    BoundingBox bounding_box;

    void validate_dimensionality() const;
    void validate_axes_order() const;
};

#endif /* METADATAHANDLE_H */
