#ifndef VDS_SLICE_INTERPOLATION_HPP
#define VDS_SLICE_INTERPOLATION_HPP

#include <vector>

#include "verticalwindow.hpp"

#include <boost/math/interpolators/makima.hpp>

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
    T stop  = start + src_window.size() * step;

    std::vector< T > xs;
    xs.reserve(ys.size());

    for (int i = start; i < stop; i += step) {
        xs.push_back(i);
    }

    using boost::math::interpolators::makima;
    auto spline = makima< std::vector< T >>(std::move(xs), std::move(ys));

    for (int i = 0; i < dst_window.size(); i++) {
        auto x = dst_window.at(i, reference_point);
        dst_buffer[i] = spline(x);
    }
}

#endif // VDS_SLICE_INTERPOLATION_HPP
