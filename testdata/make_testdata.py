import argparse
import os
from typing import List

from common import *

from well_known import WellKnownCategory
from samples10 import Samples10Category
from varsize import VarsizeCategory
from formats import FormatsCategory
from cube_intersection import CubeIntersectionCategory
from invalid_data import InvalidDataCategory


categories: List[common.Category] = [
    WellKnownCategory(),
    Samples10Category(),
    VarsizeCategory(),
    FormatsCategory(),
    CubeIntersectionCategory(),
    InvalidDataCategory(),
]


def define_common_args_parser():
    common_args_parser = argparse.ArgumentParser(add_help=False)
    common_args_parser.add_argument(
        "-o",
        "--openvds_dir_path",
        type=str,
        default="",
        help="Path to the directory containing openvds binaries",
    )
    return common_args_parser


def define_category_args_parser(category: common.Category):
    category_args_parser = argparse.ArgumentParser(add_help=False)
    category_args_parser.add_argument(
        "-d",
        "--dirpath",
        type=str,
        default=category.dirpath,
        help="Path to the directory where file will be created",
    )
    return category_args_parser


def define_specification_args_parser(specification: common.CommandSpecification):
    specification_args_parser = argparse.ArgumentParser(add_help=False)

    specification_args_parser.add_argument(
        "-f",
        "--filename",
        type=str,
        default=specification.filename,
        help="Name of the output vds file",
    )

    specification_args_parser.add_argument(
        "-c",
        "--cloud_path",
        type=str,
        default=specification.cloud_path,
        help="Path to the output vds file in the cloud, in form of [container]/[directories].",
    )

    specification_args_parser.add_argument(
        "-u",
        "--upload_to_cloud",
        action="store_true",
        help="""Whether file should be uploaded to cloud under cloud_path.
                If true, environment variables ACCOUNT_NAME (without .blob.core.windows.net postfix) and SAS must be set.
                """,
    )

    for custom in specification.custom_args:
        specification_args_parser.add_argument(
            f"--{custom.name}",
            type=custom.type,
            help=custom.description,
            required=custom.required
        )
    return specification_args_parser


tool_description = f"""
Entry point for VDS testdata file creation.

Choose category from {[category.name for category in categories]} or choose 'all' to run all eligible categories.
Follow the subcommand instructions to proceed.
Default parameters should be enough to replicate test setup, but override values can be useful for investigations.

Remember that each run modifies the testdata, as vds files contain creation time.

Upload to cloud is turned off by default, though cloud paths are provided for files used in e2e and performance testing.
If file needs updating it should be done between respective PR code review approval and merge.
"""


def define_parser():
    parser = argparse.ArgumentParser(
        description=tool_description, formatter_class=argparse.RawDescriptionHelpFormatter
    )

    subparsers = parser.add_subparsers(
        dest='category', description="Test data category:", required=True
    )

    common_args_parser = define_common_args_parser()

    for category in categories:
        specifications_description = "\n".join(
            [f""" - {specification.case}:
                {specification.description}""" for specification in category.specifications()]
        )
        category_description = f"""
{category.description}.

Choose category from:
{specifications_description}
{"or choose 'all' to run all of them." if category.all_allowed else ""}
"""
        category_parser = subparsers.add_parser(
            category.name, description=category_description,  formatter_class=argparse.RawDescriptionHelpFormatter
        )
        category_subparsers = category_parser.add_subparsers(
            dest='case', description='ID of the test data case.', required=True
        )

        category_args_parser = define_category_args_parser(category)

        specification: common.CommandSpecification
        for specification in category.specifications():
            spec_args_parser = define_specification_args_parser(specification)

            parents = [
                common_args_parser,
                category_args_parser,
                spec_args_parser
            ]

            category_subparsers.add_parser(
                specification.case,
                description=specification.description,
                parents=parents,
                formatter_class=argparse.ArgumentDefaultsHelpFormatter
            )

        if category.all_allowed:
            parents = [common_args_parser, category_args_parser]
            category_subparsers.add_parser(
                "all",
                description="Runs all cases in category and stores them under default names. Cloud upload is not supported.",
                parents=parents,
                formatter_class=argparse.ArgumentDefaultsHelpFormatter
            )

    parents = [common_args_parser]
    subparsers.add_parser(
        "all",
        description="""
        Runs all cases in all categories that support 'all' running and stores files under default directories and default filenames.
        Cloud upload is not supported.
        """,
        parents=parents,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    return parser


if __name__ == "__main__":
    parser = define_parser()
    args = parser.parse_args()

    if args.category == "all":
        for category in categories:
            if category.all_allowed:
                for specification in category.specifications():
                    filepath = str(Path(category.dirpath) /
                                   Path(specification.filename))

                    testdata_case = TestdataCase(
                        case=specification.case, filepath=filepath, openvds_dir_path=args.openvds_dir_path)
                    category.generate_testdata(testdata_case)
    else:
        category: Category = next(
            c for c in categories if c.name == args.category)
        if args.case == "all":
            for specification in category.specifications():
                filepath = str(Path(args.dirpath) /
                               Path(specification.filename))

                testdata_case = TestdataCase(
                    case=specification.case, filepath=filepath, openvds_dir_path=args.openvds_dir_path)
                category.generate_testdata(testdata_case)
        else:
            account_name = ""
            sas = ""
            if args.upload_to_cloud:
                account_name = os.getenv("ACCOUNT_NAME")
                sas = os.getenv("SAS")

                if not account_name or not sas:
                    raise ValueError(
                        "If upload to cloud is requested, both ACCOUNT_NAME and SAS environment variables must be set.")

            filepath = str(Path(args.dirpath) / Path(args.filename))

            testdata_case = TestdataCase(
                **vars(args), filepath=filepath, account_name=account_name, sas=sas)
            category.generate_testdata(testdata_case)

            if args.upload_to_cloud:
                common.upload_to_azure(testdata_case)
