#ifndef VDS_SLICE_AXIS_HPP
#define VDS_SLICE_AXIS_HPP

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

class Axis {
public:
    Axis(
        OpenVDS::VolumeDataLayout const * const layout,
        int const dimension
    );

    int nsamples() const noexcept(true);

    float min() const noexcept(true);
    float max() const noexcept(true);

    float stepsize() const noexcept (true);

    std::string unit() const noexcept(true);
    int dimension() const noexcept(true);

    std::string name() const noexcept (true);

    bool inrange(float coordinate) const noexcept(true);
    float to_sample_position(float coordinate) noexcept(false);

    bool operator!=(Axis const & other) const noexcept(false);

private:
    int const m_dimension;
    OpenVDS::VolumeDataAxisDescriptor m_axis_descriptor;
};

#endif /* VDS_SLICE_AXIS_HPP */
