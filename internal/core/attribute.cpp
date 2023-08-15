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

Horizon::Window Horizon::at(std::size_t i) const noexcept (false) {
    std::size_t begin = m_buffer_offsets[i];
    std::size_t end = m_buffer_offsets[i+1];

    return {
        Horizon::VerticalIt(this->m_ptr + begin),
        Horizon::VerticalIt(this->m_ptr + end)
    };
}

void AttributeMap::AttributeComputeParams::update(
    InputIt begin,
    InputIt end,
    std::size_t size,
    std::size_t reference_index
) noexcept (true) {
    this->begin = begin;
    this->end = end;
    this->size = size;
    this->reference_index = reference_index;
}

float Value::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    auto ptr = params.begin;
    std::advance(ptr, params.reference_index);
    return *ptr;
}

float Min::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    return *std::min_element(params.begin, params.end);
}

float Max::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    return *std::max_element(params.begin, params.end);
}

float MaxAbs::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    auto max = *std::max_element(params.begin, params.end,
    [](const double& a, const double& b) { 
            return std::abs(a) < std::abs(b); 
        }
    );
    return std::abs(max);
}

float Mean::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    double sum = std::accumulate(params.begin, params.end, 0.0);
    return sum / params.size;
}

float MeanAbs::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    double sum = std::accumulate(params.begin, params.end, 0.0,
        [](double acc, double x) { return acc + std::abs(x); });
    return sum / params.size;
}

float MeanPos::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    int count = 0;
    double sum = std::accumulate(params.begin, params.end, 0.0,
        [&](double acc, double x) {
            if (x > 0) {
                count ++;
                return acc + x;
            }
            return acc;
        }
    );
    return count > 0 ? sum / count : 0;
}

float MeanNeg::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    int count = 0;
    double sum = std::accumulate(params.begin, params.end, 0.0,
        [&](double acc, double x) {
            if (x < 0) {
                count ++;
                return acc + x;
            }
            return acc;
        }
    );
    return count > 0 ? sum / count : 0;
}

float Median::compute(
    AttributeComputeParams const & params
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
    auto temp = std::vector<double>(params.begin, params.end);
    const auto middle_right = temp.begin() + params.size / 2;
    std::nth_element(temp.begin(), middle_right, temp.end());
    if (params.size % 2 == 0) {
        const auto max_left = std::max_element(temp.begin(), middle_right);
        return (*max_left + *middle_right) / 2;
    }
    else {
        return *middle_right;
    }
}

float Rms::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    float sum = std::accumulate(params.begin, params.end, 0.0,
        [](double a, double b) {
            return a + std::pow(b, 2);
        }
    );
    return std::sqrt(sum / params.size);
}

namespace {

double variance(
    AttributeMap::AttributeComputeParams const & params
){
    double sum = std::accumulate(params.begin, params.end, 0.0);
    double mean = sum / params.size;
    double stdSum = std::accumulate(params.begin, params.end, 0.0,
        [&](double a, double b){ return a + std::pow(b - mean, 2); }
    );
    return stdSum / params.size;
}

} // namespace

float Var::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    return variance(params);
}

float Sd::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    return std::sqrt(variance(params));
}

float SumPos::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    double sum = 0.0;
    std::for_each(params.begin, params.end, [&](double x) {
         if (x > 0) { sum += x; }
    });
    return sum; 
}

float SumNeg::compute(
    AttributeComputeParams const & params
) noexcept (false) {
    double sum = 0.0;
    std::for_each(params.begin, params.end, [&](double x) {
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
    AttributeMap::AttributeComputeParams & params
) {
    for (auto& attr : attributes) {
        auto value = attr->compute(params);
        attr->write(value, pos);
    }
}

void calc_attributes(
    Horizon const& horizon,
    RegularSurface const& reference,
    RegularSurface const& top,
    RegularSurface const& bottom,
    VerticalWindow& src_window,
    VerticalWindow& dst_window,
    std::vector< std::unique_ptr< AttributeMap > >& attrs,
    std::size_t from,
    std::size_t to
) noexcept (false) {
    auto fill = horizon.fillvalue();
    AttributeMap::AttributeComputeParams params;

    for (std::size_t i = from; i < to; ++i) {
        auto above = reference.value(i) - top.value(i);
        auto below = bottom.value(i) - reference.value(i);

        src_window.move(above, below);
        dst_window.move(above, below);

        auto data  = horizon.at(i);

        if (data.begin() == data.end()) {
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
            reference.value(i)
        );

        params.update(
            dst_buffer.begin(),
            dst_buffer.end(),
            dst_window.size(),
            dst_window.nsamples_above()
        );
        
        compute_all<double>(attrs, i, params);
    }
}
