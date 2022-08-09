ACCOUNT_NAME=
SAS=

python make_prestack.py prestack.segy
SEGYImport --prestack --url "file://." --vdsfile prestack_default.vds prestack.segy
VDSCopy "prestack_default.vds" "azureSAS://$ACCOUNT_NAME.blob.core.windows.net/testdata/prestack/prestack_default" --compression-method=None -d "Suffix=?$SAS"

# or import directly to the cloud (and see note about decompression)
