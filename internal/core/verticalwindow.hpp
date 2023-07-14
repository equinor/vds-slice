#ifndef VDS_SLICE_VERTICAL_WINDOW_HPP
#define VDS_SLICE_VERTICAL_WINDOW_HPP

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>


/** Vertical window 
 *
 * The vertical window defines a window around some arbitrary reference point.
 * E.g. as a window around an horizon/surface when calculating attributes.
 * 
 * The VerticalWindow is purely a definition of a window's shape. It
 * does not encapsulate any data, nor even a reference point for which the
 * window spans around. This makes the window definition valid for all
 * reference points. This choice is motivated by performance.
 */
struct VerticalWindow {
    VerticalWindow(
        float above,
        float below,
        float stepsize,
        std::size_t margin = 0,
        float initial_sample_offset = std::nanf("1")
    );

    std::size_t nsamples_above() const noexcept (true);

    std::size_t nsamples_below() const noexcept (true);

    float stepsize() const noexcept (true);

    std::size_t size() const noexcept (true);

    float nearest(float depth) const noexcept (false);

    /** Get the depth at a specific index in the window, given a reference
     * depth */
    float at(std::size_t index, float ref_sample) const noexcept (false);
private:
    float m_above;
    float m_below;
    float m_stepsize;
    float m_initial_sample_offset;
    int   m_margin;
};

#endif // VDS_SLICE_VERTICAL_WINDOW_HPP
