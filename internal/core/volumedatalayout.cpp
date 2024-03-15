#include "volumedatalayout.hpp"
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

    if (vds_a.origin[0] == vds_b.origin[0] && vds_a.origin[1] == vds_b.origin[1])

        if (m_layout_a->GetDimensionality() != m_layout_b->GetDimensionality()) {
            throw detail::bad_request("Different number of dimensions");
        }

    for (int32_t dimension = 0; dimension < Dimensionality_Max; dimension++) {

        if (dimension < this->GetDimensionality()) {

            if (strcmp(this->GetDimensionName(dimension), "Inline") == 0)
                m_iline_index = dimension;
            else if (strcmp(this->GetDimensionName(dimension), "Crossline") == 0)
                m_xline_index = dimension;
            else if (strcmp(this->GetDimensionName(dimension), "Sample") == 0)
                m_sample_index = dimension;

            int32_t step_size_a = (m_layout_a->GetDimensionMax(dimension) - m_layout_a->GetDimensionMin(dimension)) / (m_layout_a->GetDimensionNumSamples(dimension) - 1);
            int32_t step_size_b = (m_layout_b->GetDimensionMax(dimension) - m_layout_b->GetDimensionMin(dimension)) / (m_layout_b->GetDimensionNumSamples(dimension) - 1);

            if (step_size_a != step_size_b) {
                throw detail::bad_request("Step size mismatch in axis: " + std::to_string(dimension));
            }

            m_dimensionStepSize[dimension] = step_size_a;
            m_dimensionCoordinateMin[dimension] = std::max(m_layout_a->GetDimensionMin(dimension), m_layout_b->GetDimensionMin(dimension));
            m_dimensionCoordinateMax[dimension] = std::min(m_layout_a->GetDimensionMax(dimension), m_layout_b->GetDimensionMax(dimension));

            m_dimensionNumSamples[dimension] = 1 + ((m_dimensionCoordinateMax[dimension] - m_dimensionCoordinateMin[dimension]) / m_dimensionStepSize[dimension]);

            float offset = (m_layout_b->GetDimensionMin(dimension) - m_layout_a->GetDimensionMin(dimension)) / step_size_a;

            if (std::abs(std::round(offset) - offset) > 0.00001) {
                throw detail::bad_request("Offset mismatch in axis: " + std::to_string(dimension));
            }

            m_dimensionIndexOffset_a[dimension] = (m_dimensionCoordinateMin[dimension] - m_layout_a->GetDimensionMin(dimension)) / m_dimensionStepSize[dimension];
            m_dimensionIndexOffset_b[dimension] = (m_dimensionCoordinateMin[dimension] - m_layout_b->GetDimensionMin(dimension)) / m_dimensionStepSize[dimension];

        } else {
            m_dimensionNumSamples[dimension] = 1;
        }
    }
}

int DoubleVolumeDataLayout::GetDimensionality() const {
    // Constructor ensures that dimensionality is the same for m_layout_a and m_layout_b.
    return m_layout_a->GetDimensionality();
}

OpenVDS::VolumeDataAxisDescriptor DoubleVolumeDataLayout::GetAxisDescriptor(int dimension) const {

    // OpenVDS::VolumeDataAxisDescriptor* t = &(OpenVDS::VolumeDataAxisDescriptor());

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
        int dimMax = Dimensionality_Max;
        throw std::runtime_error("Not valid dimension");
    }
    return m_dimensionNumSamples[dimension];
}

OpenVDS::VDSIJKGridDefinition DoubleVolumeDataLayout::GetVDSIJKGridDefinitionFromMetadata() const {

    OpenVDS::VDSIJKGridDefinition grid_definition_a = m_layout_a->GetVDSIJKGridDefinitionFromMetadata();
    OpenVDS::VDSIJKGridDefinition double_grid_definition;

    double_grid_definition.dimensionMap = grid_definition_a.dimensionMap;
    double_grid_definition.iUnitStep = grid_definition_a.iUnitStep;
    double_grid_definition.jUnitStep = grid_definition_a.jUnitStep;
    double_grid_definition.kUnitStep = grid_definition_a.kUnitStep;

    OpenVDS::DoubleVector2 meta_origin = m_layout_a->GetMetadataDoubleVector2("SurveyCoordinateSystem", "Origin");
    OpenVDS::DoubleVector2 in_line_spacing = m_layout_a->GetMetadataDoubleVector2("SurveyCoordinateSystem", "InlineSpacing");
    OpenVDS::DoubleVector2 cross_line_spacing = m_layout_a->GetMetadataDoubleVector2("SurveyCoordinateSystem", "CrosslineSpacing");

    OpenVDS::DoubleVector2 iline_vector = {in_line_spacing.X * (this->m_dimensionCoordinateMin[m_iline_index]), in_line_spacing.Y * (this->m_dimensionCoordinateMin[m_iline_index])};
    OpenVDS::DoubleVector2 xline_vector = {cross_line_spacing.X * (this->m_dimensionCoordinateMin[m_xline_index]), cross_line_spacing.Y * (this->m_dimensionCoordinateMin[m_xline_index])};

    double_grid_definition.origin.X = meta_origin.X + iline_vector.X + xline_vector.X;
    double_grid_definition.origin.Y = meta_origin.Y + iline_vector.Y + xline_vector.Y;
    double_grid_definition.origin.Z = -this->m_dimensionCoordinateMin[m_sample_index];

    return double_grid_definition;
}

const char* DoubleVolumeDataLayout::GetDimensionName(int dimension) const {
    const char* name_a = m_layout_a->GetDimensionName(dimension);
    const char* name_b = m_layout_b->GetDimensionName(dimension);
    if (strcmp(name_a, name_b) == 0) {
        return m_layout_a->GetDimensionName(dimension);
    } else {
        throw detail::bad_request("Dimension name mismatch for dimension: " + std::to_string(dimension));
    }
}

const char* DoubleVolumeDataLayout::GetDimensionUnit(int dimension) const {
    const char* unit_a = m_layout_a->GetDimensionUnit(dimension);
    const char* unit_b = m_layout_b->GetDimensionUnit(dimension);
    if (strcmp(unit_a, unit_b) == 0) {
        return m_layout_a->GetDimensionUnit(dimension);
    } else {
        throw detail::bad_request("Dimension unit mismatch for dimension: " + std::to_string(dimension));
    }
}

float DoubleVolumeDataLayout::GetDimensionMin(int dimension) const {
    return m_dimensionCoordinateMin[dimension];
}

float DoubleVolumeDataLayout::GetDimensionMax(int dimension) const {
    return m_dimensionCoordinateMax[dimension];
}

float DoubleVolumeDataLayout::GetDimensionIndexOffset_a(int dimension) const {
    return m_dimensionIndexOffset_a[dimension];
}

float DoubleVolumeDataLayout::GetDimensionIndexOffset_b(int dimension) const {
    return m_dimensionIndexOffset_b[dimension];
}
