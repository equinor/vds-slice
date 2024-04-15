import http
import pytest
import json
from utils.cloud import *
from shared_test_functions import *


@pytest.mark.parametrize("path, payload", [
    ("slice", payload_merge(connection_payload(VDS_URL, DUMMY_SAS), slice_payload())),
    ("fence", payload_merge(connection_payload(VDS_URL, DUMMY_SAS), fence_payload())),
    ("metadata", connection_payload(VDS_URL, DUMMY_SAS)),
    ("attributes/surface/along", payload_merge(connection_payload(VDS_URL, DUMMY_SAS), attributes_along_surface_payload())),
    ("attributes/surface/between", payload_merge(connection_payload(VDS_URL, DUMMY_SAS), attributes_between_surfaces_payload())),
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
    ("slice",payload_merge(connection_payload(VDS_URL, DUMMY_SAS), slice_payload())),
    ("fence", payload_merge(connection_payload(VDS_URL, DUMMY_SAS), fence_payload())),
    ("metadata", connection_payload(VDS_URL, DUMMY_SAS)),
    ("attributes/surface/along", payload_merge(connection_payload([SAMPLES10_URL], [DUMMY_SAS] ), attributes_along_surface_payload())),
    ("attributes/surface/between", payload_merge(connection_payload([SAMPLES10_URL], [DUMMY_SAS]), attributes_between_surfaces_payload())),
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
    ("slice", payload_merge(connection_payload(VDS_URL, DUMMY_SAS), slice_payload())),
    ("fence",   payload_merge(connection_payload(VDS_URL, DUMMY_SAS), fence_payload())),
    ("metadata",   connection_payload(VDS_URL, DUMMY_SAS)),
    ("attributes/surface/along", payload_merge(connection_payload([SAMPLES10_URL], [DUMMY_SAS] ), attributes_along_surface_payload())),
    ("attributes/surface/between", payload_merge(connection_payload([SAMPLES10_URL], [DUMMY_SAS] ), attributes_between_surfaces_payload())),
])
@pytest.mark.parametrize("vds, sas, expected", [
    (f'{SAMPLES10_URL}?{gen_default_sas()}', '', http.HTTPStatus.OK),
    (f'{SAMPLES10_URL}?{gen_default_sas()}', None, http.HTTPStatus.OK),
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
