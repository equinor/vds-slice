#include <cmath>
#include <stdexcept>

#include "axis.hpp"
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

SurfaceBoundedSubVolume* make_subvolume(
    MetadataHandle const& metadata,
    RegularSurface const& reference,
    RegularSurface const& top,
    RegularSurface const& bottom
) {
    if (!(reference.grid() == top.grid() && reference.grid() == bottom.grid())) {
        throw std::runtime_error("Expected surfaces to have the same plane and size");
    }

    auto transform = metadata.coordinate_transformer();

    auto iline = metadata.iline();
    auto xline = metadata.xline();
    auto sample = metadata.sample();

    RawSegmentBlueprint segment_blueprint = RawSegmentBlueprint(sample.stepsize(), sample.min());
    auto subvolume = new SurfaceBoundedSubVolume(reference, top, bottom, segment_blueprint);
    auto const horizontal_grid = subvolume->horizontal_grid();

    subvolume->m_segment_offsets[0] = 0;

    /**
     * Try to establish how far away from the start each segment in the
     * subvolume would lay, so we could concurrently fetch data to different
     * parts of the subvolume. If segment is supposed to have data then we set
     * the beginning of the new segment to the start of the previous one + size
     * of the previous one. If segment is empty (because no data exists or user
     * is not interested), simply set beginning of the next segment same as
     * current one as no data is expected to be fetched.
     */
    for (int i = 0; i < horizontal_grid.size(); ++i) {
        float reference_depth = reference[i];
        float top_depth = top[i];
        float bottom_depth = bottom[i];

        if (
            reference_depth == reference.fillvalue() ||
            top_depth == top.fillvalue() ||
            bottom_depth == bottom.fillvalue()
        ) {
            subvolume->m_segment_offsets[i + 1] = subvolume->m_segment_offsets[i];
            continue;
        }

        if (
            reference_depth < top_depth ||
            reference_depth > bottom_depth
        ) {
            throw std::runtime_error(
                "Planes are not ordered as top <= reference <= bottom"
            );
        }

        auto const cdp = horizontal_grid.to_cdp(i);
        auto ij = transform.WorldToAnnotation({cdp.x, cdp.y, 0});

        if (not iline.inrange(ij[0]) or not xline.inrange(ij[1])) {
            subvolume->m_segment_offsets[i + 1] = subvolume->m_segment_offsets[i];
            continue;
        }

        subvolume->m_segment_offsets[i + 1] =
            subvolume->m_segment_offsets[i] + segment_blueprint.size(top_depth, bottom_depth);
    }
    subvolume->m_data.reserve(subvolume->m_segment_offsets[horizontal_grid.size()]);

    return subvolume;
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
