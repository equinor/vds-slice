import http
import pytest
import time
import json
from utils.cloud import *
from shared_test_functions import *

from azure.storage import blob
from azure.storage import filedatalake

# tests here make sense if caching is enabled on the endpoint server

cached_data_paths_payloads = [
    ("slice", payload_merge(connection_payload(vds=[VDS_URL], sas=[DUMMY_SAS]), slice_payload())),
    ("fence", payload_merge(connection_payload(vds=[VDS_URL], sas=[DUMMY_SAS]), fence_payload())),
    ("attributes/surface/along", payload_merge(connection_payload([SAMPLES10_URL], sas=[DUMMY_SAS]), attributes_along_surface_payload())),
    ("attributes/surface/between", payload_merge(connection_payload([SAMPLES10_URL], sas=[DUMMY_SAS]), attributes_between_surfaces_payload())),
]


def make_caching_call(payload, path, sas=None):
    """
    makes a payload call to path.
    used as a payload caching call
    """
    if sas == None:
        sas = generate_container_signature(
            STORAGE_ACCOUNT_NAME,
            CONTAINER,
            STORAGE_ACCOUNT_KEY,
            permission=blob.ContainerSasPermissions(read=True))
    payload.update({"sas": sas})
    res = send_request(path, "post", payload)
    assert res.status_code == http.HTTPStatus.OK


@pytest.mark.parametrize("path, payload", cached_data_paths_payloads)
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
    make_caching_call(payload, path)

    payload.update({"sas": token})
    res = send_request(path, "post", payload)
    assert res.status_code == status
    if error:
        assert error in json.loads(res.content)['error']


@pytest.mark.parametrize("path, payload", cached_data_paths_payloads)
def test_cache_request_with_expired_token(path, payload):
    expire_time = 2
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME,
        CONTAINER,
        STORAGE_ACCOUNT_KEY,
        lifespan=expire_time,
        permission=blob.ContainerSasPermissions(read=True))

    make_caching_call(payload, path, sas)
    time.sleep(expire_time + 1)

    payload.update({"sas": sas})
    res = send_request(path, "post", payload)
    assert res.status_code == http.HTTPStatus.INTERNAL_SERVER_ERROR
    assert "403 Server failed to authenticate the request" in json.loads(res.content)[
        'error']


def test_fence_missing_fillvalue_vs_0():
    # bug test:
    # Hashes of fence call with "fillvalue=0" and "fillvalue not supplied"
    # were identical. That made call with no fill value return wrong response

    path = "fence"
    fence_payload = {
        "coordinateSystem": "ij",
        "coordinates": [[0, 0], [1, 1], [2, 2], [3, 3]],
    }

    def assure_missing_fillvalue_call_fails():
        payload_without_fillvalue = payload_merge(
            connection_payload(vds=[VDS_URL], sas=[gen_default_sas()]),
            fence_payload
        )

        res = send_request(path, "post", payload_without_fillvalue)
        assert res.status_code == http.HTTPStatus.BAD_REQUEST
        msg = "Coordinate (2.000000,2.000000) is out of boundaries"
        assert msg in json.loads(res.content)['error']

    assure_missing_fillvalue_call_fails()

    payload_with_fillvalue = payload_merge(
        connection_payload(vds=[VDS_URL], sas=[gen_default_sas()]),
        {**fence_payload, 'fillvalue': 0}
    )
    make_caching_call(payload_with_fillvalue, path)

    assure_missing_fillvalue_call_fails()
