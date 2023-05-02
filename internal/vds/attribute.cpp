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

void Horizon::calc_attribute(attributes::Attribute attr) const {
    using namespace attributes;

    AttributeFillVisitor fill(this->fillvalue());
    auto calculate = [&](const float& front) {
        if (front == this->fillvalue()) {
            std::visit(fill, attr);
        } else {
            AttributeComputeVisitor compute(
                VerticalIt(&front),
                VerticalIt(&front + this->vsize())
            );
            std::visit(compute, attr);
        }
    };

    std::for_each(this->begin(), this->end(), calculate);
}
