import http
import pytest
import json
from utils.cloud import *
from shared_test_functions import *

from azure.storage import blob
from azure.storage import filedatalake

# tests here make sense if caching is enabled on the endpoint server

@pytest.mark.parametrize("path, payload", [
    ("slice", payload_merge(connection_payload(vds=[VDS_URL], sas=[DUMMY_SAS]), slice_payload())),
    ("fence", payload_merge(connection_payload(vds=[VDS_URL], sas=[DUMMY_SAS]), fence_payload())),
    ("attributes/surface/along", payload_merge(connection_payload([SAMPLES10_URL], sas=[DUMMY_SAS]), attributes_along_surface_payload())),
    ("attributes/surface/between", payload_merge(connection_payload([SAMPLES10_URL], sas=[DUMMY_SAS]), attributes_between_surfaces_payload())),
])
@pytest.mark.parametrize("token, status, error", [
    (generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY,
        permission=blob.ContainerSasPermissions(read=True)),
     http.HTTPStatus.OK, None),
    (generate_account_signature(
        STORAGE_ACCOUNT_NAME, STORAGE_ACCOUNT_KEY, permission=blob.AccountSasPermissions(
            read=True), resource_types=blob.ResourceTypes(container=True, object=True)),
     http.HTTPStatus.OK, None),
    (generate_blob_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, f'{VDS}/VolumeDataLayout', STORAGE_ACCOUNT_KEY,
        permission=blob.BlobSasPermissions(read=True)),
     http.HTTPStatus.INTERNAL_SERVER_ERROR, "403 Server failed to authenticate the request"),
    pytest.param(
        generate_directory_signature(
            STORAGE_ACCOUNT_NAME, CONTAINER, VDS, STORAGE_ACCOUNT_KEY,
            permission=filedatalake.DirectorySasPermissions(read=True)),
        http.HTTPStatus.OK, None,
        marks=pytest.mark.skipif(
            not is_account_datalake_gen2(
                STORAGE_ACCOUNT_NAME, CONTAINER, VDS, STORAGE_ACCOUNT_KEY),
            reason="storage account is not a datalake")
    ),
])
def test_cached_data_access_with_various_sas(path, payload, token, status, error):

    def make_caching_call():
        container_sas = generate_container_signature(
            STORAGE_ACCOUNT_NAME,
            CONTAINER,
            STORAGE_ACCOUNT_KEY,
            permission=blob.ContainerSasPermissions(read=True))
        payload.update({"sas": container_sas})
        res = send_request(path, "post", payload)
        assert res.status_code == http.HTTPStatus.OK

    make_caching_call()

    payload.update({"sas": token})
    res = send_request(path, "post", payload)
    assert res.status_code == status
    if error:
        assert error in json.loads(res.content)['error']
