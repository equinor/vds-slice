#ifndef VDS_SLICE_DATAHANDLE_HPP
#define VDS_SLICE_DATAHANDLE_HPP

#include <memory>
#include <string>

#include <OpenVDS/OpenVDS.h>

#include "metadatahandle.hpp"
#include "subcube.hpp"

using voxel = float[OpenVDS::Dimensionality_Max];

class DataHandle {
    DataHandle(OpenVDS::VDSHandle handle);
    friend DataHandle* make_datahandle(const char* url,const char* credentials);
public:

    MetadataHandle const& get_metadata() const noexcept (true);

    static OpenVDS::VolumeDataFormat format() noexcept (true);

    std::int64_t subcube_buffer_size(SubCube const& subcube) noexcept (false);

    void read_subcube(
        void * const buffer,
        std::int64_t size,
        SubCube const& subcube
    ) noexcept (false);

    std::int64_t traces_buffer_size(std::size_t const ntraces) noexcept (false);

    void read_traces(
        void * const                    buffer,
        std::int64_t const              size,
        voxel const*                    coordinates,
        std::size_t const               ntraces,
        enum interpolation_method const interpolation_method
    ) noexcept (false);


    std::int64_t samples_buffer_size(std::size_t const nsamples) noexcept (false);

    void read_samples(
        void * const                    buffer,
        std::int64_t const              size,
        voxel const*                    samples,
        std::size_t const               nsamples,
        enum interpolation_method const interpolation_method
    ) noexcept (false);

private:
    OpenVDS::ScopedVDSHandle m_file_handle;
    OpenVDS::VolumeDataAccessManager m_access_manager;
    SingleMetadataHandle m_metadata;

    static int constexpr lod_level = 0;
    static int constexpr channel = 0;
};

DataHandle* make_datahandle(
    const char* url,
    const char* credentials
) noexcept (false);


#endif /* VDS_SLICE_DATAHANDLE_HPP */
