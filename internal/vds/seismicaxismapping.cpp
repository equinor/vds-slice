#include "seismicaxismapping.h"

#include<stdexcept>

PostStackAxisMap::PostStackAxisMap(int i, int x, int s) :
    inline_axis_id_(i), crossline_axis_id_(x), sample_axis_id_(s) {}

int PostStackAxisMap::iline()  const {
    return this->inline_axis_id_;
}

int PostStackAxisMap::xline()  const {
    return this->crossline_axis_id_;
}

int PostStackAxisMap::sample() const {
    return this->sample_axis_id_;
}

int PostStackAxisMap::offset() const {
    throw std::runtime_error("Offsets are not supported in post stack data.");
}
