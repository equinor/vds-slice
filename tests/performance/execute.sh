#!/usr/bin/env bash

# for some reason direct remote write from k6 to azure monitor doesn't work,
# hence local write to prometheus is used as a workaround. It could be removed
# if k6 -> Azure monitor begins to work

# prometheus doesn't use environment variables, so there is no other way to set
# secrets other then replace them in the file. Secret file path is also not
# implemented for azure at the moment
envsubst < /tests/performance/prometheus-template.yml > /tests/performance/prometheus.yml

prometheus --config.file=/tests/performance/prometheus.yml --web.enable-remote-write-receiver &

python /tests/performance/performance.py "$1"

