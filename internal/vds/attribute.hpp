#ifndef VDS_SLICE_ATTRIBUTE_HPP
#define VDS_SLICE_ATTRIBUTE_HPP

#include "regularsurface.hpp"
#include "verticalwindow.hpp"
#include <cstddef>
#include <iterator>
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
 * full vertical window. I.e. how many samples there are at every horizontal
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
    struct StridedIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = float;
        using pointer           = const float*;
        using reference         = const float&;

        StridedIterator(pointer cur, std::size_t step) : cur(cur), step(step) {}

        reference operator*()  { return *this->cur; }
        pointer   operator->() { return  this->cur; }

        StridedIterator& operator++() { this->cur += this->step; return *this; };

        friend
        bool operator==(StridedIterator const& lhs, StridedIterator const& rhs) {
            return lhs.cur == rhs.cur and lhs.step == rhs.step;
        }

        friend
        bool operator!=(StridedIterator const& lhs, StridedIterator const& rhs) {
            return !(lhs == rhs);
        }
    private:
        pointer cur;
        std::size_t step;
    };

    struct VerticalIterator : public StridedIterator {
        VerticalIterator(StridedIterator::pointer cur) : StridedIterator(cur, 1) {}
    };

public:
    Horizon(
        const float* data,
        std::size_t hsize,
        std::size_t vsize,
        float       fillvalue
    ) : m_ptr(data), m_hsize(hsize), m_vsize(vsize), m_fillvalue(fillvalue)
    {}

    using HorizontalIt = StridedIterator;
    using VerticalIt   = VerticalIterator;

    HorizontalIt begin() const noexcept (true);
    HorizontalIt end()   const noexcept (true);

    struct Window {
        Window(VerticalIt begin, VerticalIt end) : m_begin(begin), m_end(end) {}

        VerticalIt& begin() noexcept (true) { return this->m_begin; }
        VerticalIt& end()   noexcept (true) { return this->m_end; }
    private:
        VerticalIt m_begin;
        VerticalIt m_end;
    };

    /* Vertical size of the horizon*/
    std::size_t vsize() const noexcept (true) { return this->m_vsize; };

    /* Number of points in the horizontal plane */
    std::size_t hsize() const noexcept (true) { return this->m_hsize; };

    /* Size of attributes maps calculated by this Horizon */
    std::size_t mapsize() const noexcept (true) { return this->hsize() * sizeof(float); };

    Window at(std::size_t i) const noexcept (false);

    float fillvalue() const noexcept (true) { return this->m_fillvalue; };

private:
    const float* m_ptr;
    std::size_t  m_hsize;
    std::size_t  m_vsize;
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

    virtual float compute(
        std::vector< double >::iterator begin,
        std::vector< double >::iterator end
    ) noexcept (false) = 0;

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
    Value(void* dst, std::size_t size, std::size_t idx)
        : AttributeMap(dst, size), idx(idx)
    {}

    float compute(
        std::vector< double >::iterator begin,
        std::vector< double >::iterator end
    ) noexcept (false) override;

private:
    std::size_t idx;
};


class Min final : public AttributeMap {
public:
    Min(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(
        std::vector< double >::iterator begin,
        std::vector< double >::iterator end
    ) noexcept (false) override;
};

class Max final : public AttributeMap {
public:
    Max(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(
        std::vector< double >::iterator begin,
        std::vector< double >::iterator end
    ) noexcept (false) override;
};

class Mean final : public AttributeMap {
public:
    Mean(void* dst, std::size_t size, std::size_t vsize)
        : AttributeMap(dst, size), vsize(vsize)
    {}

    float compute(
        std::vector< double >::iterator begin,
        std::vector< double >::iterator end
    ) noexcept (false) override;

private:
    /** vsize is essentially std::distance(begin, end), but that has a linear
     * complexity and would require compute() to find the same vsize for every
     * invocation. Hence it's much more efficient to store the value
     * explicitly.
     */
    std::size_t vsize;
};

class Rms final : public AttributeMap {
public:
    Rms(void* dst, std::size_t size, std::size_t vsize)
        : AttributeMap(dst, size), vsize(vsize)
    {}

    float compute(
        std::vector< double >::iterator begin,
        std::vector< double >::iterator end
    ) noexcept (false) override;

private:
    std::size_t vsize;
};

/* Calculated the population standard deviation as we are interested in
 * standard deviation strictly for the data defined by each window.
 */
class Sd final : public AttributeMap {
public:
    Sd(void* dst, std::size_t size, std::size_t vsize)
        : AttributeMap(dst, size), vsize(vsize)
    {}

    float compute(
        std::vector< double >::iterator begin,
        std::vector< double >::iterator end
    ) noexcept (false) override;

private:
    std::size_t vsize;
};


void calc_attributes(
    Horizon const& horizon,
    RegularSurface const& surface,
    VerticalWindow const& src_window,
    VerticalWindow const& dst_window,
    std::vector< std::unique_ptr< AttributeMap > >& attrs,
    std::size_t from,
    std::size_t to
) noexcept (false);

#endif /* VDS_SLICE_ATTRIBUTE_HPP */
