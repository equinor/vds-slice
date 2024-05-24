import argparse
import segyio
from make_vds import Config, create_vds


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


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="The script will generate 3 VDS files with changed grid. \
            Given segy file is expected to be structured as well_known files"
    )
    parser.add_argument(
        "-s",
        "--segyfile",
        type=str,
        help="SEGY file name"
    )
    args = parser.parse_args()

    custom_origin = "well_known_custom_origin.vds"
    custom_inline_spacing = "well_known_custom_inline_spacing.vds"
    custom_xline_spacing = "well_known_custom_xline_spacing.vds"

    shift_grid(
        segy_filename=args.segyfile, vds_filename=custom_origin, func=shift_origin
    )

    shift_grid(
        segy_filename=args.segyfile, vds_filename=custom_inline_spacing, func=double_inline_spacing
    )

    shift_grid(
        segy_filename=args.segyfile, vds_filename=custom_xline_spacing, func=double_xline_spacing
    )

# cd ../well_known
# python make_well_known.py well_known.segy
# python ../scripts/make_well_known_custom_grid.py -s well_known.segy
