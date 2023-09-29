import argparse
from make_vds import *


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


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="""
        The following script converts the SEGY file into a VDS file with different
        axes order.
        """
    )
    parser.add_argument("-s", "--segyfile", type=str, help="SEGY file name")
    parser.add_argument(
        "-v",
        "--vdsfile",
        type=str,
        default="well_known_custom_axis_order.vds",
        help="Name of the new vds file",
    )
    args = parser.parse_args()
    make_well_known_custom_axis_order(
        segy_filename=args.segyfile, vds_filename=args.vdsfile
    )
