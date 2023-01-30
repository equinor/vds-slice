#ifndef VDSSLICEREQUEST_H
#define VDSSLICEREQUEST_H

#include <string>

#include "vdsmetadatahandler.h"
#include "vdsrequestbuffer.h"
#include "vds.h"

struct SubVolume {
    struct {
        int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
        int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
    } bounds;
};

struct SliceRequestParameters {
    const ApiAxisName apiAxisName;
    const int         lineNumber;
};

class VDSSliceRequest {
    private:
        VDSMetadataHandler metadata;

    public:
    VDSSliceRequest(const std::string url, const std::string credentials) :
        metadata(url, credentials) {}

    void validateAxis(const ApiAxisName& apiAxisName);

    SubVolume requestAsSubvolume(
        const SliceRequestParameters& parameters
    );

    response getData(const SubVolume& subvolume);
};

#endif /* VDSSLICEREQUEST_H */
