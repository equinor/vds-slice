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

