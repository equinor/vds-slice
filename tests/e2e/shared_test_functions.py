import requests
import numpy as np
import os
import json
import urllib.parse
from utils.cloud import generate_container_signature

from requests_toolbelt.multipart import decoder

STORAGE_ACCOUNT_NAME = os.getenv("STORAGE_ACCOUNT_NAME")
STORAGE_ACCOUNT_KEY = os.getenv("STORAGE_ACCOUNT_KEY")
ENDPOINT = os.getenv("ENDPOINT", "http://localhost:8080").rstrip("/")
CONTAINER = "testdata"
VDS = "well_known/well_known_default"
STORAGE_ACCOUNT = f"https://{STORAGE_ACCOUNT_NAME}.blob.core.windows.net"
VDS_URL = f"{STORAGE_ACCOUNT}/{CONTAINER}/{VDS}"
SAS_KEY = "sas"

SAMPLES10_URL = f"{STORAGE_ACCOUNT}/{CONTAINER}/10_samples/10_samples_default"


def gen_default_sas():
    return generate_container_signature(STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)


def surface():
    return {
        "xinc": 7.2111,
        "yinc": 3.6056,
        "xori": 2,
        "yori": 0,
        "rotation": 33.69,
    }


def slice_payload(direction="inline", lineno=3):
    return {"direction": direction, "lineno": lineno}


def fence_payload(coordinate_system="ij", coordinates=[[0, 0]]):
    return {"coordinateSystem": coordinate_system, "coordinates": coordinates}


def connection_payload(vds: list = None, sas: list = None, binary_operator: str = None) -> dict:
    payload = {}
    if vds is None:
        payload["vds"] = [VDS_URL]
    else:
        payload["vds"] = vds

    if sas is None:
        payload["sas"] = [SAS_KEY]
    else:
        payload["sas"] = sas

    if len(payload["vds"]) == 1 and binary_operator is not None:
        payload["vds"].append(payload["vds"][0])

    if len(payload["vds"]) == 2 and len(payload["sas"]) == 1:
        payload["sas"].append(payload["sas"][0])

    if binary_operator is not None:
        payload["binary_operator"] = binary_operator

    return payload


def payload_merge(connect: dict, specific: dict) -> dict:
    connect.update(specific)
    return connect


def attributes_along_surface_payload(
        surface=surface(),
        values=[[20]],
        above=8,
        below=8,
        attributes=["samplevalue"]):

    regular_surface = {
        "values": values,
        "fillValue": -999.25,
    }
    regular_surface.update(surface)

    return {"surface": regular_surface,
            "interpolation": "nearest",
            "above": above,
            "below": below,
            "attributes": attributes}


def attributes_between_surfaces_payload(
        primaryValues=[[20]],
        secondaryValues=[[20]],
        surface=surface(),
        attributes=["max"],
        stepsize=8):

    fillValue = -999.25
    primary = {
        "values": primaryValues,
        "fillValue": fillValue
    }
    primary.update(surface)

    secondary = {
        "values": secondaryValues,
        "fillValue": fillValue
    }
    secondary.update(surface)

    return {"primarySurface": primary,
            "secondarySurface": secondary,
            "interpolation": "nearest",
            "stepsize": stepsize,
            "attributes": attributes}


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


def process_data_response(response):
    response.raise_for_status()
    multipart_data = decoder.MultipartDecoder.from_response(response)
    assert len(multipart_data.parts) == 2
    metadata = json.loads(multipart_data.parts[0].content)
    data = multipart_data.parts[1].content
    data = np.ndarray(metadata['shape'], metadata['format'], data)
    return metadata, data


def request_metadata(method):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)
    payload = connection_payload(sas=[sas])
    response_data = send_request("metadata", method, payload)
    response_data.raise_for_status()
    return response_data.json()


def request_slice(method, lineno, direction):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)
    payload = connection_payload(sas=[sas])
    payload.update(slice_payload(direction, lineno))
    response = send_request("slice", method, payload)
    return process_data_response(response)


def request_fence(method: str, coordinate_system: str = "ij", coordinates: list = [[0, 0]]):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)
    payload = connection_payload(sas=[sas])
    payload.update(fence_payload(coordinate_system, coordinates))
    response = send_request("fence", method, payload)
    return process_data_response(response)


def request_attributes_along_surface(method, values):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)
    payload = connection_payload(vds=[SAMPLES10_URL], sas=[sas])
    payload.update(attributes_along_surface_payload(values=values))
    response = send_request("attributes/surface/along", method, payload)
    return process_data_response(response)


def request_attributes_between_surfaces(method, primary, secondary):
    sas = generate_container_signature(
        STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)
    payload = connection_payload(vds=[SAMPLES10_URL], sas=[sas])
    payload.update(attributes_between_surfaces_payload(primary, secondary))
    response = send_request("attributes/surface/between", method, payload)
    return process_data_response(response)
