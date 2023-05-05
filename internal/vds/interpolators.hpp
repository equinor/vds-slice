#ifndef VDS_SLICE_INTERPOLATORS_HPP
#define VDS_SLICE_INTERPOLATORS_HPP

#include <boost/math/interpolators/cardinal_cubic_b_spline.hpp>


namespace interpolation {

template< typename T, typename InputIt, typename OutputIt >
void cubic(
    InputIt  srcbegin,
    InputIt  srcend,
    float    srcrate,
    OutputIt dstbegin,
    OutputIt dstend,
    float    dstrate,
    float    offset
) noexcept (false) {
    using namespace boost::math::interpolators;
    cardinal_cubic_b_spline< T > spline(srcbegin, srcend, 0, srcrate);

    T i = offset;
    std::transform(dstbegin, dstend, dstbegin, [&](auto const&){
        auto v = spline(i);
        i += dstrate;
        return v;
    });
}

} // interpolation

#endif // VDS_SLICE_INTERPOLATORS_HPP
