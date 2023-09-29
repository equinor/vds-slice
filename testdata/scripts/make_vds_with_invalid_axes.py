import argparse
from make_vds import *


def make_unknown_axis(filename: str) -> None:
    """
    Create a VDS file with unknown axis names.

    Args:
    filename: The filename of the output VDS file.

    Returns:
    None.

    This function creates a NumPy array with shape (4, 1, 2), assigns metadata to 3 axes with
    inline, crossline and one invalid axis name and creates a VDS file with the specified output filename.
    """
    data = np.array([[[200, 201]], [[202, 203]], [[204, 205]], [[206, 207]]])
    axes = [
        Config.Axis.from_values([15, 16], "Inline", "unitless"),
        Config.Axis.from_values([79], "Crossline", "unitless"),
        Config.Axis.from_values([1, 2, 3, 4], "Evil", "unitless"),
    ]

    config = Config(data, axes)
    create_vds(filename, config)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="The following script will genrate a VDS file with an invalid axis name"
    )
    parser.add_argument(
        "-v",
        "--vdsfile",
        type=str,
        default="invalid_axes_name.vds",
        help="Name of the new vds file",
    )
    args = parser.parse_args()
    make_unknown_axis(filename=args.vdsfile)
