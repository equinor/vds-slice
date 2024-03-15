#ifndef VDL_VOLUME_DATA_LAYOUT_HPP
#define VDL_VOLUME_DATA_LAYOUT_HPP

#include "exceptions.hpp"
#include <OpenVDS/OpenVDS.h>
#include <assert.h>
#include <vector>

class DoubleVolumeDataLayout : public OpenVDS::VolumeDataLayout {

public:
    DoubleVolumeDataLayout(
        OpenVDS::VolumeDataLayout const* const layout_a,
        OpenVDS::VolumeDataLayout const* const layout_b
    );

    uint64_t GetContentsHash() const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetContentsHash");
    }

    uint64_t GetLayoutHash() const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetLayoutHash");
    }

    int GetDimensionality() const;

    int GetChannelCount() const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelCount");
    }

    bool IsChannelAvailable(const char* channelName) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::IsChannelAvailable");
    }

    int GetChannelIndex(const char* channelName) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelIndex");
    }

    OpenVDS::VolumeDataLayoutDescriptor GetLayoutDescriptor() const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetLayoutDescriptor");
    }

    OpenVDS::VolumeDataChannelDescriptor GetChannelDescriptor(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelDescriptor");
    }

    OpenVDS::VolumeDataAxisDescriptor GetAxisDescriptor(int dimension) const;

    OpenVDS::VolumeDataFormat GetChannelFormat(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelFormat");
    }

    OpenVDS::VolumeDataComponents GetChannelComponents(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelComponents");
    }

    const char* GetChannelName(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelComponents");
    }

    const char* GetChannelUnit(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelUnit");
    }

    float GetChannelValueRangeMin(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelUnit");
    }

    float GetChannelValueRangeMax(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelValueRangeMax");
    }

    bool IsChannelDiscrete(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::IsChannelDiscrete");
    }

    bool IsChannelRenderable(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::IsChannelRenderable");
    }

    bool IsChannelAllowingLossyCompression(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::IsChannelRenderable");
    }

    bool IsChannelUseZipForLosslessCompression(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::IsChannelUseZipForLosslessCompression");
    }

    OpenVDS::VolumeDataMapping GetChannelMapping(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelMapping");
    }

    int GetDimensionNumSamples(int dimension) const;

    const char* GetDimensionName(int dimension) const;

    const char* GetDimensionUnit(int dimension) const;

    float GetDimensionMin(int dimension) const;

    float GetDimensionMax(int dimension) const;

    float GetDimensionIndexOffset_a(int dimension) const;

    float GetDimensionIndexOffset_b(int dimension) const;

    OpenVDS::VDSIJKGridDefinition GetVDSIJKGridDefinitionFromMetadata() const;

    bool IsChannelUseNoValue(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::IsChannelUseNoValue");
    }

    float GetChannelNoValue(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelNoValue");
    }

    float GetChannelIntegerScale(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelIntegerScale");
    }

    float GetChannelIntegerOffset(int channel) const {
        throw std::runtime_error("Not implemented: DoubleVolumeDataLayout::GetChannelIntegerOffset");
    }

    OpenVDS::VolumeDataLayout const* const GetLayoutA() {
        return this->m_layout_a;
    }

    OpenVDS::VolumeDataLayout const* const GetLayoutB() {
        return this->m_layout_b;
    }

    // MetadataReadAccess
    void GetMetadataBLOB(const char* category, const char* name, const void** data, size_t* size) const { throw std::runtime_error("Not implemented"); }
    bool IsMetadataIntAvailable(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }           ///< Returns true if a metadata int with the given category and name is available
    bool IsMetadataIntVector2Available(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }    ///< Returns true if a metadata IntVector2 with the given category and name is available
    bool IsMetadataIntVector3Available(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }    ///< Returns true if a metadata IntVector3 with the given category and name is available
    bool IsMetadataIntVector4Available(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }    ///< Returns true if a metadata IntVector4 with the given category and name is available
    bool IsMetadataFloatAvailable(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }         ///< Returns true if a metadata float with the given category and name is available
    bool IsMetadataFloatVector2Available(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }  ///< Returns true if a metadata FloatVector2 with the given category and name is available
    bool IsMetadataFloatVector3Available(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }  ///< Returns true if a metadata FloatVector3 with the given category and name is available
    bool IsMetadataFloatVector4Available(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }  ///< Returns true if a metadata FloatVector4 with the given category and name is available
    bool IsMetadataDoubleAvailable(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }        ///< Returns true if a metadata double with the given category and name is available
    bool IsMetadataDoubleVector2Available(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns true if a metadata DoubleVector2 with the given category and name is available
    bool IsMetadataDoubleVector3Available(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns true if a metadata DoubleVector3 with the given category and name is available
    bool IsMetadataDoubleVector4Available(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns true if a metadata DoubleVector4 with the given category and name is available
    bool IsMetadataStringAvailable(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }        ///< Returns true if a metadata string with the given category and name is available
    bool IsMetadataBLOBAvailable(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }          ///< Returns true if a metadata BLOB with the given category and name is available

    int GetMetadataInt(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }                        ///< Returns the metadata int with the given category and name
    OpenVDS::IntVector2 GetMetadataIntVector2(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns the metadata IntVector2 with the given category and name
    OpenVDS::IntVector3 GetMetadataIntVector3(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns the metadata IntVector3 with the given category and name
    OpenVDS::IntVector4 GetMetadataIntVector4(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns the metadata IntVector4 with the given category and name

    float GetMetadataFloat(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }                        ///< Returns the metadata float with the given category and name
    OpenVDS::FloatVector2 GetMetadataFloatVector2(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns the metadata FloatVector2 with the given category and name
    OpenVDS::FloatVector3 GetMetadataFloatVector3(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns the metadata FloatVector3 with the given category and name
    OpenVDS::FloatVector4 GetMetadataFloatVector4(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns the metadata FloatVector4 with the given category and name

    double GetMetadataDouble(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); }                        ///< Returns the metadata double with the given category and name
    OpenVDS::DoubleVector2 GetMetadataDoubleVector2(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns the metadata DoubleVector2 with the given category and name
    OpenVDS::DoubleVector3 GetMetadataDoubleVector3(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns the metadata DoubleVector3 with the given category and name
    OpenVDS::DoubleVector4 GetMetadataDoubleVector4(const char* category, const char* name) const { throw std::runtime_error("Not implemented"); } ///< Returns the metadata DoubleVector4 with the given category and name

    const char* GetMetadataString(const char* category, const char* name) const {
        return "";
        // throw std::runtime_error("Not implemented");
    }
    OpenVDS::MetadataKeyRange GetMetadataKeys() const { throw std::runtime_error("Not implemented"); }

private:
    int32_t m_dimensionNumSamples[Dimensionality_Max];
    int32_t m_dimensionCoordinateMin[Dimensionality_Max];
    int32_t m_dimensionCoordinateMax[Dimensionality_Max];
    int32_t m_dimensionStepSize[Dimensionality_Max];
    int32_t m_dimensionIndexOffset_a[Dimensionality_Max];
    int32_t m_dimensionIndexOffset_b[Dimensionality_Max];

    int32_t m_iline_index;
    int32_t m_xline_index;
    int32_t m_sample_index;

    OpenVDS::VolumeDataLayout const* const m_layout_a;
    OpenVDS::VolumeDataLayout const* const m_layout_b;
};

#endif /* VDL_VOLUME_DATA_LAYOUT_HPP */
