#include <string>
#include <stdexcept>

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/Vector.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

#include "metadatahandle.hpp"
#include "openvdsvoxelvector.hpp"

OpenvdsVoxelVector MetadataHandle::CoordinateTransformer::IndexToOpenvdsPosition(
    const OpenVDS::DoubleVector3& position
) const {
    return OpenvdsVoxelVector(position, this->IJKPositionToVoxelPosition(position));
}

OpenvdsVoxelVector MetadataHandle::CoordinateTransformer::AnnotationToOpenvdsPosition(
    const OpenVDS::DoubleVector3& position
) const {
    return OpenvdsVoxelVector(position, this->AnnotationToVoxelPosition(position));
}

OpenvdsVoxelVector MetadataHandle::CoordinateTransformer::WorldToOpenvdsPosition(
    const OpenVDS::DoubleVector3& position
) const {
    return OpenvdsVoxelVector(position, this->WorldToVoxelPosition(position));
}

OpenvdsVoxelVector MetadataHandle::CoordinateTransformer::ToOpenvdsPosition(
    const OpenVDS::DoubleVector3& coordinate,
    enum coordinate_system coordinate_system
) const {
    switch (coordinate_system) {
        case INDEX:
            return this->IndexToOpenvdsPosition(coordinate);
        case ANNOTATION:
            return this->AnnotationToOpenvdsPosition(coordinate);
        case CDP:
            return this->WorldToOpenvdsPosition(coordinate);
        default: {
            throw std::runtime_error("Unhandled coordinate system");
        }
    }
}

bool OutOfRange(const Axis& axis, double value) noexcept(true){
    if (value == OpenvdsVoxelVector::NOVALUE) return false;
    return value < 0 or value >= axis.nsamples();
}

bool MetadataHandle::CoordinateTransformer::IsOpenvdsPositionOutOfRangeIline(
    double value
) const noexcept(true){
    return OutOfRange(this->m_metadata.iline(), value);
}

bool MetadataHandle::CoordinateTransformer::IsOpenvdsPositionOutOfRangeXline(
    double value
) const noexcept(true){
    return OutOfRange(this->m_metadata.xline(), value);
}

bool MetadataHandle::CoordinateTransformer::IsOpenvdsPositionOutOfRangeSample(
    double value
) const noexcept(true){
    return OutOfRange(this->m_metadata.sample(), value);
}

bool MetadataHandle::CoordinateTransformer::IsOpenvdsPositionOutOfRange(
    const OpenvdsVoxelVector& position
) const noexcept(true){
    return
        this->IsOpenvdsPositionOutOfRangeIline(position.InlineValue()) or
        this->IsOpenvdsPositionOutOfRangeXline(position.CrosslineValue()) or
        this->IsOpenvdsPositionOutOfRangeSample(position.SampleValue());
}

Axis MetadataHandle::CoordinateTransformer::WhereOpenvdsPositionOutOfRange(
    const OpenvdsVoxelVector& position
) const noexcept(false){
    if (this->IsOpenvdsPositionOutOfRangeIline(position.InlineValue()))
        return this->m_metadata.iline();
    if (this->IsOpenvdsPositionOutOfRangeXline(position.CrosslineValue()))
        return this->m_metadata.xline();
    if (this->IsOpenvdsPositionOutOfRangeSample(position.SampleValue()))
        return this->m_metadata.sample();

    throw std::runtime_error("No openvds position out of range");
}
