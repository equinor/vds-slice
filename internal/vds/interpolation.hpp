#ifndef VDS_SLICE_INTERPOLATION_HPP
#define VDS_SLICE_INTERPOLATION_HPP

#include <vector>

#include "verticalwindow.hpp"

#include <boost/math/interpolators/makima.hpp>

template< typename T >
struct StrideGenerator {
    StrideGenerator(T start, T step) : cur(start), step(step) {}

    T operator()() {
        T tmp = this->cur;
        this->cur += this->step;
        return tmp;
    }
private:
    T cur;
    T step;
};

template< typename T >
void cubic_makima(
    std::vector< T > ys,
    VerticalWindow const& src_window,
    std::vector< T >& dst_buffer,
    VerticalWindow const& dst_window,
    float reference_point
) noexcept (false) {
    T nearest = src_window.nearest(reference_point);
    T start = src_window.at(0, nearest);
    T step  = src_window.stepsize();

    std::vector< T > xs(ys.size());
    std::generate(xs.begin(), xs.end(), StrideGenerator(start, step));

    using boost::math::interpolators::makima;
    auto spline = makima< std::vector< double >>(std::move(xs), std::move(ys));

    start = dst_window.at(0, reference_point);
    step  = dst_window.stepsize();
    auto gen_x = StrideGenerator(start, step);
    std::generate(dst_buffer.begin(), dst_buffer.end(), [&spline, &gen_x](){
        return spline(gen_x());
    });
}

#endif // VDS_SLICE_INTERPOLATION_HPP
