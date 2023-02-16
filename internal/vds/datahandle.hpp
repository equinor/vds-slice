#ifndef VDS_SLICE_DATAHANDLE_HPP
#define VDS_SLICE_DATAHANDLE_HPP

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

#include "metadatahandle.hpp"
#include "subvolume.hpp"

class DataHandle {
public:
    DataHandle(std::string const url, std::string const credentials);

    MetadataHandle const& get_metadata() const noexcept (true);

    std::int64_t subvolume_buffer_size(SubVolume const& subvolume) noexcept (false);

    void read_subvolume(
        void * const buffer,
        std::int64_t size,
        SubVolume const& subvolume
    ) noexcept (false);

private:
    OpenVDS::ScopedVDSHandle m_file_handle;
    OpenVDS::VolumeDataAccessManager m_access_manager;
    // Must be pointer due to delayed initialization.
    std::unique_ptr<MetadataHandle> m_metadata;
};

#endif /* VDS_SLICE_DATAHANDLE_HPP */
