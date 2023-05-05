#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "attribute.hpp"

VerticalWindow::VerticalWindow(float above, float below, float samplerate)
    : m_above(above), m_below(below), m_samplerate(samplerate)
{
    if (this->samplerate() <= 0)
        throw std::invalid_argument("samplerate must be > 0");
    if (this->m_above < 0)
        throw std::invalid_argument("Above must be >= 0");
    if (this->m_below < 0)
        throw std::invalid_argument("below must be >= 0");
}

float VerticalWindow::nsamples_above() const noexcept (true) {
    return this->m_above / this->m_samplerate;
}

float VerticalWindow::nsamples_below() const noexcept (true) {
    return this->m_below / this->m_samplerate;
}

float VerticalWindow::samplerate() const noexcept (true) {
    return this->m_samplerate;
}

std::size_t VerticalWindow::size() const noexcept (true) {
    return this->nsamples_above() + 1 + this->nsamples_below();
}

void VerticalWindow::squeeze() noexcept (true) {
    this->m_above -= std::fmod(this->m_above, this->m_samplerate);
    this->m_below -= std::fmod(this->m_below, this->m_samplerate);
}

Horizon::HorizontalIt Horizon::begin() const noexcept (true) {
    auto const& vertical = this->vertical();
    return HorizontalIt(this->m_ptr, vertical.size());
}

Horizon::HorizontalIt Horizon::end() const noexcept (true) {
    auto const& vertical = this->vertical();
    return HorizontalIt(this->m_ptr + this->size(), vertical.size());
}

std::size_t Horizon::size() const noexcept (true) {
    auto const& surface  = this->surface();
    auto const& vertical = this->vertical();

    return surface.size() * vertical.size();
}

RegularSurface const& Horizon::surface() const noexcept (true) {
    return this->m_surface;
}

VerticalWindow const& Horizon::vertical() const noexcept (true) {
    return this->m_vertical;
}

void Horizon::calc_attributes(
    std::vector< attributes::Attribute >& attrs
) const noexcept (false) {
    using namespace attributes;

    std::size_t const vsize = this->vertical().size();

    AttributeFillVisitor fill(this->fillvalue());
    auto calculate = [&](const float& front) {
        if (front == this->fillvalue()) {
            std::for_each(attrs.begin(), attrs.end(), [&](auto& attr) {
                std::visit(fill, attr);
            });
        } else {
            AttributeComputeVisitor compute(
                VerticalIt(&front),
                VerticalIt(&front + vsize)
            );
            std::for_each(attrs.begin(), attrs.end(), [&](auto& attr) {
                std::visit(compute, attr);
            });
        }
    };

    std::for_each(this->begin(), this->end(), calculate);
}
