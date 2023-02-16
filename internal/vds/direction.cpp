#include "direction.hpp"

#include <stdexcept>

#include <OpenVDS/KnownMetadata.h>

#include "vds.h"

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
