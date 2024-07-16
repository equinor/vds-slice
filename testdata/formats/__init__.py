from pathlib import Path
import tempfile

import common

from .make_formats import create_with_format


supported_formats = [1, 2, 3, 5, 8, 10, 11, 16]


class FormatsCategory(common.Category):
    def __init__(self):
        super().__init__(
            name="formats",
            description="Files with various segy formats converted into vds.",
            dirpath="formats"
        )

    def specifications(self):
        specs = []
        for format in supported_formats:
            specs.append(
                common.CommandSpecification(
                    case=str(format),
                    description=f"File with values of segy type {format}",
                    filename=f"format{format}.vds",
                )
            )
        return specs

    def generate_testdata(self, testdata: common.TestdataCase):
        with tempfile.TemporaryDirectory() as tmp:
            segy_path = Path(tmp) / 'temp.segy'
            create_with_format(segy_path, testdata.case)
            common.segy_to_vds(testdata, segy_path)
