
import http
import numpy as np
import pytest
import json
from utils.cloud import *
from shared_test_functions import *


@pytest.mark.parametrize("path, payload, error_code, error", [
    (
        "slice",
        payload_merge(connection_payload(vds=[VDS_URL], sas=[DUMMY_SAS]), slice_payload(direction="inline", lineno=4)),
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
        connection_payload(vds=[f'{STORAGE_ACCOUNT}/{CONTAINER}/notfound'], sas=[DUMMY_SAS]),
        http.HTTPStatus.INTERNAL_SERVER_ERROR,
        "The specified blob does not exist"
    ),
    (
        "attributes/surface/along",
        payload_merge(connection_payload(vds=[VDS_URL], sas=[DUMMY_SAS]), attributes_along_surface_payload(surface={})),
        http.HTTPStatus.BAD_REQUEST,
        "Error:Field validation for"
    ),
    (
        "attributes/surface/between",
        payload_merge(connection_payload(vds=[VDS_URL], sas=[DUMMY_SAS]), attributes_between_surfaces_payload(
            primaryValues=[[1]], secondaryValues=[[1], [1, 1]])),
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
