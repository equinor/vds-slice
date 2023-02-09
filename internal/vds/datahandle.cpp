#include "datahandle.h"

#include <stdexcept>

DataHandle::DataHandle(const std::string url, const std::string credentials)
{
    OpenVDS::Error error;
    this->file_handle = OpenVDS::Open(url, credentials, error);
    if(error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }
    this->access_manager = OpenVDS::GetAccessManager(this->file_handle);

    auto const * const vds_layout = OpenVDS::GetLayout(this->file_handle);
    metadata = std::unique_ptr<MetadataHandle>(
        new MetadataHandle(vds_layout)
    );
}

response DataHandle::fence(const FenceRequestParameters& parameters) {
    FenceRequest fence_request(*(this->metadata), this->access_manager);
    return fence_request.get_data(parameters);
}

response DataHandle::slice(const SliceRequestParameters& parameters) {
    SliceRequest slice_request(*(this->metadata), this->access_manager);
    return slice_request.get_data(parameters);
}
