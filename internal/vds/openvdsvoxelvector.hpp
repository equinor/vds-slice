#ifndef VDS_SLICE_OPENVDSVOXELVECTOR_HPP
#define VDS_SLICE_OPENVDSVOXELVECTOR_HPP

#include <string>
#include <stdexcept>

#include <OpenVDS/Vector.h>

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
 * Assumes axes order in the vds file to be Sample-Crossline-Inline.
 */
class OpenvdsVoxelVector : public OpenVDS::DoubleVector3{
public:
    OpenvdsVoxelVector(double sample, double xline, double iline) :
        OpenVDS::DoubleVector3(sample, xline, iline) {};

    /**
     * Constructs a new OpenvdsVoxelVector based on input and output vectors for
     * IJKCoordinateTransformer VoxelPosition functions
     *
     * @param original Coordinate vector in IJK/Annotation/CDP system
     * @param calculated Coordinate vector received from calling ToVoxelPosition functions.
     * Dimensions are switched according to positions in openvds file.
     */
    OpenvdsVoxelVector(const OpenVDS::DoubleVector3& original, const OpenVDS::DoubleVector3& calculated) :
        OpenVDS::DoubleVector3(
            original.Z == NOVALUE ? NOVALUE : calculated.X + 0.5,
            original.Y == NOVALUE ? NOVALUE : calculated.Y + 0.5,
            original.X == NOVALUE ? NOVALUE : calculated.Z + 0.5
        ){}

    double InlineValue() const noexcept(true)    { return this->Z; }
    double CrosslineValue() const noexcept(true) { return this->Y; }
    double SampleValue() const noexcept(true)    { return this->X; }

    static constexpr double NOVALUE = -999.25;
};

#endif /* VDS_SLICE_OPENVDSVOXELVECTOR_HPP */
