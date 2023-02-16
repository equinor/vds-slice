#include "datahandle.hpp"

#include <stdexcept>

#include <OpenVDS/OpenVDS.h>

#include "metadatahandle.hpp"
#include "subvolume.hpp"

DataHandle::DataHandle(std::string const url, std::string const credentials) {
    OpenVDS::Error error;
    this->m_file_handle = OpenVDS::Open(url, credentials, error);
    if(error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }
    this->m_access_manager = OpenVDS::GetAccessManager(this->m_file_handle);

    this->m_metadata = std::unique_ptr<MetadataHandle>(
        new MetadataHandle(this->m_access_manager.GetVolumeDataLayout())
    );
}

MetadataHandle const& DataHandle::get_metadata() const noexcept (true) {
    return *(this->m_metadata);
}

std::int64_t DataHandle::subvolume_buffer_size(
    SubVolume const& subvolume
) noexcept (false) {
    auto const * layout = this->m_access_manager.GetVolumeDataLayout();
    auto format = layout->GetChannelFormat(0);

    std::int64_t size = this->m_access_manager.GetVolumeSubsetBufferSize(
        subvolume.bounds.lower,
        subvolume.bounds.upper,
        format,
        0,
        0
    );

    return size;
}

void DataHandle::read_subvolume(
    void * const buffer,
    std::int64_t size,
    SubVolume const& subvolume
) noexcept (false) {
    auto const * layout = this->m_access_manager.GetVolumeDataLayout();
    auto format = layout->GetChannelFormat(0);
    auto request = this->m_access_manager.RequestVolumeSubset(
        buffer,
        size,
        OpenVDS::Dimensions_012,
        0,
        0,
        subvolume.bounds.lower,
        subvolume.bounds.upper,
        format
    );

    request.get()->WaitForCompletion();
}
