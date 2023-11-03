#ifndef VDS_SLICE_SUBCUBE_HPP
#define VDS_SLICE_SUBCUBE_HPP

#include <OpenVDS/OpenVDS.h>

#include "axis.hpp"
#include "metadatahandle.hpp"
#include "ctypes.h"

struct SubCube {
    struct {
        int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
        int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
    } bounds;

    SubCube(MetadataHandle const& metadata);

    void set_slice(
        Axis const&                  axis,
        int const                    lineno,
        enum coordinate_system const coordinate_system
    );

    void constrain(
        MetadataHandle const& metadata,
        std::vector< Bound > const& bounds
    ) noexcept (false);

    std::size_t size() const noexcept(true);
};

#endif /* VDS_SLICE_SUBCUBE_HPP */
