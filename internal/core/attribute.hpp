#ifndef VDS_SLICE_ATTRIBUTE_HPP
#define VDS_SLICE_ATTRIBUTE_HPP

#include "regularsurface.hpp"
#include "subvolume.hpp"
#include "verticalwindow.hpp"
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

/** Windowed horizon
 *
 * Definition and layout of a windowed horizon
 * -------------------------------------------
 *
 * A windowed horizon is a horizon where we have multiple vertical
 * sample values for each horizontal position. Typically defined by a window
 * above and a window below the horizon definition. As illustrated by this
 * *flat* horizon:
 *                       *------*
 *                      /      /|
 *                     /      / |
 *                  - *------* /|
 *     window above | |      |/ *
 * horizon ---->    - |------| /
 *     window below | |      |/
 *                  - *------*
 *
 *
 * Note that the above and below window is just and example of how to think
 * about this data structure. This class, however, doesn't care about the
 * window definition in that much detail. It only needs to know the size of the
 * current vertical window. I.e. how many samples there are at each horizontal
 * position.
 *
 * As the illustration show, the windowed horizon is a 3D array. It's assumed
 * that the vertical axis is the fastest moving. I.e. that vertical samples at
 * the same horizontal position are contiguous in memory. There are no
 * assumptions on the order of the 2 horizontal axes.
 *
 * Class features
 * --------------
 *
 * In short this class is an abstraction around a float pointer to a 3D array
 * with the properties described above. The reason for the raw pointer is to be
 * compatible with C, and hence CGO and GO.
 */
class Horizon{
private:
    struct StepSizedIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = float;
        using pointer           = const float*;
        using reference         = const float&;

        StepSizedIterator(pointer cur, std::size_t step) : cur(cur), step(step) {}

        reference operator*()  { return *this->cur; }
        pointer   operator->() { return  this->cur; }

        StepSizedIterator& operator++() { this->cur += this->step; return *this; };

        friend
        bool operator==(StepSizedIterator const& lhs, StepSizedIterator const& rhs) {
            return lhs.cur == rhs.cur and lhs.step == rhs.step;
        }

        friend
        bool operator!=(StepSizedIterator const& lhs, StepSizedIterator const& rhs) {
            return !(lhs == rhs);
        }
    private:
        pointer cur;
        std::size_t step;
    };

    struct VerticalIterator : public StepSizedIterator {
        VerticalIterator(StepSizedIterator::pointer cur) : StepSizedIterator(cur, 1) {}
    };

public:
    Horizon(
        const float* data,
        std::size_t  hsize,
        std::size_t* buffer_offsets,
        float        fillvalue
    ) : m_ptr(data), m_hsize(hsize), m_buffer_offsets(buffer_offsets), m_fillvalue(fillvalue)
    {}

    using VerticalIt   = VerticalIterator;

    struct Window {
        Window(VerticalIt begin, VerticalIt end) : m_begin(begin), m_end(end) {}

        VerticalIt& begin() noexcept (true) { return this->m_begin; }
        VerticalIt& end()   noexcept (true) { return this->m_end; }
    private:
        VerticalIt m_begin;
        VerticalIt m_end;
    };

    /* Number of points in the horizontal plane */
    std::size_t hsize() const noexcept (true) { return this->m_hsize; };

    /* Size of attributes maps calculated by this Horizon */
    std::size_t mapsize() const noexcept (true) { return this->hsize() * sizeof(float); };

    Window at(std::size_t i) const noexcept (false);

    float fillvalue() const noexcept (true) { return this->m_fillvalue; };

private:
    const float* m_ptr;
    std::size_t  m_hsize;
    std::size_t* m_buffer_offsets;
    float        m_fillvalue;
};

/* Base class for attribute calculations
 *
 * The base class main role is to centralize buffer writes such that
 * implementations of new attributes, in the form of derived classes, don't
 * have to deal with pointer arithmetic.
 */
class AttributeMap {
public:
    AttributeMap(void* dst, std::size_t size) : dst(dst), size(size) {};

    /** Iterator type for computations
     *
     * All attribute computes produce a float, but the computation itself
     * should happen on doubles to not lose precision during intermediate
     * computation steps.
     */
    using InputIt = std::vector< double >::iterator;

    struct AttributeComputeParams{
        InputIt begin;
        InputIt end;
        std::size_t size;
        std::size_t reference_index;

        void update(
            InputIt begin,
            InputIt end,
            std::size_t size,
            std::size_t reference_index
        ) noexcept (true) ;
    };

    virtual float compute(ResampledSegment const & segment) noexcept (false) = 0;

    void write(float value, std::size_t index) {
        std::size_t offset = index * sizeof(float);

        if (offset >= this->size) {
            throw std::out_of_range("Attempting write outside attribute buffer");
        }

        memcpy((char*)this->dst + offset, &value, sizeof(float));
    }

    virtual ~AttributeMap() {};
private:
    void*       dst;
    std::size_t size;
};

struct Value final : public AttributeMap {
    Value(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};


class Min final : public AttributeMap {
public:
    Min(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class Max final : public AttributeMap {
public:
    Max(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class MaxAbs final : public AttributeMap {
public:
    MaxAbs(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class Mean final : public AttributeMap {
public:
    Mean(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class MeanAbs final : public AttributeMap {
public:
    MeanAbs(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class MeanPos final : public AttributeMap {
public:
    MeanPos(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class MeanNeg final : public AttributeMap {
public:
    MeanNeg(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class Median final : public AttributeMap {
public:
    Median(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class Rms final : public AttributeMap {
public:
    Rms(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

/* Calculated the population variance as we are interested in variance strictly
 * for the data defined by each window.
 */
class Var final : public AttributeMap {
public:
    Var(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

/* Calculated the population standard deviation as we are interested in
 * standard deviation strictly for the data defined by each window.
 */
class Sd final : public AttributeMap {
public:
    Sd(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class SumPos final : public AttributeMap {
public:
    SumPos(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class SumNeg final : public AttributeMap {
public:
    SumNeg(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

void calc_attributes(
    SurfaceBoundedSubVolume const& src_subvolume,
    ResampledSegmentBlueprint const* dst_segment_blueprint,
    std::vector< std::unique_ptr< AttributeMap > >& attrs,
    std::size_t from,
    std::size_t to
) noexcept (false);

#endif /* VDS_SLICE_ATTRIBUTE_HPP */
