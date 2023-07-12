#include "datahandle.hpp"

#include <stdexcept>

#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/OpenVDS.h>

#include "metadatahandle.hpp"
#include "subvolume.hpp"

namespace {

OpenVDS::InterpolationMethod to_interpolation(interpolation_method interpolation) {
    switch (interpolation)
    {
        case NEAREST: return OpenVDS::InterpolationMethod::Nearest;
        case LINEAR: return OpenVDS::InterpolationMethod::Linear;
        case CUBIC: return OpenVDS::InterpolationMethod::Cubic;
        case ANGULAR: return OpenVDS::InterpolationMethod::Angular;
        case TRIANGULAR: return OpenVDS::InterpolationMethod::Triangular;
        default: {
            throw std::runtime_error("Unhandled interpolation method");
        }
    }
}

} /* namespace */

DataHandle* make_datahandle(
    const char* url,
    const char* credentials
) {
    OpenVDS::Error error;
    auto handle = OpenVDS::Open(url, credentials, error);
    if(error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }
    return new DataHandle(std::move(handle));
}

DataHandle::DataHandle(OpenVDS::VDSHandle handle)
    : m_file_handle(handle)
    , m_access_manager(OpenVDS::GetAccessManager(handle))
    , m_metadata(m_access_manager.GetVolumeDataLayout())
{}

MetadataHandle const& DataHandle::get_metadata() const noexcept (true) {
    return this->m_metadata;
}

OpenVDS::VolumeDataFormat DataHandle::format() noexcept (true) {
    /*
     * We always want to request data in
     * OpenVDS::VolumeDataFormat::Format_R32 format for slice.
     * For fence documentation says: "The traces are always in 32-bit floating
     * point format."
     */
    return OpenVDS::VolumeDataFormat::Format_R32;
}

std::int64_t DataHandle::subvolume_buffer_size(
    SubVolume const& subvolume
) noexcept (false) {
    std::int64_t size = this->m_access_manager.GetVolumeSubsetBufferSize(
        subvolume.bounds.lower,
        subvolume.bounds.upper,
        DataHandle::format(),
        DataHandle::lod_level,
        DataHandle::channel
    );

    return size;
}

void DataHandle::read_subvolume(
    void * const buffer,
    std::int64_t size,
    SubVolume const& subvolume
) noexcept (false) {
    auto request = this->m_access_manager.RequestVolumeSubset(
        buffer,
        size,
        OpenVDS::Dimensions_012,
        DataHandle::lod_level,
        DataHandle::channel,
        subvolume.bounds.lower,
        subvolume.bounds.upper,
        DataHandle::format()
    );
    bool const success = request.get()->WaitForCompletion();

    if (!success) {
        throw std::runtime_error("Failed to read from VDS.");
    }
}

std::int64_t DataHandle::traces_buffer_size(std::size_t const ntraces) noexcept (false) {
    int const dimension = this->get_metadata().sample().dimension();
    return this->m_access_manager.GetVolumeTracesBufferSize(ntraces, dimension);
}

void DataHandle::read_traces(
    void * const                    buffer,
    std::int64_t const              size,
    voxel const*                    coordinates,
    std::size_t const               ntraces,
    enum interpolation_method const interpolation_method
) noexcept (false) {
    int const dimension = this->get_metadata().sample().dimension();

    auto request = this->m_access_manager.RequestVolumeTraces(
        (float*)buffer,
        size,
        OpenVDS::Dimensions_012,
        DataHandle::lod_level,
        DataHandle::channel,
        coordinates,
        ntraces,
        ::to_interpolation(interpolation_method),
        dimension
    );
    bool const success = request.get()->WaitForCompletion();

    if (!success) {
        throw std::runtime_error("Failed to read from VDS.");
    }
}

std::int64_t DataHandle::samples_buffer_size(
    std::size_t const nsamples
) noexcept (false) {
    return this->m_access_manager.GetVolumeSamplesBufferSize(
        nsamples,
        DataHandle::channel
    );
}

void DataHandle::read_samples(
    void * const                    buffer,
    std::int64_t const              size,
    voxel const*                    samples,
    std::size_t const               nsamples,
    enum interpolation_method const interpolation_method
) noexcept (false) {
    auto request = this->m_access_manager.RequestVolumeSamples(
        (float*)buffer,
        size,
        OpenVDS::Dimensions_012,
        DataHandle::lod_level,
        DataHandle::channel,
        samples,
        nsamples,
        ::to_interpolation(interpolation_method)
    );

    bool const success = request.get()->WaitForCompletion();
    if (!success) {
        throw std::runtime_error("Failed to read from VDS.");
    }
}
