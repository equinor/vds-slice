FROM openvds as builder
COPY . /
RUN g++ \
    /internal/vds/vds.cpp \
    /tests/memory/memory.cpp \
    -I/open-vds/Dist/OpenVDS/include \
    -I/internal \
    -I/tests/memory \
    -L/open-vds/Dist/OpenVDS/lib \
    -lopenvds \
    -o /memory.out


FROM memoryrunner
COPY --from=builder /open-vds/Dist/OpenVDS/lib/* /open-vds/
ENV LD_LIBRARY_PATH=/open-vds

COPY ./tests /tests
COPY --from=builder /memory.out /tests/memory/memory.out

RUN python -m pip install -r /tests/memory/requirements-dev.txt
ENV PYTHONPATH "${PYTHONPATH}:/tests"

ENTRYPOINT [ "python", "/tests/memory/memory.py"]
