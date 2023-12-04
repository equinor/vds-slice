import json
import urllib
import requests

class PayLoad:

    def __init__(self,
                 _storage_account_name,
                 _storage_account_key,
                 _endpoint,
                 _container) -> None:

        self.storage_account_name = _storage_account_name
        self.storage_account_key = _storage_account_key
        self.endpoint = _endpoint
        self.container = _container
        self.vds = "well_known/well_known_default"
        self.storage_account = f"https://{self.storage_account_name}.blob.core.windows.net"
        self.vds_url = f"{self.storage_account}/{self.container}/{self.vds}"
        self.samples10_url = f"{self.storage_account}/{self.container}/10_samples/10_samples_default"

    def send_request(self, _path: str, _method: str, _payload: str):
        """Send a request to the API

        Args:
            _path (str): URL path
            _method (str): get or post
            _payload (str): Input parameters

        Raises:
            ValueError: _description_

        Returns:
            _type_: _description_
        """

        if _method == "get":
            json_payload = json.dumps(_payload)
            encoded_payload = urllib.parse.quote(json_payload)
            data = requests.get(
                f'{self.endpoint}/{_path}?query={encoded_payload}',
                timeout=10)
        elif _method == "post":
            data = requests.post(f'{self.endpoint}/{_path}', json=_payload,
                                 timeout=10)
        else:
            raise ValueError(f'Unknown method {_method}')
        return data

    def slice(self, vds=None, direction="inline", lineno=3, sas="sas"):
        if vds is None:
            vds = self.vds_url
        return {
            "vds": vds,
            "direction": direction,
            "lineno": lineno,
            "sas": sas
        }

    def fence(self, vds=None, coordinate_system="ij", coordinates=[[0, 0]], sas="sas"):
        if vds is None:
            vds = self.vds_url
        return {
            "vds": vds,
            "coordinateSystem": coordinate_system,
            "coordinates": coordinates,
            "sas": sas
        }

    def metadata(self, vds=None, sas="sas"):
        if vds is None:
            vds = self.vds_url
        return {
            "vds": vds,
            "sas": sas
        }

    def surface(self):
        return {
            "xinc": 7.2111,
            "yinc": 3.6056,
            "xori": 2,
            "yori": 0,
            "rotation": 33.69,
        }

    def attributes_along_surface(
        self,
        vds=None,
        surface=None,
        values=[[20]],
        above=8,
        below=8,
        attributes=["samplevalue"],
        sas="sas"
    ):
        if vds is None:
            vds = self.samples10_url
        if surface is None:
            surface = self.surface()
        regular_surface = {
            "values": values,
            "fillValue": -999.25,
        }
        regular_surface.update(surface)

        request = {
            "surface": regular_surface,
            "interpolation": "nearest",
            "vds": vds,
            "sas": sas,
            "above": above,
            "below": below,
            "attributes": attributes
        }
        return request

    def attributes_between_surfaces(
        self,
        primaryValues=[[20]],
        secondaryValues=[[20]],
        vds=None,
        surface=None,
        attributes=["max"],
        stepsize=8,
        sas="sas"
    ):
        if vds is None:
            vds = self.samples10_url
        if surface is None:
            surface = self.surface()
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
        request = {
            "primarySurface": primary,
            "secondarySurface": secondary,
            "interpolation": "nearest",
            "vds": vds,
            "sas": sas,
            "stepsize": stepsize,
            "attributes": attributes
        }
        request.update(surface)
        return request
