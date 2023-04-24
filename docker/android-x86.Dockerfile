# This image uses the context from scripts directory
# To build, run the following command from the project root directory:
# docker build -t i96751414/torrest-cpp-android-x86:latest -f docker/android-x86.Dockerfile scripts/

ARG CROSS_COMPILER_TAG=latest
FROM i96751414/cross-compiler-android-x86:${CROSS_COMPILER_TAG}

ENV PREFIX "${CROSS_ROOT}"
ENV CMAKE_OPTS -DCMAKE_POSITION_INDEPENDENT_CODE=ON
ENV RANGE_PARSER_OPTS "${CMAKE_OPTS}"
ENV NLOHMANN_JSON_OPTS "${CMAKE_OPTS}"
ENV SPDLOG_OPTS "${CMAKE_OPTS}"
ENV OATPP_OPTS "-DOATPP_LINK_ATOMIC=OFF ${CMAKE_OPTS}"
ENV OATPP_SWAGGER_OPTS "${CMAKE_OPTS}"
ENV BOOST_CONFIG "using clang : : ${CROSS_TRIPLE}-clang++ ;"
ENV BOOST_OPTS target-os=android cxxflags=-fPIC cflags=-fPIC
ENV OPENSSL_OPTS android-x86 -fPIC
ENV OPENSSL_CROSS_COMPILE "${CROSS_TRIPLE}-"
ENV LIBTORRENT_OPTS "${CMAKE_OPTS}"
# ENV CMAKE_TOOLCHAIN_FILE is already set on the base image

COPY install_dependencies.sh versions.env /tmp/
COPY patches /tmp/patches
RUN /tmp/install_dependencies.sh --static --apply-android-patch \
    && rm -rf /tmp/*
