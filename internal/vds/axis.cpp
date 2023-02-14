#include "axis.h"

#include <stdexcept>

#include <OpenVDS/IJKCoordinateTransformer.h>
#include <OpenVDS/KnownMetadata.h>


Axis::Axis(
    OpenVDS::VolumeDataLayout const * const layout,
    int const                               dimension
) : m_dimension(dimension),
    m_axis_descriptor(layout->GetAxisDescriptor(dimension))
{}

int Axis::min() const noexcept(true) {
    return this->m_axis_descriptor.GetCoordinateMin();
}

int Axis::max() const noexcept(true) {
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

std::string Axis::name() const noexcept(true) {
    return this->m_axis_descriptor.GetName();
}
