#include "volumedatalayout.hpp"
#include <OpenVDS/KnownMetadata.h>
#include <OpenVDS/VolumeDataAxisDescriptor.h>

DoubleVolumeDataLayout::DoubleVolumeDataLayout(
    OpenVDS::VolumeDataLayout const* const layout_a,
    OpenVDS::VolumeDataLayout const* const layout_b
) : m_layout_a(layout_a),
    m_layout_b(layout_b) {

    OpenVDS::VDSIJKGridDefinition vds_a = m_layout_a->GetVDSIJKGridDefinitionFromMetadata();
    OpenVDS::VDSIJKGridDefinition vds_b = m_layout_b->GetVDSIJKGridDefinitionFromMetadata();

    if (vds_a.dimensionMap != vds_b.dimensionMap)
        throw std::runtime_error("Mismatch in VDS dimension map");

    if (vds_a.iUnitStep != vds_b.iUnitStep)
        throw std::runtime_error("Mismatch in iUnitStep");

    if (vds_a.jUnitStep != vds_b.jUnitStep)
        throw std::runtime_error("Mismatch in jUnitStep");

    if (vds_a.kUnitStep != vds_b.kUnitStep)
        throw std::runtime_error("Mismatch in kUnitStep");

    if (m_layout_a->GetDimensionality() != m_layout_b->GetDimensionality())
        throw detail::bad_request("Different number of dimensions");

    auto const crs = OpenVDS::KnownMetadata::SurveyCoordinateSystemCRSWkt();
    std::string crs_a = m_layout_a->GetMetadataString(crs.GetCategory(), crs.GetName());
    std::string crs_b = m_layout_b->GetMetadataString(crs.GetCategory(), crs.GetName());
    if (crs_a.compare(crs_b) != 0) {
        throw detail::bad_request("Coordinate reference system (CRS) mismatch: " + crs_a + " versus " + crs_b);
    }

    for (int32_t dimension = 0; dimension < Dimensionality_Max; dimension++) {

        if (dimension < this->GetDimensionality()) {

            const char* unit_a = m_layout_a->GetDimensionUnit(dimension);
            const char* unit_b = m_layout_b->GetDimensionUnit(dimension);
            if (strcmp(unit_a, unit_b) != 0) {
                throw detail::bad_request("Dimension unit mismatch for dimension: " + std::to_string(dimension));
            }

            const char* name_a = m_layout_a->GetDimensionName(dimension);
            const char* name_b = m_layout_b->GetDimensionName(dimension);
            if (strcmp(name_a, name_b) != 0) {
                throw detail::bad_request("Dimension name mismatch for dimension: " + std::to_string(dimension));
            }

            if (strcmp(this->GetDimensionName(dimension), "Inline") == 0)
                m_iline_index = dimension;
            else if (strcmp(this->GetDimensionName(dimension), "Crossline") == 0)
                m_xline_index = dimension;
            else if (strcmp(this->GetDimensionName(dimension), "Sample") == 0)
                m_sample_index = dimension;

            m_dimensionStepSize[dimension] = (m_layout_a->GetDimensionMax(dimension) - m_layout_a->GetDimensionMin(dimension)) / (m_layout_a->GetDimensionNumSamples(dimension) - 1);
            int32_t step_size_b = (m_layout_b->GetDimensionMax(dimension) - m_layout_b->GetDimensionMin(dimension)) / (m_layout_b->GetDimensionNumSamples(dimension) - 1);

            if (m_dimensionStepSize[dimension] != step_size_b) {
                throw detail::bad_request("Step size mismatch in axis: " + std::to_string(dimension));
            }

            m_dimensionCoordinateMin[dimension] = std::max(m_layout_a->GetDimensionMin(dimension), m_layout_b->GetDimensionMin(dimension));
            m_dimensionCoordinateMax[dimension] = std::min(m_layout_a->GetDimensionMax(dimension), m_layout_b->GetDimensionMax(dimension));

            m_dimensionNumSamples[dimension] = 1 + ((m_dimensionCoordinateMax[dimension] - m_dimensionCoordinateMin[dimension]) / m_dimensionStepSize[dimension]);

            // Verify that the offset is an integer number of steps
            float offset = (m_layout_b->GetDimensionMin(dimension) - m_layout_a->GetDimensionMin(dimension)) / m_dimensionStepSize[dimension];
            if (std::abs(std::round(offset) - offset) > 0.00001) {
                throw detail::bad_request("Offset mismatch in axis: " + std::to_string(dimension));
            }

            m_dimensionIndexOffset_a[dimension] = (m_dimensionCoordinateMin[dimension] - m_layout_a->GetDimensionMin(dimension)) / m_dimensionStepSize[dimension];
            m_dimensionIndexOffset_b[dimension] = (m_dimensionCoordinateMin[dimension] - m_layout_b->GetDimensionMin(dimension)) / m_dimensionStepSize[dimension];

        } else {
            m_dimensionNumSamples[dimension] = 1;
            m_dimensionIndexOffset_a[dimension] = 0;
            m_dimensionIndexOffset_b[dimension] = 0;
        }
    }

    OpenVDS::DoubleVector2 meta_origin_a = m_layout_a->GetMetadataDoubleVector2("SurveyCoordinateSystem", "Origin");
    OpenVDS::DoubleVector2 meta_origin_b = m_layout_b->GetMetadataDoubleVector2("SurveyCoordinateSystem", "Origin");

    if (std::abs(meta_origin_a.X - meta_origin_b.X) > 0.0001 || std::abs(meta_origin_a.Y - meta_origin_b.Y) > 0.0001) {
        throw std::runtime_error("Mismatch in origin position (GetMetadataDoubleVector2)");
    }
}

