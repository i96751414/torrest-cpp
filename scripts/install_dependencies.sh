#!/bin/bash
set -eo pipefail

: "${SUDO:=sudo}"
: "${CXX_STANDARD:=14}"
: "${PREFIX:=/usr/local}"
: "${BOOST_CONFIG:="using gcc ;"}"

function usage() {
  [ -n "$1" ] && echo "Invalid option/argument provided: $1"
  cat <<EOF
Usage: $(basename "${0}") [OPTIONS]

Script for building and install necessary dependencies in order to compile torrest-cpp.
If no arguments are provided, all dependencies are built and installed.

Additional environment variables can also be passed, such as:
  SUDO (default: ${SUDO})
  CXX_STANDARD (default: ${CXX_STANDARD})
  PREFIX (default: ${PREFIX})
  BOOST_CONFIG (default: "${BOOST_CONFIG}")
  OPENSSL_PLATFORM (default: not set)
  OPENSSL_CROSS_COMPILE (default: not set)

optional arguments:
  --nlohmann-json   Build and install nlohmann json
  --spdlog          Build and install spdlog
  --oatpp           Build and install oatpp
  --oatpp-swagger   Build and install oatpp-swagger
  --openssl         Build and install openssl
  --boost           Build and install boost
  --libtorrent      Build and install libtorrent
  -s, --static      Do a static build
  -h, --help        Show this message

EOF
  [ -n "$1" ] && exit 1 || exit 0
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
allowed_opts='nlohmann-json|spdlog|oatpp|oatpp-swagger|openssl|boost|libtorrent'
all=true
static=false

while [ $# -gt 0 ]; do
  case "$1" in
  -h | --help) usage ;;
  -s | --static) static=true ;;
  --*)
    [[ "${1:2}" =~ ^(${allowed_opts})$ ]] || usage "$1" && declare "${1//-/}"=true
    all=false
    ;;
  *) usage "$1" ;;
  esac
  shift
done

checkRequirement cmake

scripts_path=$(dirname "$(readlink -f "$0")")
source "${scripts_path}/versions.env"

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
  ${SUDO} rm -rf "${tmp_dir:?}/"*
}

function buildCmake() {
  mkdir "${cmake_build_dir}"
  # -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain/file.cmake
  local cmake_options=(-B "${cmake_build_dir}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD="${CXX_STANDARD}"
    -DCMAKE_INSTALL_PREFIX="${PREFIX}")
  [ "${static}" == true ] && cmake_options+=(-DBUILD_SHARED_LIBS=OFF) || cmake_options+=(-DBUILD_SHARED_LIBS=ON)
  cmake "${cmake_options[@]}" "$@"
  cmake --build "${cmake_build_dir}" -j"$(nproc)"
  ${SUDO} cmake --install "${cmake_build_dir}"
}

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
  echo "- Building oatpp ${OATPP_VERSION}"
  buildCmake -DOATPP_BUILD_TESTS=OFF
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
  if [ -n "${OPENSSL_PLATFORM}" ]; then
    CROSS_COMPILE="${OPENSSL_CROSS_COMPILE}" ./Configure threads no-shared "${OPENSSL_PLATFORM}" --prefix="${PREFIX}"
  else
    ./config threads no-shared --prefix="${PREFIX}"
  fi
  make -j"$(nproc)"
  ${SUDO} make install
  cleanup
fi

if requires "boost"; then
  echo "- Downloading boost ${BOOST_VERSION}"
  download "https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION//./_}.tar.gz"
  echo "- Building boost ${BOOST_VERSION}"
  ./bootstrap.sh --prefix="${PREFIX}"
  echo "${BOOST_CONFIG}" >user-config.jam
  boost_options=(--with-date_time --with-system --with-chrono --with-random --prefix="${PREFIX}"
    --user-config=user-config.jam variant=release threading=multi cxxflags=-std=c++"${CXX_STANDARD}")
  [ "${static}" == true ] && boost_options+=(link=static)
  ${SUDO} ./b2 "${boost_options[@]}" install
  cleanup
fi

if requires "libtorrent"; then
  echo "- Downloading libtorrent ${LIBTORRENT_VERSION}"
  download "https://github.com/arvidn/libtorrent/archive/${LIBTORRENT_VERSION//./_}.tar.gz"
  echo "- Building libtorrent ${LIBTORRENT_VERSION}"
  buildCmake -Ddeprecated-functions=OFF
  cleanup
fi
