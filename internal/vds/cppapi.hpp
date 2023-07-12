#ifndef VDS_SLICE_CPPAPI_HPP
#define VDS_SLICE_CPPAPI_HPP

#include "ctypes.h"

#include "attribute.hpp"
#include "datahandle.hpp"
#include "direction.hpp"
#include "regularsurface.hpp"

namespace cppapi {

void slice(
    DataHandle& handle,
    Direction const direction,
    int lineno,
    response* out
) noexcept (false);

void fence(
    DataHandle& handle,
    enum coordinate_system coordinate_system,
    const float* coordinates,
    size_t npoints,
    enum interpolation_method interpolation_method,
    response* out
) noexcept (false);

void horizon(
    DataHandle& handle,
    RegularSurface const& surface,
    float above,
    float below,
    enum interpolation_method interpolation,
    response* out
) noexcept (false);

void attributes(
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

void slice_metadata(
    DataHandle& handle,
    Direction const direction,
    int lineno,
    response* out
) noexcept (false);


void fence_metadata(
    DataHandle& handle,
    size_t npoints,
    response* out
) noexcept (false);

void metadata(
    DataHandle& handle,
    response* out
) noexcept (false);

void attributes_metadata(
    DataHandle& handle,
    std::size_t nrows,
    std::size_t ncols,
    response* out
) noexcept (false);

} // namespace cppapi

#endif // VDS_SLICE_CPPAPI_HPP
