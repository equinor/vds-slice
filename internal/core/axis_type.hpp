#ifndef VDS_SLICE_AXIS_TYPE_HPP
#define VDS_SLICE_AXIS_TYPE_HPP

#include <string>

enum class AxisType {
    ILINE,
    XLINE,
    SAMPLE
};

std::string axis_type_to_string(AxisType axis_type) noexcept(false);

#endif /* VDS_SLICE_AXIS_TYPE_HPP */
