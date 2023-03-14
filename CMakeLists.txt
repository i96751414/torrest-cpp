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

feature_option(build_library "Build torrest as a library" OFF)
feature_option(static_runtime "Build torrest with static runtime" OFF)

if (static_runtime)
    set(Boost_USE_STATIC_RUNTIME ON)
endif ()

find_package(range_parser 1.0.1 REQUIRED)
find_package(nlohmann_json 3.11.0 REQUIRED)
find_package(spdlog 1.8.5 REQUIRED)
find_package(oatpp 1.3.0 REQUIRED)
find_package(oatpp-swagger 1.3.0 REQUIRED)
find_package(OpenSSL 1.1.1 REQUIRED)
find_package(Boost 1.72.0 REQUIRED COMPONENTS filesystem program_options)
find_package(LibtorrentRasterbar 1.2.14 REQUIRED COMPONENTS torrent-rasterbar)

set(torrest_sources
        src/utils/log.cpp
        src/utils/mime.cpp
        src/utils/utils.cpp
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
        src/api/connection_provider.cpp
        src/torrest.cpp
        src/main.cpp)

if (build_library)
    feature_option(BUILD_SHARED_LIBS "Build torrest as a shared library" ON)
    add_library(torrest ${torrest_sources})
    target_compile_definitions(torrest PUBLIC TORREST_LIBRARY)
else ()
    add_executable(torrest ${torrest_sources})
endif ()

target_include_directories(torrest PRIVATE src PUBLIC {Boost_INCLUDE_DIRS})

target_link_libraries(torrest
        range_parser::range_parser
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        oatpp::oatpp
        oatpp::oatpp-swagger
        OpenSSL::Crypto OpenSSL::SSL
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        LibtorrentRasterbar::torrent-rasterbar)

get_filename_component(default_swagger_local_resources "${oatpp-swagger_INCLUDE_DIRS}/../bin/oatpp-swagger/res" ABSOLUTE)
feature(swagger_local_resources STRING with_swagger_local_resources
        "Swagger local resources path (to use when with_swagger_local_resources=ON)"
        "${default_swagger_local_resources}")

target_optional_compile_definitions(torrest PUBLIC FEATURE NAME enable_swagger DEFAULT ON
        DESCRIPTION "Enables swagger" ENABLED TORREST_ENABLE_SWAGGER)
target_optional_compile_definitions(torrest PUBLIC FEATURE NAME with_swagger_local_resources DEFAULT OFF
        DESCRIPTION "Enables swagger with local resources" ENABLED TORREST_ENABLE_SWAGGER
        "OATPP_SWAGGER_RES_PATH=\"${swagger_local_resources}\"")
target_optional_compile_definitions(torrest PUBLIC FEATURE NAME enable_shutdown DEFAULT ON
        DESCRIPTION "Enables shutdown endpoint" ENABLED TORREST_ENABLE_SHUTDOWN)
target_optional_compile_definitions(torrest PUBLIC FEATURE NAME enable_extended_connections DEFAULT ON
        DESCRIPTION "Enables extended connections" ENABLED TORREST_EXTENDED_CONNECTIONS)
target_optional_compile_definitions(torrest PUBLIC FEATURE NAME enable_torrent_buffering_status DEFAULT OFF
        DESCRIPTION "Enables torrent buffering status" ENABLED TORREST_ENABLE_TORRENT_BUFFERING_STATUS)
target_optional_compile_definitions(torrest PUBLIC FEATURE NAME legacy_read_piece DEFAULT OFF
        DESCRIPTION "Uses legacy read piece method" ENABLED TORREST_LEGACY_READ_PIECE)

if (LibtorrentRasterbar_VERSION VERSION_GREATER_EQUAL 2 AND legacy_read_piece)
    message(WARNING "legacy_read_piece not supported on libtorrent ${LibtorrentRasterbar_VERSION}")
endif ()

add_feature_info(target_path target_path "Path where to copy the target")
if (DEFINED target_path)
    add_custom_command(TARGET torrest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:torrest>" "${target_path}")
endif ()

feature_summary(DEFAULT_DESCRIPTION WHAT ALL)
