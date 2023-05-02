#ifndef VDS_SLICE_ATTRIBUTE_HPP
#define VDS_SLICE_ATTRIBUTE_HPP

#include <functional>
#include <variant>

namespace attributes {
namespace {

/* Base class for attribute calculations
 *
 * The base class centralizes buffer writes. It's write interface is designed
 * for succesive writes and the class maintains it's own pointer to the next
 * element in the dst buffer to be written.
 */
template< typename Derived >
struct AttributeBase {
public:
    AttributeBase(void* dst, std::size_t size, std::size_t initial_offset)
        : dst(dst), size(size), next(initial_offset)
    {};

    template< typename InputIt >
    void compute(InputIt begin, InputIt end) noexcept (false) {
        static_cast< Derived& >(*this).compute(begin, end);
    }

    void write(float value) {
        if (this->next >= this->size) {
            throw std::out_of_range("Attempting write outside attribute buffer");
        }

        char* dst = (char*)this->dst + this->next * sizeof(float);
        memcpy(dst, &value, sizeof(float));
        ++this->next;
    }
private:
    void*       dst;
    std::size_t size;
    std::size_t next;
};

} // namespace

struct Min : public AttributeBase< Min > {
    Min(void* dst, std::size_t size, std::size_t initial_offset = 0)
        : AttributeBase< Min >(dst, size, initial_offset)
    {}

    template< typename InputIt >
    void compute(InputIt begin, InputIt end) noexcept (false) {
        float value = *std::min_element(begin, end);
        this->write(value);
    }
};

struct Max : public AttributeBase< Max > {
    Max(void* dst, std::size_t size, std::size_t initial_offset = 0)
        : AttributeBase< Max >(dst, size, initial_offset)
    {}

    template< typename InputIt >
    void compute(InputIt begin, InputIt end) noexcept (false) {
        float value = *std::max_element(begin, end);
        this->write(value);
    }
};

struct Mean : public AttributeBase< Mean > {
    Mean(void*      dst,
        std::size_t size,
        std::size_t vsize,
        std::size_t initial_offset = 0
    ) : AttributeBase< Mean >(dst, size, initial_offset), vsize(vsize)
    {}

    template< typename InputIt >
    void compute(InputIt begin, InputIt end) noexcept (false) {
        float sum = std::accumulate(begin, end, 0.0f);
        this->write(sum / this->vsize);
    }
private:
    std::size_t vsize;
};

struct Rms : public AttributeBase< Rms > {
    Rms(void*       dst,
        std::size_t size,
        std::size_t vsize,
        std::size_t initial_offset = 0
    ) : AttributeBase< Rms >(dst, size, initial_offset), vsize(vsize)
    {}

    template< typename InputIt >
    void compute(InputIt begin, InputIt end) noexcept (false) {
        float sum = std::accumulate(begin, end, 0,
            [](float a, float b) {
                return a + std::pow(b, 2);
            }
        );
        this->write(std::sqrt(sum / this->vsize));
    }
private:
    std::size_t vsize;
};

using Attribute = std::variant<
    Min,
    Max,
    Mean,
    Rms
>;

struct AttributeFillVisitor {
    AttributeFillVisitor(float fillvalue) : fillvalue(fillvalue) {}

    template< typename Attr >
    void operator()(Attr& attribute) const {
        attribute.write(this->fillvalue);
    }

private:
    float fillvalue;
};

template< typename InputIt >
struct AttributeComputeVisitor {
    AttributeComputeVisitor(InputIt begin, InputIt end)
        : begin(begin), end(end)
    {}

    template< typename Attr >
    void operator()(Attr& attribute) const {
        attribute.compute(this->begin, this->end);
    }

private:
    InputIt begin;
    InputIt end;
};

} // namespace attributes

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
 *
 * `calc_attribute` is the main interface and provides a way for the caller to
 * specify a calculation (in form of a lambda) that is applied to every
 * vertical window. This makes it very easy for the consumer to implement their
 * own attributes without having to implement correct looping of the windows
 * each time.
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

    /* Vertical size of the horizon*/
    std::size_t vsize() const noexcept (true) { return this->m_vsize; };

    /* Number of points in the horizontal plane */
    std::size_t hsize() const noexcept (true) { return this->m_hsize; };

    /* Size of attributes maps calculated by this Horizon */
    std::size_t mapsize() const noexcept (true) { return this->hsize() * sizeof(float); };

    float fillvalue() const noexcept (true) { return this->m_fillvalue; };

    void calc_attributes(std::vector< attributes::Attribute >& attrs) const;

private:
    HorizontalIt begin() const noexcept (true);
    HorizontalIt end()   const noexcept (true);

    const float* m_ptr;
    std::size_t  m_hsize;
    std::size_t  m_vsize;
    float        m_fillvalue;
};

#endif /* VDS_SLICE_ATTRIBUTE_HPP */
