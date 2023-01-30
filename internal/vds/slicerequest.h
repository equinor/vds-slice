#ifndef SLICEREQUEST_H
#define SLICEREQUEST_H

#include <string>

#include "metadatahandler.h"
#include "requestbuffer.h"
#include "vds.h"

struct SliceRequestParameters {
    const ApiAxisName apiAxisName;
    const int         lineNumber;
};

namespace vds {

struct SubVolume {
    struct {
        int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
        int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
    } bounds;
};

class SliceRequest {
    private:
        MetadataHandler metadata;

    public:
    SliceRequest(const std::string url, const std::string credentials) :
        metadata(url, credentials) {}

    void validateAxis(const ApiAxisName& apiAxisName);

    SubVolume requestAsSubvolume(
        const SliceRequestParameters& parameters
    );

    response getData(const SubVolume& subvolume);
};

 } /* namespace vds */

#endif /* SLICEREQUEST_H */
