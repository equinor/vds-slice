import http
import os
import json
# import urllib.parse
import pytest
# import requests
from utils.cloud import Cloud as a_cloud
from utils import cloud as zz_cloud
from utils.payload import PayLoad as pl

STORAGE_ACCOUNT_NAME = os.getenv("STORAGE_ACCOUNT_NAME")
STORAGE_ACCOUNT_KEY = os.getenv("STORAGE_ACCOUNT_KEY")
ENDPOINT = os.getenv("ENDPOINT", "http://localhost:8080").rstrip("/")
CONTAINER = "testdata"
VDS = "well_known/well_known_default"
STORAGE_ACCOUNT = f"https://{STORAGE_ACCOUNT_NAME}.blob.core.windows.net"
VDS_URL = f"{STORAGE_ACCOUNT}/{CONTAINER}/{VDS}"

SAMPLES10_URL = f"{STORAGE_ACCOUNT}/{CONTAINER}/10_samples/10_samples_default"

payload = pl(STORAGE_ACCOUNT_NAME,
             STORAGE_ACCOUNT_KEY,
             ENDPOINT,
             CONTAINER)

cloud = a_cloud(STORAGE_ACCOUNT_NAME,
                CONTAINER,
                STORAGE_ACCOUNT_KEY)


@pytest.mark.parametrize("_path, _payload", [
    ("slice", payload.slice()),
    ("fence", payload.fence()),
    ("metadata", payload.metadata()),
    ("attributes/surface/along", payload.attributes_along_surface()),
    ("attributes/surface/between", payload.attributes_between_surfaces()),
])
@pytest.mark.parametrize("_sas, _allowed_error_messages", [
    (
        "something_not_sassy",
        [
            "409 Public access is not permitted",
            "401 Server failed to authenticate the request"
        ]
    )
])
def test_assure_no_unauthorized_access(_path, _payload, _sas, _allowed_error_messages):
    _payload.update({"sas": _sas})
    res = payload.send_request(_path, "post", _payload)
    assert res.status_code == http.HTTPStatus.INTERNAL_SERVER_ERROR
    error_body = json.loads(res.content)['error']
    assert any([error_msg in error_body for error_msg in _allowed_error_messages]), \
        f'error body \'{error_body}\' does not contain any of the valid errors {_allowed_error_messages}'


@pytest.mark.parametrize("_path, _payload", [
    ("slice", payload.slice(vds=VDS_URL)),
    ("fence", payload.fence(vds=VDS_URL)),
    ("attributes/surface/along", payload.attributes_along_surface()),
    ("attributes/surface/between", payload.attributes_between_surfaces()),
])
@pytest.mark.parametrize("_token, _status, _error", [
    (cloud.generate_container_signature(permission=cloud.blob_container_sas_permissions(read=True)),
     http.HTTPStatus.OK, None),
    (cloud.generate_account_signature(permission=cloud.blob_account_sas_permissions(read=True),
                                      resource_types=cloud.blob_resource_types(container=True,
                                                                               object=True)),
     http.HTTPStatus.OK, None),
    (cloud.generate_blob_signature(f'{VDS}/VolumeDataLayout',
                                   permission=cloud.blob_sas_permissions(read=True)),
     http.HTTPStatus.INTERNAL_SERVER_ERROR, "403 Server failed to authenticate the request"),
    pytest.param(
        cloud.generate_directory_signature(
            VDS, permission=cloud.storage_filedatalake.DirectorySasPermissions(read=True)),
        http.HTTPStatus.OK, None,
        marks=pytest.mark.skipif(
            not cloud.is_account_datalake_gen2(VDS),
            reason="storage account is not a datalake")
    ),
])
# test makes sense if caching is enabled on the endpoint server
def test_cached_data_access_with_various_sas(_path, _payload, _token, _status, _error):

    def make_caching_call():
        container_sas = cloud.generate_container_signature(
            permission=cloud.blob_container_sas_permissions(read=True))
        _payload.update({"sas": container_sas})
        res = payload.send_request(_path, "post", _payload)
        assert res.status_code == http.HTTPStatus.OK

    make_caching_call()

    _payload.update({"sas": _token})
    res = payload.send_request(_path, "post", _payload)
    assert res.status_code == _status
    if _error:
        assert _error in json.loads(res.content)['error']


@pytest.mark.parametrize("_path, _payload", [
    ("slice", payload.slice()),
    ("fence", payload.fence()),
    ("metadata", payload.metadata()),
    ("attributes/surface/along", payload.attributes_along_surface()),
    ("attributes/surface/between", payload.attributes_between_surfaces()),
])
def test_assure_only_allowed_storage_accounts(_path, _payload):
    _payload.update({
        "vds": "https://dummy.blob.core.windows.net/container/blob",
    })
    res = payload.send_request(_path, "post", _payload)
    assert res.status_code == http.HTTPStatus.BAD_REQUEST
    body = json.loads(res.content)
    assert "unsupported storage account" in body['error']


@pytest.mark.parametrize("_path, _payload, _error_code, _error", [
    (
        "slice",
        payload.slice(direction="inline", lineno=4),
        http.HTTPStatus.BAD_REQUEST,
        "Invalid lineno: 4, valid range: [1.00:5.00:2.00]"
    ),
    (
        "fence",
        {"param": "irrelevant"},
        http.HTTPStatus.BAD_REQUEST,
        "Error:Field validation for"
    ),
    (
        "metadata",
        payload.metadata(vds=f'{STORAGE_ACCOUNT}/{CONTAINER}/notfound'),
        http.HTTPStatus.INTERNAL_SERVER_ERROR,
        "The specified blob does not exist"
    ),
    (
        "attributes/surface/along",
        payload.attributes_along_surface(surface={}),
        http.HTTPStatus.BAD_REQUEST,
        "Error:Field validation for"
    ),
    (
        "attributes/surface/between",
        payload.attributes_between_surfaces(
            primaryValues=[[1]], secondaryValues=[[1], [1, 1]]),
        http.HTTPStatus.BAD_REQUEST,
        "Surface rows are not of the same length"
    ),
])
def test_errors(_path, _payload, _error_code, _error):
    sas = cloud.generate_container_signature()
    _payload.update({"sas": sas})
    res = payload.send_request(_path, "post", _payload)
    assert res.status_code == _error_code

    body = json.loads(res.content)
    assert _error in body['error']


@pytest.mark.parametrize("_path, _payload", [
    ("slice",   payload.slice()),
    ("fence",   payload.fence()),
    ("metadata",   payload.metadata()),
    ("attributes/surface/along", payload.attributes_along_surface()),
    ("attributes/surface/between", payload.attributes_between_surfaces()),
])
@pytest.mark.parametrize("_vds, _sas, _expected", [
    (f'{SAMPLES10_URL}?{cloud.generate_container_signature()}', '', http.HTTPStatus.OK),
    (f'{SAMPLES10_URL}?invalid_sas',
     cloud.generate_container_signature(), http.HTTPStatus.OK),
    (SAMPLES10_URL, '', http.HTTPStatus.BAD_REQUEST)
])
def test_sas_token_in_url(_path, _payload, _vds, _sas, _expected):
    _payload.update({
        "vds": _vds,
        "sas": _sas
    })
    res = payload.send_request(_path, "post", _payload)
    assert res.status_code == _expected
