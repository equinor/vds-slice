import datetime
import numpy as np
import os
import openvds
import segyio
import typing


class Config:
    class Axis:
        def __init__(self, size, min, max, name, units):
            self.size  = size
            self.min   = min
            self.max   = max
            self.name  = name
            self.units = units

        @classmethod
        def from_values(cls, values, name, units):
            return cls(len(values), values[0], values[-1], name, units)

        @classmethod
        def inline(cls, segy: segyio.SegyFile):
            return Config.Axis.from_values(
                segy.ilines,
                openvds.KnownAxisNames.inline(),
                openvds.KnownUnitNames.unitless(),
            )

        @classmethod
        def xline(cls, segy: segyio.SegyFile):
            return Config.Axis.from_values(
                segy.xlines,
                openvds.KnownAxisNames.crossline(),
                openvds.KnownUnitNames.unitless(),
            )

        @classmethod
        def time(cls, segy: segyio.SegyFile, unit=openvds.KnownUnitNames.millisecond()):
            return Config.Axis.from_values(
                segy.samples,
                openvds.KnownAxisNames.time(),
                unit,
            )

        @classmethod
        def depth(cls, segy: segyio.SegyFile, unit=openvds.KnownUnitNames.meter()):
            return Config.Axis.from_values(
                segy.samples,
                openvds.KnownAxisNames.depth(),
                unit,
            )

        @classmethod
        def samples(
            cls, segy: segyio.SegyFile, unit=openvds.KnownUnitNames.millisecond()
        ):
            return Config.Axis.from_values(
                segy.samples,
                openvds.KnownAxisNames.sample(),
                unit,
            )

    class CDP:
        def __init__(self, vds_origin, inline_spacing, xline_spacing):
            self.vds_origin     = vds_origin
            self.inline_spacing = inline_spacing
            self.xline_spacing  = xline_spacing

        @classmethod
        def from_segy(cls, segy: segyio.SegyFile):
            cdp_x = segyio.su.cdpx
            cdp_y = segyio.su.cdpy

            origin_header = segy.header[0]
            scaler = abs(origin_header[segyio.su.scalco])
            # As per the SEGY specifications, if the scaler value is negative,
            # it should be divided, whereas if the value is positive, it should
            # be multiplied. Since we only use negative scaler values, our
            # computation logic only involves division. However, if we encounter
            # positive scaler values, the computation logic will need to be
            # adjusted to apply multiplication instead.
            # ref https://library.seg.org/pb-assets/technical-standards/seg_y_rev1-1686080991247.pdf
            # (page 14, line 71-72)

            # Casting the generator into a list causes problems so we iterate using next function
            ilines = segy.header.iline[segy.ilines[0]]
            ilheader = next(ilines)
            ilheader = next(ilines)

            xlines = segy.header.xline[segy.xlines[0]]
            xldiff = segy.xlines[1] - segy.xlines[0]

            # Calculating crosslines spacing the distance between the points in crosslines direction
            xlines_spacing = (
                ((ilheader[cdp_x] / scaler) - (origin_header[cdp_x] / scaler))
                / xldiff,
                ((ilheader[cdp_y] / scaler) - (origin_header[cdp_y] / scaler))
                / xldiff,
            )

            xlheader = next(xlines)
            xlheader = next(xlines)

            ildiff = segy.ilines[1] - segy.ilines[0]

            # Calculating inlines spacing the distance between the points in inlines direction
            ilines_spacing = (
                ((xlheader[cdp_x] / scaler) - (origin_header[cdp_x] / scaler))
                / ildiff,
                ((xlheader[cdp_y] / scaler) - (origin_header[cdp_y] / scaler))
                / ildiff,
            )

            segy_origin = [origin_header[cdp_x] / scaler, origin_header[cdp_y] / scaler]

            # Calculating the VDS origin point from SEGY origin and inlines and xlines spacing
            ilines_untill_0 = segy.ilines[0]
            xlines_untill_0 = segy.xlines[0]

            vds_origin = segy_origin
            while ilines_untill_0:  # moving in inline direction toward the origin
                vds_origin = [x - y for x, y in zip(vds_origin, ilines_spacing)]
                ilines_untill_0 = ilines_untill_0 - 1

            while xlines_untill_0:  # moving in xline direction toward the origin
                vds_origin = [x - y for x, y in zip(vds_origin, xlines_spacing)]
                xlines_untill_0 = xlines_untill_0 - 1

            return cls(vds_origin, ilines_spacing, xlines_spacing)

    class ImportInfo:
        def __init__(
            self,
            file_name,
            display_name,
            input_file_size,
            input_timestamp=datetime.datetime.now().isoformat(),
            import_timestamp=datetime.datetime.now().isoformat(),
        ):
            self.file_name        = file_name
            self.display_name     = display_name
            self.input_file_size  = input_file_size
            self.input_timestamp  = input_timestamp
            self.import_timestamp = import_timestamp

        @classmethod
        def from_file(cls, filename: str):
            return cls(
                filename,
                filename,
                str(os.path.getsize(filename)),
                datetime.datetime.now().isoformat(),
                datetime.datetime.now().isoformat(),
            )

    def __init__(
        self,
        data,
        axes: typing.List[Axis],
        cdp: CDP = CDP([0, 0], [1, 0], [0, 1]),
        import_info: ImportInfo = ImportInfo("unknown", "UFO", "666"),
    ):
        self.axes = axes
        self.cdp  = cdp
        self.data = data
        self.import_info = import_info


