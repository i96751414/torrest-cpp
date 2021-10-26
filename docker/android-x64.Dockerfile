# This image uses the context from scripts directory
# To build, run the following command from the project root directory:
# docker build -t i96751414/torrest-cpp-android-x64:latest -f docker/android-x64.Dockerfile scripts/

ARG CROSS_COMPILER_TAG=latest
FROM i96751414/cross-compiler-android-x64:${CROSS_COMPILER_TAG}

ENV PREFIX "${CROSS_ROOT}"
ENV BOOST_CONFIG "using clang : : ${CROSS_TRIPLE}-clang++ ;"
ENV BOOST_OPTS target-os=linux cxxflags=-fPIC cflags=-fPIC
ENV OPENSSL_OPTS linux-generic64 -fPIC
ENV OPENSSL_CROSS_COMPILE "${CROSS_TRIPLE}-"
# ENV CMAKE_TOOLCHAIN_FILE is already set on the base image

COPY install_dependencies.sh versions.env /tmp/
RUN /tmp/install_dependencies.sh --static \
    && rm /tmp/*