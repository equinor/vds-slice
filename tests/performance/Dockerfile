FROM python:3.10-slim
RUN apt-get update && apt-get install -y \
    curl

ARG k6_version=0.49.0
RUN mkdir k6
RUN curl \
    -L https://github.com/grafana/k6/releases/download/v${k6_version}/k6-v${k6_version}-linux-amd64.tar.gz \
    -o k6-${k6_version}.tar.gz
RUN tar xzf k6-${k6_version}.tar.gz -C k6 --strip-components=1

ENV PATH "$PATH:k6"

COPY ./tests/utils /tests/utils
COPY ./tests/performance /tests/performance
RUN python -m pip install -r /tests/performance/requirements-dev.txt
ENV PYTHONPATH "${PYTHONPATH}:/tests"
