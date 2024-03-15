#ifndef VDS_SLICE_DATAHANDLE_HPP
#define VDS_SLICE_DATAHANDLE_HPP

#include <memory>
#include <string>
#include<functional>

#include <OpenVDS/OpenVDS.h>

#include "metadatahandle.hpp"
#include "subcube.hpp"

using voxel = float[OpenVDS::Dimensionality_Max];
using binary_function = std::function<void(float *, const float *, std::size_t)>;

class SingleDataHandle {
    SingleDataHandle(OpenVDS::VDSHandle handle);
    friend SingleDataHandle* make_single_datahandle(const char* url,const char* credentials);
public:
    MetadataHandle const& get_metadata() const noexcept(true);

    static OpenVDS::VolumeDataFormat format() noexcept(true);

    std::int64_t subcube_buffer_size(SubCube const& subcube) noexcept(false);

    void read_subcube(
        void* const buffer,
        std::int64_t size,
        SubCube const& subcube
    ) noexcept(false);

    std::int64_t traces_buffer_size(std::size_t const ntraces) noexcept(false);

    void read_traces(
        void* const buffer,
        std::int64_t const size,
        voxel const* coordinates,
        std::size_t const ntraces,
        enum interpolation_method const interpolation_method
    ) noexcept(false);

    std::int64_t samples_buffer_size(std::size_t const nsamples) noexcept(false);

    void read_samples(
        void* const buffer,
        std::int64_t const size,
        voxel const* samples,
        std::size_t const nsamples,
        enum interpolation_method const interpolation_method
    ) noexcept(false);

private:
    OpenVDS::ScopedVDSHandle m_file_handle;
    OpenVDS::VolumeDataAccessManager m_access_manager;
    SingleMetadataHandle m_metadata;

    static int constexpr lod_level = 0;
    static int constexpr channel = 0;
};

SingleDataHandle* make_single_datahandle(
    const char* url,
    const char* credentials
) noexcept (false);

class DoubleDataHandle {
    DoubleDataHandle(OpenVDS::VDSHandle handle_a, OpenVDS::VDSHandle handle_b, binary_function binary_operator);
    friend DoubleDataHandle* make_double_datahandle(
        const char* url_a,
        const char* credentials_a,
        const char* url_b,
        const char* credentials_b,
        binary_function binary_operator
    );

public:
    MetadataHandle const& get_metadata() const noexcept(true);

    static OpenVDS::VolumeDataFormat format() noexcept(true);

    std::int64_t subcube_buffer_size(SubCube const& subcube) noexcept(false);

    void read_subcube(
        void* const buffer,
        std::int64_t size,
        SubCube const& subcube
    ) noexcept(false);

    std::int64_t traces_buffer_size(std::size_t const ntraces) noexcept(false);

    void read_traces(
        void* const buffer,
        std::int64_t const size,
        voxel const* coordinates,
        std::size_t const ntraces,
        enum interpolation_method const interpolation_method
    ) noexcept(false);

    std::int64_t samples_buffer_size(std::size_t const nsamples) noexcept(false);

    void read_samples(
        void* const buffer,
        std::int64_t const size,
        voxel const* samples,
        std::size_t const nsamples,
        enum interpolation_method const interpolation_method
    ) noexcept(false);

private:
    OpenVDS::ScopedVDSHandle m_file_handle_a;
    OpenVDS::ScopedVDSHandle m_file_handle_b;
    OpenVDS::VolumeDataAccessManager m_access_manager_a;
    OpenVDS::VolumeDataAccessManager m_access_manager_b;
    DoubleVolumeDataLayout m_layout;
    DoubleMetadataHandle m_metadata;
    SingleMetadataHandle m_metadata_a;
    SingleMetadataHandle m_metadata_b;
    binary_function m_binary_operator;

    static int constexpr lod_level = 0;
    static int constexpr channel = 0;

    SubCube offset_bounds(SubCube subcube, SingleMetadataHandle metadata);
};

DoubleDataHandle* make_double_datahandle(
    const char* url_a, const char* credentials_a,
    const char* url_b, const char* credentials_b,
    binary_function binary_operator
) noexcept(false);

#endif /* VDS_SLICE_DATAHANDLE_HPP */
