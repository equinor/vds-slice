import datetime
from azure.storage import blob
from azure.storage import filedatalake
from azure.core import exceptions

class Cloud:

    def __init__(self,
                 _account_name,
                 _container_name,
                 _account_key,
                 ) -> None:
        self.account_name = _account_name
        self.container_name = _container_name
        self.account_key = _account_key
        self.blob_sas_permissions = blob.BlobSasPermissions
        self.blob_container_sas_permissions = blob.ContainerSasPermissions
        self.blob_account_sas_permissions = blob.AccountSasPermissions
        self.blob_resource_types = blob.ResourceTypes
        self.storage_filedatalake = filedatalake

    def generate_account_signature(
        self,
        account_name=None,
        account_key=None,
        lifespan=120,
        permission=None,
        resource_types=None,
    ):
        """
        Create account signature for azure request
        """
        expiry = datetime.datetime.utcnow() + datetime.timedelta(seconds=lifespan)
        if account_name is None:
            account_name = self.account_name
        if account_key is None:
            account_key = self.account_key
        if permission is None:
            permission = blob.AccountSasPermissions(read=True)
        if resource_types is None:
            resource_types = blob.ResourceTypes(container=True, object=True)

        return blob.generate_account_sas(
            account_name=account_name,
            account_key=account_key,
            resource_types=resource_types,
            permission=permission,
            expiry=expiry)

    def generate_container_signature(
        self,
        account_name=None,
        container_name=None,
        account_key=None,
        lifespan=120,
        permission=None
    ):
        """
        Create container signature for azure request
        """
        expiry = datetime.datetime.utcnow() + datetime.timedelta(seconds=lifespan)
        if account_name is None:
            account_name = self.account_name
        if container_name is None:
            container_name = self.container_name
        if account_key is None:
            account_key = self.account_key
        if permission is None:
            permission = blob.ContainerSasPermissions(read=True)

        return blob.generate_container_sas(
            account_name=account_name,
            container_name=container_name,
            account_key=account_key,
            permission=permission,
            expiry=expiry)

    def generate_directory_signature(
        self,
        file_system_name,
        account_name=None,
        directory_name=None,
        account_key=None,
        lifespan=120,
        permission=None
    ):
        """
        Create directory signature for azure request
        """
        expiry = datetime.datetime.utcnow() + datetime.timedelta(seconds=lifespan)
        if account_name is None:
            account_name = self.account_name
        if directory_name is None:
            directory_name = self.container_name
        if account_key is None:
            account_key = self.account_key
        if permission is None:
            permission = filedatalake.FileSasPermissions(read=True)

        return filedatalake.generate_directory_sas(
            account_name=account_name,
            file_system_name=file_system_name,
            directory_name=directory_name,
            credential=account_key,
            permission=permission,
            expiry=expiry)

    def generate_blob_signature(
        self,
        blob_name,
        account_name=None,
        container_name=None,
        account_key=None,
        lifespan=120,
        permission=None
    ):
        """
        Create blob signature for azure request
        """
        expiry = datetime.datetime.utcnow() + datetime.timedelta(seconds=lifespan)
        if account_name is None:
            account_name = self.account_name
        if container_name is None:
            container_name = self.container_name
        if account_key is None:
            account_key = self.account_key
        if permission is None:
            permission = blob.BlobSasPermissions(read=True)

        return blob.generate_blob_sas(
            account_name=account_name,
            container_name=container_name,
            blob_name=blob_name,
            account_key=account_key,
            permission=permission,
            expiry=expiry)

    def is_account_datalake_gen2(
        self,
        directory_name,
        account_name=None,
        container_or_filesystem_name=None,
        account_key=None,
    ):
        """
        For the moment datalake gen2 is not our primary storage,
        but as we aim to support it too, we want to allow
        additional tests to be run on it
        """

        if account_name is None:
            account_name = self.account_name
        if container_or_filesystem_name is None:
            container_or_filesystem_name = self.container_name
        if account_key is None:
            account_key = self.account_key

        service_client = filedatalake.DataLakeServiceClient(
            account_url=f"https://{account_name}.dfs.core.windows.net", credential=account_key)
        file_system_client = service_client.get_file_system_client(
            file_system=container_or_filesystem_name)
        directory_client = file_system_client.get_directory_client(
            directory_name)
        try:
            directory_client.get_directory_properties()
        except exceptions.ResourceNotFoundError:
            return False
        return True
