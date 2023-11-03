#include "axis.hpp"

#include <stdexcept>

#include <OpenVDS/IJKCoordinateTransformer.h>
#include <OpenVDS/KnownMetadata.h>


Axis::Axis(
    OpenVDS::VolumeDataLayout const * const layout,
    int const                               dimension
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
    return (this->min() - 0.5 * this->stepsize()) <= coordinate &&
           (this->max() + 0.5 * this->stepsize()) >  coordinate;
}

float Axis::to_sample_position(float coordinate) noexcept(false) {
    return this->m_axis_descriptor.CoordinateToSamplePosition(coordinate);
}

bool Axis::operator!=(Axis const &other) const noexcept(false) {

    if (this->nsamples() != other.nsamples()) {
        throw std::runtime_error(
            "Axis: " + this->name() +
            ": Mismatch in number of samples: " +
            std::to_string(this->nsamples()) +
            " != " + std::to_string(other.nsamples()));
        return true;
    }

    if (this->min() != other.min()) {
        throw std::runtime_error(
            "Axis: " + this->name() +
            ": Mismatch in min value: " +
            std::to_string(this->min()) +
            " != " + std::to_string(other.min()));
        return true;
    }

    if (this->max() != other.max()) {
        throw std::runtime_error(
            "Axis: " + this->name() +
            ": Mismatch in max value: " +
            std::to_string(this->max()) +
            " != " + std::to_string(other.max()));
        return true;
    }

    if (this->stepsize() != other.stepsize()) {
        throw std::runtime_error(
            "Axis: " + this->name() +
            ": Mismatch in stepsize: " +
            std::to_string(this->stepsize()) +
            " != " + std::to_string(other.stepsize()));
        return true;
    }

    if (this->unit() != other.unit()) {
        throw std::runtime_error(
            "Axis: " + this->name() +
            ": Mismatch in unit: " +
            this->unit() +
            " != " + other.unit());
        return true;
    }

    if (this->dimension() != other.dimension()) {
        throw std::runtime_error(
            "Axis: " + this->name() +
            ": Mismatch in dimension: " +
            std::to_string(this->dimension()) +
            " != " + std::to_string(other.dimension()));
        return true;
    }

    if (this->name() != other.name()) {
        throw std::runtime_error(
            "Axis: " + this->name() +
            ": Mismatch in name: " +
            this->name() +
            " != " + other.name());
        return true;
    }

    return false;
}
