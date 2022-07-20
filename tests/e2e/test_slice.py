import requests
import numpy as np
import os
from utils.cloud import generate_container_signature, generate_account_signature

STORAGE_ACCOUNT_NAME = os.getenv("STORAGE_ACCOUNT_NAME")
STORAGE_ACCOUNT_KEY = os.getenv("STORAGE_ACCOUNT_KEY")
ENDPOINT = os.getenv("ENDPOINT").rstrip("/")
CONTAINER = "testdata"
VDS = "defined/defined_default"

def getslice(lineno, direction):
    payload = {
        "direction": direction,
        "lineno": lineno,
        "vds": "{}/{}".format(CONTAINER, VDS),
        # container sas is not enough?
        # sas = generate_container_signature(STORAGE_ACCOUNT_NAME, CONTAINER, STORAGE_ACCOUNT_KEY)
        "sas": generate_account_signature(STORAGE_ACCOUNT_NAME, STORAGE_ACCOUNT_KEY)
    }

    print(payload)

    rmeta = requests.post(f'{ENDPOINT}/slice/metadata', data = payload)
    rdata = requests.post(f'{ENDPOINT}/slice', data = payload)
    rmeta.raise_for_status()

    meta = rmeta.json()
    shape = (
        meta['axis'][0]['samples'],
        meta['axis'][1]['samples']
    )
    data = np.ndarray(shape, 'f4', rdata.content)
    return meta, data


def test_inline_slice():
    _, slice = getslice(5, 'inline')
    print (slice)
    assert len(slice) == 2
    np.testing.assert_array_equal(slice[0], np.array([116, 117, 118, 119]))
    np.testing.assert_array_equal(slice[1], np.array([120, 121, 122, 123]))
