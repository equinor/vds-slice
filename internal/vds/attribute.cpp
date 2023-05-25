#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <numeric>
#include <memory>
#include <stdexcept>
#include <vector>

#include "attribute.hpp"

Horizon::HorizontalIt Horizon::begin() const noexcept (true) {
    return HorizontalIt(this->m_ptr, this->vsize());
}

Horizon::HorizontalIt Horizon::end() const noexcept (true) {
    return HorizontalIt(this->m_ptr + this->hsize() * this->vsize(), this->vsize());
}

Horizon::Window Horizon::at(std::size_t i) const noexcept (false) {
    std::size_t begin = i * this->vsize();
    std::size_t end = begin + this->vsize();

    return {
        Horizon::VerticalIt(this->m_ptr + begin),
        Horizon::VerticalIt(this->m_ptr + end)
    };
}

float Value::compute(
    Horizon::VerticalIt begin,
    Horizon::VerticalIt end
) noexcept (false) {
    std::advance(begin, this->idx);
    return *begin;
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

void calc_attributes(
    Horizon const& horizon,
    std::vector< std::unique_ptr< AttributeMap > >& attrs
) noexcept (false) {
    auto fill = horizon.fillvalue();

    std::size_t i = 0;
    auto calculate = [&](const float& front) {
        if (front != fill) {
            std::for_each(attrs.begin(), attrs.end(), [&](std::unique_ptr< AttributeMap>& attr) {
                float value = attr->compute(
                    Horizon::VerticalIt(&front),
                    Horizon::VerticalIt(&front + horizon.vsize())
                );
                attr->write(value, i);
            });
        } else {
            std::for_each(attrs.begin(), attrs.end(), [&](std::unique_ptr< AttributeMap >& attr) {
                attr->write(fill, i);
            });
        }

        ++i;
    };

    std::for_each(horizon.begin(), horizon.end(), calculate);
}
