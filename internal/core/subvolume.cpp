#include <cassert>
#include <cmath>
#include <stdexcept>

#include "axis.hpp"
#include "subvolume.hpp"
#include "utils.hpp"

#include <boost/math/interpolators/makima.hpp>
using boost::math::interpolators::makima;

static const float tolerance = 1e-3f;

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

enum class Border {
    Top,
    Bottom
};

SurfaceBoundedSubVolume* make_subvolume(
    MetadataHandle const& metadata,
    RegularSurface const& reference,
    RegularSurface const& top,
    RegularSurface const& bottom
) {
    if (!(reference.grid() == top.grid() && reference.grid() == bottom.grid())) {
        throw std::runtime_error("Expected surfaces to have the same plane and size");
    }

    CoordinateTransformer const& transform = metadata.coordinate_transformer();

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

        if (not iline.inrange_with_margin(ij[0]) or not xline.inrange_with_margin(ij[1])) {
            subvolume->m_segment_offsets[i + 1] = subvolume->m_segment_offsets[i];
            continue;
        }

        if (not sample.inrange(top_depth) or
            not sample.inrange(bottom_depth))
        {
            auto row = horizontal_grid.row(i);
            auto col = horizontal_grid.col(i);
            throw std::runtime_error(
                "Vertical window is out of vertical bounds at"
                " row: " + std::to_string(row) +
                " col:" + std::to_string(col) +
                ". Request: [" + utils::to_string_with_precision(top_depth) +
                ", " + utils::to_string_with_precision(bottom_depth) +
                "]. Seismic bounds: [" + utils::to_string_with_precision(sample.min())
                + ", " + utils::to_string_with_precision(sample.max()) + "]"
            );
        }

        auto calculate_margin = [&](Border border) {
            std::int8_t margin = segment_blueprint.preferred_margin();
            while (margin > 0) {
                float sample_position;
                if (border == Border::Top) {
                    sample_position = segment_blueprint.top_sample_position(top_depth, margin);
                } else {
                    sample_position = segment_blueprint.bottom_sample_position(bottom_depth, margin);
                }
                if (sample.inrange(sample_position)) {
                    return margin;
                }
                --margin;
            }
            return margin;

        };

        std::int8_t top_margin = calculate_margin(Border::Top);
        bool is_top_margin_atypical = (top_margin != segment_blueprint.preferred_margin());

        std::int8_t bottom_margin = calculate_margin(Border::Bottom);
        bool is_bottom_margin_atypical = (bottom_margin != segment_blueprint.preferred_margin());

        // limitation from makima samples interpolation algorithm
        const int min_samples = 4;
        assert(
            (void("Current logic relies on relationship between min_samples and preferred_margin"),
             min_samples == 2 * segment_blueprint.preferred_margin())
        );
        auto size = segment_blueprint.size(top_depth, bottom_depth, top_margin, bottom_margin);
        if (size < min_samples) {
            if (is_top_margin_atypical && is_bottom_margin_atypical) {
                throw std::runtime_error(
                    "Segment size is too small. Top margin: " +
                    std::to_string(top_margin) + ", bottom margin: " +
                    std::to_string(bottom_margin)
                );
            }

            int diff = min_samples - size;
            if (is_top_margin_atypical) {
                bottom_margin += diff;
                is_bottom_margin_atypical = true;
            } else {
                top_margin += diff;
                is_top_margin_atypical = true;
            }
        }

        if (is_top_margin_atypical) {
            subvolume->m_segment_top_margins.emplace(i, top_margin);
        }

        subvolume->m_segment_offsets[i + 1] =
            subvolume->m_segment_offsets[i] + segment_blueprint.size(top_depth, bottom_depth, top_margin, bottom_margin);
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
        top_margin(index), bottom_margin(index),
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

    /*
     * Regarding use of data at the array edge: in majority of cases
     * interpolated area near the edges won't be used as segment samples.
     * Exception are cases where user requested data near trace border. Here we
     * allow algorithm to choose spline itself. Supplying additional edge
     * samples with arbitrary value seems unnecessary.
     */
    auto spline = makima<std::vector<double>>(std::move(src_points), std::move(src_data));

    auto dst = dst_segment.begin();
    for (int j = 0; j < dst_points.size(); ++j) {
        *dst = spline(dst_points.at(j));
        std::advance(dst, 1);
    }
}
