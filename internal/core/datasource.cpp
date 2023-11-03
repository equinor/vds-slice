#include "datasource.hpp"
#include "datahandle.hpp"

OvdsDataSource::OvdsDataSource(const char *url, const char *credentials) {
    this->handle = make_datahandle(url, credentials);
}

OvdsDataSource::~OvdsDataSource() {
    if (this->handle != NULL) {
        delete (this->handle);
        this->handle = NULL;
    }
}

int OvdsDataSource::validate_metadata() const noexcept(false) {
    return STATUS_DATASOURCE_OK;
}

MetadataHandle const &OvdsDataSource::get_metadata() const noexcept(true) {
    return this->handle->get_metadata();
}

std::int64_t OvdsDataSource::samples_buffer_size(std::size_t const nsamples) noexcept(false) {
    return this->handle->samples_buffer_size(nsamples);
}

void OvdsDataSource::read_samples(
    void *const buffer,
    std::int64_t const size,
    voxel const *samples,
    std::size_t const nsamples,
    enum interpolation_method const interpolation_method) noexcept(false) {
    return this->handle->read_samples(buffer, size, samples, nsamples, interpolation_method);
}

std::int64_t OvdsDataSource::subcube_buffer_size(SubCube const &subcube) noexcept(false) {
    return this->handle->subcube_buffer_size(subcube);
}

void OvdsDataSource::read_subcube(
    void *const buffer,
    std::int64_t size,
    SubCube const &subcube) noexcept(false) {
    this->handle->read_subcube(buffer, size, subcube);
}

void OvdsDataSource::read_traces(
    void *const buffer,
    std::int64_t const size,
    voxel const *coordinates,
    std::size_t const ntraces,
    enum interpolation_method const interpolation_method) noexcept(false) {

    this->handle->read_traces(buffer, size, coordinates, ntraces, interpolation_method);
}

std::int64_t OvdsDataSource::traces_buffer_size(std::size_t const ntraces) noexcept(false) {
    return this->handle->traces_buffer_size(ntraces);
}

OvdsDataSource *make_ovds_datasource(
    const char *url,
    const char *credentials) {
    return new OvdsDataSource(url, credentials);
}

OvdsMultiDataSource::OvdsMultiDataSource(
    const char *url_A, const char *credentials_A,
    const char *url_B, const char *credentials_B,
    void (*func)(float *, float *, float *, std::size_t)) {
    this->handle_A = make_datahandle(url_A, credentials_A);
    this->handle_B = make_datahandle(url_B, credentials_B);
    this->func = func;
    this->validate_metadata();
}

OvdsMultiDataSource::~OvdsMultiDataSource() {
    if (this->handle_A != NULL) {
        delete (this->handle_A);
        this->handle_A = NULL;
    }
    if (this->handle_B != NULL) {
        delete (this->handle_B);
        this->handle_B = NULL;
    }
}

int OvdsMultiDataSource::validate_metadata() const noexcept(false) {

    MetadataHandle metadata_handle_A = this->handle_A->get_metadata();
    MetadataHandle metadata_handle_B = this->handle_B->get_metadata();

    if (metadata_handle_A.iline() != metadata_handle_B.iline())
        return STATUS_DATASOURCE_ILINE_MISMATCH;

    if (metadata_handle_A.xline() != metadata_handle_B.xline())
        return STATUS_DATASOURCE_XLINE_MISMATCH;

    if (metadata_handle_A.sample() != metadata_handle_B.sample())
        return STATUS_DATASOURCE_SAMPLE_MISMATCH;

    return STATUS_DATASOURCE_OK;
}

MetadataHandle const &OvdsMultiDataSource::get_metadata() const noexcept(true) {
    // Returns an arbitrary metadata handle
    return this->handle_A->get_metadata();
}

std::int64_t OvdsMultiDataSource::samples_buffer_size(std::size_t const nsamples) noexcept(false) {
    return this->handle_A->samples_buffer_size(nsamples);
}

