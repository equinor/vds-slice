#include "direction.hpp"

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

bool Direction::is_iline() const noexcept (true) {
    switch (this->name()) {
        case I: // fallthrough
        case INLINE:
            return true;
        default:
            return false;
    }
}

bool Direction::is_xline() const noexcept (true) {
    switch (this->name()) {
        case J: // fallthrough
        case CROSSLINE:
            return true;
        default:
            return false;
    }
}

bool Direction::is_sample() const noexcept (true) {
    switch (this->name()) {
        case K:     // fallthrough
        case DEPTH: // fallthrough
        case TIME:  // fallthrough
        case SAMPLE:
            return true;
        default:
            return false;
    }
}
