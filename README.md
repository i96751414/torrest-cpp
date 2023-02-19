# torrest-cpp

[![Build Status](https://github.com/i96751414/torrest-cpp/actions/workflows/build.yml/badge.svg)](https://github.com/i96751414/torrest-cpp/actions/workflows/build.yml)
[![cross-build](https://github.com/i96751414/torrest-cpp/actions/workflows/cross-compilers.yml/badge.svg)](https://github.com/i96751414/torrest-cpp/actions/workflows/cross-compilers.yml)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/f16ca71d4f034660ac593fafce2479b7)](https://www.codacy.com/gh/i96751414/torrest-cpp/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=i96751414/torrest-cpp&amp;utm_campaign=Badge_Grade)
[![CodeQL](https://github.com/i96751414/torrest-cpp/actions/workflows/codeql.yml/badge.svg)](https://github.com/i96751414/torrest-cpp/actions/workflows/codeql.yml)

Torrent service with a REST api, specially made for streaming. This is the C++ implementation of the
[Torrest (golang)](https://github.com/i96751414/torrest) project.

## Requirements

In order to build Torrest, you will need C++ (14 or newer) installed and also [CMake](https://cmake.org/) (at least
version 3.17). Additionally, you need a build system to actually schedule builds.

### Dependencies

Torrest has several external dependencies. Below is a list of them and the minimum version required:

-   [range-parser](https://github.com/i96751414/range-parser-cpp) (v1.0.1)
-   [nlohmann-json](https://github.com/nlohmann/json) (v3.11.0)
-   [spdlog](https://github.com/gabime/spdlog) (v1.8.5)
-   [oatpp](https://github.com/oatpp/oatpp) (v1.3.0)
-   [oatpp-swagger](https://github.com/oatpp/oatpp-swagger) (v1.3.0)
-   [openssl](https://www.openssl.org) (1.1.1f)
-   [boost](https://www.boost.org) (1.72.0)
-   [libtorrent](https://github.com/arvidn/libtorrent) (v1.2.14)

## Building

This section describes how to build Torrest using CMake.

### Installing dependencies

For local development, all the project dependencies can be installed automatically by running the
`scripts/install_dependencies.sh` script. For a better understanding of how to use the script, run:

```shell
./scripts/install_dependencies.sh --help
```

By default, the `install_dependencies.sh` script will use the versions specified in `scripts/versions.env` file. In case
you want to modify a version of a dependency, you can simply modify the `versions.env` file.

### Compiling Torrest

After installing the required dependencies, in order to build Torrest using CMake, simply run the following commands:

```shell
cmake -B cmake-build -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build --target torrest -- -j "$(nproc)"
```

Then, the `torrest` binary can be found inside the `cmake-build` directory. Note, the above command specifies a `Debug`
build type, however, different [CMAKE_BUILD_TYPE](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html)
values may be passed (e.g. `Release`).

### Build Configurations

Below is the list of supported CMake build configurations (which are then mapped to macros during the build):

| CMake configuration             | Default | Maps to macro                           | Description                                                    |
|---------------------------------|---------|-----------------------------------------|----------------------------------------------------------------|
| enable_swagger                  | ON      | TORREST_ENABLE_SWAGGER                  | Enables swagger on http://localhost:8080/swagger/ui endpoint   |
| with_swagger_local_resources    | OFF     | OATPP_SWAGGER_RES_PATH                  | Sets the swagger resources path to the oatpp-swagger directory |
| enable_shutdown                 | ON      | TORREST_ENABLE_SHUTDOWN                 | Enables the shutdown endpoint (http://localhost:8080/shutdown) |
| enable_extended_connections     | ON      | TORREST_EXTENDED_CONNECTIONS            | Enables oatpp extended connections                             |
| enable_torrent_buffering_status | OFF     | TORREST_ENABLE_TORRENT_BUFFERING_STATUS | Enables torrent buffering status                               |
| legacy_read_piece               | OFF     | TORREST_LEGACY_READ_PIECE               | Uses legacy read piece method (libtorrent v1 only)             |

## Cross Compiling

One can also cross compile Torrest to the platforms listed in the below table. To do so, multiple docker images were
created (see `docker` directory) in order to support this process.

| Platform        | Size                                                                                                                                                                                  | Dockerfile                                                         |
|-----------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------|
| dev (linux-x64) | [![dev](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-dev/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-dev)                               | [docker/dev.Dockerfile](docker/dev.Dockerfile)                     |
| android-arm     | [![android-arm](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-android-arm/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-android-arm)       | [docker/android-arm.Dockerfile](docker/android-arm.Dockerfile)     |
| android-arm64   | [![android-arm64](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-android-arm64/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-android-arm64) | [docker/android-arm64.Dockerfile](docker/android-arm64.Dockerfile) |
| android-x64     | [![android-x64](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-android-x64/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-android-x64)       | [docker/android-x64.Dockerfile](docker/android-x64.Dockerfile)     |
| android-x86     | [![android-x86](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-android-x86/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-android-x86)       | [docker/android-x86.Dockerfile](docker/android-x86.Dockerfile)     |
| darwin-x64      | [![darwin-x64](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-darwin-x64/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-darwin-x64)          | [docker/darwin-x64.Dockerfile](docker/darwin-x64.Dockerfile)       |
| linux-armv7     | [![linux-armv7](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-linux-armv7/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-linux-armv7)       | [docker/linux-armv7.Dockerfile](docker/linux-armv7.Dockerfile)     |
| linux-arm64     | [![linux-arm64](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-linux-arm64/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-linux-arm64)       | [docker/linux-arm64.Dockerfile](docker/linux-arm64.Dockerfile)     |
| linux-x64       | [![linux-x64](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-linux-x64/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-linux-x64)             | [docker/linux-x64.Dockerfile](docker/linux-x64.Dockerfile)         |
| linux-x86       | [![linux-x86](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-linux-x86/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-linux-x86)             | [docker/linux-x86.Dockerfile](docker/linux-x86.Dockerfile)         |
| windows-x64     | [![windows-x64](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-windows-x64/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-windows-x64)       | [docker/windows-x64.Dockerfile](docker/windows-x64.Dockerfile)     |
| windows-x86     | [![windows-x86](https://img.shields.io/docker/image-size/i96751414/torrest-cpp-windows-x86/latest)](https://hub.docker.com/repository/docker/i96751414/torrest-cpp-windows-x86)       | [docker/windows-x86.Dockerfile](docker/windows-x86.Dockerfile)     |

### Building images

In order to build all docker images, simply run the following command:

```shell
make -f docker/Makefile build
```

For building a specific image, run:

```shell
make -f docker/Makefile build-<platform>
```

### Cross compiling Torrest

Similarly, one can also build Torrest using the pre-built docker image. To do so, one needs to use the `torrest` target:

```shell
make -f docker/Makefile torrest
```

Or for a specific platform:

```shell
make -f docker/Makefile torrest-<platform>
```

## Running Torrest

After building Torrest, one can run it like any other binary, that is:

```shell
./torrest
```

Although Torrest doesn't have any required arguments, it accepts optional arguments:

| Argument       | Type   | Default       | Description            |
|----------------|--------|---------------|------------------------|
| -p, --port     | uint16 | 8080          | The server listen port |
| -s, --settings | string | settings.json | The settings path      |
| --log-level    | string | INFO          | The global log level   |
| -v, --version  | n/a    | n/a           | Print version          |
| -h, --help     | n/a    | n/a           | Print help message     |
