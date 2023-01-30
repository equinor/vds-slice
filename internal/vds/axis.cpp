#include "axis.h"

#include <stdexcept>

#include <OpenVDS/KnownMetadata.h>

namespace vds {

Axis::Axis(
            const ApiAxisName apiAxisName,
            OpenVDS::VolumeDataLayout const * vdsLayout
) : apiAxisName(apiAxisName) {
    switch (this->apiAxisName) {
        case I:
        case INLINE:
            this->apiIndex = 0;
            this->vdsIndex = 2;
            break;
        case J:
        case CROSSLINE:
            this->apiIndex = 1;
            this->vdsIndex = 1;
            break;
        case K:
        case DEPTH:
        case TIME:
        case SAMPLE:
            this->apiIndex = 2;
            this->vdsIndex = 0;
            break;
        default: {
            throw std::runtime_error("Unhandled axis");
        }
    }

    // Get VDS axis descriptor
    vdsAxisDescriptor = vdsLayout->GetAxisDescriptor(this->vdsIndex);

    //Validate expections here now
    if (this->getCoordinateSystem() == CoordinateSystem::ANNOTATION) {
        auto transformer = OpenVDS::IJKCoordinateTransformer(vdsLayout);
        if (not transformer.AnnotationsDefined()) {
            throw std::runtime_error("VDS doesn't define annotations");
        }
    }
}

int Axis::getMin() const {
    return this->vdsAxisDescriptor.GetCoordinateMin();
}

int Axis::getMax() const {
    return this->vdsAxisDescriptor.GetCoordinateMax();
}

int Axis::getNumberOfPoints() const {
    return this->vdsAxisDescriptor.GetNumSamples();
}

std::string Axis::getUnit() const {
    std::string unit = this->vdsAxisDescriptor.GetUnit();
    return unit;
}

int Axis::getVdsIndex() const {
    return this->vdsIndex;
}

int Axis::getApiIndex() const {
    return this->apiIndex;
}

std::string Axis::getVdsName() const {
    std::string name = this->vdsAxisDescriptor.GetName();
    return name;
}

std::string Axis::getApiName() const {
// Find axis name defined by API
    switch (this->apiAxisName) {
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

CoordinateSystem Axis::getCoordinateSystem() const {
    switch (this->apiAxisName) {
        case I:
        case J:
        case K:
            return CoordinateSystem::INDEX;
        case INLINE:
        case CROSSLINE:
        case DEPTH:
        case TIME:
        case SAMPLE:
            return CoordinateSystem::ANNOTATION;
        default: {
            throw std::runtime_error("Unhandled coordinate system");
        }
    }
}

} /* namespace vds */
