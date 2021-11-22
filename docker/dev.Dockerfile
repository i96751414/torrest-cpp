# This image uses the context from scripts directory
# To build, run the following command from the project root directory:
# docker build -t i96751414/torrest-cpp-dev:latest -f docker/dev.Dockerfile scripts/

FROM debian:stretch-slim

ARG CMAKE_VERSION=3.20.3

RUN apt-get update && apt-get install -y --no-install-recommends \
        wget make build-essential \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN wget "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
    --no-check-certificate -qO- | tar --strip-components=1 -xz --one-top-level=/usr

COPY install_dependencies.sh versions.env /tmp/
RUN /tmp/install_dependencies.sh \
    && rm /tmp/*
