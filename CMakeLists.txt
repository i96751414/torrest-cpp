cmake_minimum_required(VERSION 3.17)
project(torrest
        DESCRIPTION "C++ implementation of torrest - a torrent streaming engine with a REST api"
        HOMEPAGE_URL "https://github.com/i96751414/torrest-cpp")

set(CMAKE_CXX_STANDARD 14)

find_package(nlohmann_json 3.9.0 REQUIRED)
find_package(spdlog 1.8.5 REQUIRED)
find_package(oatpp 1.2.5 REQUIRED)
find_package(oatpp-swagger 1.2.5 REQUIRED)

add_definitions(
        -DOATPP_SWAGGER_RES_PATH="${oatpp-swagger_INCLUDE_DIRS}/../bin/oatpp-swagger/res"
)

add_executable(torrest
        src/api/error_handler.cpp
        src/api/logger.cpp
        src/settings/settings.cpp
        src/main.cpp)

target_include_directories(torrest PRIVATE src)

target_link_libraries(torrest
        stdc++fs
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        oatpp::oatpp
        oatpp::oatpp-swagger)
