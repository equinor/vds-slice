"""
This script is designed to convert a SEGY file into a VDS file format.
One of the features that it offers is the ability to customize the order of the 
"xline," "inline," and "sample" values to suit your specific needs. 

To change the order, set the desired order at line [60] in "axisDescriptor" 
variable and run the script.

Usage:
    python make_well_known_custom_order.py [--segyfile][--vdsfile][--crsWkt]

Options:
    --segyfile [Requird]
    --vdsfile  [default: "well_known_custom_order.vds"]
    --crsWkt   [default: "utmXX"]

"""
import argparse
import datetime
import itertools
import numpy as np
import os
import openvds
import segyio


def create_defined(segy_filename: str, vds_filename: str, crs_wkt: str) -> None:
    src = segyio.open(segy_filename)

    layoutDescriptor = openvds.VolumeDataLayoutDescriptor(
        openvds.VolumeDataLayoutDescriptor.BrickSize.BrickSize_64,
        0,
        0,
        4,
        openvds.VolumeDataLayoutDescriptor.LODLevels.LODLevels_None,
        openvds.VolumeDataLayoutDescriptor.Options.Options_None,
    )

    compressionMethod = openvds.CompressionMethod(0)
    compressionTolerance = 0.0
    inline_axis = openvds.VolumeDataAxisDescriptor(
        len(src.ilines),
        openvds.KnownAxisNames.inline(),
        openvds.KnownUnitNames.unitless(),
        src.ilines[0],
        src.ilines[-1],
    )
    xline_axis = openvds.VolumeDataAxisDescriptor(
        len(src.xlines),
        openvds.KnownAxisNames.crossline(),
        openvds.KnownUnitNames.unitless(),
        src.xlines[0],
        src.xlines[-1],
    )
    sample_axis = openvds.VolumeDataAxisDescriptor(
        len(src.samples),
        openvds.KnownAxisNames.sample(),
        openvds.KnownUnitNames.millisecond(),
        src.samples[0],
        src.samples[-1],
    )
    # Set desird order of axis here
    axisDescriptors = [
        xline_axis,
        inline_axis,
        sample_axis,
    ]

    source_data = segyio.tools.cube(src)
    channelDescriptors = [
        openvds.VolumeDataChannelDescriptor(
            openvds.VolumeDataChannelDescriptor.Format.Format_R32,
            openvds.VolumeDataChannelDescriptor.Components.Components_1,
            "Amplitude",  # Channel Name
            "",  # Unit
            np.min(source_data),
            np.max(source_data),
        )
    ]

    cdp_x = segyio.su.cdpx
    cdp_y = segyio.su.cdpy

    origin_header = src.header[0]
    scale = abs(origin_header[segyio.su.scalco])

    # casting genrator to list causing problem so we iterating using next fuction
    il = src.header.iline[src.ilines[0]]
    il_header = next(il)
    il_header = next(il)

    xl = src.header.xline[src.xlines[0]]
    xl_dif = np.diff(src.xlines).tolist()

    # Calculating xline spacing the distance between the point in xlines direction 
    xline_spacing = (
        ((il_header[cdp_x] / scale) - (origin_header[cdp_x] / scale)) / xl_dif[0], 
        ((il_header[cdp_y] / scale) - (origin_header[cdp_y] / scale)) / xl_dif[0]
    )

    xl_header = next(xl)  # TODO need to fix this logic and make it genaric
    xl_header = next(xl)

    il_dif = np.diff(src.ilines).tolist()
    
    # Calculating iline spacing the distance between the point in ilines direction 
    iline_spacing = (
        ((xl_header[cdp_x] / scale) - (origin_header[cdp_x] / scale)) / il_dif[0],
        ((xl_header[cdp_y] / scale) - (origin_header[cdp_y] / scale)) / il_dif[0]
    )

    segy_origin = [origin_header[cdp_x] / scale, origin_header[cdp_y] / scale]
    
    # Calculating the vds origin point from segy origin and inlines and xlines spacing 
    iline_first_indx = src.ilines[0]
    xline_first_indx = src.xlines[0]

    while iline_first_indx:  # moving in inline dirction toward the origin
        vds_origin = [x - y for x, y in zip(segy_origin, iline_spacing)]
        iline_first_indx = iline_first_indx - 1

    while xline_first_indx:  # moving in xline dirction toward the origin
        vds_origin = [x - y for x, y in zip(vds_origin, xline_spacing)]
        xline_first_indx = xline_first_indx - 1

    metaData = openvds.MetadataContainer()
    metaData.setMetadataDoubleVector2(
        openvds.KnownMetadata.surveyCoordinateSystemOrigin().category,
        openvds.KnownMetadata.surveyCoordinateSystemOrigin().name,
        vds_origin,
    )
    metaData.setMetadataDoubleVector2(
        openvds.KnownMetadata.surveyCoordinateSystemInlineSpacing().category,
        openvds.KnownMetadata.surveyCoordinateSystemInlineSpacing().name,
        iline_spacing,
    )
    metaData.setMetadataDoubleVector2(
        openvds.KnownMetadata.surveyCoordinateSystemCrosslineSpacing().category,
        openvds.KnownMetadata.surveyCoordinateSystemCrosslineSpacing().name,
        xline_spacing,
    )
    metaData.setMetadataString(
        openvds.KnownMetadata.surveyCoordinateSystemUnit().category,
        openvds.KnownMetadata.surveyCoordinateSystemUnit().name,
        "m",
    )
    metaData.setMetadataString(
        openvds.KnownMetadata.surveyCoordinateSystemCRSWkt().category,
        openvds.KnownMetadata.surveyCoordinateSystemCRSWkt().name,
        crs_wkt,
    )
    metaData.setMetadataString(
        openvds.KnownMetadata.importInformationInputFileName().category,
        openvds.KnownMetadata.importInformationInputFileName().name,
        segy_filename,
    )
    metaData.setMetadataString(
        openvds.KnownMetadata.importInformationDisplayName().category,
        openvds.KnownMetadata.importInformationDisplayName().name,
        segy_filename,
    )
    metaData.setMetadataString(
        openvds.KnownMetadata.importInformationInputTimeStamp().category,
        openvds.KnownMetadata.importInformationInputTimeStamp().name,
        datetime.datetime.now().isoformat(), #TODO get input time from segy file
    )
    metaData.setMetadataString(
        openvds.KnownMetadata.importInformationImportTimeStamp().category,
        openvds.KnownMetadata.importInformationImportTimeStamp().name,
        datetime.datetime.now().isoformat(),
    )
    metaData.setMetadataString(
        openvds.KnownMetadata.importInformationInputFileSize().category,
        openvds.KnownMetadata.importInformationInputFileSize().name,
        str(os.path.getsize(segy_filename)),
    )
    vds = openvds.create(
        vds_filename,
        "",  # connection string
        layoutDescriptor,
        axisDescriptors,
        channelDescriptors,
        metaData,
        compressionMethod,
        compressionTolerance,
    )

    manager = openvds.getAccessManager(vds)
    accessor = manager.createVolumeDataPageAccessor(
        openvds.DimensionsND.Dimensions_012,
        0,  # LOD
        0,  # channel
        8,  # Max pages in cache
        openvds.IVolumeDataAccessManager.AccessMode.AccessMode_Create,
    )

    for c in range(accessor.getChunkCount()):
        page = accessor.createPage(c)
        buf = np.array(page.getWritableBuffer(), copy=False)
        # We are using all the possible permutations of all 3 axes to adjust data
        # in such a way that the data dimensions are aline with the buffer dimensions
        dimensions_perm = list(itertools.permutations((0, 1, 2)))
        data = next(
            (
                np.transpose(source_data, dim)
                for dim in dimensions_perm
                if buf.shape == np.transpose(source_data, dim).shape
            ),
            None,
        )

        buf = data
        page.release()

    accessor.commit()
    openvds.close(vds)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="The following script converts the SEGY file to VDS format."
    )
    parser.add_argument(
        "-s",
        "--segyfile",
        type=str,
        help="segy file name"
    )
    parser.add_argument(
        "-v",
        "--vdsfile",
        type=str,
        default="well_known_custom_order.vds",
        help="Name of the new vds file",
    )
    parser.add_argument(
        "-c",
        "--crsWkt",
        type=str,
        default="utmXX",
        help="SurveyCoordinateSystem"
    )
    args = parser.parse_args()

    create_defined(args.segyfile, args.vdsfile, args.crsWkt)
