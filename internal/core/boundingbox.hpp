#ifndef ONESEISMIC_API_BOUNDINGBOX_HPP
#define ONESEISMIC_API_BOUNDINGBOX_HPP

#include <utility>
#include <vector>

#include "coordinate_transformer.hpp"

#include <OpenVDS/OpenVDS.h>

class BoundingBox {
public:
    explicit BoundingBox(
        int const nilines,
        int const nxlines,
        CoordinateTransformer const& transformer
    ) : m_nilines(nilines), m_nxlines(nxlines), m_transformer(transformer)
    {}

    std::vector< std::pair<int, int> >       index()      noexcept (true);
    std::vector< std::pair<int, int> >       annotation() noexcept (true);
    std::vector< std::pair<double, double> > world()      noexcept (true);
private:
    int const m_nilines;
    int const m_nxlines;
    CoordinateTransformer const& m_transformer;
};

#endif /* ONESEISMIC_API_BOUNDINGBOX_HPP */
