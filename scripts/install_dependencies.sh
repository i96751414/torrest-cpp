#!/bin/bash
set -eo pipefail

allowed_opts='range-parser|nlohmann-json|spdlog|oatpp|oatpp-swagger|openssl|boost|libtorrent'
scripts_path=$(dirname "$(readlink -f "$0")")
env_path="${scripts_path}/versions.env"
jobs=$(nproc)

: "${CXX_STANDARD:=14}"
: "${PREFIX:=/usr/local}"
: "${BOOST_CONFIG:="using gcc ;"}"

if [ -z "${CMD}" ] && [ "$(id -u)" != 0 ]; then
  if command -v sudo &>/dev/null; then
    CMD=sudo
  fi
fi

function printAllowedOptions() {
  IFS='|' read -ra options <<<"${allowed_opts}"
  for opt in "${options[@]}"; do
    printf '  --%-19s Build and install %s\n' "${opt}" "${opt}"
  done
}

function usage() {
  cat <<EOF
Usage: $(basename "${0}") [OPTIONS]

Script for building and install necessary dependencies in order to compile torrest-cpp.
If no arguments are provided, all dependencies are built and installed.

Additional environment variables can also be passed, such as:
  CMD (default: automatic)
  CXX_STANDARD (default: 14)
  PREFIX (default: /usr/local)
  OATPP_OPTS (default: not set)
  BOOST_CONFIG (default: "using gcc ;")
  BOOST_OPTS (default: not set)
  OPENSSL_OPTS (default: not set)
  LIBTORRENT_OPTS (default: not set)
  OPENSSL_CROSS_COMPILE (default: not set)
  CMAKE_TOOLCHAIN_FILE (default: not set)

optional arguments:
$(printAllowedOptions)
  --fix-mingw-headers   Fix mingw 'WinSock2' and 'WS2tcpip' headers name before building.
  -s, --static          Do a static build
  -r, --static-runtime  Build with static runtime
  -e, --env             Path of file containing versions environment variables (default: ${env_path})
  -j, --jobs            Build jobs number (default: ${jobs})
  -v, --verbose         Do a verbose build
  -h, --help            Show this message

EOF
  exit "$1"
}

function invalidOpt() {
  echo "Invalid option/argument provided: $1"
  usage 1
}

function validateFile() {
  if [ ! -f "${1}" ]; then
    echo "${2} requires a valid file"
    usage 1
  fi
}

function validateNumber() {
  if [[ ! "${1}" =~ ^[0-9]+$ ]] || [ "${1}" -le 0 ]; then
    echo "${2} requires a number greater than 0"
    exit 1
  fi
}

function checkRequirement() {
  local package=$1
  local apt_package=${2:-${package}}
  if ! command -v "${package}" &>/dev/null; then
    echo "'${package}' is not installed. Please install it by running 'sudo apt install ${apt_package}'"
    exit 1
  fi
}

# Parse options
all=true
static=false
static_runtime=false
fix_mingw_headers=false
verbose=false

while [ $# -gt 0 ]; do
  case "$1" in
  --) shift && break ;;
  -h | --help) usage 0 ;;
  -v | --verbose) verbose=true ;;
  -s | --static) static=true ;;
  -r | --static-runtime) static_runtime=true ;;
  -e | --env) validateFile "$2" "$1" && shift && env_path="$1" ;;
  -j | --jobs) validateNumber "$2" "$1" && shift && jobs="$1" ;;
  --fix-mingw-headers) fix_mingw_headers=true ;;
  --*) [[ "${1:2}" =~ ^(${allowed_opts})$ ]] || invalidOpt "$1" && declare "${1//-/}"=true && all=false ;;
  -*) invalidOpt "$1" ;;
  *) break ;;
  esac
  shift
done

