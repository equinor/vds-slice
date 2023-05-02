#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <numeric>
#include <stdexcept>

#include "attribute.hpp"

Horizon::HorizontalIt Horizon::begin() const noexcept (true) {
    return HorizontalIt(this->m_ptr, this->vsize());
}

Horizon::HorizontalIt Horizon::end() const noexcept (true) {
    return HorizontalIt(this->m_ptr + this->hsize() * this->vsize(), this->vsize());
}

void Horizon::calc_attribute(AttributeMap& attr) const {
    auto fill = this->fillvalue();

    std::size_t i = 0;
    auto calculate = [&](const float& front) {
        auto value = fill;
        if (front != fill) {
            value = attr.compute(
                VerticalIt(&front),
                VerticalIt(&front + this->vsize())
            );
        }

        attr.write(value, i);
        ++i;
    };

    std::for_each(this->begin(), this->end(), calculate);
}

float Min::compute(
    Horizon::VerticalIt begin,
    Horizon::VerticalIt end
) noexcept (false) {
    return *std::min_element(begin, end);
}

float Max::compute(
    Horizon::VerticalIt begin,
    Horizon::VerticalIt end
) noexcept (false) {
    return *std::max_element(begin, end);
}

float Mean::compute(
    Horizon::VerticalIt begin,
    Horizon::VerticalIt end
) noexcept (false) {
    float sum = std::accumulate(begin, end, 0.0f);
    return sum / this->vsize;
}

float Rms::compute(
    Horizon::VerticalIt begin,
    Horizon::VerticalIt end
) noexcept (false) {
    float sum = std::accumulate(begin, end, 0.0f,
        [](float a, float b) {
            return a + std::pow(b, 2);
        }
    );
    return std::sqrt(sum / this->vsize);
}

float Sd::compute(
    Horizon::VerticalIt begin,
    Horizon::VerticalIt end
) noexcept (false) {
    float sum = std::accumulate(begin, end, 0.0f);
    float mean = sum / vsize;
    float stdSum = std::accumulate(begin, end, 0.0f,
        [&](float a, float b){ return a + std::pow(b - mean, 2); }
    );
    return std::sqrt(stdSum / vsize);
}
