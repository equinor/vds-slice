#ifndef VDSDATAHANDLER_H
#define VDSDATAHANDLER_H

#include <string>

#include "vdsmetadatahandler.h"
#include "vds.h"


// TODO: Should this be derived from the MetadataHandler? To some extend "Data"
// might be interpreted as more general than MetaData. We could also rename it
// VDSVolumeDataHandler to make the distinction more clear.
class VDSDataHandler {
    private:
    VDSMetadataHandler metadata;

    struct SubVolume {
        struct {
            int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
            int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
        } bounds;
    };

    public:

    VDSDataHandler(const std::string url, const std::string credentials);

    response getSlice(const ApiAxisName& axisName, const int lineNumber);

    response getFence(
        const CoordinateSystem    coordinateSystem,
        float const *             coordinates,
        const size_t              numberOfPoints,
        const InterpolationMethod interpolationMethod
    );
};

#endif /* VDSDATAHANDLER_H */
