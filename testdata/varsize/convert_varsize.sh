ACCOUNT_NAME=
SAS=
ILINES=
XLINES=
SAMPLES=

python make_varsize.py varsize.segy $ILINES $XLINES $SAMPLES
SEGYImport --url "file://." --vdsfile varsize.vds varsize.segy
VDSCopy "varsize.vds" "azureSAS://$ACCOUNT_NAME.blob.core.windows.net/testdata/varsize/varsize_${ILINES}_${XLINES}_${SAMPLES}" --compression-method=None -d "Suffix=?$SAS"
