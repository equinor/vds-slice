#include "metadatahandler.h"

#include <list>
#include <utility>

#include <OpenVDS/KnownMetadata.h>

namespace vds {

MetadataHandler::MetadataHandler(
    const std::string url,
    const std::string credentials
)  {
    OpenVDS::Error error;
    this->vdsHandle = std::make_shared<OpenVDS::ScopedVDSHandle>(
                          OpenVDS::Open(url, credentials, error)
                      );
    if(error.code != 0) {
        throw std::runtime_error("Could not open VDS: " + error.string);
    }

    auto accessManager = OpenVDS::GetAccessManager(*this->vdsHandle);
    this->vdsLayout = accessManager.GetVolumeDataLayout();

    //Verify assumptions
    // Expect 3D VDS
    constexpr int expectedDimensionality = 3;
    if (this->vdsLayout->GetDimensionality() != expectedDimensionality) {
        throw std::runtime_error(
            "Unsupported VDS, expected 3 dimensions, got " +
            std::to_string(this->vdsLayout->GetDimensionality())
        );
    }

    // TODO: Axis order correct?
    // Expect: Sample, Crossline, Inline
    std::list<std::pair<int, char const *>> expectedAxisIndexAndName = {
        {0, OpenVDS::KnownAxisNames::Sample()},
        {1, OpenVDS::KnownAxisNames::Crossline()},
        {2, OpenVDS::KnownAxisNames::Inline()}
    };
    for (const auto& indexAndName: expectedAxisIndexAndName) {
        const int expectedAxisIndex = indexAndName.first;
        char const * expectedAxisName = indexAndName.second;
        char const * actualAxisName = this->vdsLayout->GetDimensionName(
                                          expectedAxisIndex
                                      );

        if (strcmp(expectedAxisName, actualAxisName) != 0) {
            std::string msg = "Unsupported axis ordering. Expected axis "
                              + std::string(expectedAxisName)
                              + " (axis index: "
                              + std::to_string(expectedAxisIndex)
                              + "), got "
                              + std::string(actualAxisName);
            throw std::runtime_error(msg);
        }
    }
}

// TODO: Could also have a member of Axis for inline direction and return copy
//       or reference
Axis MetadataHandler::getInline() const {
    return Axis(ApiAxisName::INLINE, this->vdsLayout);
}
// TODO: Could also have a member of Axis for crossline direction and return
//       copy or reference
Axis MetadataHandler::getCrossline() const {
    return Axis(ApiAxisName::CROSSLINE, this->vdsLayout);
}
// TODO: Could also have a member of Axis for sample direction and return copy
//       or reference
Axis MetadataHandler::getSample() const {
    return Axis(ApiAxisName::SAMPLE, this->vdsLayout);
}

Axis MetadataHandler::getAxis(const ApiAxisName axisName) const {
    // TODO: Do we want this check here? Could be beneficial if this object
    //       holds axes for inline, crossline and sample axis.
    //switch(an) {
    //    case ApiAxisName::I:
    //    case ApiAxisName::INLINE:
    //        return this->getInline();
    //    case ApiAxisName::J:
    //    case ApiAxisName::CROSSLINE:
    //        return this->getCrossline();
    //    case ApiAxisName::DEPTH:
    //    case ApiAxisName::SAMPLE:
    //    case ApiAxisName::TIME:
    //        return this->getSample();
    //}
    return Axis(axisName, this->vdsLayout);
}

BoundingBox MetadataHandler::getBoundingBox() const {
    return BoundingBox(this->vdsLayout);
}

std::string MetadataHandler::getFormat() const {
    return "<f4";
}

std::string MetadataHandler::getCRS() const {
    const auto crs = OpenVDS::KnownMetadata::SurveyCoordinateSystemCRSWkt();
    return this->vdsLayout->GetMetadataString(crs.GetCategory(), crs.GetName());
}

OpenVDS::VolumeDataFormat MetadataHandler::getChannelFormat(
    const int channelIndex
){
    return this->vdsLayout->GetChannelFormat(channelIndex);
}


std::shared_ptr<OpenVDS::ScopedVDSHandle> MetadataHandler::getVDSHandle() {
    return this->vdsHandle;
}

OpenVDS::InterpolationMethod MetadataHandler::getInterpolation(
    InterpolationMethod interpolation
) {
    switch (interpolation)
    {
        case NEAREST: return OpenVDS::InterpolationMethod::Nearest;
        case LINEAR: return OpenVDS::InterpolationMethod::Linear;
        case CUBIC: return OpenVDS::InterpolationMethod::Cubic;
        case ANGULAR: return OpenVDS::InterpolationMethod::Angular;
        case TRIANGULAR: return OpenVDS::InterpolationMethod::Triangular;
        default: {
            throw std::runtime_error("Unhandled interpolation method");
        }
    }
}

} /* namespace vds */
