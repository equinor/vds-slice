import http
import numpy as np
import pytest
import json
import re
from utils.cloud import *
from shared_test_functions import *

from azure.storage import blob
from azure.storage import filedatalake


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
        "x": {"annotation": "Sample", "max": 16.0, "min": 4.0, "samples" : 4, "stepsize": 4.0, "unit": "ms"},
        "y": {"annotation": "Crossline", "max": 11.0, "min": 10.0, "samples" : 2, "stepsize": 1.0, "unit": "unitless"},
        "shape": [ 2, 4],
        "format": "<f4",
        "geospatial": [[14.0, 8.0], [12.0, 11.0]]
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
    metadata = dict(request_metadata(method))
    expected_metadata = {
        "axis": [
            {"annotation": "Inline",    "max": 5.0,  "min": 1.0,
                "samples": 3, "stepsize": 2.0, "unit": "unitless"},
            {"annotation": "Crossline", "max": 11.0, "min": 10.0,
                "samples": 2, "stepsize": 1.0, "unit": "unitless"},
            {"annotation": "Sample",    "max": 16.0, "min": 4.0,
                "samples": 4, "stepsize": 4.0, "unit": "ms"}
        ],
        "boundingBox": {
            "cdp": [[2, 0], [14, 8], [12, 11], [0, 3]],
            "ilxl": [[1, 10], [5, 10], [5, 11], [1, 11]],
            "ij": [[0, 0], [2, 0], [2, 1], [0, 1]]
        },
        "crs": "utmXX",
        "inputFileName": "well_known.segy",
        "importTimeStamp": "^\\d{4}-\\d{2}-\\d{2}[A-Z]\\d{2}:\\d{2}:\\d{2}\\.\\d{3}[A-Z]$"
    }

    expected_import_ts = expected_metadata.get("importTimeStamp")
    actual_import_ts = metadata.get("importTimeStamp")
    assert re.compile(expected_import_ts).match(
        actual_import_ts), f"Not a valid import Time Stamp {actual_import_ts} in metadata"
    expected_metadata["importTimeStamp"] = "dummy"
    metadata["importTimeStamp"] = "dummy"
    assert expected_metadata == metadata


def test_attributes_along_surface():
    values = [
        [20, 20],
        [20, 20],
        [20, 20]
    ]
    meta, data = request_attributes_along_surface("post", values)

    expected = np.array([[-0.5, 0.5], [-8.5, 6.5], [16.5, -16.5]])
    assert np.array_equal(data, expected)

    expected_meta = json.loads("""
    {
        "shape": [3, 2],
        "format": "<f4"
    }
    """)
    assert meta == expected_meta


def test_attributes_between_surfaces():
    primary = [
        [12, 12],
        [12, 14],
        [22, 12]
    ]
    secondary = [
        [30,   28],
        [27.5, 29],
        [24,   12]
    ]
    meta, data = request_attributes_between_surfaces(
        "post", primary, secondary)

    expected = np.array([[1.5, 2.5], [-8.5, 7.5], [18.5, -8.5]])
    assert np.array_equal(data, expected)

    expected_meta = json.loads("""
    {
        "shape": [3, 2],
        "format": "<f4"
    }
    """)
    assert meta == expected_meta


@pytest.mark.parametrize("path, payload", [
    ("slice", make_slice_request()),
    ("fence", make_fence_request()),
    ("metadata", make_metadata_request()),
    ("attributes/surface/along", make_attributes_along_surface_request()),
    ("attributes/surface/between", make_attributes_between_surfaces_request()),
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
    ("slice", make_slice_request(vds=VDSURL)),
    ("fence", make_fence_request(vds=VDSURL)),
    ("attributes/surface/along", make_attributes_along_surface_request()),
    ("attributes/surface/between", make_attributes_between_surfaces_request()),
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
    ("slice", make_slice_request()),
    ("fence", make_fence_request()),
    ("metadata", make_metadata_request()),
    ("attributes/surface/along", make_attributes_along_surface_request()),
    ("attributes/surface/between", make_attributes_between_surfaces_request()),
])
def test_assure_only_allowed_storage_accounts(path, payload):
    payload.update({
        "vds": "https://dummy.blob.core.windows.net/container/blob",
    })
    res = send_request(path, "post", payload)
    assert res.status_code == http.HTTPStatus.BAD_REQUEST
    body = json.loads(res.content)
    assert "unsupported storage account" in body['error']


@pytest.mark.parametrize("path, payload, error_code, error", [
    (
        "slice",
        make_slice_request(direction="inline", lineno=4),
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
        make_metadata_request(vds=f'{STORAGE_ACCOUNT}/{CONTAINER}/notfound'),
        http.HTTPStatus.INTERNAL_SERVER_ERROR,
        "The specified blob does not exist"
    ),
    (
        "attributes/surface/along",
        make_attributes_along_surface_request(surface={}),
        http.HTTPStatus.BAD_REQUEST,
        "Error:Field validation for"
    ),
    (
        "attributes/surface/between",
        make_attributes_between_surfaces_request(
            primaryValues=[[1]], secondaryValues=[[1], [1, 1]]),
        http.HTTPStatus.BAD_REQUEST,
        "Surface rows are not of the same length"
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


@pytest.mark.parametrize("path, payload", [
    ("slice",   make_slice_request()),
    ("fence",   make_fence_request()),
    ("metadata",   make_metadata_request()),
    ("attributes/surface/along", make_attributes_along_surface_request()),
    ("attributes/surface/between", make_attributes_between_surfaces_request()),
])
@pytest.mark.parametrize("vds, sas, expected", [
    (f'{SAMPLES10_URL}?{gen_default_sas()}', '', http.HTTPStatus.OK),
    (f'{SAMPLES10_URL}?invalid_sas', gen_default_sas(), http.HTTPStatus.OK),
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
