#ifndef AXIS_H
#define AXIS_H

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

    int min() const noexcept(true);
    int max() const noexcept(true);

    std::string unit() const noexcept(true);
    int dimension() const noexcept(true);

    std::string name() const noexcept(true);
private:
    int const m_dimension;
    OpenVDS::VolumeDataAxisDescriptor m_axis_descriptor;
};

#endif /* AXIS_H */
