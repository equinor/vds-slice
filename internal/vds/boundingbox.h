#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include <utility>
#include <vector>

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

class BoundingBox {
public:
    explicit BoundingBox(
        const OpenVDS::VolumeDataLayout *layout
    ) : layout(layout)
    {
        transformer = OpenVDS::IJKCoordinateTransformer(layout);
    }

    std::vector< std::pair<int, int> >       index()      noexcept (true);
    std::vector< std::pair<int, int> >       annotation() noexcept (true);
    std::vector< std::pair<double, double> > world()      noexcept (true);
private:
    OpenVDS::IJKCoordinateTransformer transformer;
    const OpenVDS::VolumeDataLayout*  layout;
};

#endif /* BOUNDINGBOX_H */
