#include <cmath>
#include <stdexcept>

#include "regularsurface.hpp"

Point AffineTransformation::operator*(Point p) const noexcept (true) {
    return {
        this->at(0)[0] * p.x + this->at(0)[1] * p.y + this->at(0)[2],
        this->at(1)[0] * p.x + this->at(1)[1] * p.y + this->at(1)[2],
    };
};


AffineTransformation AffineTransformation::from_rotation(
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


Point RegularSurface::coordinate(
    std::size_t const row,
    std::size_t const col
) const noexcept (false) {
    if (row >= this->nrows()) throw std::runtime_error("Row out of range");
    if (col >= this->ncols()) throw std::runtime_error("Col out of range");

    Point point {static_cast<double>(row), static_cast<double>(col)};

    return this->m_transformation * point;
}

float RegularSurface::value(
    std::size_t const row,
    std::size_t const col
) const noexcept (false) {
    if (row >= this->nrows()) throw std::runtime_error("Row out of range");
    if (col >= this->ncols()) throw std::runtime_error("Col out of range");

    return this->m_data[row * this->ncols() + col];
};

float RegularSurface::at(std::size_t i) const noexcept (false) {
    if (i >= this->size()) throw std::runtime_error("index out of range");

    return this->m_data[i];
}
