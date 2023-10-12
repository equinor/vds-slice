#include <cmath>

#include "subvolume.hpp"

#include <boost/math/interpolators/makima.hpp>
using boost::math::interpolators::makima;

static const float tolerance = 1e-3f;

float fmod_with_tolerance(float x, float y) {
    float remainder = std::fmod(x, y);

    if (std::abs(remainder - y) < tolerance) {
        return 0;
    }
    return remainder;
}

float floor_with_tolerance(float x) {
    float ceil = std::ceil(x);

    if (std::abs(ceil - x) < tolerance) {
        return ceil;
    }
    return std::floor(x);
}

float ceil_with_tolerance(float x) {
    float floor = std::floor(x);

    if (std::abs(x - floor) < tolerance) {
        return floor;
    }
    return std::ceil(x);
}

void SurfaceBoundedSubVolume::reinitialize(
    std::size_t index,
    RawSegment& segment
) const {
    segment.reinitialize(
        m_ref[index], m_top[index], m_bottom[index],
        m_data.begin() + m_segment_offsets[index], m_data.begin() + m_segment_offsets[index + 1]
    );
}

void SurfaceBoundedSubVolume::reinitialize(
    std::size_t index,
    ResampledSegment& segment
) const {
    segment.reinitialize(m_ref[index], m_top[index], m_bottom[index]);
}

void resample(RawSegment const& src_segment, ResampledSegment& dst_segment) {
    /**
     * Interpolation and attribute calculation should be performed on
     * doubles to avoid loss of precision in these intermediate steps.
     */
    std::vector<double> src_points = src_segment.sample_positions();
    /**
     * something weird is going on here. If dst_points creation is moved after makima
     * interpolation on machine under test whole attributes function performance
     * became unstable. On some calls it was a bit faster than the one with
     * current lines order, on some calls it is twice as slow. Reason is
     * unknown.
     */
    std::vector<double> dst_points = dst_segment.sample_positions();

    std::vector<double> src_data(src_segment.begin(), src_segment.end());

    auto spline = makima<std::vector<double>>(std::move(src_points), std::move(src_data));

    auto dst = dst_segment.begin();
    for (int j = 0; j < dst_points.size(); ++j) {
        *dst = spline(dst_points.at(j));
        std::advance(dst, 1);
    }
}
