#include "verticalwindow.hpp"

VerticalWindow::VerticalWindow(
    float stepsize,
    std::size_t margin,
    float initial_sample_offset
) : m_stepsize(stepsize)
  , m_margin(margin)
{
    if (not std::isnan(initial_sample_offset)) {
        initial_sample_offset = std::fmod(initial_sample_offset, this->stepsize());
    }

    this->m_initial_sample_offset = initial_sample_offset;
}
std::size_t VerticalWindow::nsamples_above() const noexcept (true) {
    return std::floor(this->m_above / this->stepsize()) + this->m_margin;
}

std::size_t VerticalWindow::nsamples_below() const noexcept (true) {
    return std::floor(this->m_below / this->stepsize()) + this->m_margin;
}

float VerticalWindow::stepsize() const noexcept (true) { 
    return this->m_stepsize;
};

std::size_t VerticalWindow::size() const noexcept (true) {
    return this->nsamples_above() + 1 + this->nsamples_below();
}

float VerticalWindow::nearest(float depth) const noexcept (false) {
    if (std::isnan(this->m_initial_sample_offset)) {
        throw std::logic_error("cannot use nearest without shift");
    }

    return depth - std::remainder(depth - this->m_initial_sample_offset, this->stepsize());
}

float VerticalWindow::at(std::size_t index, float ref_sample) const noexcept (false) {
    if (index >= this->size()) {
        std::out_of_range(
            std::to_string(index) +
            " out of range of window. (size = " +
            std::to_string(this->size()) +
            ")"
        );
    }

    /** distance_to_ref - relation to index and above/below
     *
     *      window       index    distance_from_ref
     *     (step = 2)    
     * 
     * above  -            0         -1
     *        |
     *        |
     * ref    x            1          0
     *        |
     *        |
     *        -            2          1
     *        |
     *        |
     * below  -            3          2
     */ 

    int distance_from_ref = static_cast< int >(index) - this->nsamples_above();
    return ref_sample + distance_from_ref * this->stepsize();
}

void VerticalWindow::move(float above, float below) noexcept (true) {
        this->m_above = above;
        this->m_below = below;
    }
