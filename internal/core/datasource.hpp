#ifndef VDS_SLICE_DATA_SOURCE_HPP
#define VDS_SLICE_DATA_SOURCE_HPP

#include "datahandle.hpp"
#include "metadatahandle.hpp"

using voxel = float[OpenVDS::Dimensionality_Max];
using binary_function = std::function<void(float *, const float *, std::size_t)>;

/// @brief Generic data source
class DataSource {

public:
    virtual ~DataSource() {}

    virtual MetadataHandle const &get_metadata() const noexcept(true) = 0;

    virtual std::int64_t samples_buffer_size(std::size_t const nsamples) noexcept(false) = 0;

    virtual void read_samples(
        void *const buffer,
        std::int64_t const size,
        voxel const *samples,
        std::size_t const nsamples,
        enum interpolation_method const interpolation_method) noexcept(false) = 0;

    virtual std::int64_t subcube_buffer_size(SubCube const &subcube) noexcept(false) = 0;

    virtual void read_subcube(
        void *const buffer,
        std::int64_t size,
        SubCube const &subcube) noexcept(false) = 0;

    virtual std::int64_t traces_buffer_size(std::size_t const ntraces) noexcept(false) = 0;

    virtual void read_traces(
        void *const buffer,
        std::int64_t const size,
        voxel const *coordinates,
        std::size_t const ntraces,
        enum interpolation_method const interpolation_method) noexcept(false) = 0;
};

class SingleDataSource : public DataSource {

public:
    SingleDataSource(const char *url, const char *credentials);

    ~SingleDataSource();

    MetadataHandle const &get_metadata() const noexcept(true);

    std::int64_t samples_buffer_size(std::size_t const nsamples) noexcept(false);

    void read_samples(
        void *const buffer,
        std::int64_t const size,
        voxel const *samples,
        std::size_t const nsamples,
        enum interpolation_method const interpolation_method) noexcept(false);

    std::int64_t subcube_buffer_size(SubCube const &subcube) noexcept(false);

    void read_subcube(
        void *const buffer,
        std::int64_t size,
        SubCube const &subcube) noexcept(false);

    std::int64_t traces_buffer_size(std::size_t const ntraces) noexcept(false);

    void read_traces(
        void *const buffer,
        std::int64_t const size,
        voxel const *coordinates,
        std::size_t const ntraces,
        enum interpolation_method const interpolation_method) noexcept(false);

private:
    DataHandle *handle;
};

SingleDataSource *make_single_datasource(
    const char *url,
    const char *credentials) noexcept(false);

class DoubleDataSource : public DataSource {

public:
    DoubleDataSource(
        const char *url_A,
        const char *credentials_A,
        const char *url_B,
        const char *credentials_B,
        binary_function binary_operator);

    ~DoubleDataSource();

    MetadataHandle const &get_metadata() const noexcept(true);

    std::int64_t samples_buffer_size(std::size_t const nsamples) noexcept(false);

    void read_samples(
        void *const buffer,
        std::int64_t const size,
        voxel const *samples,
        std::size_t const nsamples,
        enum interpolation_method const interpolation_method) noexcept(false);

    std::int64_t subcube_buffer_size(SubCube const &subcube) noexcept(false);

    void read_subcube(
        void *const buffer,
        std::int64_t size,
        SubCube const &subcube) noexcept(false);

    std::int64_t traces_buffer_size(std::size_t const ntraces) noexcept(false);

    void read_traces(
        void *const buffer,
        std::int64_t const size,
        voxel const *coordinates,
        std::size_t const ntraces,
        enum interpolation_method const interpolation_method) noexcept(false);

private:
    DataSource *handle_A;
    DataSource *handle_B;
    MetadataHandle *metadata;
    binary_function binary_operator;
};

DoubleDataSource *make_double_datasource(
    const char *url_A,
    const char *credentials_A,
    const char *url_B,
    const char *credentials_B,
    binary_function binary_operator);

void inplace_subtraction(
    float *buffer_A,
    const float *buffer_B,
    std::size_t nsamples) noexcept(true);

void inplace_addition(
    float *buffer_A,
    const float *buffer_B,
    std::size_t nsamples) noexcept(true);

void inplace_multiplication(
    float *buffer_A,
    const float *buffer_B,
    std::size_t nsamples) noexcept(true);

void inplace_division(
    float *buffer_A,
    const float *buffer_B,
    std::size_t nsamples) noexcept(true);

#endif /* VDS_SLICE_DATA_SOURCE_HPP */