std::int64_t OvdsMultiDataSource::subcube_buffer_size(SubCube const &subcube) noexcept(false) {
    return this->handle_A->subcube_buffer_size(subcube);
}

void OvdsMultiDataSource::read_subcube(
    void *const buffer,
    std::int64_t size,
    SubCube const &subcube) noexcept(false) {
    std::vector<float> buffer_A(subcube.size());
    std::vector<float> buffer_B(subcube.size());
    this->handle_A->read_subcube(buffer_A.data(), size, subcube);
    this->handle_B->read_subcube(buffer_B.data(), size, subcube);

    this->func(buffer_A.data(), buffer_B.data(), (float *)buffer, size);
}

std::int64_t OvdsMultiDataSource::traces_buffer_size(
    std::size_t const ntraces) noexcept(false) {
    return this->handle_A->traces_buffer_size(ntraces);
}

/// @brief Returns apply the function to trace from source A and B
/// @param buffer to float matrix of trace_length x ntraces
/// @param size Number of bytes in buffer
/// @param coordinates
/// @param ntraces
/// @param interpolation_method
void OvdsMultiDataSource::read_traces(
    void *const buffer,
    std::int64_t const size,
    voxel const *coordinates,
    std::size_t const ntraces,
    interpolation_method const interpolation_method) noexcept(false) {

    int const trace_length = this->get_metadata().sample().dimension();

    std::vector<float> buffer_A(trace_length * ntraces);
    std::vector<float> buffer_B(trace_length * ntraces);

    this->handle_A->read_samples(buffer_A.data(), size, coordinates, ntraces, interpolation_method);
    this->handle_B->read_samples(buffer_B.data(), size, coordinates, ntraces, interpolation_method);

    this->func(buffer_A.data(), buffer_B.data(), (float *)buffer, trace_length * ntraces);
}

void OvdsMultiDataSource::read_samples(
    void *const buffer,
    std::int64_t const size,
    voxel const *samples,
    std::size_t const nsamples,
    interpolation_method const interpolation_method) noexcept(false) {

    std::vector<float> buffer_A(nsamples);
    std::vector<float> buffer_B(nsamples);

    this->handle_A->read_samples(buffer_A.data(), size, samples, nsamples, interpolation_method);
    this->handle_B->read_samples(buffer_B.data(), size, samples, nsamples, interpolation_method);

    this->func(buffer_A.data(), buffer_B.data(), (float *)buffer, nsamples);
}

OvdsMultiDataSource *make_ovds_multi_datasource(
    const char *url_A,
    const char *credentials_A,
    const char *url_B,
    const char *credentials_B,
    void (*func)(float *, float *, float *, std::size_t)) noexcept(false) {
    return new OvdsMultiDataSource(url_A, credentials_A, url_B, credentials_B, func);
}

void subtraction(float *buffer_A, float *buffer_B, float *out_buffer, std::size_t nsamples) noexcept(false) {
    for (std::size_t i = 0; i < nsamples; i++) {
        out_buffer[i] = buffer_A[i] - buffer_B[i];
    }
}

void addition(float *buffer_A, float *buffer_B, float *out_buffer, std::size_t nsamples) noexcept(false) {
    for (std::size_t i = 0; i < nsamples; i++) {
        out_buffer[i] = buffer_A[i] + buffer_B[i];
    }
}

void multiplication(float *buffer_A, float *buffer_B, float *out_buffer, std::size_t nsamples) noexcept(false) {
    for (std::size_t i = 0; i < nsamples; i++) {
        out_buffer[i] = buffer_A[i] * buffer_B[i];
    }
}

void division(float *buffer_A, float *buffer_B, float *out_buffer, std::size_t nsamples) noexcept(false) {
    for (std::size_t i = 0; i < nsamples; i++) {
        out_buffer[i] = buffer_A[i] / buffer_B[i];
    }
}
