from pathlib import Path
import subprocess
from typing import List

from abc import ABC, abstractmethod


class CommandArgument:
    """
    Custom command argument for argparse
    """

    def __init__(self, name, type, description, required):
        self.name = name
        self.type = type
        self.description = description
        self.required = required


class CommandSpecification:
    """
    Specification to be used in argparser when generating subcommand for a testcase

    Attributes:
        case (str): Name of the test as would be shown to the user.
        description (str): The description of the case.
        filename (str): Default name produced file is to be stored under.
        cloud_path (str): Default cloud path where the file should be uploaded.
        openvds_dir_path (str): Default path to the directory where openVDS binaries are located.
        custom_args (List[CommandArgument]): List of the arguments custom for the command.
    """

    def __init__(self, case, description, filename, cloud_path="", openvds_dir_path="", custom_args: List[CommandArgument] = []):
        self.case = case
        self.description = description
        self.filename = filename
        self.cloud_path = cloud_path
        self.openvds_dir_path = openvds_dir_path
        self.custom_args = custom_args


class TestdataCase:
    def __init__(self, case, filepath, cloud_path="", account_name="", sas="", openvds_dir_path="", **kwargs):
        """
        All gathered information needed for creation of testdata file

        Args:
            case (str): ID of particular testdata case
            filepath (str): Filepath for vds file
            cloud_path (str): Path in the azure storage account where file is to be uploaded, in form of [container]/[directories]
            account_name (str): Name of the account (without .blob.core.windows.net) to upload data to
            sas (str): credentials used to upload the file.
            openvds_dir_path (str): Path to the directory where openVDS binaries are located.
            kwargs (dict): Additional keyword arguments
        """
        self.case = case
        self.filepath = filepath
        self.cloud_path = cloud_path
        self.account_name = account_name
        self.sas = sas
        self.openvds_dir_path = openvds_dir_path
        self.kwargs = kwargs


class Category(ABC):
    """Groups testdata cases"""
    @abstractmethod
    def __init__(self, name, description, dirpath, all_allowed=True):
        """
        Args:
            name: Category name
            description: Category description
            dirpath: Path to the directory where all the files are created by default
            all_allowed: Indicates if it is allowed to run all the scripts in the category at once. Defaults to True.
        """
        self.name = name
        self.description = description
        self.dirpath = dirpath
        self.all_allowed = all_allowed

    @abstractmethod
    def specifications(self) -> List[CommandSpecification]:
        """All cases belonging to the category"""
        pass

    @abstractmethod
    def generate_testdata(self, testdata: TestdataCase) -> None:
        """Creates testdata"""
        pass


def segy_to_vds(testdata: TestdataCase, segypath, *args, **kwargs):
    segy_import_path = Path(testdata.openvds_dir_path) / "SEGYImport"
    command = [
        segy_import_path, "--url", "file://.", "--vdsfile", testdata.filepath, segypath
    ]
    command.extend(args)

    crs = kwargs.pop('crs', None)
    if crs:
        command.append(f"--crs-wkt={crs}")

    for key, value in kwargs.items():
        command.append(f"--{key}={value}")

    subprocess.run(command)


def upload_to_azure(testdata: TestdataCase):
    vds_copy_path = Path(testdata.openvds_dir_path) / "VDSCopy"
    azure_url = f"azureSAS://{testdata.account_name}.blob.core.windows.net/{testdata.cloud_path}"

    subprocess.run([vds_copy_path, testdata.filepath, azure_url,
                   "--compression-method=None", "-d", f"Suffix={testdata.sas}"])
