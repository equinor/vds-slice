#ifndef VDS_SLICE_OPENVDSVOXELVECTOR_HPP
#define VDS_SLICE_OPENVDSVOXELVECTOR_HPP

#include <string>
#include <stdexcept>

#include <OpenVDS/Vector.h>

#include "metadatahandle.hpp"

/**
 * Represents a vector in a coordinate system used by openvds Request- functions.

     |---Voxel index 0---|----Voxel index 1---|........|-----Voxel index nsamples-1------|
     |                   |                    |        |                                 |
     |                   |                    |        |                                 |
 -> (0)-----(0.5)-------(1)-------(1.5)------(2)...(nsamples-1)---(nsamples-0.5)------(nsamples)
              |                     |                                  |
              |                     |                                  |
              |                     |                                  |
     Voxel position 0       Voxel position 1               Voxel position nsamples-1
          Sample 0               Sample 1                     Sample nsamples-1

 * -> Shows how coordinate is stored in OpenvdsVoxelVector and its relation to actual
 * sample positions and results from ToVoxelIndex/ToVoxelPosition functions.
 *
 * In contrast to openvds data in the vector are always stored in
 * Inline/Crossline/Sample order.
 */
class OpenvdsVoxelVector : public OpenVDS::DoubleVector3{
public:
    OpenvdsVoxelVector(double iline, double xline, double sample) :
        OpenVDS::DoubleVector3(iline, xline, sample) {};

    /**
     * Constructs a new OpenvdsVoxelVector based on input and output vectors for
     * IJKCoordinateTransformer VoxelPosition functions
     *
     * @param metadata
     * @param original Coordinate vector in IJK/Annotation/CDP system
     * @param calculated Coordinate vector received from calling ToVoxelPosition functions.
     * Dimensions are switched according to positions in openvds file.
     */
    OpenvdsVoxelVector(
        const MetadataHandle& metadata,
        const OpenVDS::DoubleVector3& original,
        const OpenVDS::DoubleVector3& calculated
    ) :
        OpenVDS::DoubleVector3(
            original[0] == NOVALUE ? NOVALUE : calculated[metadata.iline().dimension()]  + 0.5,
            original[1] == NOVALUE ? NOVALUE : calculated[metadata.xline().dimension()]  + 0.5,
            original[2] == NOVALUE ? NOVALUE : calculated[metadata.sample().dimension()] + 0.5
        ){}

    double InlineValue() const noexcept(true)    { return (*this)[0]; }
    double CrosslineValue() const noexcept(true) { return (*this)[1]; }
    double SampleValue() const noexcept(true)    { return (*this)[2]; }

    static constexpr double NOVALUE = -999.25;
};

#endif /* VDS_SLICE_OPENVDSVOXELVECTOR_HPP */
