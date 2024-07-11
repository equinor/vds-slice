
from enum import Enum, unique
from pathlib import Path
import tempfile
import common

from .make_well_known import create_defined
from .transform_well_known import *


@unique
class WellKnown(Enum):
    DEFAULT = "default"
    DEPTH_SAMPLE_AXIS = "depth_sample_axis"
    CUSTOM_AXIS_ORDER = "custom_axis_order"
    CUSTOM_ORIGIN = "custom_origin"
    CUSTOM_INLINE_SPACING = "custom_inline_spacing"
    CUSTOM_XLINE_SPACING = "custom_xline_spacing"


class WellKnownCategory(common.Category):
    def __init__(self):
        super().__init__(
            name="well_known",
            description="Related to well known data (simple data with clear samples)",
            dirpath="well_known"
        )

    def specifications(self):
        return [
            common.CommandSpecification(
                case=WellKnown.DEFAULT.value,
                description="Default well known test file",
                filename="well_known_default.vds",
                cloud_path="testdata/well_known/well_known_default"
            ),
            common.CommandSpecification(
                case=WellKnown.DEPTH_SAMPLE_AXIS.value,
                description="As default, but with depth axis instead of time",
                filename="well_known_depth_axis.vds"
            ),
            common.CommandSpecification(
                case=WellKnown.CUSTOM_AXIS_ORDER.value,
                description="As default, but with axis order other than sample-xline-inline",
                filename="well_known_custom_axis_order.vds"
            ),
            common.CommandSpecification(
                case=WellKnown.CUSTOM_ORIGIN.value,
                description="As default, but with different origin",
                filename="well_known_custom_origin.vds"
            ),
            common.CommandSpecification(
                case=WellKnown.CUSTOM_INLINE_SPACING.value,
                description="As default, but with different inline spacing",
                filename="well_known_custom_inline_spacing.vds"
            ),
            common.CommandSpecification(
                case=WellKnown.CUSTOM_XLINE_SPACING.value,
                description="As default, but with different xline spacing",
                filename="well_known_custom_xline_spacing.vds"
            ),
        ]

    def generate_testdata(self, testdata: common.TestdataCase):
        with tempfile.TemporaryDirectory() as tmp:
            segy_path = str(Path(tmp) / "well_known.segy")
            create_defined(segy_path)

            match WellKnown(testdata.case):
                case WellKnown.DEFAULT:
                    common.segy_to_vds(testdata, segy_path, crs="utmXX")
                case WellKnown.DEPTH_SAMPLE_AXIS:
                    turn_to_depth_axis(segy_path, testdata.filepath)
                case WellKnown.CUSTOM_AXIS_ORDER:
                    make_well_known_custom_axis_order(
                        segy_path, testdata.filepath)
                case WellKnown.CUSTOM_ORIGIN:
                    shift_grid(segy_path, testdata.filepath, func=shift_origin)
                case WellKnown.CUSTOM_INLINE_SPACING:
                    shift_grid(segy_path, testdata.filepath,
                               func=double_inline_spacing)
                case WellKnown.CUSTOM_XLINE_SPACING:
                    shift_grid(segy_path, testdata.filepath,
                               func=double_xline_spacing)
                case _:
                    raise ValueError("Not implemented")
