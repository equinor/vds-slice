# project is for the moment server only (no client in the same repo), so it deserves to be in the root dir

FROM golang:1.18 as builder

ENV DEBIAN_FRONTEND="noninteractive"
RUN apt-get update
RUN apt-get install -y \
    build-essential \
    git curl \
    libboost-dev \
    libcurl4-openssl-dev \
    libuv1-dev \
    libboost-all-dev \
    libxml2-dev \
    libfmt-dev \
    libssl-dev \
    uuid-dev \
    libpng-dev cmake


WORKDIR /
RUN git clone https://community.opengroup.org/erha/open-vds.git
WORKDIR /open-vds
RUN git checkout enable-cache

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


WORKDIR /src
COPY go.mod go.sum ./
RUN go mod download && go mod verify

ARG CGO_CPPFLAGS="-I/open-vds/Dist/OpenVDS/include"
ARG CGO_LDFLAGS="-L/open-vds/Dist/OpenVDS/lib"

COPY . .
RUN GOBIN=/server go install -a ./...

FROM golang:1.18
ENV DEBIAN_FRONTEND="noninteractive"
RUN apt-get update
RUN apt-get install -y openssl libuv1  libgomp1  libcurl4 libxml2 libboost-log1.74.0


COPY --from=builder /open-vds/Dist/OpenVDS/lib/* /lib/x86_64-linux-gnu/
COPY --from=builder /server /server

RUN groupadd -r user && useradd -r -g user user
USER 999

ENV OPENVDS_AZURESDKFORCPP=1
ENTRYPOINT [ "/server/query" ]
