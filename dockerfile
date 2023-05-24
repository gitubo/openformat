FROM alpine:latest as development


COPY dl-cdn.alpinelinux.org.crt /usr/local/share/ca-certificates/mycert.crt
RUN cat /usr/local/share/ca-certificates/mycert.crt >> /etc/ssl/certs/ca-certificates.crt
RUN echo "" >> /etc/ssl/certs/ca-certificates.crt
COPY zscaler_root_ca.crt /usr/local/share/ca-certificates/mycert.crt
RUN cat /usr/local/share/ca-certificates/mycert.crt >> /etc/ssl/certs/ca-certificates.crt
RUN echo "" >> /etc/ssl/certs/ca-certificates.crt

RUN apk update --no-cache && apk upgrade --no-cache 
RUN apk add --no-cache ca-certificates openssl
RUN update-ca-certificates

RUN apk add --no-cache \
        build-base \
        cmake \
        gcc \
        g++ \
        make \
        gdb \
        git \
        curl \
        wget \
        libc-dev \
        c-ares-dev \
        re2-dev
RUN apk add --no-cache \
        protobuf \
        protobuf-dev \
        grpc \
        grpc-dev 
RUN apk add --no-cache \
        valgrind

WORKDIR /app

RUN wget https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.zip
RUN unzip v3.11.2.zip

## COMPILING APP
COPY proto ./proto
WORKDIR /app/proto
RUN mkdir -p cpp
RUN protoc --proto_path=. --cpp_out=cpp --grpc_out=cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` service.proto
RUN mkdir python
RUN protoc --python_out=python --grpc_out=python --plugin=protoc-gen-grpc=`which grpc_python_plugin` service.proto
WORKDIR /app
COPY test ./test
COPY catalog ./catalog
COPY include ./include
COPY src ./src
COPY CMakeLists.txt .
WORKDIR /app/build
RUN cmake ..
RUN make -j 4

## PRODUCTION
FROM alpine:latest as production

COPY dl-cdn.alpinelinux.org.crt /usr/local/share/ca-certificates/mycert.crt
RUN cat /usr/local/share/ca-certificates/mycert.crt >> /etc/ssl/certs/ca-certificates.crt
RUN echo "" >> /etc/ssl/certs/ca-certificates.crt
COPY zscaler_root_ca.crt /usr/local/share/ca-certificates/mycert.crt
RUN cat /usr/local/share/ca-certificates/mycert.crt >> /etc/ssl/certs/ca-certificates.crt
RUN echo "" >> /etc/ssl/certs/ca-certificates.crt

RUN apk update --no-cache && apk upgrade --no-cache 
RUN apk add --no-cache ca-certificates openssl
RUN update-ca-certificates

RUN apk add --no-cache grpc-dev 
WORKDIR /app
COPY --from=development /app/catalog /catalog
COPY --from=development /app/build/openformat /app/openformat 
COPY --from=development /app/build/client /app/client
CMD ["./openformat"]
