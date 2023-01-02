#ifndef POSTSTACK_H
#define POSTSTACK_H

#include "seismichandle.h"
#include "vds.h"

enum AxisDirection : int {
    X=0,
    Y=1,
    Z=2
};

/** Verifies dimension is 3D and axis order */
class PostStackHandle : public SeismicHandle {

public:
    PostStackHandle(std::string url, std::string conn);

    requestdata get_slice(
        const Axis          axis,
        const int           line_number,
        const LevelOfDetail level_of_detail = LevelOfDetail::Default,
        const Channel       channel = Channel::Default
    );
};

#endif /* POSTSTACK_H */
