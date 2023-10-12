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

/**
 * Represents part of the trace.
 */
class Segment {
public:
    virtual std::size_t size() const noexcept = 0;

    virtual float top_sample_position() const noexcept = 0;

    virtual float bottom_sample_position() const noexcept = 0;

    /**
     * All positions of samples in the segment
     */
    std::vector<double> sample_positions() const {
        std::size_t size = this->size();
        std::vector<double> positions;
        positions.reserve(size);
        float top_sample_position = this->top_sample_position();
        for (int index = 0; index < size; ++index) {
            positions.push_back(this->sample_position_at(index, top_sample_position));
        }
        return positions;
    }

    /**
     * Position of sample at provided index, given that top sample is at position 0
     */
    float sample_position_at(std::size_t index) const noexcept{
        return Segment::sample_position_at(index, this->top_sample_position());
    }

protected:
    Segment(
        const float reference,
        const float top_boundary,
        const float bottom_boundary
    )
        : m_reference(reference), m_top_boundary(top_boundary),
          m_bottom_boundary(bottom_boundary) {}

    /**
     * Position of sample at provided index, given position of the top sample at index 0
     * Note: top_sample_position is not calculated but provided so that it could be cached
     */
    float sample_position_at(std::size_t index, float top_sample_position) const noexcept{
        return top_sample_position + this->blueprint()->stepsize() * index;
    }

    /*
      Re-initialization functions are introduced for performance reason only. In
      theory it would be nice to create a new immutable object for every cell on
      the horizontal plane. Yet the number of cells could reach 10 million, and
      object creation starts to take its toll. Updating existing objects appears
      to be faster then creating new ones.
     */
    void reinitialize(float reference, float top, float bottom) noexcept {
        this->m_reference = reference;
        this->m_top_boundary = top;
        this->m_bottom_boundary = bottom;
    }

    virtual SegmentBlueprint const* blueprint() const noexcept = 0;
    float m_reference;
    float m_top_boundary;
    float m_bottom_boundary;
};

/**
 * Describes a segment which doesn't manage its own data, but has a view to it.
 * It our case these are segments on the raw vds data.
 */
class RawSegment : public Segment {
public:
    RawSegment(
        const float reference,
        const float top_boundary,
        const float bottom_boundary,
        std::vector<float>::const_iterator data_begin,
        std::vector<float>::const_iterator data_end,
        RawSegmentBlueprint const* blueprint
    )
        : Segment(reference, top_boundary, bottom_boundary),
          m_blueprint(blueprint),
          m_data_begin(data_begin),
          m_data_end(data_end) {}

    void reinitialize(
        float reference,
        float top_boundary,
        float bottom_boundary,
        std::vector<float>::const_iterator data_begin,
        std::vector<float>::const_iterator data_end
    ) noexcept {
        Segment::reinitialize(reference, top_boundary, bottom_boundary);
        this->m_data_begin = data_begin;
        this->m_data_end = data_end;
    }

    std::size_t size() const noexcept {
        return this->m_blueprint->size(m_top_boundary, m_bottom_boundary);
    }

    float top_sample_position() const noexcept {
        return this->m_blueprint->top_sample_position(m_top_boundary);
    }
    float bottom_sample_position() const noexcept {
        return this->m_blueprint->bottom_sample_position(m_bottom_boundary);
    }

    std::vector<float>::const_iterator begin() const noexcept { return m_data_begin; }
    std::vector<float>::const_iterator end() const noexcept { return m_data_end; }

protected:
    SegmentBlueprint const* blueprint() const noexcept {
        return m_blueprint;
    }

private:
    RawSegmentBlueprint const* m_blueprint;
    std::vector<float>::const_iterator m_data_begin;
    std::vector<float>::const_iterator m_data_end;
};

/**
 * Describes a segment which manages its own data. In our case these are the
 * resampled segments.
 *
 * This class is needed as we do not want to have a SubVolume object for
 * resampled data. We do not need to store such a big chunk of data in the
 * memory as we can store just small ones, perform computations and dispose of
 * the data immediately.
 */
class ResampledSegment : public Segment {
public:
    ResampledSegment(
        float reference,
        float top_boundary,
        float bottom_boundary,
        ResampledSegmentBlueprint const* blueprint
    )
        : Segment(reference, top_boundary, bottom_boundary), m_blueprint(blueprint) {
        this->m_data = std::vector<double>(this->size());
    }

    void reinitialize(float reference, float top_boundary, float bottom_boundary) {
        Segment::reinitialize(reference, top_boundary, bottom_boundary);

        this->m_data.resize(this->size());
    }

    std::vector<double>::iterator begin() noexcept { return m_data.begin(); }
    std::vector<double>::iterator end() noexcept { return m_data.end(); }

    std::vector<double>::const_iterator begin() const noexcept { return m_data.begin(); }
    std::vector<double>::const_iterator end() const noexcept { return m_data.end(); }

    std::size_t size() const noexcept {
        return this->m_blueprint->size(m_reference, m_top_boundary, m_bottom_boundary);
    }

    float top_sample_position() const noexcept {
        return this->m_blueprint->top_sample_position(m_reference, m_top_boundary);
    }
    float bottom_sample_position() const noexcept {
        return this->m_blueprint->bottom_sample_position(m_reference, m_bottom_boundary);
    }

    std::size_t reference_index() const noexcept {
        return this->m_blueprint->nsamples_above(m_reference, m_top_boundary);
    }

protected:
    SegmentBlueprint const* blueprint() const noexcept {
        return m_blueprint;
    }

private:
    ResampledSegmentBlueprint const* m_blueprint;
    std::vector<double> m_data;
};

#endif /* VDS_SLICE_SUBVOLUME_HPP */
