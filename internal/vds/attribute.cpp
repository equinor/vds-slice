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
