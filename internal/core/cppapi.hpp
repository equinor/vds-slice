#ifndef VDS_SLICE_CPPAPI_HPP
#define VDS_SLICE_CPPAPI_HPP

#include <vector>

#include "ctypes.h"

#include "attribute.hpp"
#include "datahandle.hpp"
#include "datasource.hpp"
#include "direction.hpp"
#include "regularsurface.hpp"
#include "subvolume.hpp"

namespace cppapi {

void slice(
    DataSource& datasource,
    Direction const direction,
    int lineno,
    std::vector< Bound > const& bounds,
    response* out
) noexcept (false);

void fence(
    DataSource& datasource,
    enum coordinate_system coordinate_system,
    const float* coordinates,
    size_t npoints,
    enum interpolation_method interpolation_method,
    const float* fillValue,
    response* out
) noexcept (false);

void fetch_subvolume(
    DataSource& datasource,
    SurfaceBoundedSubVolume& subvolume,
    enum interpolation_method interpolation,
    std::size_t from,
    std::size_t to
) noexcept (false);

void attributes(
    SurfaceBoundedSubVolume const& src_subvolume,
    ResampledSegmentBlueprint const* dst_segment_blueprint,
    enum attribute* attributes,
    std::size_t nattributes,
    std::size_t from,
    std::size_t to,
    void** out
) noexcept (false);

/**
 * Given two input surfaces, primary and secondary, updates third surface,
 * aligned, which is expected to be shaped as primary surface, with data
 * belonging to secondary surface.
 *
 * For each point on primary surface a nearest point on the secondary surface
 * will be found and its value will be written to the resulting aligned surface.
 *
 * If according to the algorithm described above surfaces appear to intersect,
 * exception would be thrown.
 *
 * If the resulting point appears to be out of secondary surface bounds, aligned
 * surface fillvalue will be stored at the position. If for the primary or
 * secondary surface at the point the value of the data is surface's
 * corresponding fillvalue, aligned surface fillvalue will be stored at the
 * postion.
 *
 * Additionally a parameter primary_is_top would be set determining whether
 * primary or resulting aligned surface appeared on top of another.
 */
void align_surfaces(
    RegularSurface const& primary,
    RegularSurface const& secondary,
    RegularSurface &aligned,
    bool* primary_is_top
) noexcept (false);

void slice_metadata(
    DataSource& datasource,
    Direction const direction,
    int lineno,
    std::vector< Bound > const& bounds,
    response* out
) noexcept (false);


void fence_metadata(
    DataSource& datasource,
    size_t npoints,
    response* out
) noexcept (false);

void metadata(
    DataSource& datasource,
    response* out
) noexcept (false);

void attributes_metadata(
    DataSource& datasource,
    std::size_t nrows,
    std::size_t ncols,
    response* out
) noexcept (false);

} // namespace cppapi

#endif // VDS_SLICE_CPPAPI_HPP
