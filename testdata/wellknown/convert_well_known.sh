ACCOUNT_NAME=
SAS=

python make_well_known.py well_known.segy
SEGYImport --url "file://." --vdsfile well_known_default.vds well_known.segy
VDSCopy "well_known_default.vds" "azureSAS://$ACCOUNT_NAME.blob.core.windows.net/testdata/wellknown/well_known_default" --compression-method=None -d "Suffix=?$SAS"

# or import directly to the cloud (and see note about decompression)
