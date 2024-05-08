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
    this->binary_operator = binary_operator;
    this->handle = make_double_datahandle(url_A, credentials_A, url_B, credentials_B, binary_operator);
    this->metadata = &(this->handle->get_metadata());
}

DoubleDataSource::~DoubleDataSource() {
    if (this->handle != NULL) {
        delete (this->handle);
        this->handle = NULL;
    }
}

MetadataHandle const& DoubleDataSource::get_metadata() const noexcept(true) {
    return (MetadataHandle const&)*(this->metadata);
}

std::int64_t DoubleDataSource::samples_buffer_size(std::size_t const nsamples) noexcept(false) {
    return this->handle->samples_buffer_size(nsamples);
}

void DoubleDataSource::read_samples(
    void* const buffer,
    std::int64_t const size,
    voxel const* samples,
    std::size_t const nsamples,
    interpolation_method const interpolation_method
) noexcept(false) {
    return this->handle->read_samples(buffer, size, samples, nsamples, interpolation_method);
}

std::int64_t DoubleDataSource::subcube_buffer_size(SubCube const& subcube) noexcept(false) {
    return this->handle->subcube_buffer_size(subcube);
}

void DoubleDataSource::read_subcube(
    void* const buffer,
    std::int64_t size,
    SubCube const& subcube
) noexcept(false) {
    this->handle->read_subcube((float*)buffer, size, subcube);
}

std::int64_t DoubleDataSource::traces_buffer_size(
    std::size_t const ntraces
) noexcept(false) {
    return this->handle->traces_buffer_size(ntraces);
}

void DoubleDataSource::read_traces(
    void* const buffer,
    std::int64_t const size,
    voxel const* coordinates,
    std::size_t const ntraces,
    interpolation_method const interpolation_method
) noexcept(false) {
    this->handle->read_traces((float*)buffer, size, coordinates, ntraces, interpolation_method);
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
