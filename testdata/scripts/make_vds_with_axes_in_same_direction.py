import argparse
from make_vds import *


def make_unknown_axis(filename: str) -> None:
    """
    Create a VDS file with two axes describing the same dimension.

    Args:
    filename: The filename of the output VDS file.

    Returns:
    None.

    This function creates a NumPy array with shape (4, 2, 2), assigns metadata to 3 axes with
    inline, time and depth names and creates a VDS file with the specified output filename.
    """
    data = np.array(
        [
            [[200, 201], [202, 203]],
            [[204, 205], [206, 207]],
            [[208, 209], [210, 211]],
            [[212, 213], [214, 215]]
        ])
    axes = [
        Config.Axis.from_values([15, 16], "Time", "ms"),
        Config.Axis.from_values([79, 80], "Depth", "m"),
        Config.Axis.from_values([1, 2, 3, 4], "Inline", "unitless"),
    ]

    config = Config(data, axes)
    create_vds(filename, config)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="The following script will generate a VDS file with two axes in same direction (sample axis) "
    )
    parser.add_argument(
        "-v",
        "--vdsfile",
        type=str,
        default="invalid_same_axes_direction.vds",
        help="Name of the new vds file",
    )
    args = parser.parse_args()
    make_unknown_axis(filename=args.vdsfile)
