from common.make_vds import *

def make_two_axes_in_same_direction(filename: str) -> None:
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
