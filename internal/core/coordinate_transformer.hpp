#ifndef VDS_SLICE_COORDINATE_TRANSFORMER_HPP
#define VDS_SLICE_COORDINATE_TRANSFORMER_HPP

#include <stdexcept>

#include <OpenVDS/IJKCoordinateTransformer.h>

class CoordinateTransformer {
public:
    virtual OpenVDS::IntVector3 VoxelIndexToIJKIndex(const OpenVDS::IntVector3& voxelIndex) const = 0;

    virtual OpenVDS::DoubleVector3 IJKIndexToWorld(const OpenVDS::IntVector3& ijkIndex) const = 0;

    virtual OpenVDS::DoubleVector3 IJKIndexToAnnotation(const OpenVDS::IntVector3& ijkIndex) const = 0;
    virtual OpenVDS::DoubleVector3 IJKPositionToAnnotation(const OpenVDS::DoubleVector3& ijkPosition) const = 0;
    virtual OpenVDS::DoubleVector3 WorldToAnnotation(OpenVDS::DoubleVector3 worldPosition) const = 0;
};

class SingleCoordinateTransformer : public CoordinateTransformer {
public:
    SingleCoordinateTransformer(OpenVDS::IJKCoordinateTransformer transformer)
        : coordinate_transformer(transformer) {}

    OpenVDS::IntVector3 AnnotationToIJKIndex(const OpenVDS::DoubleVector3& annotationPosition) const {
        return coordinate_transformer.AnnotationToIJKIndex(annotationPosition);
    }

    OpenVDS::IntVector3 VoxelIndexToIJKIndex(const OpenVDS::IntVector3& voxelIndex) const {
        return coordinate_transformer.VoxelIndexToIJKIndex(voxelIndex);
    }

    OpenVDS::DoubleVector3 IJKIndexToWorld(const OpenVDS::IntVector3& ijkIndex) const {
        return coordinate_transformer.IJKIndexToWorld(ijkIndex);
    }

    OpenVDS::DoubleVector3 IJKIndexToAnnotation(const OpenVDS::IntVector3& ijkIndex) const {
        return coordinate_transformer.IJKIndexToAnnotation(ijkIndex);
    }

    OpenVDS::DoubleVector3 IJKPositionToAnnotation(const OpenVDS::DoubleVector3& ijkPosition) const {
        return coordinate_transformer.IJKPositionToAnnotation(ijkPosition);
    }

    OpenVDS::DoubleVector3 WorldToAnnotation(OpenVDS::DoubleVector3 worldPosition) const {
        return coordinate_transformer.WorldToAnnotation(worldPosition);
    }

    OpenVDS::IntVector3 IJKToVoxelDimensionMap() const {
        return coordinate_transformer.IJKToVoxelDimensionMap();
    }

private:
    OpenVDS::IJKCoordinateTransformer coordinate_transformer;
};

class DoubleCoordinateTransformer : public CoordinateTransformer {
public:
    DoubleCoordinateTransformer(
        SingleCoordinateTransformer const& transformer_a,
        SingleCoordinateTransformer const& transformer_b
    )
        : m_transformer_a(transformer_a) {

        if (transformer_a.IJKToVoxelDimensionMap() != transformer_b.IJKToVoxelDimensionMap()) {
            throw std::runtime_error("Coordinate Transformers have different dimension maps");
        }

        /*
         * For each dimension, intersection 0 index corresponds to either 0-line
         * in cube a or 0-line in cube b. By representing 0 in cube b as index
         * in cube a we get the distance between cubes a and b. For each
         * dimension, if
         * - distance between a and b > 0, then cube b is further than cube a
         *   and intersection 0 corresponds to cube b 0, which in cube a index
         *   coordinate system is the distance between a and b
         * - distance between a and b < 0, then cube a is further than cube b
         *   and intersection 0 corresponds to cube a 0, which in cube a index
         *   coordinate system is 0
         * - distance between a and b = 0, then cubes a and b are on the same
         *   line and intersection 0 corresponds to this line, which in cube a
         *   index coordinate system is 0
         */

        auto cube_b_zero_as_annotation = transformer_b.IJKIndexToAnnotation(OpenVDS::IntVector3(0, 0, 0));
        auto cube_b_zero_as_cube_a_index = transformer_a.AnnotationToIJKIndex(cube_b_zero_as_annotation);
        for (int index = 0; index < 3; ++index) {
            if (cube_b_zero_as_cube_a_index[index] >= 0) {
                this->m_intersection_zero_as_cube_a_index[index] = cube_b_zero_as_cube_a_index[index];
            } else {
                this->m_intersection_zero_as_cube_a_index[index] = 0;
            }
        }

        auto intersection_zero_as_annotation = transformer_a.IJKIndexToAnnotation(this->m_intersection_zero_as_cube_a_index);
        this->m_intersection_zero_as_cube_b_index = transformer_b.AnnotationToIJKIndex(intersection_zero_as_annotation);
    }

