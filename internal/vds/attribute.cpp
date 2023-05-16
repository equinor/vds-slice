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

template< typename Func >
void Horizon::calc_attribute(void* dst, std::size_t size, Func func) const {
    if (size < this->mapsize()) {
        throw std::runtime_error("Buffer to small");
    }

    std::size_t offset = 0;
    std::for_each(this->begin(), this->end(), [&](const float& front) {
        float value = (front == this->fillvalue())
            ? this->fillvalue()
            : func(VerticalIt(&front), VerticalIt(&front + this->vsize()))
        ;

        memcpy((char*)dst + offset * sizeof(float), &value, sizeof(float));
        ++offset;
    });
}

namespace attributes {

void min(Horizon const& horizon, void* dst, std::size_t size) noexcept (false) {
    auto minfunc = [](Horizon::VerticalIt beg, Horizon::VerticalIt end) {
        return *std::min_element(beg, end);
    };

    return horizon.calc_attribute(dst, size, minfunc);
}

void max(Horizon const& horizon, void* dst, std::size_t size) noexcept (false) {
    auto maxfunc = [](Horizon::VerticalIt beg, Horizon::VerticalIt end) {
        return *std::max_element(beg, end);
    };

    return horizon.calc_attribute(dst, size, maxfunc);
}

void mean(Horizon const& horizon, void* dst, std::size_t size) noexcept (false) {
    std::size_t vsize = horizon.vsize();

    auto meanfunc = [vsize](Horizon::VerticalIt beg, Horizon::VerticalIt end) {
        float sum = std::accumulate(beg, end, 0.0f);
        return sum / vsize;
    };

    return horizon.calc_attribute(dst, size, meanfunc);
}

void rms(Horizon const& horizon, void* dst, std::size_t size) noexcept (false) {
    std::size_t vsize = horizon.vsize();

    auto rmsfunc = [vsize](Horizon::VerticalIt beg, Horizon::VerticalIt end) {
        float sum = std::accumulate(beg, end, 0,
            [](float a, float b) {
                return a + std::pow(b, 2);
            }
        );
        return std::sqrt(sum / vsize);
    };

    return horizon.calc_attribute(dst, size, rmsfunc);
}

void sd(Horizon const& horizon, void* dst, std::size_t size) noexcept (false) {
    std::size_t vsize = horizon.vsize();

    auto sdfunc = [vsize](Horizon::VerticalIt beg, Horizon::VerticalIt end) {
        float sum = std::accumulate(beg, end, 0.0f);
        float mean = sum / vsize;
        float stdSum = std::accumulate(beg, end, 0.0f,
        [&](float a, float b){
            return a + std::pow(b - mean, 2);
            }
        );
        return std::sqrt(stdSum / vsize);
    };

    return horizon.calc_attribute(dst, size, sdfunc);
}

} // namespace attributes
