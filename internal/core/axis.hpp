#ifndef VDS_SLICE_AXIS_HPP
#define VDS_SLICE_AXIS_HPP

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

class Axis {
public:
    Axis(
        OpenVDS::VolumeDataLayout const* const layout,
        int const dimension
    );

    int nsamples() const noexcept(true);

    float min() const noexcept(true);
    float max() const noexcept(true);

    float stepsize() const noexcept (true);

    std::string unit() const noexcept(true);
    int dimension() const noexcept(true);

    std::string name() const noexcept (true);

    /**
     * Checks if coordinate falls inside axis range
     */
    bool inrange(float coordinate) const noexcept(true);

    /**
     * Checks if coordinate falls inside axis or is inside half a sample
     * outside the boundary with inclusivity as
     *
     * [-0.5*stepsize + min; max +0.5*stepsize)
     */
    bool inrange_with_margin(float coordinate) const noexcept(true);
    float to_sample_position(float coordinate) noexcept(false);

private:
    int const m_dimension;
    OpenVDS::VolumeDataAxisDescriptor m_axis_descriptor;
};

#endif /* VDS_SLICE_AXIS_HPP */
