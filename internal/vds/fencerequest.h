#ifndef FENCEREQUEST_H
#define FENCEREQUEST_H

#include <string>

#include <OpenVDS/OpenVDS.h>

#include "metadatahandler.h"
#include "requestbuffer.h"
#include "vds.h"

struct FenceRequestParameters {
    const CoordinateSystem    coordinateSystem;
    float const *             coordinates;
    const size_t              numberOfPoints;
    const InterpolationMethod interpolationMethod;
};

namespace vds {

using FencePointList = float[][OpenVDS::Dimensionality_Max];
class FenceRequest {
    private:
        MetadataHandler metadata;
    public:
    FenceRequest(const std::string url, const std::string credentials) :
        metadata(url, credentials) {}

    std::unique_ptr<FencePointList>
    requestAsPointList(const FenceRequestParameters& parameters);

    response getData(std::unique_ptr<FencePointList>& points,
                     const FenceRequestParameters& parameters);
};

} /* namespace vds */

#endif /* FENCEREQUEST_H */
