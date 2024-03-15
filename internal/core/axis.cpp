#include "axis.hpp"

#include "exceptions.hpp"
#include <stdexcept>

#include "utils.hpp"
#include <OpenVDS/IJKCoordinateTransformer.h>
#include <OpenVDS/KnownMetadata.h>

Axis::Axis(
    OpenVDS::VolumeDataLayout const* const layout,
    int const dimension
) : m_dimension(dimension),
    m_axis_descriptor(layout->GetAxisDescriptor(dimension)) {}

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

float Axis::stepsize() const noexcept(true) {
    return (this->max() - this->min()) / (this->nsamples() - 1);
}

std::string Axis::name() const noexcept(true) {
    return this->m_axis_descriptor.GetName();
}

bool Axis::inrange(float coordinate) const noexcept(true) {
    return (this->min() - 0.5 * this->stepsize()) <= coordinate &&
           (this->max() + 0.5 * this->stepsize()) > coordinate;
}

float Axis::to_sample_position(float coordinate) noexcept(false) {
    return this->m_axis_descriptor.CoordinateToSamplePosition(coordinate);
}

void Axis::assert_equal(Axis const& other) noexcept(false) {

    if (this->nsamples() != other.nsamples()) {
        throw detail::bad_request(
            "Axis: " + this->name() +
            ": Mismatch in number of samples: " +
            utils::to_string_with_precision(this->nsamples()) +
            " != " + utils::to_string_with_precision(other.nsamples())
        );
    }

    if (this->min() != other.min()) {
        throw detail::bad_request(
            "Axis: " + this->name() +
            ": Mismatch in min value: " +
            utils::to_string_with_precision(this->min()) +
            " != " + utils::to_string_with_precision(other.min())
        );
    }

    if (this->max() != other.max()) {
        throw detail::bad_request(
            "Axis: " + this->name() +
            ": Mismatch in max value: " +
            utils::to_string_with_precision(this->max()) +
            " != " + utils::to_string_with_precision(other.max())
        );
    }

    // Stepsize is a data integrity check.
    // If min,max and nsamples are equal stepsize is equal for consistent data.
    if (this->stepsize() != other.stepsize()) {
        throw detail::bad_request(
            "Axis: " + this->name() +
            ": Mismatch in stepsize: " +
            std::to_string(this->stepsize()) +
            " != " + std::to_string(other.stepsize())
        );
    }

    if (this->unit() != other.unit()) {
        throw detail::bad_request(
            "Axis: " + this->name() +
            ": Mismatch in unit: " +
            this->unit() +
            " != " + other.unit()
        );
    }

    // Ignore order of dimensions

    if (this->name() != other.name()) {
        throw detail::bad_request(
            "Axis: " + this->name() +
            ": Mismatch in name: " +
            this->name() +
            " != " + other.name()
        );
    }
}
