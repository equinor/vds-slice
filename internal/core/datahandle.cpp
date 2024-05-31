#include "datahandle.hpp"

#include <stdexcept>

#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/OpenVDS.h>

#include "exceptions.hpp"
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

OpenVDS::VolumeDataFormat DataHandle::format() noexcept(true) {
    /*
     * We always want to request data in OpenVDS::VolumeDataFormat::Format_R32
     * format for slice. For fence documentation says: "The traces/samples are
     * always in 32-bit floating point format."
     */
    return OpenVDS::VolumeDataFormat::Format_R32;
}

SingleDataHandle make_single_datahandle(
    const char* url,
    const char* credentials
) {
    OpenVDS::Error error;
    auto handle = OpenVDS::Open(url, credentials, error);
    if(error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }
    return SingleDataHandle(handle);
}

SingleDataHandle::SingleDataHandle(OpenVDS::VDSHandle handle)
    : m_file_handle(handle), m_access_manager(OpenVDS::GetAccessManager(handle)), m_metadata(m_access_manager.GetVolumeDataLayout()) {}

SingleMetadataHandle const& SingleDataHandle::get_metadata() const noexcept(true) {
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

DoubleDataHandle make_double_datahandle(
    const char* url_a,
    const char* credentials_a,
    const char* url_b,
    const char* credentials_b,
    enum binary_operator binary_symbol
) noexcept(false) {

    OpenVDS::Error error_a;
    auto handle_a = OpenVDS::Open(url_a, credentials_a, error_a);
    if (error_a.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error_a.string);
    }
    OpenVDS::Error error_b;
    auto handle_b = OpenVDS::Open(url_b, credentials_b, error_b);
    if (error_b.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error_b.string);
    }
    return DoubleDataHandle(std::move(handle_a), std::move(handle_b), binary_symbol);
}

DoubleDataHandle::DoubleDataHandle(OpenVDS::VDSHandle handle_a, OpenVDS::VDSHandle handle_b, enum binary_operator binary_symbol)
    : m_file_handle_a(handle_a),
      m_file_handle_b(handle_b),
      m_access_manager_a(OpenVDS::GetAccessManager(handle_a)),
      m_access_manager_b(OpenVDS::GetAccessManager(handle_b)),
      m_metadata_a(m_access_manager_a.GetVolumeDataLayout()),
      m_metadata_b(m_access_manager_b.GetVolumeDataLayout()),
      m_metadata(&m_metadata_a, &m_metadata_b, binary_symbol) {

    if (binary_symbol == NO_OPERATOR)
        throw detail::bad_request("Invalid function");
    else if (binary_symbol == ADDITION)
        this->m_binary_operator = &inplace_addition;
    else if (binary_symbol == SUBTRACTION)
        this->m_binary_operator = &inplace_subtraction;
    else if (binary_symbol == MULTIPLICATION)
        this->m_binary_operator = &inplace_multiplication;
    else if (binary_symbol == DIVISION)
        this->m_binary_operator = &inplace_division;
    else
        throw detail::bad_request("Invalid binary_operator string");
}

