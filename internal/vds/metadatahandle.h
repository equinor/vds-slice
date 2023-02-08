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

    Axis const& get_inline()    const noexcept (true);
    Axis const& get_crossline() const noexcept (true);
    Axis const& get_sample()    const noexcept (true);

    BoundingBox const& get_bounding_box() const noexcept (true);
    std::string        get_format()       const noexcept (true);
    std::string        get_crs()          const noexcept (true);
private:
    OpenVDS::VolumeDataLayout const * const layout;

    Axis const inline_axis;
    Axis const crossline_axis;
    Axis const sample_axis;

    BoundingBox bounding_box;

    void validate_dimensionality() const;
    void validate_axes_order() const;
};

#endif /* METADATAHANDLE_H */