    OpenVDS::IntVector3 VoxelIndexToIJKIndex(const OpenVDS::IntVector3& voxelIndex) const {
        // voxel index to ijk index depends only on dimensions order, which
        // should be the same for all the cubes
        return m_transformer_a.VoxelIndexToIJKIndex(voxelIndex);
    }
    OpenVDS::DoubleVector3 IJKIndexToWorld(const OpenVDS::IntVector3& ijkIndex) const {
        auto ijkIndexInCubeA = as_cube_a_ijk_index(ijkIndex);
        return m_transformer_a.IJKIndexToWorld(ijkIndexInCubeA);
    }

    OpenVDS::DoubleVector3 IJKIndexToAnnotation(const OpenVDS::IntVector3& ijkIndex) const {
        auto ijkIndexInCubeA = as_cube_a_ijk_index(ijkIndex);
        return m_transformer_a.IJKIndexToAnnotation(ijkIndexInCubeA);
    }

    OpenVDS::DoubleVector3 WorldToAnnotation(OpenVDS::DoubleVector3 worldPosition) const {
        // both intersection cube and cube a anyway have the same
        // world/annotation data
        return m_transformer_a.WorldToAnnotation(worldPosition);
    }
    OpenVDS::DoubleVector3 IJKPositionToAnnotation(const OpenVDS::DoubleVector3& ijkPosition) const {
        auto ijkPosistionInCubeA = as_cube_a_ijk_position(ijkPosition);
        return m_transformer_a.IJKPositionToAnnotation(ijkPosistionInCubeA);
    }

    void to_cube_a_voxel_position(float* out_cube_a_position, float const* intersection_cube_position) const {
        for (int ijk_index = 0; ijk_index < 3; ++ijk_index) {
            auto voxel_index = m_transformer_a.IJKToVoxelDimensionMap()[ijk_index];
            out_cube_a_position[voxel_index] = intersection_cube_position[voxel_index] +
                                               this->m_intersection_zero_as_cube_a_index[ijk_index];
        };
    }

    void to_cube_b_voxel_position(float* out_cube_b_position, float const* intersection_cube_position) const {
        for (int ijk_index = 0; ijk_index < 3; ++ijk_index) {
            auto voxel_index = m_transformer_a.IJKToVoxelDimensionMap()[ijk_index];
            out_cube_b_position[voxel_index] = intersection_cube_position[voxel_index] +
                                               this->m_intersection_zero_as_cube_b_index[ijk_index];
        }
    }

private:
    OpenVDS::IntVector3 as_cube_a_ijk_index(const OpenVDS::IntVector3& ijkIndex) const {
        auto as_cube_a_index = OpenVDS::IntVector3(ijkIndex);
        for (int index = 0; index < 3; ++index) {
            as_cube_a_index[index] += this->m_intersection_zero_as_cube_a_index[index];
        }
        return as_cube_a_index;
    }

    OpenVDS::DoubleVector3 as_cube_a_ijk_position(const OpenVDS::DoubleVector3& ijkPosition) const {
        auto as_cube_a_position = OpenVDS::DoubleVector3(ijkPosition);
        for (int index = 0; index < 3; ++index) {
            as_cube_a_position[index] += this->m_intersection_zero_as_cube_a_index[index];
        }
        return as_cube_a_position;
    }

    SingleCoordinateTransformer const& m_transformer_a;
    OpenVDS::IntVector3 m_intersection_zero_as_cube_a_index;
    OpenVDS::IntVector3 m_intersection_zero_as_cube_b_index;
};

#endif /* VDS_SLICE_COORDINATE_TRANSFORMER */