if [ $# -gt 0 ]; then
  echo "No positional arguments expected"
  usage 1
fi

[ "${verbose}" == true ] && set -x

checkRequirement cmake
# shellcheck source=versions.env
source "${env_path}"

tmp_dir=$(mktemp -d /tmp/torrest-build-XXXXXXXXXXX)
trap 'rm -rf "${tmp_dir}"' EXIT
cd "${tmp_dir}"

cmake_build_dir="cmake-build"

function requires() {
  local name="${1//-/}"
  [ -n "${!name}" ] || [ "${all}" = true ] && return 0 || return 1
}

function download() {
  wget --no-check-certificate -qO- "$1" | tar -C "${tmp_dir}" --strip=1 -xz
}

function cleanup() {
  ${CMD} rm -rf "${tmp_dir:?}/"*
}

function buildCmake() {
  mkdir "${cmake_build_dir}"
  local cmake_options=(-B "${cmake_build_dir}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD="${CXX_STANDARD}"
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG" -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG")
  [ "${static}" == true ] && cmake_options+=(-DBUILD_SHARED_LIBS=OFF) || cmake_options+=(-DBUILD_SHARED_LIBS=ON)
  [ -n "${CMAKE_TOOLCHAIN_FILE}" ] && cmake_options+=(-DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}")
  cmake "${cmake_options[@]}" "$@"
  cmake --build "${cmake_build_dir}" -j"${jobs}"
  ${CMD} cmake --install "${cmake_build_dir}"
}

function mingwFixHeaders() {
  echo "- Fixing mingw headers"
  find "${tmp_dir}" -type f -exec sed -i -e 's/WinSock2.h/winsock2.h/i' -e 's/WS2tcpip.h/ws2tcpip.h/i' {} +
}

function parseArgsToArray() {
  ARGS=()
  while IFS= read -r -d ''; do
    ARGS+=("${REPLY}")
  done < <(xargs -r printf '%s\0' <<<"${1}")
}

if requires "range-parser"; then
  echo "- Downloading range-parser ${RANGE_PARSER_VERSION}"
  download "https://github.com/i96751414/range-parser-cpp/archive/${RANGE_PARSER_VERSION}.tar.gz"
  echo "- Building range-parser ${RANGE_PARSER_VERSION}"
  buildCmake
  cleanup
fi

if requires "nlohmann-json"; then
  echo "- Downloading nlohmann-json ${NLOHMANN_JSON_VERSION}"
  download "https://github.com/nlohmann/json/archive/${NLOHMANN_JSON_VERSION}.tar.gz"
  echo "- Building nlohmann-json ${NLOHMANN_JSON_VERSION}"
  buildCmake -DJSON_BuildTests=OFF
  cleanup
fi

if requires "spdlog"; then
  echo "- Downloading spdlog ${SPDLOG_VERSION}"
  download "https://github.com/gabime/spdlog/archive/${SPDLOG_VERSION}.tar.gz"
  buildCmake
  cleanup
fi

if requires "oatpp"; then
  echo "- Downloading oatpp ${OATPP_VERSION}"
  download "https://github.com/oatpp/oatpp/archive/${OATPP_VERSION}.tar.gz"
  [ "${fix_mingw_headers}" == true ] && mingwFixHeaders
  parseArgsToArray "${OATPP_OPTS}"
  echo "- Building oatpp ${OATPP_VERSION}"
  buildCmake -DOATPP_BUILD_TESTS=OFF "${ARGS[@]}"
  cleanup
fi

if requires "oatpp-swagger"; then
  echo "- Downloading oatpp-swagger ${OATPP_SWAGGER_VERSION}"
  download "https://github.com/oatpp/oatpp-swagger/archive/${OATPP_SWAGGER_VERSION}.tar.gz"
  echo "- Building oatpp-swagger ${OATPP_SWAGGER_VERSION}"
  buildCmake -DOATPP_BUILD_TESTS=OFF
  cleanup
fi

if requires "openssl"; then
  echo "- Downloading openssl ${OPENSSL_VERSION}"
  download "https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz"
  echo "- Building openssl ${OPENSSL_VERSION}"
  parseArgsToArray "${OPENSSL_OPTS}"
  opts=(threads no-shared "${ARGS[@]}" --prefix="${PREFIX}")
  [ "${static}" == true ] && opts+=(no-shared) || opts+=(shared)
  if [ -n "${OPENSSL_CROSS_COMPILE}" ]; then
    CROSS_COMPILE="${OPENSSL_CROSS_COMPILE}" ./Configure "${opts[@]}"
  else
    ./config "${opts[@]}"
  fi
  make -j"${jobs}"
  ${CMD} make install
  cleanup
fi

if requires "boost"; then
  echo "- Downloading boost ${BOOST_VERSION}"
  download "https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION//./_}.tar.gz"
  echo "- Building boost ${BOOST_VERSION}"
  ./bootstrap.sh --prefix="${PREFIX}"
  echo "${BOOST_CONFIG}" >user-config.jam
  boost_options=(-j"${jobs}" --with-date_time --with-system --with-filesystem --with-program_options --with-chrono --with-random
    --prefix="${PREFIX}" --user-config=user-config.jam variant=release threading=multi cxxflags=-std=c++"${CXX_STANDARD}")
  [ "${static}" == true ] && boost_options+=(link=static)
  [ "${static_runtime}" == true ] && boost_options+=(runtime-link=static)
  parseArgsToArray "${BOOST_OPTS}"
  ${CMD} ./b2 "${boost_options[@]}" "${ARGS[@]}" install
  cleanup
fi

if requires "libtorrent"; then
  echo "- Downloading libtorrent ${LIBTORRENT_VERSION}"
  download "https://github.com/arvidn/libtorrent/releases/download/${LIBTORRENT_VERSION}/libtorrent-rasterbar-${LIBTORRENT_VERSION#v}.tar.gz"
  # Fix for Windows - https://github.com/arvidn/libtorrent/issues/7190
  sed -i "s|\(-D_WIN32_WINNT\)=[[:digit:]x]\+|\1=0x0601|g" "${tmp_dir}/CMakeLists.txt"
  # Fix for MingGW - https://github.com/godotengine/godot/issues/59409
  # find "${tmp_dir}/src" -name "*.cpp" -type f -exec sed -i "s|\(#include <iphlpapi.h>\)|#include <wincrypt.h>\n\1|g" {} +
  echo "- Building libtorrent ${LIBTORRENT_VERSION}"
  parseArgsToArray "${LIBTORRENT_OPTS}"
  opts=("${ARGS[@]}" -Ddeprecated-functions=OFF)
  [ "${static_runtime}" == true ] && opts+=(-Dstatic_runtime=ON)
  buildCmake "${opts[@]}"
  cleanup
fi
