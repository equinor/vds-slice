#ifndef VDS_SLICE_REGULAR_SURFACE_HPP
#define VDS_SLICE_REGULAR_SURFACE_HPP

#include <array>

struct Point {
    double x;
    double y;
};

struct AffineTransformation : private std::array< std::array< double, 3>, 2 > {
    using base_type = std::array< std::array< double, 3 >, 2 >;

    explicit AffineTransformation(base_type x) : base_type(std::move(x)) {}

    Point operator*(Point p) const noexcept (true);

    static AffineTransformation from_rotation(
        double xori,
        double yori,
        double xinc,
        double yinc,
        double rot
    ) noexcept (true);

    /* Make inverse transformation to the one created from rotation */
    static AffineTransformation inverse_from_rotation(
        double xori,
        double yori,
        double xinc,
        double yinc,
        double rot
    )noexcept (true);
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

public:
    RegularSurface(
        const float* data,
        std::size_t  nrows,
        std::size_t  ncols,
        float xori,
        float yori,
        float xinc,
        float yinc,
        float rot,
        float fillvalue
    ) : m_data(data),
        m_nrows(nrows),
        m_ncols(ncols),
        m_fillvalue(fillvalue),
        m_transformation(
            AffineTransformation::from_rotation(xori, yori, xinc, yinc, rot)),
        m_inverse_transformation(
            AffineTransformation::inverse_from_rotation(xori, yori, xinc, yinc, rot))
    {}

    /* Grid position (row, col) -> world coordinates */
    Point to_cdp(
        std::size_t const row,
        std::size_t const col
    ) const noexcept (false);

    /* World coordinates -> grid position */
    Point from_cdp(
        Point point
    ) const noexcept (false);

    /* Value at grid position (row, col) */
    float value(
        std::size_t const row,
        std::size_t const col
    ) const noexcept (false);

    float value(std::size_t i) const noexcept (false);
    
    float fillvalue() const noexcept (true) { return this->m_fillvalue; };

    std::size_t nrows() const noexcept (true) { return this->m_nrows; };
    std::size_t ncols() const noexcept (true) { return this->m_ncols; };
    std::size_t size()  const noexcept (true) { return this->ncols() * this->nrows(); };
private:
    const float* m_data;
    std::size_t  m_nrows;
    std::size_t  m_ncols;
    float        m_fillvalue;
    AffineTransformation    m_transformation;
    AffineTransformation    m_inverse_transformation;
};

// } // namespace surface

#endif /* VDS_SLICE_REGULAR_SURFACE_HPP */
