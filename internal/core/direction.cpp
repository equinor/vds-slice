#include "direction.hpp"
#include "axis_type.hpp"

#include <stdexcept>

#include <OpenVDS/KnownMetadata.h>

#include "ctypes.h"

enum coordinate_system Direction::coordinate_system() const noexcept(false) {
    switch (this->name()) {
        case I:
        case J:
        case K:
            return coordinate_system::INDEX;
        case INLINE:
        case CROSSLINE:
        case DEPTH:
        case TIME:
        case SAMPLE:
            return coordinate_system::ANNOTATION;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}

std::string Direction::to_string() const noexcept(false) {
    switch (this->name()) {
        case I:         return std::string( OpenVDS::KnownAxisNames::I()         );
        case J:         return std::string( OpenVDS::KnownAxisNames::J()         );
        case K:         return std::string( OpenVDS::KnownAxisNames::K()         );
        case INLINE:    return std::string( OpenVDS::KnownAxisNames::Inline()    );
        case CROSSLINE: return std::string( OpenVDS::KnownAxisNames::Crossline() );
        case DEPTH:     return std::string( OpenVDS::KnownAxisNames::Depth()     );
        case TIME:      return std::string( OpenVDS::KnownAxisNames::Time()      );
        case SAMPLE:    return std::string( OpenVDS::KnownAxisNames::Sample()    );
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}

enum axis_name Direction::name() const noexcept(true) {
    return this->m_axis_name;
}

AxisType Direction::axis_type() const noexcept(false) {
    switch (this->name()) {
        case I:
        case INLINE:
            return AxisType::ILINE;
        case J:
        case CROSSLINE:
            return AxisType::XLINE;
        case K:
        case DEPTH:
        case TIME:
        case SAMPLE:
            return AxisType::SAMPLE;
        default:
            throw std::runtime_error("Unhandled axis");
    }
}

bool Direction::is_iline() const noexcept(false) {
    return this->axis_type() == AxisType::ILINE;
}

bool Direction::is_xline() const noexcept(false) {
    return this->axis_type() == AxisType::XLINE;
}

bool Direction::is_sample() const noexcept(false) {
    return this->axis_type() == AxisType::SAMPLE;
}
