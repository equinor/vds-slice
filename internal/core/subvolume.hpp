#ifndef VDS_SLICE_SUBVOLUME_HPP
#define VDS_SLICE_SUBVOLUME_HPP

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

float fmod_with_tolerance(float x, float y);
float floor_with_tolerance(float x);
float ceil_with_tolerance(float x);

/**
 * Represents a view over continuous segment of trace's data. Class knows
 * nothing about values of the data and operates only on the "samples" axis.
 *
 * Class intends to represent common logic and properties of data segments
 * describing different parts of same data volume. While segment boundaries
 * might change from one segment to another, some properties (like data sample
 * positions or margin) would be constant for every segment.
 *
 * Example axis:
 *
 * 0 2   6   10  14  18  22  26  30  34
 * --*---*---*---*---*---*---*---*---*---
 *        |        |         |
 *       top   reference   bottom
 *
 * where * represents a sample. Stepsize is 4.
 * bottom (boundary), top (boundary) and reference would be unique for each case.
 * In the example top = 7, reference = 16, bottom = 26
 */
class SegmentBlueprint {
public:
    /**
     * @param stepsize Distance between sequential samples
     * @param margin Maximum number of samples silently added to the segment
     * from above and below.
     */
    SegmentBlueprint(float stepsize, std::size_t margin)
        : m_stepsize(stepsize), m_margin(margin) {
        if (stepsize <= 0) {
            throw std::runtime_error("Stepsize must be positive");
        }
    }

    /**
     * Segment size in number of samples
     * It is expected that top_boundary <= bottom_boundary.
     * sample_offset must be < stepsize.
     */
    std::size_t size(float sample_offset, float top_boundary, float bottom_boundary) const noexcept {
        return to_below_sample_number(sample_offset, bottom_boundary) - to_above_sample_number(sample_offset, top_boundary) + 1;
    }

    /**
     * Position of the sample that is closest to the top boundary but is not outside of the segment.
     * sample_offset must be < stepsize.
     */
    float top_sample_position(float sample_offset, float top_boundary) const noexcept {
        return to_above_sample_number(sample_offset, top_boundary) * this->stepsize() + sample_offset;
    }

    /**
     * Position of the sample that is closest to the bottom boundary but is not outside of the segment.
     * sample_offset must be < stepsize.
     */
    float bottom_sample_position(float sample_offset, float bottom_boundary) const noexcept {
        return to_below_sample_number(sample_offset, bottom_boundary) * this->stepsize() + sample_offset;
    }

    float stepsize() const { return m_stepsize; }
protected:

    /**
     * Sequence number of the closest sample that is <= position
     * sample_offset must be < stepsize.
     */
    int to_below_sample_number(float sample_offset, float position) const noexcept {
        return floor_with_tolerance((position - sample_offset) / this->stepsize()) + this->margin();
    }

    /**
     * Sequence number of the closest sample that is >= position
     * sample_offset must be < stepsize.
     */
    int to_above_sample_number(float sample_offset, float position) const noexcept {
        return ceil_with_tolerance((position - sample_offset) / this->stepsize()) - this->margin();
    }

    std::size_t margin() const { return m_margin; }

    float m_stepsize;
    std::size_t m_margin;
};

/**
 * Represents a blueprint on the the VDS data.
 *
 * Stepsize and existing sample position are expected to be retrieved from the
 * file. Margin is used for better interpolation at data edges and to cover up
 * for slight variations in calculations to make sure we always retrieve all
 * desired data.
 */
class RawSegmentBlueprint : public SegmentBlueprint {
public:
    RawSegmentBlueprint(float stepsize, float sample_position) : SegmentBlueprint(stepsize, 2) {
        m_sample_offset = fmod_with_tolerance(sample_position, this->stepsize());
    }

    std::size_t size(float top_boundary, float bottom_boundary) const noexcept {
        return SegmentBlueprint::size(m_sample_offset, top_boundary, bottom_boundary);
    }

    float top_sample_position(float top_boundary) const noexcept {
        return SegmentBlueprint::top_sample_position(m_sample_offset, top_boundary);
    }

    float bottom_sample_position(float bottom_boundary) const noexcept {
        return SegmentBlueprint::bottom_sample_position(m_sample_offset, bottom_boundary);
    }

private:
    // offset of the first sample from 0, aka m_sample_offset < stepsize
    float m_sample_offset;
};

/**
 * Represents a blueprint for the resampled data on which final attribute
 * calculations will be performed.
 *
 * Stepsize is provided by the user. Reference points are used as new sample
 * positions. No margin wanted.
 */
class ResampledSegmentBlueprint : public SegmentBlueprint {
public:
    ResampledSegmentBlueprint(float stepsize) : SegmentBlueprint(stepsize, 0) {}

    std::size_t size(float reference, float top_boundary, float bottom_boundary) const noexcept {
        float sample_offset = fmod_with_tolerance(reference, this->stepsize());
        return SegmentBlueprint::size(sample_offset, top_boundary, bottom_boundary);
    }

    float top_sample_position(float reference, float top_boundary) const noexcept {
        float sample_offset = fmod_with_tolerance(reference, this->stepsize());
        return SegmentBlueprint::top_sample_position(sample_offset, top_boundary);
    }

    float bottom_sample_position(float reference, float bottom_boundary) const noexcept {
        float sample_offset = fmod_with_tolerance(reference, this->stepsize());
        return SegmentBlueprint::bottom_sample_position(sample_offset, bottom_boundary);
    }

    /**
     * Number of samples that fits into a segment from reference to top boundary.
     * It is expected that reference >= top_boundary.
     */
    std::size_t nsamples_above(float reference, float top_boundary) const noexcept {
        float sample_offset = fmod_with_tolerance(reference, this->stepsize());
        std::size_t top_sample_number = to_above_sample_number(sample_offset, top_boundary);
        std::size_t reference_sample_number = std::round((reference - sample_offset) / this->stepsize());
        return reference_sample_number - top_sample_number;
    }

};

#endif /* VDS_SLICE_SUBVOLUME_HPP */
