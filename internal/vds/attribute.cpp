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
    return HorizontalIt(this->m_ptr, this->vsize());
}

Horizon::HorizontalIt Horizon::end() const noexcept (true) {
    return HorizontalIt(this->m_ptr + this->size(), this->vsize());
}

std::size_t Horizon::size() const noexcept (true) {
    return this->hsize() * this->vsize();
}

void Horizon::calc_attributes(
    std::vector< attributes::Attribute >& attrs
) const noexcept (false) {
    using namespace attributes;

    AttributeFillVisitor fill(this->fillvalue());
    auto calculate = [&](const float& front) {
        if (front == this->fillvalue()) {
            std::for_each(attrs.begin(), attrs.end(), [&](auto& attr) {
                std::visit(fill, attr);
            });
        } else {
            AttributeComputeVisitor compute(
                VerticalIt(&front),
                VerticalIt(&front + this->vsize())
            );
            std::for_each(attrs.begin(), attrs.end(), [&](auto& attr) {
                std::visit(compute, attr);
            });
        }
    };

    std::for_each(this->begin(), this->end(), calculate);
}
