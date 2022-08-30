FROM python:3.8-buster

ENV version 0.38.0
RUN mkdir k6
RUN curl \
    -L https://github.com/grafana/k6/releases/download/v$version/k6-v$version-linux-amd64.tar.gz \
    -o k6-$version.tar.gz
RUN tar xf k6-$version.tar.gz -C k6 --strip-components=1

ENV PATH "$PATH:k6"

COPY ./tests/utils /tests/utils
COPY ./tests/performance /tests/performance
RUN python -m pip install -r /tests/performance/requirements-dev.txt
ENV PYTHONPATH "${PYTHONPATH}:/tests"
