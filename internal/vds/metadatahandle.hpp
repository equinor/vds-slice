#ifndef VDS_SLICE_METADATAHANDLE_HPP
#define VDS_SLICE_METADATAHANDLE_HPP

#include <string>

#include <OpenVDS/OpenVDS.h>
#include <OpenVDS/Vector.h>
#include <OpenVDS/IJKCoordinateTransformer.h>

#include "axis.hpp"
#include "boundingbox.hpp"
#include "direction.hpp"

class OpenvdsVoxelVector;

class MetadataHandle {
public:
    MetadataHandle(OpenVDS::VolumeDataLayout const * const layout);

    Axis const& iline()  const noexcept (true);
    Axis const& xline()  const noexcept (true);
    Axis const& sample() const noexcept (true);

    BoundingBox bounding_box() const noexcept (true);
    std::string crs()          const noexcept (true);
    std::string input_filename() const noexcept (true);

    Axis const& get_axis(Direction const direction) const noexcept (false);

    /**
     * Extends IJKCoordinateTransformer with ability to translate to openvds
     * positions - aka system used in Request- functions.

     * We have a problem that OpenVDS' ToVoxelPosition and data request
     * functions have different definitions of where a datapoint is. E.g. A
     * transformer will return (0,0,0) for the first sample in the cube. The
     * request functions on the other hand assume the data is located in the
     * center of a voxel. I.e. that the first sample is at (0.5, 0.5, 0.5).
     *
     * Thus extending IJKCoordinateTransformer with new methods helps us to
     * bridge this gap.
     */
    class CoordinateTransformer : public OpenVDS::IJKCoordinateTransformer{
    public:
        CoordinateTransformer(const MetadataHandle& metadata ) :
            OpenVDS::IJKCoordinateTransformer(metadata.m_layout), m_metadata(metadata) {};

        OpenvdsVoxelVector IndexToOpenvdsPosition(
            const OpenVDS::DoubleVector3& position
        ) const noexcept (false);
        OpenvdsVoxelVector AnnotationToOpenvdsPosition(
            const OpenVDS::DoubleVector3& position
        ) const noexcept (false);
        OpenvdsVoxelVector WorldToOpenvdsPosition(
            const OpenVDS::DoubleVector3& position
        ) const noexcept (false);

        OpenvdsVoxelVector ToOpenvdsPosition(
            const OpenVDS::DoubleVector3& coordinate,
            enum coordinate_system coordinate_system
        ) const noexcept (false);

        bool IsOpenvdsPositionOutOfRangeIline (double value) const noexcept(true);
        bool IsOpenvdsPositionOutOfRangeXline (double value) const noexcept(true);
        bool IsOpenvdsPositionOutOfRangeSample(double value) const noexcept(true);
        bool IsOpenvdsPositionOutOfRange   (const OpenvdsVoxelVector& position) const noexcept(true);
        Axis WhereOpenvdsPositionOutOfRange(const OpenvdsVoxelVector& position) const noexcept(false);

    private:
        const MetadataHandle& m_metadata;
    };

    CoordinateTransformer coordinate_transformer() const noexcept (true);
private:
    OpenVDS::VolumeDataLayout const * const m_layout;

    Axis const m_iline;
    Axis const m_xline;
    Axis const m_sample;

    void dimension_validation() const;
    void axis_order_validation() const;
};

#endif /* VDS_SLICE_METADATAHANDLE_HPP */
