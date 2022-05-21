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

feature_option(BUILD_SHARED_LIBS "build torrest as a shared library" ON)
feature_option(static_runtime "build torrest with static runtime" OFF)

if (NOT BUILD_SHARED_LIBS)
    set(Boost_USE_STATIC_LIBS ON)
    set(OPENSSL_USE_STATIC_LIBS ON)
endif ()
if (static_runtime)
    set(Boost_USE_STATIC_RUNTIME ON)
endif ()

find_package(range_parser 1.0.1 REQUIRED)
find_package(nlohmann_json 3.9.0 REQUIRED)
find_package(spdlog 1.8.5 REQUIRED)
find_package(oatpp 1.3.0 REQUIRED)
find_package(oatpp-swagger 1.3.0 REQUIRED)
find_package(OpenSSL 1.1.1 REQUIRED)
find_package(Boost 1.72.0 REQUIRED COMPONENTS filesystem)
find_package(LibtorrentRasterbar 1.2.14 REQUIRED COMPONENTS torrent-rasterbar)

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

target_include_directories(torrest PRIVATE src PUBLIC {Boost_INCLUDE_DIRS})

target_link_libraries(torrest
        range_parser::range_parser
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        oatpp::oatpp
        oatpp::oatpp-swagger
        OpenSSL::Crypto OpenSSL::SSL
        ${Boost_FILESYSTEM_LIBRARY}
        LibtorrentRasterbar::torrent-rasterbar)

target_optional_compile_definitions(torrest PUBLIC FEATURE NAME enable_swagger DEFAULT ON
        DESCRIPTION "Enables swagger" ENABLED TORREST_ENABLE_SWAGGER)
target_optional_compile_definitions(torrest PUBLIC FEATURE NAME with_swagger_local_resources DEFAULT OFF
        DESCRIPTION "Enables swagger with local resources" ENABLED TORREST_ENABLE_SWAGGER
        OATPP_SWAGGER_RES_PATH="${oatpp-swagger_INCLUDE_DIRS}/../bin/oatpp-swagger/res")
target_optional_compile_definitions(torrest PUBLIC FEATURE NAME enable_shutdown DEFAULT ON
        DESCRIPTION "Enables shutdown endpoint" ENABLED TORREST_ENABLE_SHUTDOWN)
target_optional_compile_definitions(torrest PUBLIC FEATURE NAME enable_extended_connections DEFAULT ON
        DESCRIPTION "Enables extended connections" ENABLED TORREST_EXTENDED_CONNECTIONS)
target_optional_compile_definitions(torrest PUBLIC FEATURE NAME enable_torrent_buffering_status DEFAULT OFF
        DESCRIPTION "Enables torrent buffering status" ENABLED TORREST_ENABLE_TORRENT_BUFFERING_STATUS)

if (DEFINED target_path)
    add_custom_command(TARGET torrest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:torrest>" "${target_path}")
endif ()

feature_summary(DEFAULT_DESCRIPTION WHAT ALL)
