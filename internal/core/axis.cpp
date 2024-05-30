#include "axis.hpp"

#include <stdexcept>
#include "exceptions.hpp"

#include <OpenVDS/IJKCoordinateTransformer.h>
#include <OpenVDS/KnownMetadata.h>
#include "utils.hpp"


Axis::Axis(
    OpenVDS::VolumeDataLayout const* const layout,
    int const                              dimension
) : m_dimension(dimension),
    m_axis_descriptor(layout->GetAxisDescriptor(dimension))
{}

float Axis::min() const noexcept(true) {
    return this->m_axis_descriptor.GetCoordinateMin();
}

float Axis::max() const noexcept(true) {
    return this->m_axis_descriptor.GetCoordinateMax();
}

int Axis::nsamples() const noexcept(true) {
    return this->m_axis_descriptor.GetNumSamples();
}

std::string Axis::unit() const noexcept(true) {
    return this->m_axis_descriptor.GetUnit();
}

int Axis::dimension() const noexcept(true) {
    return this->m_dimension;
}

float Axis::stepsize() const noexcept (true) {
    return (this->max() - this->min()) / (this->nsamples() - 1);
}

std::string Axis::name() const noexcept(true) {
    return this->m_axis_descriptor.GetName();
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

