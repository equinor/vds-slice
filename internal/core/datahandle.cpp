#include "datahandle.hpp"

#include <stdexcept>

#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/OpenVDS.h>

#include "metadatahandle.hpp"
#include "subcube.hpp"

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

SingleDataHandle* make_single_datahandle(
    const char* url,
    const char* credentials
) {
    OpenVDS::Error error;
    auto handle = OpenVDS::Open(url, credentials, error);
    if(error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }
    return new SingleDataHandle(std::move(handle));
}

SingleDataHandle::SingleDataHandle(OpenVDS::VDSHandle handle)
    : m_file_handle(handle), m_access_manager(OpenVDS::GetAccessManager(handle)), m_metadata(m_access_manager.GetVolumeDataLayout()) {}

MetadataHandle const& SingleDataHandle::get_metadata() const noexcept(true) {
    return this->m_metadata;
}

OpenVDS::VolumeDataFormat SingleDataHandle::format() noexcept(true) {
    /*
     * We always want to request data in OpenVDS::VolumeDataFormat::Format_R32
     * format for slice. For fence documentation says: "The traces/samples are
     * always in 32-bit floating point format."
     */
    return OpenVDS::VolumeDataFormat::Format_R32;
}

std::int64_t SingleDataHandle::subcube_buffer_size(
    SubCube const& subcube
) noexcept (false) {
    std::int64_t size = this->m_access_manager.GetVolumeSubsetBufferSize(
        subcube.bounds.lower,
        subcube.bounds.upper,
        SingleDataHandle::format(),
        SingleDataHandle::lod_level,
        SingleDataHandle::channel
    );

    return size;
}

void SingleDataHandle::read_subcube(
    void* const buffer,
    std::int64_t size,
    SubCube const& subcube
) noexcept (false) {
    auto request = this->m_access_manager.RequestVolumeSubset(
        buffer,
        size,
        OpenVDS::Dimensions_012,
        SingleDataHandle::lod_level,
        SingleDataHandle::channel,
        subcube.bounds.lower,
        subcube.bounds.upper,
        SingleDataHandle::format()
    );
    bool const success = request.get()->WaitForCompletion();

    if (!success) {
        throw std::runtime_error("Failed to read from VDS.");
    }
}

std::int64_t SingleDataHandle::traces_buffer_size(std::size_t const ntraces) noexcept(false) {
    int const dimension = this->get_metadata().sample().dimension();
    return this->m_access_manager.GetVolumeTracesBufferSize(ntraces, dimension);
}

void SingleDataHandle::read_traces(
    void* const buffer,
    std::int64_t const size,
    voxel const* coordinates,
    std::size_t const ntraces,
    enum interpolation_method const interpolation_method
) noexcept (false) {
    int const dimension = this->get_metadata().sample().dimension();

    auto request = this->m_access_manager.RequestVolumeTraces(
        (float*)buffer,
        size,
        OpenVDS::Dimensions_012,
        SingleDataHandle::lod_level,
        SingleDataHandle::channel,
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

std::int64_t SingleDataHandle::samples_buffer_size(
    std::size_t const nsamples
) noexcept (false) {
    return this->m_access_manager.GetVolumeSamplesBufferSize(
        nsamples,
        SingleDataHandle::channel
    );
}

void SingleDataHandle::read_samples(
    void* const buffer,
    std::int64_t const size,
    voxel const* samples,
    std::size_t const nsamples,
    enum interpolation_method const interpolation_method
) noexcept (false) {
    auto request = this->m_access_manager.RequestVolumeSamples(
        (float*)buffer,
        size,
        OpenVDS::Dimensions_012,
        SingleDataHandle::lod_level,
        SingleDataHandle::channel,
        samples,
        nsamples,
        ::to_interpolation(interpolation_method)
    );

    bool const success = request.get()->WaitForCompletion();
    if (!success) {
        throw std::runtime_error("Failed to read from VDS.");
    }
}
