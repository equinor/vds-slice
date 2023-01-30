#ifndef FENCEREQUEST_H
#define FENCEREQUEST_H

#include <string>

#include <OpenVDS/OpenVDS.h>

#include "vdsmetadatahandler.h"
#include "vdsrequestbuffer.h"
#include "vds.h"

struct FenceRequestParameters {
    const CoordinateSystem    coordinateSystem;
    float const *             coordinates;
    const size_t              numberOfPoints;
    const InterpolationMethod interpolationMethod;
};

using VDSFencePointList = float[][OpenVDS::Dimensionality_Max];
class VDSFenceRequest {
    private:
        VDSMetadataHandler metadata;
    public:
    VDSFenceRequest(const std::string url, const std::string credentials) :
        metadata(url, credentials) {}

    std::unique_ptr<VDSFencePointList>
    requestAsPointList(const FenceRequestParameters& parameters);

    response getData(std::unique_ptr<VDSFencePointList>& points,
                     const FenceRequestParameters& parameters);
};

#endif /* FENCEREQUEST_H */
