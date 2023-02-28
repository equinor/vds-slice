for format in 1 2 3 5 8 10 11 16
do
    filename=format$format
    python make_formats.py $filename.segy $format
    SEGYImport --url "file://." --vdsfile $filename.vds $filename.segy
done
