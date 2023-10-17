#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <numeric>
#include <memory>
#include <stdexcept>
#include <vector>

#include "attribute.hpp"
#include "regularsurface.hpp"

float Value::compute(
    ResampledSegment const & segment
) noexcept (false) {
    auto ptr = segment.begin();
    std::advance(ptr, segment.reference_index());
    return *ptr;
}

float Min::compute(
    ResampledSegment const & segment
) noexcept (false) {
    return *std::min_element(segment.begin(), segment.end());
}

float Max::compute(
    ResampledSegment const & segment
) noexcept (false) {
    return *std::max_element(segment.begin(), segment.end());
}

float MaxAbs::compute(
    ResampledSegment const & segment
) noexcept (false) {
    auto max = *std::max_element(segment.begin(), segment.end(),
    [](const double& a, const double& b) { 
            return std::abs(a) < std::abs(b); 
        }
    );
    return std::abs(max);
}

float Mean::compute(
    ResampledSegment const & segment
) noexcept (false) {
    double sum = std::accumulate(segment.begin(), segment.end(), 0.0);
    return sum / segment.size();
}

float MeanAbs::compute(
    ResampledSegment const & segment
) noexcept (false) {
    double sum = std::accumulate(segment.begin(), segment.end(), 0.0,
        [](double acc, double x) { return acc + std::abs(x); });
    return sum / segment.size();
}

float MeanPos::compute(
    ResampledSegment const & segment
) noexcept (false) {
    int count = 0;
    double sum = std::accumulate(segment.begin(), segment.end(), 0.0,
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
    ResampledSegment const & segment
) noexcept (false) {
    int count = 0;
    double sum = std::accumulate(segment.begin(), segment.end(), 0.0,
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
    ResampledSegment const & segment
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
    auto temp = std::vector<double>(segment.begin(), segment.end());
    const auto middle_right = temp.begin() + segment.size() / 2;
    std::nth_element(temp.begin(), middle_right, temp.end());
    if (segment.size() % 2 == 0) {
        const auto max_left = std::max_element(temp.begin(), middle_right);
        return (*max_left + *middle_right) / 2;
    }
    else {
        return *middle_right;
    }
}

float Rms::compute(
    ResampledSegment const & segment
) noexcept (false) {
    float sum = std::accumulate(segment.begin(), segment.end(), 0.0,
        [](double a, double b) {
            return a + std::pow(b, 2);
        }
    );
    return std::sqrt(sum / segment.size());
}

namespace {

double variance(
    ResampledSegment const & segment
){
    double sum = std::accumulate(segment.begin(), segment.end(), 0.0);
    double mean = sum / segment.size();
    double stdSum = std::accumulate(segment.begin(), segment.end(), 0.0,
        [&](double a, double b){ return a + std::pow(b - mean, 2); }
    );
    return stdSum / segment.size();
}

} // namespace

float Var::compute(
    ResampledSegment const & segment
) noexcept (false) {
    return variance(segment);
}

float Sd::compute(
    ResampledSegment const & segment
) noexcept (false) {
    return std::sqrt(variance(segment));
}

float SumPos::compute(
    ResampledSegment const & segment
) noexcept (false) {
    double sum = 0.0;
    std::for_each(segment.begin(), segment.end(), [&](double x) {
         if (x > 0) { sum += x; }
    });
    return sum; 
}

float SumNeg::compute(
    ResampledSegment const & segment
) noexcept (false) {
    double sum = 0.0;
    std::for_each(segment.begin(), segment.end(), [&](double x) {
         if (x < 0) { sum += x; }
    });
    return sum;
}

void calc_attributes(
    SurfaceBoundedSubVolume const& src_subvolume,
    ResampledSegmentBlueprint const* dst_segment_blueprint,
    std::vector< std::unique_ptr< AttributeMap > >& attrs,
    std::size_t from,
    std::size_t to
) noexcept (false) {
    auto fill = src_subvolume.fillvalue();

    RawSegment src_segment = src_subvolume.vertical_segment(from);
    ResampledSegment dst_segment =  ResampledSegment(0, 0, 0, dst_segment_blueprint);

    for (std::size_t i = from; i < to; ++i) {
        if (src_subvolume.is_empty(i)) {
            for (auto& attr : attrs) {
                attr->write(fill, i);
            }
            continue;
        }

        src_subvolume.reinitialize(i, src_segment);
        src_subvolume.reinitialize(i, dst_segment);
        resample(src_segment, dst_segment);

        for (auto& attr : attrs) {
            auto value = attr->compute(dst_segment);
            attr->write(value, i);
        }
    }
}
