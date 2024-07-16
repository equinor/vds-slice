import segyio
import numpy as np

from common.make_vds import Config, create_vds


def turn_to_depth_axis(segy_filename: str, vds_filename: str) -> None:
    """
    Turns segy file into vds, makes sample axis to depth
    """
    src = segyio.open(segy_filename)
    data = segyio.tools.cube(src)
    depth_axis = Config.Axis.samples(src)
    depth_axis.units = "m"
    axes = [depth_axis, Config.Axis.xline(src), Config.Axis.inline(src)]
    cdp = Config.CDP.from_segy(src)
    import_info = Config.ImportInfo.from_file(segy_filename)
    config = Config(data, axes, cdp, import_info)
    create_vds(vds_filename, config)
    src.close()


def make_well_known_custom_axis_order(segy_filename: str, vds_filename: str) -> None:
    """
    Create a VDS file with custom axis order with well-known seismic data.

    Args:
    segy_filename: The filename of the input SEGY file.
    vds_filename : The filename of the output VDS file.

    Returns:
    None.

    This function opens a SEGY file with segyio, creates a cube with transposed
    axes, and creates a VDS file with create_vds with the user provided output
    filename.
    """
    src = segyio.open(segy_filename)
    data = np.transpose(segyio.tools.cube(src), (2, 0, 1))
    axes = [Config.Axis.xline(src), Config.Axis.inline(src), Config.Axis.samples(src)]
    cdp = Config.CDP.from_segy(src)
    import_info = Config.ImportInfo.from_file(segy_filename)
    config = Config(data, axes, cdp, import_info)
    create_vds(vds_filename, config)
    src.close()


def shift_origin(cdp):
    cdp.vds_origin[0] += cdp.inline_spacing[0] + cdp.xline_spacing[0]
    cdp.vds_origin[1] += cdp.inline_spacing[1] + cdp.xline_spacing[1]


def double_inline_spacing(cdp):
    cdp.inline_spacing = tuple(x * 2 for x in cdp.inline_spacing)


def double_xline_spacing(cdp):
    cdp.xline_spacing = tuple(x * 2 for x in cdp.xline_spacing)


def shift_grid(segy_filename: str, vds_filename: str, func) -> None:
    src = segyio.open(segy_filename)
    data = segyio.tools.cube(src)
    axes = [
        Config.Axis.samples(src),
        Config.Axis.xline(src),
        Config.Axis.inline(src)
    ]
    cdp = Config.CDP.from_segy(src)
    func(cdp)
    import_info = Config.ImportInfo.from_file(segy_filename)
    config = Config(data, axes, cdp, import_info)
    create_vds(vds_filename, config)
    src.close()
