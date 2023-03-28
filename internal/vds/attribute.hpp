#ifndef VDS_SLICE_ATTRIBUTE_HPP
#define VDS_SLICE_ATTRIBUTE_HPP

#include <functional>

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

    /** Workhorse for attribute calculation
     *
     * The provided lambda func is called with an iterator pair (begin, end)
     * for each vertical window in the horizon. The results of each lambda call
     * is memcpy'd into the output-buffer 'dst'. 'dst' needs to be at least
     * this->mapsize() large. Example of an lambda that will calculate the
     * minimum value of each vertical window:
     *
     *     auto min_value = [](It beg, It end) {
     *         return *std::min_element(beg, end);
     *     }
     *
     * Calling calc_attribute() like so, will write the entire attribute to dst:
     *
     *     horizon.calc_attribute(dst, size, min_value)
     *
     * Like the horzion itself, attribute values are written as floating
     * points.
     *
     * The first value of each window is compared to fillvalue. If true, no
     * computation takes place for that window and the output buffer will be
     * populated with the fillvalue instead.
     */
    template< typename Func >
    void calc_attribute(void* dst, std::size_t size, Func func) const;

private:
    HorizontalIt begin() const noexcept (true);
    HorizontalIt end()   const noexcept (true);

    const float* m_ptr;
    std::size_t  m_hsize;
    std::size_t  m_vsize;
    float        m_fillvalue;
};

namespace attributes {

/* Attribute computations */

void  min(Horizon const& horizon, void* dst, std::size_t size) noexcept (false);
void  max(Horizon const& horizon, void* dst, std::size_t size) noexcept (false);
void mean(Horizon const& horizon, void* dst, std::size_t size) noexcept (false);

} // namespace attributes

#endif /* VDS_SLICE_ATTRIBUTE_HPP */
