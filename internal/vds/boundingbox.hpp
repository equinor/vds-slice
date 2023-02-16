#ifndef VDS_SLICE_BOUNDINGBOX_HPP
#define VDS_SLICE_BOUNDINGBOX_HPP

#include <utility>
#include <vector>

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

class BoundingBox {
public:
    explicit BoundingBox(
        int const nilines,
        int const nxlines,
        OpenVDS::IJKCoordinateTransformer transformer
    ) : m_nilines(nilines), m_nxlines(nxlines), m_transformer(transformer)
    {}

    std::vector< std::pair<int, int> >       index()      noexcept (true);
    std::vector< std::pair<int, int> >       annotation() noexcept (true);
    std::vector< std::pair<double, double> > world()      noexcept (true);
private:
    int const m_nilines;
    int const m_nxlines;
    OpenVDS::IJKCoordinateTransformer m_transformer;
};

#endif /* VDS_SLICE_BOUNDINGBOX_HPP */
