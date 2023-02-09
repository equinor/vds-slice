#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include <utility>
#include <vector>

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

#include "axis.h"

namespace openvds_adapter {

class BoundingBox {
public:
    explicit BoundingBox(
        const OpenVDS::VolumeDataLayout *layout,
        const Axis& inline_axis,
        const Axis& crossline_axis
    ) : ils(inline_axis.get_number_of_points()-1),
        xls(crossline_axis.get_number_of_points()-1)
    {
        transformer = OpenVDS::IJKCoordinateTransformer(layout);
    }

    std::vector< std::pair<int, int> >       index()      noexcept (true);
    std::vector< std::pair<int, int> >       annotation() noexcept (true);
    std::vector< std::pair<double, double> > world()      noexcept (true);
private:
    OpenVDS::IJKCoordinateTransformer transformer;

    // Maximum index of inline axis
    const int ils;
    // Maximum index of crossline axis
    const int xls;
};

} /* namespace openvds_adapter */

#endif /* BOUNDINGBOX_H */
