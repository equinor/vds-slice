#include "seismicaxismapping.h"

#include<stdexcept>

PostStackAxisMap::PostStackAxisMap(int i, int x, int s) :
    inline_axis_id_(i), crossline_axis_id_(x), sample_axis_id_(s) {}

int PostStackAxisMap::iline() const noexcept (true) {
    return this->inline_axis_id_;
}

int PostStackAxisMap::xline() const noexcept (true) {
    return this->crossline_axis_id_;
}

int PostStackAxisMap::sample() const noexcept (true) {
    return this->sample_axis_id_;
}

int PostStackAxisMap::offset() const {
    throw std::runtime_error("Offsets are not supported in post stack data.");
}

int PostStackAxisMap::dimension_from( const int voxel ) const {
    switch (voxel) {
        case 0: return inline_axis_id_;
        case 1: return crossline_axis_id_;
        case 2: return sample_axis_id_;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}

int PostStackAxisMap::voxel_from( const int dimension ) const {
    // Mapping is involution, i.e., it is its own inversion
    return dimension_from( dimension );
}
