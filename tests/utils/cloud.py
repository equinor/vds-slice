from azure.storage import blob
import datetime

def generate_account_signature(
    account_name,
    account_key,
    lifespan=120,
):
    """
    Create account signature for azure request
    """
    expiry = datetime.datetime.utcnow() + datetime.timedelta(seconds=lifespan)
    permission = blob.AccountSasPermissions(read=True, list=True)
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
):
    """
    Create container signature for azure request
    """
    expiry = datetime.datetime.utcnow() + datetime.timedelta(seconds=lifespan)
    permission = blob.ContainerSasPermissions(read=True, list=True)

    return blob.generate_container_sas(
        account_name=account_name,
        container_name=container_name,
        account_key=account_key,
        permission=permission,
        expiry=expiry)
