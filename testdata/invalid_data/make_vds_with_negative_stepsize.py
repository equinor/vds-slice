from common.make_vds import *

def make_negative_stepsize(filename: str) -> None:
    data = np.array(
        [
            [[200, 201], [202, 203]],
            [[204, 205], [206, 207]],
            [[208, 209], [210, 211]],
            [[212, 213], [214, 215]]
        ])
    axes = [
        Config.Axis.from_values([16, 15], "Inline", "unitless"),
        Config.Axis.from_values([79, 80], "Crossline", "unitless"),
        Config.Axis.from_values([10, 20, 30, 40], "Depth", "unitless"),
    ]

    config = Config(data, axes)
    create_vds(filename, config)
