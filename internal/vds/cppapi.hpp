#ifndef VDS_SLICE_CPPAPI_HPP
#define VDS_SLICE_CPPAPI_HPP

#include "vds.h"

#include "attribute.hpp"
#include "datahandle.hpp"
#include "direction.hpp"
#include "regularsurface.hpp"

void fetch_slice(
    DataHandle& handle,
    Direction const direction,
    int lineno,
    response* out
) noexcept (false);

void fetch_fence(
    DataHandle& handle,
    enum coordinate_system coordinate_system,
    const float* coordinates,
    size_t npoints,
    enum interpolation_method interpolation_method,
    response* out
) noexcept (false);

void fetch_horizon(
    DataHandle& handle,
    RegularSurface const& surface,
    float above,
    float below,
    enum interpolation_method interpolation,
    response* out
) noexcept (false);

void calculate_attribute(
    DataHandle& handle,
    Horizon const& horizon,
    RegularSurface const& surface,
    VerticalWindow const& src_window,
    VerticalWindow const& dst_window,
    enum attribute* attributes,
    std::size_t nattributes,
    std::size_t from,
    std::size_t to,
    void** out
) noexcept (false);

void fetch_slice_metadata(
    DataHandle& handle,
    Direction const direction,
    int lineno,
    response* out
) noexcept (false);


void fetch_fence_metadata(
    DataHandle& handle,
    size_t npoints,
    response* out
) noexcept (false);

void metadata(
    DataHandle& handle,
    response* out
) noexcept (false);

void fetch_attribute_metadata(
    DataHandle& handle,
    std::size_t nrows,
    std::size_t ncols,
    response* out
) noexcept (false);

#endif // VDS_SLICE_CPPAPI_HPP
