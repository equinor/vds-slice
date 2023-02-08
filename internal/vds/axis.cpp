#include "axis.h"

#include <stdexcept>

#include <OpenVDS/IJKCoordinateTransformer.h>
#include <OpenVDS/KnownMetadata.h>

Axis::Axis(
    const enum api_axis_name api_name,
    OpenVDS::VolumeDataLayout const * layout
) : api_name(api_name) {
    switch (this->api_name) {
        case I:
        case INLINE:
            this->api_index = 0;
            this->vds_index = 2;
            break;
        case J:
        case CROSSLINE:
            this->api_index = 1;
            this->vds_index = 1;
            break;
        case K:
        case DEPTH:
        case TIME:
        case SAMPLE:
            this->api_index = 2;
            this->vds_index = 0;
            break;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }

    this->vds_axis_descriptor = layout->GetAxisDescriptor(this->vds_index);
}

int Axis::get_min() const noexcept(true) {
    return this->vds_axis_descriptor.GetCoordinateMin();
}

int Axis::get_max() const noexcept(true) {
    return this->vds_axis_descriptor.GetCoordinateMax();
}

int Axis::get_number_of_points() const noexcept(true) {
    return this->vds_axis_descriptor.GetNumSamples();
}

std::string Axis::get_unit() const noexcept(true) {
    return this->vds_axis_descriptor.GetUnit();
}

int Axis::get_vds_index() const noexcept(true) {
    return this->vds_index;
}

int Axis::get_api_index() const noexcept(true) {
    return this->api_index;
}

std::string Axis::get_vds_name() const noexcept(true) {
    return this->vds_axis_descriptor.GetName();
}

std::string Axis::get_api_name() const {
    switch (this->api_name) {
        case I:
            return OpenVDS::KnownAxisNames::I();
        case INLINE:
            return OpenVDS::KnownAxisNames::Inline();
        case J:
            return OpenVDS::KnownAxisNames::J();
        case CROSSLINE:
            return OpenVDS::KnownAxisNames::Crossline();
        case K:
            return OpenVDS::KnownAxisNames::K();
        case DEPTH:
            return OpenVDS::KnownAxisNames::Depth();
        case TIME:
            return OpenVDS::KnownAxisNames::Time();
        case SAMPLE:
            return OpenVDS::KnownAxisNames::Sample();
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }
}

coordinate_system Axis::get_coordinate_system() const {
    switch (this->api_name) {
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
            throw std::runtime_error("Unhandled coordinate system");
        }
    }
}