def create_vds(vds_filename: str, config: Config) -> None:
    """
    Create a VDS file with specified metadata and data.

    Args:
    vds_filename (str): The filename of the output VDS file.
    config (Config): The configuration object for VDS.

    Returns:
    None.

    This function creates a VDS file with the given filename and configuration.
    It assigns layouts to the VDS, specifies the data format, creates metadata
    for survey coordinates, input files, and display names, and writes data to
    the VDS.

    """
    layout_descriptor = openvds.VolumeDataLayoutDescriptor(
        openvds.VolumeDataLayoutDescriptor.BrickSize.BrickSize_64,
        0,
        0,
        4,
        openvds.VolumeDataLayoutDescriptor.LODLevels.LODLevels_None,
        openvds.VolumeDataLayoutDescriptor.Options.Options_None,
    )

    axes_descriptors = list(
        map(
            lambda axis: openvds.VolumeDataAxisDescriptor(
                axis.size,
                axis.name,
                axis.units,
                axis.min,
                axis.max,
            ),
            config.axes,
        )
    )
    channel_descriptors = [
        openvds.VolumeDataChannelDescriptor(
            openvds.VolumeDataChannelDescriptor.Format.Format_R32,
            openvds.VolumeDataChannelDescriptor.Components.Components_1,
            "Amplitude",  # Channel Name
            "",  # Unit
            np.min(config.data),
            np.max(config.data),
        )
    ]

    meta_data = openvds.MetadataContainer()
    meta_data.setMetadataDoubleVector2(
        openvds.KnownMetadata.surveyCoordinateSystemOrigin().category,
        openvds.KnownMetadata.surveyCoordinateSystemOrigin().name,
        config.cdp.vds_origin,
    )
    meta_data.setMetadataDoubleVector2(
        openvds.KnownMetadata.surveyCoordinateSystemInlineSpacing().category,
        openvds.KnownMetadata.surveyCoordinateSystemInlineSpacing().name,
        config.cdp.inline_spacing,
    )
    meta_data.setMetadataDoubleVector2(
        openvds.KnownMetadata.surveyCoordinateSystemCrosslineSpacing().category,
        openvds.KnownMetadata.surveyCoordinateSystemCrosslineSpacing().name,
        config.cdp.xline_spacing,
    )
    meta_data.setMetadataString(
        openvds.KnownMetadata.surveyCoordinateSystemUnit().category,
        openvds.KnownMetadata.surveyCoordinateSystemUnit().name,
        "m",
    )
    meta_data.setMetadataString(
        openvds.KnownMetadata.surveyCoordinateSystemCRSWkt().category,
        openvds.KnownMetadata.surveyCoordinateSystemCRSWkt().name,
        "utmXX",
    )
    meta_data.setMetadataString(
        openvds.KnownMetadata.importInformationInputFileName().category,
        openvds.KnownMetadata.importInformationInputFileName().name,
        config.import_info.file_name,
    )
    meta_data.setMetadataString(
        openvds.KnownMetadata.importInformationDisplayName().category,
        openvds.KnownMetadata.importInformationDisplayName().name,
        config.import_info.display_name,
    )
    meta_data.setMetadataString(
        openvds.KnownMetadata.importInformationInputTimeStamp().category,
        openvds.KnownMetadata.importInformationInputTimeStamp().name,
        config.import_info.input_timestamp,
    )
    meta_data.setMetadataString(
        openvds.KnownMetadata.importInformationImportTimeStamp().category,
        openvds.KnownMetadata.importInformationImportTimeStamp().name,
        config.import_info.import_timestamp,
    )
    meta_data.setMetadataString(
        openvds.KnownMetadata.importInformationInputFileSize().category,
        openvds.KnownMetadata.importInformationInputFileSize().name,
        config.import_info.input_file_size,
    )
    vds = openvds.create(
        vds_filename,
        "",  # connection string
        layout_descriptor,
        axes_descriptors,
        channel_descriptors,
        meta_data,
        openvds.CompressionMethod(0),
        0.0,  # Compression Tolerance
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
        (min, max) = page.getMinMax()
        buf[:, :, :] = config.data[min[2] : max[2], min[1] : max[1], min[0] : max[0]]

        page.release()

    accessor.commit()
    openvds.close(vds)
