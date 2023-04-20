#ifndef VDS_SLICE_AXIS_HPP
#define VDS_SLICE_AXIS_HPP

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

#include "vds.h"

class Axis {
public:
    Axis(
        OpenVDS::VolumeDataLayout const * const layout,
        int const dimension
    );

    int nsamples() const noexcept(true);

    float min() const noexcept(true);
    float max() const noexcept(true);

    float stride() const noexcept (true);

    std::string unit() const noexcept(true);
    int dimension() const noexcept(true);

    std::string name() const noexcept (true);
private:
    int const m_dimension;
    OpenVDS::VolumeDataAxisDescriptor m_axis_descriptor;
};

#endif /* VDS_SLICE_AXIS_HPP */
