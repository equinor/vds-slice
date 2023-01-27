#ifndef VDSMETADATAHANDLER_H
#define VDSMETADATAHANDLER_H

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

#include "axis.h"
#include "boundingbox.h"

class VDSMetadataHandler {
    private:
    std::shared_ptr<OpenVDS::ScopedVDSHandle> vdsHandle;
    OpenVDS::VolumeDataLayout const * vdsLayout;

    public:
    VDSMetadataHandler(const std::string url, const std::string credentials);

    Axis getInline()    const;
    Axis getCrossline() const;
    Axis getSample()    const;
    Axis getAxis(const ApiAxisName axisName) const;

    BoundingBox getBoundingBox() const;
    std::string getFormat()      const;
    std::string getCRS()         const;

    OpenVDS::VolumeDataFormat getChannelFormat(const int channelIndex);
    //TODO: Currently needed such that DataHandler can access the VDS dataset
    //      which is opened by VDSMetadataHandler. Returning a shared pointer
    //      makes sure that the VDSHandler is alive during access, i.e., the
    //      file handle has not been destroyed.
    std::shared_ptr<OpenVDS::ScopedVDSHandle> getVDSHandle();

    static OpenVDS::InterpolationMethod getInterpolation(
        InterpolationMethod interpolation);
};


#endif /* VDSMETADATAHANDLER_H */
