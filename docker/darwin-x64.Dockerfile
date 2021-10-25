# This image uses the context from scripts directory
# To build, run the following command from the project root directory:
# docker build -t i96751414/torrest-cpp-darwin-x64:latest -f docker/darwin-x64.Dockerfile scripts/

ARG CROSS_COMPILER_TAG=latest
FROM i96751414/cross-compiler-darwin-x64:${CROSS_COMPILER_TAG}

ENV PREFIX "${CROSS_ROOT}"
ENV BOOST_CONFIG "using clang : : ${CROSS_TRIPLE}-c++ -fvisibility=hidden -fvisibility-inlines-hidden ;"
ENV BOOST_OPTS target-os=darwin
ENV OPENSSL_PLATFORM darwin64-x86_64-cc
ENV OPENSSL_CROSS_COMPILE "${CROSS_TRIPLE}-"
# ENV CMAKE_TOOLCHAIN_FILE is already set on the base image

# Fix Boost using wrong archiver / ignoring <archiver> flags
# https://svn.boost.org/trac/boost/ticket/12573
# https://github.com/boostorg/build/blob/boost-1.63.0/src/tools/clang-darwin.jam#L133
RUN for i in ar strip ranlib; do \
        mv "/usr/bin/${i}" "/usr/bin/${i}.orig" \
        && ln -sf "${CROSS_ROOT}/bin/${CROSS_TRIPLE}-${i}" "/usr/bin/${i}"; \
    done

COPY install_dependencies.sh versions.env /tmp/
RUN /tmp/install_dependencies.sh --static \
    && rm /tmp/*

# Move back ar, strip and ranlib...
RUN for i in ar strip ranlib; do \
      mv "/usr/bin/${i}.orig" "/usr/bin/${i}"; \
    done
