#ifndef VDS_SLICE_REGULAR_SURFACE_HPP
#define VDS_SLICE_REGULAR_SURFACE_HPP

#include <array>
#include <cmath>

struct Cdp {
    double x;
    double y;
};

struct Transform : private std::array< std::array< double, 3>, 2 > {
    using base_type = std::array< std::array< double, 3 >, 2 >;

    explicit Transform(base_type x) : base_type(std::move(x)) {}

    Cdp to_world(std::size_t col, std::size_t row) const noexcept (true) {
        return {
            this->at(0)[0] * row + this->at(0)[1] * col + this->at(0)[2],
            this->at(1)[0] * row + this->at(1)[1] * col + this->at(1)[2],
        };
    };

    static Transform from_rotation(
        float xori,
        float yori,
        float xinc,
        float yinc,
        float rot
    ) noexcept (true) {
        double rad = rot * (M_PI / 180);
        return Transform(base_type({{
            xinc * std::cos(rad),  -yinc * std::sin(rad), xori,
            xinc * std::sin(rad),   yinc * std::cos(rad), yori
        }}));
    }
};


/** Regular Surface - 2D array with geospacial information
 *
 * A regular surface is defined by a 2D regular grid with a shape of nrows *
 * ncols. The grid is located in physical space. The mapping from grid
 * positions (row, col) to a world coordinates is done through an affine
 * transformation [1].
 *
 * The grid itself, although 2D by nature, is represented by a flat C array, in
 * order to pass it between Go and C++ (through C) without copying it.
 *
 * [1] https://en.wikipedia.org/wiki/Affine_transformation
 */
class RegularSurface{
public:
    RegularSurface(
        const float* data,
        std::size_t  nrows,
        std::size_t  ncols,
        Transform    transform
    ) : m_data(data), m_nrows(nrows), m_ncols(ncols), m_transform(transform)
    {}

    /* Grid position (row, col) -> world coordinates */
    Cdp coordinate(
        std::size_t const row,
        std::size_t const col
    ) noexcept (false) {
        if (row >= this->nrows()) throw std::runtime_error("Row out of range");
        if (col >= this->ncols()) throw std::runtime_error("Col out of range");

        return this->m_transform.to_world(col, row);
    }

    /* Value at grid position (row, col) */
    float sample(
        std::size_t const row,
        std::size_t const col
    ) noexcept (false) {
        if (row >= this->nrows()) throw std::runtime_error("Row out of range");
        if (col >= this->ncols()) throw std::runtime_error("Col out of range");

        return this->m_data[row * this->ncols() + col];
    };

    std::size_t nrows() noexcept (true) { return this->m_nrows; };
    std::size_t ncols() noexcept (true) { return this->m_ncols; };
    std::size_t size()  noexcept (true) { return this->ncols() * this->nrows(); };
private:
    const float* m_data;
    std::size_t  m_nrows;
    std::size_t  m_ncols;
    Transform    m_transform;
};

// } // namespace surface

#endif /* VDS_SLICE_REGULAR_SURFACE_HPP */
