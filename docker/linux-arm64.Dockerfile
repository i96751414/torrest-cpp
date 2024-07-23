# This image uses the context from scripts directory
# To build, run the following command from the project root directory:
# docker build -t i96751414/torrest-cpp-linux-arm64:latest -f docker/linux-arm64.Dockerfile scripts/

ARG CROSS_COMPILER_TAG=latest
FROM i96751414/cross-compiler-linux-arm64:${CROSS_COMPILER_TAG}

ENV PREFIX="${CROSS_ROOT}"
ENV CMAKE_OPTS="-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
ENV RANGE_PARSER_OPTS="${CMAKE_OPTS}"
ENV NLOHMANN_JSON_OPTS="${CMAKE_OPTS}"
ENV SPDLOG_OPTS="${CMAKE_OPTS}"
ENV OATPP_OPTS="-DOATPP_LINK_ATOMIC=OFF ${CMAKE_OPTS}"
ENV OATPP_SWAGGER_OPTS="${CMAKE_OPTS}"
ENV BOOST_CONFIG="using gcc : : ${CROSS_TRIPLE}-c++ ;"
ENV BOOST_OPTS="target-os=linux cxxflags=-fPIC cflags=-fPIC"
ENV OPENSSL_OPTS="linux-aarch64"
ENV OPENSSL_CROSS_COMPILE="${CROSS_TRIPLE}-"
ENV LIBTORRENT_OPTS="${CMAKE_OPTS}"
# ENV CMAKE_TOOLCHAIN_FILE is already set on the base image

COPY install_dependencies.sh versions.env /tmp/
RUN /tmp/install_dependencies.sh --static \
    && rm /tmp/*