int DoubleVolumeDataLayout::GetDimensionality() const {
    // Constructor ensures that dimensionality is the same for m_layout_a and m_layout_b.
    return m_layout_a->GetDimensionality();
}

OpenVDS::VolumeDataAxisDescriptor DoubleVolumeDataLayout::GetAxisDescriptor(int dimension) const {
    return OpenVDS::VolumeDataAxisDescriptor(
        GetDimensionNumSamples(dimension),
        GetDimensionName(dimension),
        GetDimensionUnit(dimension),
        GetDimensionMin(dimension),
        GetDimensionMax(dimension)
    );
}

int DoubleVolumeDataLayout::GetDimensionNumSamples(int dimension) const {

    if (dimension < 0 || dimension >= Dimensionality_Max) {
        throw std::runtime_error("Not valid dimension");
    }
    return m_dimensionNumSamples[dimension];
}

/// @brief Calculate the IJKGridDefinition for the intersection of cube A and B
/// @return IJKGridDefinition for cube (A ∩ B)
OpenVDS::VDSIJKGridDefinition DoubleVolumeDataLayout::GetVDSIJKGridDefinitionFromMetadata() const {

    OpenVDS::DoubleVector2 meta_origin = m_layout_a->GetMetadataDoubleVector2("SurveyCoordinateSystem", "Origin");
    OpenVDS::DoubleVector2 in_line_spacing = m_layout_a->GetMetadataDoubleVector2("SurveyCoordinateSystem", "InlineSpacing");
    OpenVDS::DoubleVector2 cross_line_spacing = m_layout_a->GetMetadataDoubleVector2("SurveyCoordinateSystem", "CrosslineSpacing");

    OpenVDS::DoubleVector2 iline_vector = {in_line_spacing.X * (this->m_dimensionCoordinateMin[m_iline_index]), in_line_spacing.Y * (this->m_dimensionCoordinateMin[m_iline_index])};
    OpenVDS::DoubleVector2 xline_vector = {cross_line_spacing.X * (this->m_dimensionCoordinateMin[m_xline_index]), cross_line_spacing.Y * (this->m_dimensionCoordinateMin[m_xline_index])};
    OpenVDS::VDSIJKGridDefinition grid_definition_a = m_layout_a->GetVDSIJKGridDefinitionFromMetadata();

    OpenVDS::VDSIJKGridDefinition double_grid_definition;

    // By the constructor check the following values are identical in A and B
    double_grid_definition.dimensionMap = grid_definition_a.dimensionMap;
    double_grid_definition.iUnitStep = grid_definition_a.iUnitStep;
    double_grid_definition.jUnitStep = grid_definition_a.jUnitStep;
    double_grid_definition.kUnitStep = grid_definition_a.kUnitStep;

    // Calculate the new origin
    double_grid_definition.origin.X = meta_origin.X + iline_vector.X + xline_vector.X;
    double_grid_definition.origin.Y = meta_origin.Y + iline_vector.Y + xline_vector.Y;
    double_grid_definition.origin.Z = -this->m_dimensionCoordinateMin[m_sample_index];

    return double_grid_definition;
}

const char* DoubleVolumeDataLayout::GetDimensionName(int dimension) const {
    // Constructor ensures that the name in `dimension` are identical for m_layout_a and m_layout_b.
    return m_layout_a->GetDimensionName(dimension);
}

const char* DoubleVolumeDataLayout::GetDimensionUnit(int dimension) const {
    // Constructor ensures that the unit in `dimension` are identical for m_layout_a and m_layout_b.
    return m_layout_a->GetDimensionUnit(dimension);
}

float DoubleVolumeDataLayout::GetDimensionMin(int dimension) const {
    return m_dimensionCoordinateMin[dimension];
}

float DoubleVolumeDataLayout::GetDimensionMax(int dimension) const {
    return m_dimensionCoordinateMax[dimension];
}

/// @brief Get the offset to (A \cap B) for cube A in given dimension
/// @param dimension The provided dimension
/// @return Calculated offset
std::uint32_t DoubleVolumeDataLayout::GetDimensionIndexOffset_a(int dimension) const {
    return m_dimensionIndexOffset_a[dimension];
}

/// @brief Get the offset to (A \cap B) for cube B in given dimension
/// @param dimension The provided dimension
/// @return Calculated offset
std::uint32_t DoubleVolumeDataLayout::GetDimensionIndexOffset_b(int dimension) const {
    return m_dimensionIndexOffset_b[dimension];
}
