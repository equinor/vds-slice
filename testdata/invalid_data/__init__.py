from enum import Enum, unique
import common

from .make_vds_with_axes_in_same_direction import make_two_axes_in_same_direction
from .make_vds_with_invalid_axis import make_unknown_axis
from .make_vds_with_negative_stepsize import make_negative_stepsize


@unique
class InvalidData(Enum):
    BAD_AXIS_NAME = "bad_axis_name"
    SAME_DIRECTION_AXES = "same_direction_axes"
    NEGATIVE_STEPSIZE = "negative_stepsize"


class InvalidDataCategory(common.Category):
    def __init__(self):
        super().__init__(
            name="invalid_data",
            description="Files that should not be processed correctly.",
            dirpath="invalid_data"
        )

    def specifications(self):
        return [
            common.CommandSpecification(
                case=InvalidData.BAD_AXIS_NAME.value,
                description="VDS file with an invalid axis name",
                filename="invalid_axis_name.vds",
            ),
            common.CommandSpecification(
                case=InvalidData.SAME_DIRECTION_AXES.value,
                description="VDS file with two axes in same direction (sample axis)",
                filename="invalid_same_axes_direction.vds",
            ),
            common.CommandSpecification(
                case=InvalidData.NEGATIVE_STEPSIZE.value,
                description="VDS file with inline axis max value < min value",
                filename="negative_stepsize.vds",
            ),
        ]

    def generate_testdata(self, testdata: common.TestdataCase):
        match InvalidData(testdata.case):
            case InvalidData.BAD_AXIS_NAME:
                make_unknown_axis(testdata.filepath)
            case InvalidData.SAME_DIRECTION_AXES:
                make_two_axes_in_same_direction(testdata.filepath)
            case InvalidData.NEGATIVE_STEPSIZE:
                make_negative_stepsize(testdata.filepath)
            case _:
                raise ValueError("Not implemented")
