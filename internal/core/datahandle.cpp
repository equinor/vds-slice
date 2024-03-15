#include "datahandle.hpp"

#include <stdexcept>

#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/OpenVDS.h>

#include "metadatahandle.hpp"
#include "subcube.hpp"

namespace {

OpenVDS::InterpolationMethod to_interpolation(interpolation_method interpolation) {
    switch (interpolation) {
        case NEAREST:
            return OpenVDS::InterpolationMethod::Nearest;
        case LINEAR:
            return OpenVDS::InterpolationMethod::Linear;
        case CUBIC:
            return OpenVDS::InterpolationMethod::Cubic;
        case ANGULAR:
            return OpenVDS::InterpolationMethod::Angular;
        case TRIANGULAR:
            return OpenVDS::InterpolationMethod::Triangular;
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
    if (error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }
    return new SingleDataHandle(std::move(handle));
}

SingleDataHandle::SingleDataHandle(OpenVDS::VDSHandle handle)
    : m_file_handle(handle)
    , m_access_manager(OpenVDS::GetAccessManager(handle))
    , m_metadata(m_access_manager.GetVolumeDataLayout())
{}

MetadataHandle const& SingleDataHandle::get_metadata() const noexcept (true) {
    return this->m_metadata;
}

OpenVDS::VolumeDataFormat SingleDataHandle::format() noexcept (true) {
    /*
     * We always want to request data in OpenVDS::VolumeDataFormat::Format_R32
     * format for slice. For fence documentation says: "The traces/samples are
     * always in 32-bit floating point format."
     */
    return OpenVDS::VolumeDataFormat::Format_R32;
}

std::int64_t SingleDataHandle::subcube_buffer_size(
    SubCube const& subcube
) noexcept(false) {
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
    void * const buffer,
    std::int64_t size,
    SubCube const& subcube
) noexcept(false) {
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

std::int64_t SingleDataHandle::traces_buffer_size(std::size_t const ntraces) noexcept (false) {
    int const dimension = this->get_metadata().sample().dimension();
    return this->m_access_manager.GetVolumeTracesBufferSize(ntraces, dimension);
}

void SingleDataHandle::read_traces(
    void * const                    buffer,
    std::int64_t const              size,
    voxel const*                    coordinates,
    std::size_t const               ntraces,
    enum interpolation_method const interpolation_method
) noexcept(false) {
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
) noexcept(false) {
    return this->m_access_manager.GetVolumeSamplesBufferSize(
        nsamples,
        SingleDataHandle::channel
    );
}

void SingleDataHandle::read_samples(
    void * const                    buffer,
    std::int64_t const              size,
    voxel const*                    samples,
    std::size_t const               nsamples,
    enum interpolation_method const interpolation_method
) noexcept(false) {
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

DoubleDataHandle* make_double_datahandle(
    const char* url_a,
    const char* credentials_a,
    const char* url_b,
    const char* credentials_b,
    binary_function binary_operator
) noexcept(false) {
    OpenVDS::Error error;
    auto handle_a = OpenVDS::Open(url_a, credentials_a, error);
    auto handle_b = OpenVDS::Open(url_b, credentials_b, error);
    if (error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }
    return new DoubleDataHandle(std::move(handle_a), std::move(handle_b), binary_operator);
}

DoubleDataHandle::DoubleDataHandle(OpenVDS::VDSHandle handle_a, OpenVDS::VDSHandle handle_b, binary_function binary_operator)
    : m_file_handle_a(handle_a)
    , m_file_handle_b(handle_b)
    , m_binary_operator(binary_operator)
    , m_access_manager_a(OpenVDS::GetAccessManager(handle_a))
    , m_access_manager_b(OpenVDS::GetAccessManager(handle_b))
    , m_metadata_a(m_access_manager_a.GetVolumeDataLayout())
    , m_metadata_b(m_access_manager_b.GetVolumeDataLayout())
    , m_layout(DoubleVolumeDataLayout(m_access_manager_a.GetVolumeDataLayout(), m_access_manager_b.GetVolumeDataLayout()))
    , m_metadata(DoubleMetadataHandle(&m_layout, &m_metadata_a, &m_metadata_b))
    {}

MetadataHandle const& DoubleDataHandle::get_metadata() const noexcept(true) {
    return this->m_metadata;
}

OpenVDS::VolumeDataFormat DoubleDataHandle::format() noexcept(true) {
    /*
     * We always want to request data in OpenVDS::VolumeDataFormat::Format_R32
     * format for slice. For fence documentation says: "The traces/samples are
     * always in 32-bit floating point format."
     */
    return OpenVDS::VolumeDataFormat::Format_R32;
}

std::int64_t DoubleDataHandle::subcube_buffer_size(
    SubCube const& subcube
) noexcept(false) {
    std::int64_t size = this->m_access_manager_a.GetVolumeSubsetBufferSize(
        subcube.bounds.lower,
        subcube.bounds.upper,
        DoubleDataHandle::format(),
        DoubleDataHandle::lod_level,
        DoubleDataHandle::channel
    );

    return size;
}

SubCube DoubleDataHandle::offset_bounds(SubCube subcube, SingleMetadataHandle metadata) {

    SubCube new_subcube = std::move(subcube);

    float iline_offset = (m_metadata.iline().min() - metadata.iline().min()) / m_metadata.iline().stepsize();
    float xline_offset = (m_metadata.xline().min() - metadata.xline().min()) / m_metadata.xline().stepsize();
    float sample_offset = (m_metadata.sample().min() - metadata.sample().min()) / m_metadata.sample().stepsize();

    new_subcube.bounds.lower[m_metadata.iline().dimension()] += iline_offset;
    new_subcube.bounds.lower[m_metadata.xline().dimension()] += xline_offset;
    new_subcube.bounds.lower[m_metadata.sample().dimension()] += sample_offset;

    new_subcube.bounds.upper[m_metadata.iline().dimension()] += iline_offset;
    new_subcube.bounds.upper[m_metadata.xline().dimension()] += xline_offset;
    new_subcube.bounds.upper[m_metadata.sample().dimension()] += sample_offset;
    return new_subcube;
}

void DoubleDataHandle::read_subcube(
    void* const buffer,
    std::int64_t size,
    SubCube const& subcube
) noexcept(false) {

    SubCube subcube_a = this->offset_bounds(subcube, m_metadata_a);
    std::vector<char> buffer_a(size);

    auto request_a = this->m_access_manager_a.RequestVolumeSubset(
        buffer,
        size,
        OpenVDS::Dimensions_012,
        DoubleDataHandle::lod_level,
        DoubleDataHandle::channel,
        subcube_a.bounds.lower,
        subcube_a.bounds.upper,
        DoubleDataHandle::format()
    );

    SubCube subcube_b = this->offset_bounds(subcube, m_metadata_b);
    std::vector<char> buffer_b(size);
    auto request_b = this->m_access_manager_b.RequestVolumeSubset(
        buffer_b.data(),
        size,
        OpenVDS::Dimensions_012,
        DoubleDataHandle::lod_level,
        DoubleDataHandle::channel,
        subcube_b.bounds.lower,
        subcube_b.bounds.upper,
        DoubleDataHandle::format()
    );

    bool const success_a = request_a.get()->WaitForCompletion();
    bool const success_b = request_b.get()->WaitForCompletion();

    if (!success_a && !success_b) {
        throw std::runtime_error("Failed to read from VDS.");
    }

    m_binary_operator((float*)buffer, (float* const)buffer_b.data(), (std::size_t)size / sizeof(float));
}

std::int64_t DoubleDataHandle::traces_buffer_size(std::size_t const ntraces) noexcept(false) {
    return this->get_metadata().sample().nsamples() * ntraces * sizeof(float);
}

void DoubleDataHandle::read_traces(
    void* const buffer,
    std::int64_t const size,
    voxel const* coordinates,
    std::size_t const ntraces,
    enum interpolation_method const interpolation_method
) noexcept(false) {
    int const dimension = this->get_metadata().sample().dimension();
    int const nsamples = this->get_metadata().sample().nsamples();
    int const buffersize = nsamples * ntraces;

    std::size_t coordinate_size = (std::size_t)(sizeof(voxel) * ntraces / sizeof(float));

    std::vector<float> coordinates_a(coordinate_size);
    std::vector<float> coordinates_b(coordinate_size);

    memcpy(coordinates_a.data(), coordinates[0], coordinate_size * sizeof(float));
    memcpy(coordinates_b.data(), coordinates[0], coordinate_size * sizeof(float));

    for (int i = 0; i < this->m_layout.Dimensionality_Max; i++) {
        for (int v = 0; v < ntraces; v++) {
            coordinates_a[v * this->m_layout.Dimensionality_Max + i] += this->m_layout.GetDimensionIndexOffset_a(i);
            coordinates_b[v * this->m_layout.Dimensionality_Max + i] += this->m_layout.GetDimensionIndexOffset_b(i);
        }
    }

    std::size_t size_a = this->m_access_manager_a.GetVolumeTracesBufferSize(ntraces, dimension);
    std::vector<float> buffer_a((std::size_t)size_a / sizeof(float));
    auto request_a = this->m_access_manager_a.RequestVolumeTraces(
        buffer_a.data(),
        size_a,
        OpenVDS::Dimensions_012,
        DoubleDataHandle::lod_level,
        DoubleDataHandle::channel,
        (voxel*)coordinates_a.data(),
        ntraces,
        ::to_interpolation(interpolation_method),
        dimension
    );

    std::size_t size_b = this->m_access_manager_b.GetVolumeTracesBufferSize(ntraces, dimension);
    std::vector<float> buffer_b((std::size_t)size_b / sizeof(float));
    auto request_b = this->m_access_manager_b.RequestVolumeTraces(
        buffer_b.data(),
        size_b,
        OpenVDS::Dimensions_012,
        DoubleDataHandle::lod_level,
        DoubleDataHandle::channel,
        (voxel*)coordinates_b.data(),
        ntraces,
        ::to_interpolation(interpolation_method),
        dimension
    );

    bool const success_a = request_a.get()->WaitForCompletion();
    bool const success_b = request_b.get()->WaitForCompletion();

    if (!success_a && !success_b) {
        throw std::runtime_error("Failed to read from VDS.");
    }

    float* floatBuffer = (float*)buffer;
    std::vector<float> res_buffer_b(buffersize);

    int counter = 0;
    int traceIndex_a = this->m_metadata_a.sample().nsamples();
    for (int i = 0; i < buffer_a.size(); i++) {
        int index = i % traceIndex_a;

        if (index >= coordinates_a[dimension] && index < coordinates_a[dimension] + nsamples) {
            floatBuffer[counter] = buffer_a[i];
            counter++;
        }
    }

    counter = 0;
    int traceIndex_b = this->m_metadata_b.sample().nsamples();
    for (int i = 0; i < buffer_b.size(); i++) {
        int index = i % traceIndex_b;

        if (index >= coordinates_b[dimension] && index < coordinates_b[dimension] + nsamples) {
            res_buffer_b[counter] = buffer_b[i];
            counter++;
        }
    }

    m_binary_operator((float*)buffer, (float* const)res_buffer_b.data(), (std::size_t)size / sizeof(float));
}

std::int64_t DoubleDataHandle::samples_buffer_size(
    std::size_t const nsamples
) noexcept(false) {
    return this->m_access_manager_a.GetVolumeSamplesBufferSize(
        nsamples,
        DoubleDataHandle::channel
    );
}

void DoubleDataHandle::read_samples(
    void* const buffer,
    std::int64_t const size,
    voxel const* samples,
    std::size_t const nsamples,
    enum interpolation_method const interpolation_method
) noexcept(false) {

    std::size_t samples_size = (std::size_t)(sizeof(voxel) * nsamples / sizeof(float));

    std::vector<float> samples_a((std::size_t)(sizeof(voxel) * nsamples));
    std::vector<float> samples_b((std::size_t)(sizeof(voxel) * nsamples));

    memcpy(samples_a.data(), samples[0], samples_size * sizeof(float));
    memcpy(samples_b.data(), samples[0], samples_size * sizeof(float));

    for (int i = 0; i < this->m_layout.Dimensionality_Max; i++) {
        for (int v = 0; v < nsamples; v++) {
            samples_a[v * this->m_layout.Dimensionality_Max + i] += this->m_layout.GetDimensionIndexOffset_a(i);
            samples_b[v * this->m_layout.Dimensionality_Max + i] += this->m_layout.GetDimensionIndexOffset_b(i);
        }
    }

    auto request_a = this->m_access_manager_a.RequestVolumeSamples(
        (float*)buffer,
        size,
        OpenVDS::Dimensions_012,
        DoubleDataHandle::lod_level,
        DoubleDataHandle::channel,
        (voxel*)samples_a.data(),
        nsamples,
        ::to_interpolation(interpolation_method)
    );

    std::vector<float> buffer_b((std::size_t)size / sizeof(float));

    auto request_b = this->m_access_manager_b.RequestVolumeSamples(
        buffer_b.data(),
        size,
        OpenVDS::Dimensions_012,
        DoubleDataHandle::lod_level,
        DoubleDataHandle::channel,
        (voxel*)samples_b.data(),
        nsamples,
        ::to_interpolation(interpolation_method)
    );

    bool const success_a = request_a.get()->WaitForCompletion();
    bool const success_b = request_b.get()->WaitForCompletion();

    if (!success_a && !success_b) {
        throw std::runtime_error("Failed to read from VDS.");
    }

    m_binary_operator((float*)buffer, (float* const)buffer_b.data(), (std::size_t)size / sizeof(float));
}
