import http
import requests
import numpy as np
import os
import pytest
import json
import urllib.parse
from utils.cloud import *

from azure.storage import blob
from azure.storage import filedatalake

from requests_toolbelt.multipart import decoder

STORAGE_ACCOUNT_NAME = os.getenv("STORAGE_ACCOUNT_NAME")
STORAGE_ACCOUNT_KEY = os.getenv("STORAGE_ACCOUNT_KEY")
ENDPOINT = os.getenv("ENDPOINT").rstrip("/")
CONTAINER = "testdata"
VDS = "well_known/well_known_default"
STORAGE_ACCOUNT = f"https://{STORAGE_ACCOUNT_NAME}.blob.core.windows.net"
VDSURL = f"{STORAGE_ACCOUNT}/{CONTAINER}/{VDS}"


def make_slice_request(vds=VDSURL, direction="inline", lineno=3, sas="sas"):
    return {
        "vds": vds,
        "direction": direction,
        "lineno": lineno,
        "sas": sas
    }


def make_fence_request(vds=VDSURL, coordinate_system="ij", coordinates=[[0, 0]], sas="sas"):
    return {
        "vds": vds,
        "coordinateSystem": coordinate_system,
        "coordinates": coordinates,
        "sas": sas
    }


def make_metadata_request(vds=VDSURL, sas="sas"):
    return {
        "vds": vds,
        "sas": sas
    }


@pytest.mark.parametrize("method", [
    ("get"),
    ("post")
])
def test_slice(method):
    meta, slice = request_slice(method, 5, 'inline')

    expected = np.array([[116, 117, 118, 119],
                         [120, 121, 122, 123]])
    assert np.array_equal(slice, expected)

    expected_meta = json.loads("""
    {
        "x": {"annotation": "Sample", "max": 16.0, "min": 4.0, "samples" : 4, "unit": "ms"},
        "y": {"annotation": "Crossline", "max": 11.0, "min": 10.0, "samples" : 2, "unit": "unitless"},
        "format": "<f4"
    }
    """)
    assert meta == expected_meta


@pytest.mark.parametrize("method", [
    ("get"),
    ("post")
])
def test_fence(method):
    meta, fence = request_fence(method, [[3, 10], [1, 11]], 'ilxl')

    expected = np.array([[108, 109, 110, 111],
                         [104, 105, 106, 107]])
    assert np.array_equal(fence, expected)

    expected_meta = json.loads("""
    {
        "shape": [ 2, 4],
        "format": "<f4"
    }
    """)
    assert meta == expected_meta


@pytest.mark.parametrize("method", [
    ("get"),
    ("post")
])
def test_metadata(method):
    metadata = request_metadata(method)
    expected_metadata = json.loads("""
    {
        "axis": [
            {"annotation": "Inline", "max": 5.0, "min": 1.0, "samples" : 3, "unit": "unitless"},
            {"annotation": "Crossline", "max": 11.0, "min": 10.0, "samples" : 2, "unit": "unitless"},
            {"annotation": "Sample", "max": 16.0, "min": 4.0, "samples" : 4, "unit": "ms"}
        ],
        "boundingBox": {
            "cdp": [[2.0, 0.0], [14.0, 8.0], [12.0, 11.0], [0.0, 3.0]],
            "ilxl": [[1, 10], [5, 10], [5, 11], [1, 11]],
            "ij": [[0, 0], [2, 0], [2, 1], [0, 1]]
        },
        "crs": "utmXX"
    }
    """)
    assert metadata == expected_metadata


@pytest.mark.parametrize("path, payload", [
    ("slice", make_slice_request()),
    ("fence", make_fence_request()),
    ("metadata", make_metadata_request()),
])
@pytest.mark.parametrize("sas, allowed_error_messages", [
    (
        "something_not_sassy",
        [
            "409 Public access is not permitted",
            "401 Server failed to authenticate the request"
        ]
    ),
    (
        generate_container_signature(
            STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY, permission=""),
        ["403 Server failed to authenticate"]
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
    ("slice", make_slice_request(vds=VDSURL)),
    ("fence", make_fence_request(vds=VDSURL))
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
    res = send_request(path, "get", payload)
    assert res.status_code == status
    if error:
        assert error in json.loads(res.content)['error']


@pytest.mark.parametrize("path, payload", [
    ("slice", make_slice_request()),
    ("fence", make_fence_request()),
    ("metadata", make_metadata_request()),
])
def test_assure_only_allowed_storage_accounts(path, payload):
    payload.update({
        "vds": "https://dummy.blob.core.windows.net/container/blob",
    })
    res = send_request(path, "get", payload)
    assert res.status_code == http.HTTPStatus.BAD_REQUEST
    body = json.loads(res.content)
    assert "unsupported storage account" in body['error']


@pytest.mark.parametrize("path, payload, error_code, error", [
    (
        "slice",
        make_slice_request(direction="inline", lineno=4),
        http.HTTPStatus.INTERNAL_SERVER_ERROR,
        "Invalid lineno: 4, valid range: [1.000000:5.000000:2.000000]"
    ),
    (
        "fence",
        {"param": "irrelevant"},
        http.HTTPStatus.BAD_REQUEST,
        "Error:Field validation for"
    ),
    (
        "metadata",
        make_metadata_request(vds=f'{STORAGE_ACCOUNT}/{CONTAINER}/notfound'),
        http.HTTPStatus.INTERNAL_SERVER_ERROR,
        "The specified blob does not exist"
    ),
])
def test_errors(path, payload, error_code, error):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)
    payload.update({"sas": sas})
    res = send_request(path, "post", payload)
    assert res.status_code == error_code

    body = json.loads(res.content)
    assert error in body['error']


def send_request(path, method, payload):
    if method == "get":
        json_payload = json.dumps(payload)
        encoded_payload = urllib.parse.quote(json_payload)
        data = requests.get(f'{ENDPOINT}/{path}?query={encoded_payload}')
    elif method == "post":
        data = requests.post(f'{ENDPOINT}/{path}', json=payload)
    else:
        raise ValueError(f'Unknown method {method}')
    return data


def request_slice(method, lineno, direction):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    payload = make_slice_request(VDSURL, direction, lineno, sas)
    rdata = send_request("slice", method, payload)
    rdata.raise_for_status()

    multipart_data = decoder.MultipartDecoder.from_response(rdata)
    assert len(multipart_data.parts) == 2
    metadata = json.loads(multipart_data.parts[0].content)
    data = multipart_data.parts[1].content

    shape = (
        metadata['y']['samples'],
        metadata['x']['samples']
    )

    data = np.ndarray(shape, metadata['format'], data)
    return metadata, data


def request_fence(method, coordinates, coordinate_system):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    payload = make_fence_request(VDSURL, coordinate_system, coordinates, sas)
    rdata = send_request("fence", method, payload)
    rdata.raise_for_status()

    multipart_data = decoder.MultipartDecoder.from_response(rdata)
    assert len(multipart_data.parts) == 2
    metadata = json.loads(multipart_data.parts[0].content)
    data = multipart_data.parts[1].content

    data = np.ndarray(metadata['shape'], metadata['format'], data)
    return metadata, data


def request_metadata(method):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    payload = make_metadata_request(VDSURL, sas)
    rdata = send_request("metadata", method, payload)
    rdata.raise_for_status()

    return rdata.json()
