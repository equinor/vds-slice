from azure.storage import blob
from azure.core import exceptions
import datetime


def generate_account_signature(
    account_name,
    account_key,
    lifespan=120,
    permission=None,
    resource_types=None,
):
    """
    Create account signature for azure request
    """
    expiry = datetime.datetime.utcnow() + datetime.timedelta(seconds=lifespan)
    if permission == None:
        permission = blob.AccountSasPermissions(read=True)
    if resource_types == None:
        resource_types = blob.ResourceTypes(container=True, object=True)

    return blob.generate_account_sas(
        account_name=account_name,
        account_key=account_key,
        resource_types=resource_types,
        permission=permission,
        expiry=expiry)


def generate_container_signature(
    account_name,
    container_name,
    account_key,
    lifespan=120,
    permission=None
):
    """
    Create container signature for azure request
    """
    expiry = datetime.datetime.utcnow() + datetime.timedelta(seconds=lifespan)
    if permission == None:
        permission = blob.ContainerSasPermissions(read=True)

    return blob.generate_container_sas(
        account_name=account_name,
        container_name=container_name,
        account_key=account_key,
        permission=permission,
        expiry=expiry)


def generate_directory_signature(
    account_name,
    file_system_name,
    directory_name,
    account_key,
    lifespan=120,
    permission=None
):
    """
    Create directory signature for azure request
    """
    from azure.storage import filedatalake
    expiry = datetime.datetime.utcnow() + datetime.timedelta(seconds=lifespan)
    if permission == None:
        permission = filedatalake.FileSasPermissions(read=True)

    return filedatalake.generate_directory_sas(
        account_name=account_name,
        file_system_name=file_system_name,
        directory_name=directory_name,
        credential=account_key,
        permission=permission,
        expiry=expiry)


def generate_blob_signature(
    account_name,
    container_name,
    blob_name,
    account_key,
    lifespan=120,
    permission=None
):
    """
    Create blob signature for azure request
    """
    expiry = datetime.datetime.utcnow() + datetime.timedelta(seconds=lifespan)
    if permission == None:
        permission = blob.BlobSasPermissions(read=True)

    return blob.generate_blob_sas(
        account_name=account_name,
        container_name=container_name,
        blob_name=blob_name,
        account_key=account_key,
        permission=permission,
        expiry=expiry)


def is_account_datalake_gen2(
    account_name,
    container_or_filesystem_name,
    directory_name,
    account_key,
):
    """
    For the moment datalake gen2 is not our primary storage,
    but as we aim to support it too, we want to allow
    additional tests to be run on it
    """

    from azure.storage import filedatalake

    service_client = filedatalake.DataLakeServiceClient(
        account_url=f"https://{account_name}.dfs.core.windows.net", credential=account_key)
    file_system_client = service_client.get_file_system_client(
        file_system=container_or_filesystem_name)
    directory_client = file_system_client.get_directory_client(directory_name)
    try:
        directory_client.get_directory_properties()
    except exceptions.ResourceNotFoundError:
        return False
    else:
        return True
