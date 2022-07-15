import requests
import numpy as np
import os
import pytest
import json
from utils.cloud import generate_container_signature

STORAGE_ACCOUNT_NAME = os.getenv("STORAGE_ACCOUNT_NAME")
STORAGE_ACCOUNT_KEY = os.getenv("STORAGE_ACCOUNT_KEY")
ENDPOINT = os.getenv("ENDPOINT").rstrip("/")
CONTAINER = "testdata"
VDS = "wellknown/well_known_default"


def request_slice(method, lineno, direction):
    vds = "{}/{}".format(CONTAINER, VDS)
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)

    if method == "get":
        data_url = f'{ENDPOINT}/slice?{sas}&direction={direction}&lineno={lineno}&vds={vds}'
        rdata = requests.get(data_url)
        rdata.raise_for_status()

        meta_url = f'{ENDPOINT}/slice/metadata?{sas}&direction={direction}&lineno={lineno}&vds={vds}'
        rmeta = requests.get(meta_url)
        rmeta.raise_for_status()
    elif method == "post":
        payload = {
            "direction": direction,
            "lineno": lineno,
            "vds": vds
        }
        rdata = requests.post(f'{ENDPOINT}/slice?{sas}', data=payload)
        rdata.raise_for_status()
        rmeta = requests.post(f'{ENDPOINT}/slice/metadata?{sas}', data=payload)
        rmeta.raise_for_status()
    else:
        raise ValueError(f'Unknown method {method}')

    meta = rmeta.json()
    shape = (
        meta['axis'][0]['samples'],
        meta['axis'][1]['samples']
    )
    data = np.ndarray(shape, 'f4', rdata.content)
    return meta, data


@pytest.mark.parametrize("method", [
    ("get"),
    ("post")
])
def test_inline_slice(method):
    meta, slice = request_slice(method, 5, 'inline')
    expected = np.array([[116, 117, 118, 119],
                         [120, 121, 122, 123]])
    assert np.array_equal(slice, expected)

    # what is format?
    expected_meta = json.loads("""
    {
        "axis": [
            {"Annotation": "Crossline", "max": 11.0, "min": 10.0, "samples" : 2, "unit": "unitless"},
            {"Annotation": "Sample", "max": 16.0, "min": 4.0, "samples" : 4, "unit": "ms"}
        ],
        "format": 3
    }
    """)
    assert meta == expected_meta


@pytest.mark.parametrize("method", [
    ("get"),
    ("post")
])
def test_crossline_slice(method):
    _, slice = request_slice(method, 11, 'crossline')
    expected = np.array([[104, 105, 106, 107],
                         [112, 113, 114, 115],
                         [120, 121, 122, 123]])
    assert np.array_equal(slice, expected)


@pytest.mark.parametrize("method", [
    ("get"),
    ("post")
])
def test_sample_slice(method):
    _, slice = request_slice(method, 8, 'time')
    expected = np.array([[101, 105],
                         [109, 113],
                         [117, 121]])
    assert np.array_equal(slice, expected)


@pytest.mark.parametrize("path", [
    ("slice"),
    ("slice/metadata")
])
@pytest.mark.parametrize("sas", [
    (""),
    (generate_container_signature(STORAGE_ACCOUNT_NAME,
     CONTAINER, STORAGE_ACCOUNT_KEY, permission=""))
])
@pytest.mark.xfail(reason="Can we return something nice when auth fails?")
def test_bad_auth(path, sas):
    params = f'&direction=inline&lineno=3&vds={CONTAINER}/{VDS}'
    res = requests.get(f'{ENDPOINT}/{path}?{sas}{params}')
    assert res.status_code == 403


@pytest.mark.parametrize("path", [
    ("slice"),
    ("slice/metadata")
])
@pytest.mark.parametrize("payload, message", [
    ({"direction": "sample", "lineno": "8", "vds": f'{CONTAINER}/{VDS}'},
     "Unable to use Sample on cube with depth units: ms"),
    ({"direction": "abracadabra", "lineno": "8", "vds": f'{CONTAINER}/{VDS}'},
     "Invalid direction 'abracadabra', valid options are: i, j, k, inline, crossline or depth/time/sample"),
    ({"direction": "inline", "lineno": 100, "vds": f'{CONTAINER}/{VDS}'},
     "Invalid lineno: 100, valid range: [1:5:2]"),
])
@pytest.mark.xfail(reason="Is there a way to inform user? graphql error approach?")
def test_errors_are_forwarded(path, payload, message):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)
    res = requests.post(f'{ENDPOINT}/{path}?{sas}', data=payload)
    print(payload)
    assert res.status_code == 400
    assert res.content == message


@pytest.mark.parametrize("path", [
    ("slice"),
    ("slice/metadata")
])
@pytest.mark.parametrize("payload", [
    ({"lineno": "100", "vds": "some/path"}),
    ({"direction": "inline", "vds": "some/path"}),
    ({"direction": "inline", "lineno": "100"}),
])
def test_missing_parameters_post(path, payload):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)
    res = requests.post(f'{ENDPOINT}/{path}?{sas}', data=payload)
    assert res.status_code == 400


@pytest.mark.parametrize("path", [
    ("slice"),
    ("slice/metadata")
])
@pytest.mark.parametrize("unexpected_params", [
    ("$comp=list"),
    ("&comp=tags"),
    ("&comp=metadata"),
    ("$comp=blobs&where=Name%20%3E%20%27C%27%20AND%20Name%20%3C%20%27D%27"),
    ("$restype=container"),
    ("&versionid=2020-01-01"),
    ("&snapshot=2011-03-09T01:42:34.9360000Z"),
    ("&timeout=1"),
    ("&blackbox=surprise"),
])
@pytest.mark.xfail(reason="Figure out which are allowed. Some cause 500 because of params, some because of sas, some are 200")
def test_unexpected_parameters_get(path, unexpected_params):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)
    params = f'&direction=inline&lineno=3&vds={CONTAINER}/{VDS}'
    params += unexpected_params
    res = requests.get(f'{ENDPOINT}/{path}?{sas}{params}')
    assert res.status_code == 400


# TODO: other blackbox tests for existing slice directions and metadata?
