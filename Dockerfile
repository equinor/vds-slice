ARG OPENVDS_IMAGE=openvds
ARG VDSSLICE_BASEIMAGE=golang:1.20-alpine3.17
FROM ${VDSSLICE_BASEIMAGE} as openvds
RUN apk --no-cache add \
    git \
    g++ \
    gcc \
    make \
    cmake \
    boost-dev \
    util-linux-dev \
    perl

WORKDIR /
RUN git clone https://community.opengroup.org/osdu/platform/domain-data-mgmt-services/seismic/open-vds.git
WORKDIR /open-vds
RUN git checkout 3.3.3

RUN cmake -S . \
    -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_JAVA=OFF \
    -DBUILD_PYTHON=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_DOCS=OFF \
    -DDISABLE_AWS_IOMANAGER=ON \
    -DDISABLE_AZURESDKFORCPP_IOMANAGER=OFF \
    -DDISABLE_GCP_IOMANAGER=ON \
    -DDISABLE_DMS_IOMANAGER=OFF \
    -DDISABLE_STRICT_WARNINGS=OFF
RUN cmake --build build   --config Release  --target install  -j 8 --verbose


FROM $OPENVDS_IMAGE as builder
WORKDIR /src
COPY go.mod go.sum ./
RUN go mod download && go mod verify
COPY . .
ARG CGO_CPPFLAGS="-I/open-vds/Dist/OpenVDS/include"
ARG CGO_LDFLAGS="-L/open-vds/Dist/OpenVDS/lib"
RUN go build -a ./...
RUN GOBIN=/tools go install github.com/swaggo/swag/cmd/swag@v1.16.2
RUN /tools/swag init --dir cmd/query,api,internal/core -g main.go --md docs


FROM builder as tester
ARG CGO_CPPFLAGS="-I/open-vds/Dist/OpenVDS/include"
ARG CGO_LDFLAGS="-L/open-vds/Dist/OpenVDS/lib"
ARG LD_LIBRARY_PATH=/open-vds/Dist/OpenVDS/lib:$LD_LIBRARY_PATH

ENV CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH:/open-vds/Dist/OpenVDS
RUN cmake -S . -B build
RUN cmake --build build
RUN ctest --test-dir build

ARG OPENVDS_AZURESDKFORCPP=1
RUN go test -race ./...


FROM builder as static_analyzer
ARG CGO_CPPFLAGS="-I/open-vds/Dist/OpenVDS/include"
ARG CGO_LDFLAGS="-L/open-vds/Dist/OpenVDS/lib"
ARG STATICCHECK_VERSION="2023.1.2"
ARG LD_LIBRARY_PATH=/open-vds/Dist/OpenVDS/lib:$LD_LIBRARY_PATH
RUN apk --no-cache add curl
RUN curl \
    -L https://github.com/dominikh/go-tools/releases/download/${STATICCHECK_VERSION}/staticcheck_linux_amd64.tar.gz \
    -o staticcheck-${STATICCHECK_VERSION}.tar.gz
RUN ls && tar xf staticcheck-${STATICCHECK_VERSION}.tar.gz
RUN ./staticcheck/staticcheck ./...


FROM builder as installer
ARG CGO_CPPFLAGS="-I/open-vds/Dist/OpenVDS/include"
ARG CGO_LDFLAGS="-L/open-vds/Dist/OpenVDS/lib"
ARG LD_LIBRARY_PATH=/open-vds/Dist/OpenVDS/lib:$LD_LIBRARY_PATH
RUN GOBIN=/server go install -a ./...

FROM ${VDSSLICE_BASEIMAGE} as runner
RUN apk --no-cache add \
    jemalloc-dev

WORKDIR /server
COPY --from=installer /open-vds/Dist/OpenVDS/lib/* /open-vds/
COPY --from=installer /server /server
COPY --from=installer /src/docs/index.html /server/docs/

RUN addgroup -S -g 1001 radix-non-root-group
RUN adduser -S -u 1001 -G radix-non-root-group radix-non-root-user
USER 1001

ENV LD_LIBRARY_PATH=/open-vds:$LD_LIBRARY_PATH
ENV OPENVDS_AZURESDKFORCPP=1
ENV LD_PRELOAD=/usr/lib/libjemalloc.so
ENTRYPOINT [ "/server/query" ]
