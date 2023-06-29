#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <numeric>
#include <memory>
#include <stdexcept>
#include <vector>

#include "attribute.hpp"
#include "interpolation.hpp"
#include "regularsurface.hpp"
#include "verticalwindow.hpp"

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
    AttributeMap::InputIt begin,
    AttributeMap::InputIt end
) noexcept (false) {
    std::advance(begin, this->idx);
    return *begin;
}

float Min::compute(
    const AttributeMap::InputIt begin,
    const AttributeMap::InputIt end
) noexcept (false) {
    return *std::min_element(begin, end);
}

float Max::compute(
    const AttributeMap::InputIt begin,
    const AttributeMap::InputIt end
) noexcept (false) {
    return *std::max_element(begin, end);
}

float MaxAbs::compute(
    const AttributeMap::InputIt begin,
    const AttributeMap::InputIt end
) noexcept (false) {
    auto max = *std::max_element(begin, end,
    [](const double& a, const double& b) { 
            return std::abs(a) < std::abs(b); 
        }
    );
    return std::abs(max);
}

float Mean::compute(
    const AttributeMap::InputIt begin,
    const AttributeMap::InputIt end
) noexcept (false) {
    double sum = std::accumulate(begin, end, 0.0);
    return sum / this->vsize;
}

float MeanAbs::compute(
    const AttributeMap::InputIt begin,
    const AttributeMap::InputIt end
) noexcept (false) {
    double sum = std::accumulate(begin, end, 0.0,
        [](double acc, double x) { return acc + std::abs(x); });
    return sum / this->vsize;
}

float Median::compute(
    const AttributeMap::InputIt begin,
    const AttributeMap::InputIt end
) noexcept (false) {
    /*
    The std::nth_element function sets the middle element of a vector in such a
    manner that all values on the right side of the middle element are greater
    than or equal to it, and all elements preceding the middle are less than or
    equal to the middle. With this approach, we don't need to sort the entire
    vector. In the case of even number of elements in the vector, we use
    std::max_element to obtain the largest element before the middle element to
    compute the average.
    */
    auto temp = std::vector<double>(begin, end);
    const auto middle_right = temp.begin() + vsize / 2;
    std::nth_element(temp.begin(), middle_right, temp.end());
    if (vsize % 2 == 0) {
        const auto max_left = std::max_element(temp.begin(), middle_right);
        return (*max_left + *middle_right) / 2;
    }
    else {
        return *middle_right;
    }
}

float Rms::compute(
    const AttributeMap::InputIt begin,
    const AttributeMap::InputIt end
) noexcept (false) {
    float sum = std::accumulate(begin, end, 0.0,
        [](double a, double b) {
            return a + std::pow(b, 2);
        }
    );
    return std::sqrt(sum / this->vsize);
}

namespace {

double variance(
    const  AttributeMap::InputIt begin,
    const  AttributeMap::InputIt end,
    std::size_t vsize
){
    double sum = std::accumulate(begin, end, 0.0);
    double mean = sum / vsize;
    double stdSum = std::accumulate(begin, end, 0.0,
        [&](double a, double b){ return a + std::pow(b - mean, 2); }
    );
    return stdSum / vsize;
}

} // namespace

float Var::compute(
    const AttributeMap::InputIt begin,
    const AttributeMap::InputIt end
) noexcept (false) {
    return variance(begin, end, vsize);
}

float Sd::compute(
    const AttributeMap::InputIt begin,
    const AttributeMap::InputIt end
) noexcept (false) {
    return std::sqrt(variance(begin, end, vsize));
}

float SumPos::compute(
    const AttributeMap::InputIt begin,
    const AttributeMap::InputIt end
) noexcept (false) {
    double sum = 0.0;
    std::for_each(begin, end, [&](double x) {
         if (x > 0) { sum += x; }
    });
    return sum; 
}

float SumNeg::compute(
    const AttributeMap::InputIt begin,
    const AttributeMap::InputIt end
) noexcept (false) {
    double sum = 0.0;
    std::for_each(begin, end, [&](double x) {
         if (x < 0) { sum += x; }
    });
    return sum;
}

void fill_all(
    std::vector< std::unique_ptr< AttributeMap > >& attributes,
    float value,
    std::size_t pos
) {
    for (auto& attr : attributes) {
        attr->write(value, pos);
    }
}

template< typename InputIt >
void compute_all(
    std::vector< std::unique_ptr< AttributeMap > >& attributes,
    std::size_t pos,
    InputIt begin,
    InputIt end
) {
    for (auto& attr : attributes) {
        auto value = attr->compute(begin, end);
        attr->write(value, pos);
    }
}

void calc_attributes(
    Horizon const& horizon,
    RegularSurface const& surface,
    VerticalWindow const& src_window,
    VerticalWindow const& dst_window,
    std::vector< std::unique_ptr< AttributeMap > >& attrs,
    std::size_t from,
    std::size_t to
) noexcept (false) {
    auto fill = horizon.fillvalue();

    for (std::size_t i = from; i < to; ++i) {
        auto depth = surface.at(i);
        auto data  = horizon.at(i);

        if (*data.begin() == fill) {
            fill_all(attrs, fill, i);
            continue;
        }

        /**
         * Interpolation and attribute calculation should be performed on
         * doubles to avoid loss of precision in these intermediate steps.
         */
        std::vector< double > src_buffer(data.begin(), data.end());
        std::vector< double > dst_buffer(dst_window.size());

        cubic_makima(
            std::move( src_buffer ),
            src_window,
            dst_buffer,
            dst_window,
            depth
        );
        
        compute_all(attrs, i, dst_buffer.begin(), dst_buffer.end());
    }
}
