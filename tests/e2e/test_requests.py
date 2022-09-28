import http
import requests
import numpy as np
import os
import pytest
import json
import urllib.parse
from utils.cloud import generate_container_signature

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
        "x": {"annotation": "Crossline", "max": 11.0, "min": 10.0, "samples" : 2, "unit": "unitless"},
        "y": {"annotation": "Sample", "max": 16.0, "min": 4.0, "samples" : 4, "unit": "ms"},
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
        "shape": [ 2, 4]
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
            "cdp": [[5.0, 0.0], [9.0, 8.0], [4.0, 11.0], [0.0, 3.0]],
            "ilxl": [[1, 10], [5, 10], [5, 11], [1, 11]],
            "ij": [[0, 0], [2, 0], [2, 1], [0, 1]]
        },
        "crs": "utmXX",
        "format": "<f4"
    }
    """)
    assert metadata == expected_metadata


@pytest.mark.parametrize("path, query", [
    ("slice", make_slice_request()),
    ("fence", make_fence_request()),
    ("metadata", make_metadata_request()),
])
@pytest.mark.parametrize("sas, error_msg", [
    (
        "something_not_sassy",
        "409 Public access is not permitted"
    ),
    (
        generate_container_signature(
            STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY, permission=""),
        "403 Server failed to authenticate"
    )
])
def test_assure_no_unauthorized_access(path, query, sas, error_msg):
    query.update({"sas": sas})
    res = requests.get(f'{ENDPOINT}/{path}',
                       params={"query": json.dumps(query)})
    assert res.status_code == http.HTTPStatus.INTERNAL_SERVER_ERROR
    body = json.loads(res.content)
    assert error_msg in body['error']


@pytest.mark.parametrize("path, query", [
    ("slice", make_slice_request()),
    ("fence", make_fence_request()),
    ("metadata", make_metadata_request()),
])
def test_assure_only_allowed_storage_accounts(path, query):
    query.update({
        "vds": "https://dummy.blob.core.windows.net/container/blob",
    })
    res = requests.get(f'{ENDPOINT}/{path}',
                       params={"query": json.dumps(query)})
    assert res.status_code == http.HTTPStatus.BAD_REQUEST
    body = json.loads(res.content)
    assert "Unsupported storage account" in body['error']


@pytest.mark.parametrize("path, query, error_code, error", [
    (
        "slice",
        {"param": "irrelevant"},
        http.HTTPStatus.BAD_REQUEST,
        "Error:Field validation for"
    ),
    (
        "fence",
        {"param": "irrelevant"},
        http.HTTPStatus.BAD_REQUEST,
        "Error:Field validation for"
    ),
    (
        "metadata",
        {"param": "irrelevant"},
        http.HTTPStatus.BAD_REQUEST,
        "Error:Field validation for"
    ),
    (
        "slice",
        make_slice_request(direction="inline", lineno=4),
        http.HTTPStatus.INTERNAL_SERVER_ERROR,
        "Invalid lineno: 4, valid range: [1:5:2]"
    ),
    (
        "fence",
        make_fence_request(coordinate_system="ij", coordinates=[[1, 2, 3]]),
        http.HTTPStatus.INTERNAL_SERVER_ERROR,
        "expected [x y] pair"
    ),
    (
        "metadata",
        make_metadata_request(vds=f'{STORAGE_ACCOUNT}/{CONTAINER}/notfound'),
        http.HTTPStatus.INTERNAL_SERVER_ERROR,
        "The specified blob does not exist"
    ),
])
def test_errors(path, query, error_code, error):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)
    query.update({"sas": sas})
    res = requests.post(f'{ENDPOINT}/{path}', json=query)
    assert res.status_code == error_code

    body = json.loads(res.content)
    assert error in body['error']


def request_slice(method, lineno, direction):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    query = make_slice_request(VDSURL, direction, lineno, sas)
    json_query = json.dumps(query)
    encoded_query = urllib.parse.quote(json_query)

    if method == "get":
        rdata = requests.get(f'{ENDPOINT}/slice?query={encoded_query}')
        rdata.raise_for_status()
    elif method == "post":
        rdata = requests.post(f'{ENDPOINT}/slice', json=query)
        rdata.raise_for_status()
    else:
        raise ValueError(f'Unknown method {method}')

    multipart_data = decoder.MultipartDecoder.from_response(rdata)
    assert len(multipart_data.parts) == 2
    metadata = json.loads(multipart_data.parts[0].content)
    data = multipart_data.parts[1].content

    shape = (
        metadata['x']['samples'],
        metadata['y']['samples']
    )

    data = np.ndarray(shape, metadata['format'], data)
    return metadata, data


def request_fence(method, coordinates, coordinate_system):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    query = make_fence_request(VDSURL, coordinate_system, coordinates, sas)
    json_query = json.dumps(query)
    encoded_query = urllib.parse.quote(json_query)

    if method == "get":
        rdata = requests.get(f'{ENDPOINT}/fence?query={encoded_query}')
        rdata.raise_for_status()
    elif method == "post":
        rdata = requests.post(f'{ENDPOINT}/fence', json=query)
        rdata.raise_for_status()
    else:
        raise ValueError(f'Unknown method {method}')

    multipart_data = decoder.MultipartDecoder.from_response(rdata)
    assert len(multipart_data.parts) == 2
    metadata = json.loads(multipart_data.parts[0].content)
    data = multipart_data.parts[1].content

    data = np.ndarray(metadata['shape'], 'f4', data)
    return metadata, data


def request_metadata(method):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    query = make_metadata_request(VDSURL, sas)
    json_query = json.dumps(query)
    encoded_query = urllib.parse.quote(json_query)

    if method == "get":
        rdata = requests.get(f'{ENDPOINT}/metadata?query={encoded_query}')
        rdata.raise_for_status()
    elif method == "post":
        rdata = requests.post(f'{ENDPOINT}/metadata', json=query)
        rdata.raise_for_status()
    else:
        raise ValueError(f'Unknown method {method}')

    return rdata.json()
