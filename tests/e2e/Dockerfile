FROM python:3.13.0a3-slim AS e2etest

COPY ./tests/e2e/requirements-dev.txt /tests/e2e/requirements-dev.txt
RUN python -m pip install --upgrade pip
RUN pip install -r /tests/e2e/requirements-dev.txt

COPY ./tests/utils /tests/utils
COPY ./tests/e2e /tests/e2e

WORKDIR /tests
ENTRYPOINT ["python","-m", "pytest", "-s", "-rsx", "e2e"]
