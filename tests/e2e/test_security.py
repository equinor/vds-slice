import http
import pytest
import json
from utils.cloud import *
from shared_test_functions import *

from azure.storage import blob
from azure.storage import filedatalake


@pytest.mark.parametrize("path, payload", [
    ("slice", slice_payload()),
    ("fence", fence_payload()),
    ("metadata", metadata_payload()),
    ("attributes/surface/along", attributes_along_surface_payload()),
    ("attributes/surface/between", attributes_between_surfaces_payload()),
])
@pytest.mark.parametrize("sas, allowed_error_messages", [
    (
        "something_not_sassy",
        [
            "409 Public access is not permitted",
            "401 Server failed to authenticate the request"
        ]
    )
])
def test_assure_no_unauthorized_access(path, payload, sas, allowed_error_messages):
    payload.update({"sas": sas})
    res = send_request(path, "post", payload)
    assert res.status_code == http.HTTPStatus.INTERNAL_SERVER_ERROR
    error_body = json.loads(res.content)['error']
    assert any([error_msg in error_body for error_msg in allowed_error_messages]), \
        f'error body \'{error_body}\' does not contain any of the valid errors {allowed_error_messages}'


@pytest.mark.parametrize("path, payload", [
    ("slice", slice_payload(vds=VDS_URL)),
    ("fence", fence_payload(vds=VDS_URL)),
    ("attributes/surface/along", attributes_along_surface_payload()),
    ("attributes/surface/between", attributes_between_surfaces_payload()),
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
# test makes sense if caching is enabled on the endpoint server
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


@pytest.mark.parametrize("path, payload", [
    ("slice", slice_payload()),
    ("fence", fence_payload()),
    ("metadata", metadata_payload()),
    ("attributes/surface/along", attributes_along_surface_payload()),
    ("attributes/surface/between", attributes_between_surfaces_payload()),
])
def test_assure_only_allowed_storage_accounts(path, payload):
    payload.update({
        "vds": "https://dummy.blob.core.windows.net/container/blob",
    })
    res = send_request(path, "post", payload)
    assert res.status_code == http.HTTPStatus.BAD_REQUEST
    body = json.loads(res.content)
    assert "unsupported storage account" in body['error']


@pytest.mark.parametrize("path, payload", [
    ("slice",   slice_payload()),
    ("fence",   fence_payload()),
    ("metadata",   metadata_payload()),
    ("attributes/surface/along", attributes_along_surface_payload()),
    ("attributes/surface/between", attributes_between_surfaces_payload()),
])
@pytest.mark.parametrize("vds, sas, expected", [
    (f'{SAMPLES10_URL}?{gen_default_sas()}', '', http.HTTPStatus.OK),
    (f'{SAMPLES10_URL}?invalid_sas', gen_default_sas(), http.HTTPStatus.BAD_REQUEST),
    (SAMPLES10_URL, '', http.HTTPStatus.BAD_REQUEST)
])
def test_sas_token_in_url(path, payload, vds, sas, expected):
    payload.update({
        "vds": vds,
        "sas": sas
    })
    res = send_request(path, "post", payload)
    assert res.status_code == expected
    gen_default_sas()
