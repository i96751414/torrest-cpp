# torrest-cpp

[![Build Status](https://github.com/i96751414/torrest-cpp/actions/workflows/build.yml/badge.svg)](https://github.com/i96751414/torrest-cpp/actions/workflows/build.yml)
[![cross-build](https://github.com/i96751414/torrest-cpp/actions/workflows/cross.yml/badge.svg)](https://github.com/i96751414/torrest-cpp/actions/workflows/cross.yml)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/f16ca71d4f034660ac593fafce2479b7)](https://www.codacy.com/gh/i96751414/torrest-cpp/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=i96751414/torrest-cpp&amp;utm_campaign=Badge_Grade)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/i96751414/torrest-cpp.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/i96751414/torrest-cpp/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/i96751414/torrest-cpp.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/i96751414/torrest-cpp/context:cpp)

Torrent service with a REST api, specially made for streaming. This is the C++ implementation of the
[Torrest (golang)](https://github.com/i96751414/torrest) project.

## Requirements

In order to build Torrest, you will need C++ (14 or newer) installed and also [CMake](https://cmake.org/) (at least
version 3.17). Additionally, you need a build system to actually schedule builds.

### Dependencies

Torrest has several external dependencies. Below is a list of them and the minimum version required:

-   [range-parser](https://github.com/i96751414/range-parser-cpp) (v1.0.1)
-   [nlohmann-json](https://github.com/nlohmann/json) (v3.9.0)
-   [spdlog](https://github.com/gabime/spdlog) (v1.9.1)
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

|CMake configuration|Default|Maps to macro|Description|
|---|---|---|---|
|enable_swagger|ON|TORREST_ENABLE_SWAGGER|Enables swagger on http://localhost:8080/swagger/ui endpoint|
|with_swagger_local_resources|OFF|OATPP_SWAGGER_RES_PATH|Sets the swagger resources path to the oatpp-swagger directory|
|enable_shutdown|ON|TORREST_ENABLE_SHUTDOWN|Enables the shutdown endpoint (http://localhost:8080/shutdown)|
|enable_extended_connections|ON|TORREST_EXTENDED_CONNECTIONS|Enables oatpp extended connections|

## Cross Compiling

One can also cross compile Torrest to the platforms listed in the below table. To do so, multiple docker images were
created (see `docker` directory) in order to support this process.

|Android|Darwin|Linux|Windows|
|-------|------|-----|-------|
|android-arm<br/>android-arm64<br/>android-x64<br/>android-x86<br/>|darwin-x64<br/><br/><br/><br/>|linux-armv7<br/>linux-arm64<br/>linux-x64<br/>linux-x86|windows-x64<br/>windows-x86<br/><br/><br/>|

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

Although Torrest doesn't have any required arguments, it accepts two optional arguments:

|Argument|Type|Default|Description|
|---|---|---|---|
|--port|uint16|8080|The server listen port|
|--settings|string|settings.json|The settings path|