DoubleMetadataHandle const& DoubleDataHandle::get_metadata() const noexcept(true) {
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

/// @brief Copy and shift the coordinates in the provided subcube to describe it in cube X coordinates.
/// @param subcube_intersect_AB Subcube is defined by (A ∩ B) for cubes A and B. Only the first three axis are currently used.
/// @param metadata_cube_X Metadata for cube X where X ∈ {A, B}.
/// @return Subcube (A ∩ B) defined in the coordinates of cube X.
SubCube DoubleDataHandle::offset_bounds(const SubCube subcube_intersect_AB, SingleMetadataHandle metadata_cube_X) {

    // Create a copy
    SubCube subcube_in_X_coordinates = std::move(subcube_intersect_AB);

    // Calculate the number of steps to offset the subcube in the three dimensions.
    float iline_offset = (m_metadata.iline().min() - metadata_cube_X.iline().min()) / m_metadata.iline().stepsize();
    float xline_offset = (m_metadata.xline().min() - metadata_cube_X.xline().min()) / m_metadata.xline().stepsize();
    float sample_offset = (m_metadata.sample().min() - metadata_cube_X.sample().min()) / m_metadata.sample().stepsize();

    subcube_in_X_coordinates.bounds.lower[m_metadata.iline().dimension()] += iline_offset;
    subcube_in_X_coordinates.bounds.lower[m_metadata.xline().dimension()] += xline_offset;
    subcube_in_X_coordinates.bounds.lower[m_metadata.sample().dimension()] += sample_offset;

    subcube_in_X_coordinates.bounds.upper[m_metadata.iline().dimension()] += iline_offset;
    subcube_in_X_coordinates.bounds.upper[m_metadata.xline().dimension()] += xline_offset;
    subcube_in_X_coordinates.bounds.upper[m_metadata.sample().dimension()] += sample_offset;
    return subcube_in_X_coordinates;
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

    if (!success_a || !success_b) {
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
    int const sample_dimension_index = this->get_metadata().sample().dimension();

    std::size_t coordinates_buffer_size = OpenVDS::Dimensionality_Max * ntraces;
    std::vector<float> coordinates_a(coordinates_buffer_size);
    auto transformer_a = this->m_metadata.coordinate_transformer();
    for (int v = 0; v < ntraces; v++) {
        transformer_a.to_cube_a_ijk_position(coordinates_a.data() + OpenVDS::Dimensionality_Max * v, coordinates[v]);
    }

    std::vector<float> coordinates_b(coordinates_buffer_size);
    auto transformer_b = this->m_metadata.coordinate_transformer();
    for (int v = 0; v < ntraces; v++) {
        transformer_b.to_cube_b_ijk_position(coordinates_b.data() + OpenVDS::Dimensionality_Max * v, coordinates[v]);
    }

    std::size_t size_a = this->m_access_manager_a.GetVolumeTracesBufferSize(ntraces, sample_dimension_index);
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
        sample_dimension_index
    );

    std::size_t size_b = this->m_access_manager_b.GetVolumeTracesBufferSize(ntraces, sample_dimension_index);
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
        sample_dimension_index
    );

    bool const success_a = request_a.get()->WaitForCompletion();
    bool const success_b = request_b.get()->WaitForCompletion();

    if (!success_a || !success_b) {
        throw std::runtime_error("Failed to read from VDS.");
    }

    // Function read_traces extracts whole traces out of corresponding files.
    // However it could happen that data files are not fully aligned in their sample dimensions.
    // That creates a need to extract from each trace data that make up the intersection.
    float* floatBuffer = (float*)buffer;
    this->extract_continuous_part_of_trace(
        &buffer_a,
        this->m_metadata_a.sample().nsamples(),
        (long)(coordinates_a[sample_dimension_index] + 0.5f),
        this->get_metadata().sample().nsamples(),
        floatBuffer);

    std::vector<float> res_buffer_b(this->get_metadata().sample().nsamples() * ntraces);
    this->extract_continuous_part_of_trace(
        &buffer_b,
        this->m_metadata_b.sample().nsamples(),
        (long)(coordinates_b[sample_dimension_index] + 0.5f),
        this->get_metadata().sample().nsamples(),
        res_buffer_b.data());

    m_binary_operator((float*)buffer, (float* const)res_buffer_b.data(), (std::size_t)size / sizeof(float));
}

void DoubleDataHandle::extract_continuous_part_of_trace(
    std::vector<float>* source_traces,
    int source_trace_length,
    long start_extract_index,
    int nsamples_to_extract,
    float* target_buffer
) {
    int ntraces = source_traces->size() / source_trace_length;
    for (int i = 0; i < ntraces; ++i) {
        float* src_trace = source_traces->data() + i * source_trace_length;
        float* dst_trace = target_buffer + i * nsamples_to_extract;
        std::memcpy(dst_trace, src_trace + start_extract_index, nsamples_to_extract * sizeof(float));
    }
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

    std::size_t samples_buffer_size = OpenVDS::Dimensionality_Max * nsamples;

    /* Note that samples contain sample positions, yet we handle them here as
     * ijk positions. That shouldn't be a problem as sample positions should
     * differ from ijk positions just by half a sample.
     */

    std::vector<float> samples_a(samples_buffer_size);
    auto transformer_a = this->m_metadata.coordinate_transformer();
    for (int v = 0; v < nsamples; v++) {
        transformer_a.to_cube_a_ijk_position(samples_a.data() + OpenVDS::Dimensionality_Max * v, samples[v]);
    }

    std::vector<float> samples_b(samples_buffer_size);
    auto transformer_b = this->m_metadata.coordinate_transformer();
    for (int v = 0; v < nsamples; v++) {
        transformer_b.to_cube_b_ijk_position(samples_b.data() + OpenVDS::Dimensionality_Max * v, samples[v]);
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

    if (!success_a || !success_b) {
        throw std::runtime_error("Failed to read from VDS.");
    }

    m_binary_operator((float*)buffer, (float* const)buffer_b.data(), (std::size_t)size / sizeof(float));
}

void inplace_subtraction(float* buffer_A, const float* buffer_B, std::size_t nsamples) noexcept(true) {
    for (std::size_t i = 0; i < nsamples; i++) {
        buffer_A[i] -= buffer_B[i];
    }
}

void inplace_addition(float* buffer_A, const float* buffer_B, std::size_t nsamples) noexcept(true) {
    for (std::size_t i = 0; i < nsamples; i++) {
        buffer_A[i] += buffer_B[i];
    }
}

void inplace_multiplication(float* buffer_A, const float* buffer_B, std::size_t nsamples) noexcept(true) {
    for (std::size_t i = 0; i < nsamples; i++) {
        buffer_A[i] *= buffer_B[i];
    }
}

void inplace_division(float* buffer_A, const float* buffer_B, std::size_t nsamples) noexcept(true) {
    for (std::size_t i = 0; i < nsamples; i++) {
        buffer_A[i] /= buffer_B[i];
    }
}
