import argparse
import segyio
from make_vds import Config, create_vds


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


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="The script will generate a VDS file with depth axis in place of sample. \
            Given segy file is expected to be structured as well_known files"
    )
    parser.add_argument(
        "-s",
        "--segyfile",
        type=str,
        help="SEGY file name"
    )
    parser.add_argument(
        "-v",
        "--vdsfile",
        type=str,
        default="well_known_depth_axis.vds",
        help="Name of the new vds file",
    )
    args = parser.parse_args()
    turn_to_depth_axis(
        segy_filename=args.segyfile, vds_filename=args.vdsfile
    )

# cd ../well_known
# python make_well_known.py well_known.segy
# python ../scripts/make_well_known_depth_sample_axis.py -s well_known.segy
