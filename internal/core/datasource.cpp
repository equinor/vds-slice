#include "exceptions.hpp"
#include "datasource.hpp"
#include "datahandle.hpp"

SingleDataSource::SingleDataSource(const char* url, const char* credentials) {
    this->handle = make_single_datahandle(url, credentials);
}

SingleDataSource::~SingleDataSource() {
    if (this->handle != NULL) {
        delete (this->handle);
        this->handle = NULL;
    }
}

MetadataHandle const& SingleDataSource::get_metadata() const noexcept(true) {
    return this->handle->get_metadata();
}

std::int64_t SingleDataSource::samples_buffer_size(std::size_t const nsamples) noexcept(false) {
    return this->handle->samples_buffer_size(nsamples);
}

void SingleDataSource::read_samples(
    void* const buffer,
    std::int64_t const size,
    voxel const* samples,
    std::size_t const nsamples,
    enum interpolation_method const interpolation_method
) noexcept(false) {
    return this->handle->read_samples(buffer, size, samples, nsamples, interpolation_method);
}

std::int64_t SingleDataSource::subcube_buffer_size(SubCube const& subcube) noexcept(false) {
    return this->handle->subcube_buffer_size(subcube);
}

void SingleDataSource::read_subcube(
    void* const buffer,
    std::int64_t size,
    SubCube const& subcube
) noexcept(false) {
    this->handle->read_subcube(buffer, size, subcube);
}

std::int64_t SingleDataSource::traces_buffer_size(std::size_t const ntraces) noexcept(false) {
    return this->handle->traces_buffer_size(ntraces);
}

void SingleDataSource::read_traces(
    void* const buffer,
    std::int64_t const size,
    voxel const* coordinates,
    std::size_t const ntraces,
    enum interpolation_method const interpolation_method
) noexcept(false) {

    this->handle->read_traces(buffer, size, coordinates, ntraces, interpolation_method);
}

SingleDataSource* make_single_datasource(
    const char* url,
    const char* credentials
) {
    return new SingleDataSource(url, credentials);
}

DoubleDataSource::DoubleDataSource(
    const char* url_A, const char* credentials_A,
    const char* url_B, const char* credentials_B,
    binary_function binary_operator
) {
    this->handle_A = make_single_datasource(url_A, credentials_A);
    this->handle_B = make_single_datasource(url_B, credentials_B);
    this->binary_operator = binary_operator;
    this->metadata = new DoubleMetadataHandle(
        this->handle_A->get_metadata(),
        this->handle_B->get_metadata()
    );
}

DoubleDataSource::~DoubleDataSource() {
    if (this->handle_A != NULL) {
        delete (this->handle_A);
        this->handle_A = NULL;
    }
    if (this->handle_B != NULL) {
        delete (this->handle_B);
        this->handle_B = NULL;
    }
}

MetadataHandle const& DoubleDataSource::get_metadata() const noexcept(true) {
    return this->handle_A->get_metadata();
}

std::int64_t DoubleDataSource::samples_buffer_size(std::size_t const nsamples) noexcept(false) {
    // Be aware that asking both sources may cost some time
    std::int64_t size_a = this->handle_A->samples_buffer_size(nsamples);
    std::int64_t size_b = this->handle_B->samples_buffer_size(nsamples);
    if (size_a != size_b) {
        throw detail::bad_request("Mismatch in sample buffer size");
    }
    return size_a;
}

void DoubleDataSource::read_samples(
    void* const buffer,
    std::int64_t const size,
    voxel const* samples,
    std::size_t const nsamples,
    interpolation_method const interpolation_method
) noexcept(false) {

    std::vector<float> buffer_B(nsamples);

    this->handle_A->read_samples((float*)buffer, size, samples, nsamples, interpolation_method);
    this->handle_B->read_samples(buffer_B.data(), size, samples, nsamples, interpolation_method);

    this->binary_operator((float*)buffer, buffer_B.data(), nsamples);
}

std::int64_t DoubleDataSource::subcube_buffer_size(SubCube const& subcube) noexcept(false) {
    // Be aware that asking both sources may cost some time
    std::int64_t size_a = this->handle_A->subcube_buffer_size(subcube);
    std::int64_t size_b = this->handle_B->subcube_buffer_size(subcube);
    if (size_a != size_b) {
        throw detail::bad_request("Mismatch in subcube buffer size");
    }
    return size_a;
}

void DoubleDataSource::read_subcube(
    void* const buffer,
    std::int64_t size,
    SubCube const& subcube
) noexcept(false) {
    std::vector<float> buffer_B((int)size / sizeof(float));
    this->handle_A->read_subcube((float*)buffer, size, subcube);
    this->handle_B->read_subcube(buffer_B.data(), size, subcube);

    this->binary_operator((float*)buffer, buffer_B.data(), (int)size / sizeof(float));
}

std::int64_t DoubleDataSource::traces_buffer_size(
    std::size_t const ntraces
) noexcept(false) {
    // Be aware that asking both sources may cost some time
    std::int64_t size_a = this->handle_A->traces_buffer_size(ntraces);
    std::int64_t size_b = this->handle_B->traces_buffer_size(ntraces);
    if (size_a != size_b) {
        throw detail::bad_request("Mismatch in trace buffer size");
    }
    return size_a;
}

void DoubleDataSource::read_traces(
    void* const buffer,
    std::int64_t const size,
    voxel const* coordinates,
    std::size_t const ntraces,
    interpolation_method const interpolation_method
) noexcept(false) {

    std::vector<float> buffer_B((int)size / sizeof(float));

    this->handle_A->read_traces((float*)buffer, size, coordinates, ntraces, interpolation_method);
    this->handle_B->read_traces(buffer_B.data(), size, coordinates, ntraces, interpolation_method);

    this->binary_operator((float*)buffer, buffer_B.data(), (int)size / sizeof(float));
}



DoubleDataSource* make_double_datasource(
    const char* url_A,
    const char* credentials_A,
    const char* url_B,
    const char* credentials_B,
    binary_function binary_operator
) noexcept(false) {
    return new DoubleDataSource(url_A, credentials_A, url_B, credentials_B, binary_operator);
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
