#ifndef VDS_SLICE_DATA_SOURCE_HPP
#define VDS_SLICE_DATA_SOURCE_HPP

#include "datahandle.hpp"
#include "metadatahandle.hpp"

using voxel = float[OpenVDS::Dimensionality_Max];

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


#endif /* VDS_SLICE_DATA_SOURCE_HPP */
