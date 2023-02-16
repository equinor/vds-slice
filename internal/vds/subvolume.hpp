#ifndef VDS_SLICE_SUBVOLUME_HPP
#define VDS_SLICE_SUBVOLUME_HPP

#include <OpenVDS/OpenVDS.h>

#include "metadatahandle.hpp"

struct SubVolume {
    struct {
        int lower[OpenVDS::VolumeDataLayout::Dimensionality_Max]{0, 0, 0, 0, 0, 0};
        int upper[OpenVDS::VolumeDataLayout::Dimensionality_Max]{1, 1, 1, 1, 1, 1};
    } bounds;

    SubVolume(MetadataHandle const& metadata) {
        auto const& iline  = metadata.iline();
        auto const& xline  = metadata.xline();
        auto const& sample = metadata.sample();

        this->bounds.upper[iline.dimension() ] = iline.nsamples();
        this->bounds.upper[xline.dimension() ] = xline.nsamples();
        this->bounds.upper[sample.dimension()] = sample.nsamples();
    }
};

#endif /* VDS_SLICE_SUBVOLUME_HPP */
