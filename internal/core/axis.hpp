#ifndef VDS_SLICE_AXIS_HPP
#define VDS_SLICE_AXIS_HPP

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

class Axis {
public:
    Axis(
        float min,
        float max,
        int nsamples,
        std::string name,
        std::string unit,
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
    const float m_min;
    const float m_max;
    const int m_nsamples;
    std::string m_name;
    std::string m_unit;

    int const m_dimension;

    // redundant, but stored as a field to avoid on-fly creation in methods used
    // in loops
    OpenVDS::VolumeDataAxisDescriptor m_axis_descriptor;
};

#endif /* VDS_SLICE_AXIS_HPP */
