#ifndef DATAHANDLE_h
#define DATAHANDLE_h

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

#include "metadatahandle.h"
#include "fencerequest.h"
#include "slicerequest.h"
#include "vds.h"

namespace openvds_adapter {

class DataHandle {
public:
    DataHandle(const std::string url, const std::string credentials);

    response fence(const FenceRequestParameters& parameters);
    response slice(const SliceRequestParameters& parameters);

    const MetadataHandle& get_metadata() const noexcept (true) {
        return *(this->metadata);
    }

private:
    OpenVDS::ScopedVDSHandle file_handle;
    OpenVDS::VolumeDataAccessManager access_manager;
    // Must be pointer due to delayed initialization.
    std::unique_ptr<MetadataHandle> metadata;
};

} /* namespace openvds_adapter */

#endif /* DATAHANDLE_h */
