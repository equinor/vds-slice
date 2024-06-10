#include "axis_type.hpp"

#include <stdexcept>

std::string axis_type_to_string(AxisType axis_type) noexcept(false) {
    switch (axis_type) {
        case AxisType::ILINE:
            return "inline";
        case AxisType::XLINE:
            return "xline";
        case AxisType::SAMPLE:
            return "sample";

        default:
            throw std::runtime_error("Unhandled axis type");
    }
}
