# This image uses the context from scripts directory
# To build, run the following command from the project root directory:
# docker build -t i96751414/torrest-cpp-windows-x64:latest -f docker/windows-x64.Dockerfile scripts/

ARG CROSS_COMPILER_TAG=latest
FROM i96751414/cross-compiler-windows-x64:${CROSS_COMPILER_TAG}

ENV PREFIX="${CROSS_ROOT}"
ENV BOOST_CONFIG="using gcc : : ${CROSS_TRIPLE}-c++ ;"
ENV BOOST_OPTS="target-os=windows address-model=64 architecture=x86 threadapi=win32"
ENV OPENSSL_OPTS="mingw64"
ENV OPENSSL_CROSS_COMPILE="${CROSS_TRIPLE}-"
# ENV CMAKE_TOOLCHAIN_FILE is already set on the base image

COPY install_dependencies.sh versions.env /tmp/
RUN /tmp/install_dependencies.sh --static --fix-mingw-headers --default-to-win7 \
    && rm /tmp/*
