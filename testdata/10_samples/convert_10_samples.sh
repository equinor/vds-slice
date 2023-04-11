ACCOUNT_NAME=
SAS=

python make_10_samples.py 10_samples.segy
SEGYImport --url "file://." --vdsfile 10_samples_default.vds 10_samples.segy --crs-wkt="utmXX"
VDSCopy "10_samples_default.vds" "azureSAS://$ACCOUNT_NAME.blob.core.windows.net/testdata/10_samples/10_samples_default" --compression-method=None -d "Suffix=?$SAS"

# or import directly to the cloud (and see note about decompression)
