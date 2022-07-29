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
VDS = "wellknown/well_known_default"


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
        "axis": [
            {"annotation": "Crossline", "max": 11.0, "min": 10.0, "samples" : 2, "unit": "unitless"},
            {"annotation": "Sample", "max": 16.0, "min": 4.0, "samples" : 4, "unit": "ms"}
        ],
        "format": 3
    }
    """)
    assert meta == expected_meta


@pytest.mark.parametrize("method", [
    ("get"),
    ("post")
])
def test_fence(method):
    data = request_fence(method, [[3, 10], [1, 11]], 'ilxl')
    expected = np.array([108, 109, 110, 111, 104, 105, 106, 107])
    assert np.array_equal(data, expected)


@pytest.mark.parametrize("path, query", [
    ("slice", {
        "vds": f'{CONTAINER}/{VDS}',
        "direction": "inline",
        "lineno": 3
    }),
    ("fence", {
        "vds": f'{CONTAINER}/{VDS}',
        "coordinate_system": "ij",
        "coordinates": [[0, 0]]
    }),
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


@pytest.mark.parametrize("path, query, error_code, error", [
    (
        "nonexisting",
        {},
        http.HTTPStatus.NOT_FOUND,
        ""
    ),
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
        "slice",
        {"vds": f'{CONTAINER}/{VDS}', "direction": "inline", "lineno": 4, },
        http.HTTPStatus.INTERNAL_SERVER_ERROR,
        "Invalid lineno: 4, valid range: [1:5:2]"
    ),
    (
        "fence",
        {"vds": f'{CONTAINER}/{VDS}', "coordinate_system": "ij",
            "coordinates": [[1, 2, 3]], },
        http.HTTPStatus.INTERNAL_SERVER_ERROR,
        "expected [x y] pair"
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
    vds = "{}/{}".format(CONTAINER, VDS)
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    query = {
        "vds": vds,
        "direction": direction,
        "lineno": lineno,
        "sas": sas
    }
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
        metadata['axis'][0]['samples'],
        metadata['axis'][1]['samples']
    )

    data = np.ndarray(shape, 'f4', data)
    return metadata, data


def request_fence(method, coordinates, coordinate_system):
    vds = "{}/{}".format(CONTAINER, VDS)
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    query = {
        "vds": vds,
        "coordinate_system": coordinate_system,
        "coordinates": coordinates,
        "sas": sas
    }
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
    metadata = multipart_data.parts[0].content
    data = multipart_data.parts[1].content

    assert len(metadata) == 0

    data = np.frombuffer(data, dtype="f4")
    return data
