#include "vds.h"

#include <algorithm>
#include <string>
#include <stdexcept>

#include "nlohmann/json.hpp"

#include "axis.h"
#include "boundingbox.h"
#include "fencerequest.h"
#include "metadatahandler.h"
#include "slicerequest.h"

using namespace std;

void response_delete(struct response* buf) {
    if (!buf)
        return;

    delete[] buf->data;
    delete[] buf->err;
    *buf = response {};
}

static response wrap_as_response( const nlohmann::json::string_t& dump ) {
    response tmp{ new char[dump.size()], nullptr, dump.size() };
    std::copy(dump.begin(), dump.end(), tmp.data);
    return tmp;
}

nlohmann::json convert_axis_to_json(
    const vds::Axis& axis
) {
    nlohmann::json doc;
    doc = {
        { "annotation", axis.getApiName()           },
        { "min",        axis.getMin()            },
        { "max",        axis.getMax()            },
        { "samples",    axis.getNumberOfPoints() },
        { "unit",       axis.getUnit()           },
    };
    return doc;
}

struct response handle_error(
    const std::exception& e
) {
    response buf {};
    buf.err = new char[std::strlen(e.what()) + 1];
    std::strcpy(buf.err, e.what());
    return buf;
}

struct response metadata(
    char const * const vds,
    char const * const credentials
) {
    try {
        vds::MetadataHandler vdsMetadata( vds, credentials );

        nlohmann::json meta;
        meta["format"] = vdsMetadata.getFormat();

        meta["crs"] = vdsMetadata.getCRS();

        auto bbox = vdsMetadata.getBoundingBox();
        meta["boundingBox"]["ij"]   = bbox.index();
        meta["boundingBox"]["cdp"]  = bbox.world();
        meta["boundingBox"]["ilxl"] = bbox.annotation();

        const vds::Axis inlineAxis = vdsMetadata.getInline();
        meta["axis"].push_back(convert_axis_to_json(inlineAxis));

        const vds::Axis crosslineAxis = vdsMetadata.getCrossline();
        meta["axis"].push_back(convert_axis_to_json(crosslineAxis));

        const vds::Axis sampleAxis = vdsMetadata.getSample();
        meta["axis"].push_back(convert_axis_to_json(sampleAxis));

        return wrap_as_response(meta.dump());
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct response slice(
    char const * const vds,
    char const * const credentials,
    const int          lineno,
    const ApiAxisName  axisName
) {
    try {
        vds::SliceRequest sliceRequest(vds, credentials);
        sliceRequest.validateAxis(axisName);
        const SliceRequestParameters requestParameters{axisName, lineno};
        const vds::SubVolume sliceAsSubvolume = sliceRequest.requestAsSubvolume(
                                               requestParameters
                                           );
        return sliceRequest.getData(sliceAsSubvolume);
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct response slice_metadata(
    char const * const vds,
    char const * const credentials,
    const ApiAxisName  axisName
) {
    try {
        vds::MetadataHandler vdsMetadata(vds, credentials);

        nlohmann::json meta;
        meta["format"] = vdsMetadata.getFormat();

        if (axisName == ApiAxisName::I or axisName == ApiAxisName::INLINE) {
            meta["x"] = convert_axis_to_json(vdsMetadata.getSample());
            meta["y"] = convert_axis_to_json(vdsMetadata.getCrossline());
        }
        else {
            if (axisName == ApiAxisName::J or axisName == ApiAxisName::CROSSLINE) {
                meta["x"] = convert_axis_to_json(vdsMetadata.getSample());
                meta["y"] = convert_axis_to_json(vdsMetadata.getInline());
            }
            else {
                meta["x"] = convert_axis_to_json(vdsMetadata.getCrossline());
                meta["y"] = convert_axis_to_json(vdsMetadata.getInline());
            }
        }

        auto bbox = vdsMetadata.getBoundingBox();
        meta["boundingBox"]["ij"]   = bbox.index();
        meta["boundingBox"]["cdp"]  = bbox.world();
        meta["boundingBox"]["ilxl"] = bbox.annotation();

        return wrap_as_response(meta.dump());
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct response fence(
    char const * const             vds,
    char const * const             credentials,
    const enum CoordinateSystem    coordinate_system,
    float const * const            coordinates,
    const size_t                   npoints,
    const enum InterpolationMethod interpolation_method
) {
    try {
        vds::FenceRequest fenceRequest(vds, credentials);
        const FenceRequestParameters requestParameters{
            coordinate_system,
            coordinates,
            npoints,
            interpolation_method
        };
        auto pointList = fenceRequest.requestAsPointList(requestParameters);
        return fenceRequest.getData(pointList, requestParameters);
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}

struct response fence_metadata(
    char const * const vds,
    char const * const credentials,
    const size_t       npoints
) {
    try {
        vds::MetadataHandler vdsMetadata(vds, credentials);

        nlohmann::json meta;
        const vds::Axis sampleAxis = vdsMetadata.getSample();
        meta["shape"] = nlohmann::json::array(
                            {npoints, sampleAxis.getNumberOfPoints() }
                        );
        meta["format"] = vdsMetadata.getFormat();

        return wrap_as_response(meta.dump());
    } catch (const std::exception& e) {
        return handle_error(e);
    }
}
