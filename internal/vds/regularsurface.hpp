#ifndef VDS_SLICE_REGULAR_SURFACE_HPP
#define VDS_SLICE_REGULAR_SURFACE_HPP

#include <array>
#include <cmath>

struct Point {
    double x;
    double y;
};


/** Regular Surface - a set of data points over the finite part of 2D plane.
 * It is represented as 2D array with geospacial information. Each array value
 * can mean anything, but in practice it would likely be the depth at the grid
 * position used to calculate the horizon.
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

    struct AffineTransformation : private std::array< std::array< double, 3>, 2 > {
        using base_type = std::array< std::array< double, 3 >, 2 >;

        explicit AffineTransformation(base_type x) : base_type(std::move(x)) {}

        Point operator*(Point p) const noexcept (true) {
            return {
                this->at(0)[0] * p.x + this->at(0)[1] * p.y + this->at(0)[2],
                this->at(1)[0] * p.x + this->at(1)[1] * p.y + this->at(1)[2],
            };
        };

        static AffineTransformation from_rotation(
            float xori,
            float yori,
            float xinc,
            float yinc,
            float rot
        ) noexcept (true) {
            double rad = rot * (M_PI / 180);
            /**
            * Matrix is composed by applying affine transformations [1] in the
            * following order:
            * - scaling by xinc, yinc
            * - counterclockwise rotation by angle rad around the center
            * - translation by the offset (xori, yori)
            *
            * By scaling unit vectors, rotating coordinate system axes and moving
            * coordinate system center to new position we transform index-based
            * rows-and-columns cartesian coordinate system into CDP-surface one.
            *
            * [1] https://en.wikipedia.org/wiki/Affine_transformation
            */
            return AffineTransformation(base_type({{
                xinc * std::cos(rad),  -yinc * std::sin(rad), xori,
                xinc * std::sin(rad),   yinc * std::cos(rad), yori
            }}));
        }
    };

public:
    RegularSurface(
        const float* data,
        std::size_t  nrows,
        std::size_t  ncols,
        float xori,
        float yori,
        float xinc,
        float yinc,
        float rot
    ) : m_data(data),
        m_nrows(nrows),
        m_ncols(ncols),
        m_transformation(
            AffineTransformation::from_rotation(xori, yori, xinc, yinc, rot))
    {}

    /* Grid position (row, col) -> world coordinates */
    Point coordinate(
        std::size_t const row,
        std::size_t const col
    ) const noexcept (false) {
        if (row >= this->nrows()) throw std::runtime_error("Row out of range");
        if (col >= this->ncols()) throw std::runtime_error("Col out of range");

        Point point {static_cast<double>(row), static_cast<double>(col)};

        return this->m_transformation * point;
    }

    /* Value at grid position (row, col) */
    float value(
        std::size_t const row,
        std::size_t const col
    ) const noexcept (false) {
        if (row >= this->nrows()) throw std::runtime_error("Row out of range");
        if (col >= this->ncols()) throw std::runtime_error("Col out of range");

        return this->m_data[row * this->ncols() + col];
    };

    std::size_t nrows() const noexcept (true) { return this->m_nrows; };
    std::size_t ncols() const noexcept (true) { return this->m_ncols; };
    std::size_t size()  const noexcept (true) { return this->ncols() * this->nrows(); };
private:
    const float* m_data;
    std::size_t  m_nrows;
    std::size_t  m_ncols;
    AffineTransformation    m_transformation;
};

// } // namespace surface

#endif /* VDS_SLICE_REGULAR_SURFACE_HPP */
