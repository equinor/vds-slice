#ifndef VDS_SLICE_DIRECTION_HPP
#define VDS_SLICE_DIRECTION_HPP

#include <string>

#include "ctypes.h"
#include "axis_type.hpp"

class Direction {
public:
    explicit Direction(enum axis_name const axis_name) : m_axis_name(axis_name) {}

    enum coordinate_system coordinate_system() const noexcept (false);
    std::string            to_string()         const noexcept (false);
    enum axis_name         name()              const noexcept (true);

    AxisType axis_type() const noexcept(false);

    bool is_iline()  const noexcept (false);
    bool is_xline()  const noexcept (false);
    bool is_sample() const noexcept (false);
private:
    enum axis_name const m_axis_name;
};


#endif /* VDS_SLICE_DIRECTION_HPP */
