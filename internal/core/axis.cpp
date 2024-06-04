#include "axis.hpp"

#include <stdexcept>
#include "exceptions.hpp"

#include <OpenVDS/IJKCoordinateTransformer.h>
#include <OpenVDS/KnownMetadata.h>
#include "utils.hpp"


Axis::Axis(
    float min, float max, int nsamples, std::string name, std::string unit, int const dimension
) : m_dimension(dimension),
    m_min(min),
    m_max(max),
    m_nsamples(nsamples),
    m_name(name),
    m_unit(unit),
    m_axis_descriptor(OpenVDS::VolumeDataAxisDescriptor(
        m_nsamples,
        m_name.c_str(),
        m_unit.c_str(),
        m_min,
        m_max
    )) {}

float Axis::min() const noexcept(true) {
    return this->m_min;
}

float Axis::max() const noexcept(true) {
    return this->m_max;
}

int Axis::nsamples() const noexcept(true) {
    return this->m_nsamples;
}

std::string Axis::unit() const noexcept(true) {
    return this->m_unit;
}

int Axis::dimension() const noexcept(true) {
    return this->m_dimension;
}

float Axis::stepsize() const noexcept (true) {
    return (this->max() - this->min()) / (this->nsamples() - 1);
}

std::string Axis::name() const noexcept(true) {
    return this->m_name;
}

bool Axis::inrange(float coordinate) const noexcept(true) {
    return this->min() <= coordinate && this->max() >= coordinate;
}

bool Axis::inrange_with_margin(float coordinate) const noexcept(true) {
    return (this->min() - 0.5 * this->stepsize()) <= coordinate &&
           (this->max() + 0.5 * this->stepsize()) > coordinate;
}

float Axis::to_sample_position(float coordinate) noexcept(false) {
    return this->m_axis_descriptor.CoordinateToSamplePosition(coordinate);
}

