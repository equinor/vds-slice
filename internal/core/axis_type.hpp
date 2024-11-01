#ifndef ONESEISMIC_API_AXIS_TYPE_HPP
#define ONESEISMIC_API_AXIS_TYPE_HPP

#include <string>

enum class AxisType {
    ILINE,
    XLINE,
    SAMPLE
};

std::string axis_type_to_string(AxisType axis_type) noexcept(false);

#endif /* ONESEISMIC_API_AXIS_TYPE_HPP */
