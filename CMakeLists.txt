cmake_minimum_required(VERSION 3.17)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(TorrestMacros)

read_version("${CMAKE_CURRENT_SOURCE_DIR}/src/version.h" TORREST_VERSION)

project(torrest
        VERSION ${TORREST_VERSION}
        DESCRIPTION "C++ implementation of torrest - a torrent streaming engine with a REST api"
        HOMEPAGE_URL "https://github.com/i96751414/torrest-cpp")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json 3.9.0 REQUIRED)
find_package(spdlog 1.8.5 REQUIRED)
find_package(oatpp 1.2.5 REQUIRED)
find_package(oatpp-swagger 1.2.5 REQUIRED)
find_package(LibtorrentRasterbar 1.2.14 REQUIRED COMPONENTS torrent-rasterbar)

add_definitions(
        -DOATPP_SWAGGER_RES_PATH="${oatpp-swagger_INCLUDE_DIRS}/../bin/oatpp-swagger/res"
        -DTORREST_ENABLE_SHUTDOWN
        -DTORREST_ENABLE_SWAGGER
)

add_executable(torrest
        src/utils/mime.cpp
        src/utils/conversion.cpp
        src/settings/settings.cpp
        src/bittorrent/service.cpp
        src/bittorrent/torrent.cpp
        src/bittorrent/file.cpp
        src/bittorrent/reader.cpp
        src/api/body/empty_body.cpp
        src/api/body/reader_body.cpp
        src/api/error_handler.cpp
        src/api/logger.cpp
        src/api/logger_interceptor.cpp
        src/main.cpp)

target_include_directories(torrest PRIVATE src)

target_link_libraries(torrest
        stdc++fs
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        oatpp::oatpp
        oatpp::oatpp-swagger
        LibtorrentRasterbar::torrent-rasterbar)
