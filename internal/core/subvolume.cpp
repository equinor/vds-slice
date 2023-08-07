#include "subvolume.hpp"

#include <stdexcept>

#include "axis.hpp"
#include "metadatahandle.hpp"
#include "ctypes.h"

namespace {

int lineno_annotation_to_voxel(
    int lineno,
    Axis const& axis
) {
    float min    = axis.min();
    float max    = axis.max();
    float stride = axis.stride();

    float voxelline = (lineno - min) / stride;

    if (lineno < min || lineno > max || std::floor(voxelline) != voxelline) {
        throw std::runtime_error(
            "Invalid lineno: " + std::to_string(lineno) +
            ", valid range: [" + std::to_string(min) +
            ":" + std::to_string(max) +
            ":" + std::to_string(stride) + "]"
        );
    }

    return voxelline;
}

int lineno_index_to_voxel(
    int lineno,
    Axis const& axis
) {
    /* Line-numbers in IJK match Voxel - do bound checking and return*/
    int min = 0;
    int max = axis.nsamples() - 1;

    if (lineno < min || lineno > max) {
        throw std::runtime_error(
            "Invalid lineno: " + std::to_string(lineno) +
            ", valid range: [" + std::to_string(min) +
            ":" + std::to_string(max) +
            ":1]"
        );
    }

    return lineno;
}

int to_voxel(
    Axis const& axis,
    int const lineno,
    enum coordinate_system const system
) {
    switch (system) {
        case ANNOTATION: {
            return ::lineno_annotation_to_voxel(lineno, axis);
        }
        case INDEX: {
            return ::lineno_index_to_voxel(lineno, axis);
        }
        default: {
            throw std::runtime_error("Unhandled coordinate system");
        }
    }
}

} /* namespace */

SubVolume::SubVolume(MetadataHandle const& metadata) {
    auto const& iline  = metadata.iline();
    auto const& xline  = metadata.xline();
    auto const& sample = metadata.sample();

    this->bounds.upper[iline.dimension() ] = iline.nsamples();
    this->bounds.upper[xline.dimension() ] = xline.nsamples();
    this->bounds.upper[sample.dimension()] = sample.nsamples();
}

void SubVolume::set_slice(
    Axis const&                  axis,
    int const                    lineno,
    enum coordinate_system const coordinate_system
) {
    int voxelline = ::to_voxel(axis, lineno, coordinate_system);

    this->bounds.lower[axis.dimension()] = voxelline;
    this->bounds.upper[axis.dimension()] = voxelline + 1;
}
