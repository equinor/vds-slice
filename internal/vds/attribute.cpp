#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "attribute.hpp"
#include "interpolators.hpp"

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
    this->m_above -= std::remainder(this->m_above, this->m_samplerate);
    this->m_below -= std::remainder(this->m_below, this->m_samplerate);
}

float VerticalWindow::snap(float x) const noexcept (true) {
    return x - std::fmod(x, this->samplerate());
}

VerticalWindow VerticalWindow::fit_to_samplerate(
    float samplerate
) const noexcept (false) {
    if (samplerate < this->samplerate()) {
        throw std::invalid_argument("upsampling not supported");
    }

    float const above = this->m_above;
    float const below = this->m_below + 1 * samplerate;

    return {above, below, samplerate};
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
    std::vector< attributes::Attribute >& attrs,
    VerticalWindow target
) const noexcept (false) {
    auto const& surface  = this->surface();
    auto const& vertical = this->vertical();

    std::size_t const vsize = vertical.size();

    float const srcrate = vertical.samplerate();
    float const dstrate = target.samplerate();

    std::vector< double > vsrc(vsize);
    std::vector< double > vdst(target.size());

    using namespace attributes;
    AttributeFillVisitor    fill(this->fillvalue());
    AttributeComputeVisitor compute(vdst.begin(), vdst.end());

    std::size_t i = 0;
    auto calculate = [&](const float& front) {
        if (front == this->fillvalue()) {
            std::for_each(attrs.begin(), attrs.end(), [&](auto& attr) {
                std::visit(fill, attr);
            });
            ++i;
            return;
        }

        float const depth = surface.at(i);
        float const vshift = std::fmod(depth, vertical.samplerate());

        std::transform(
            VerticalIt(&front),
            VerticalIt(&front + vsize),
            vsrc.begin(),
            [](float x) { return (double)x; }
        );

        interpolation::cubic< double >(
            vsrc.begin(),
            vsrc.end(),
            srcrate,
            vdst.begin(),
            vdst.end(),
            dstrate,
            vshift
        );

        std::for_each(attrs.begin(), attrs.end(), [&](auto& attr) {
            std::visit(compute, attr);
        });

        ++i;
    };

    std::for_each(this->begin(), this->end(), calculate);
}
