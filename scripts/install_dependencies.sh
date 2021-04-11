#!/bin/bash
set -eo pipefail

: "${SUDO:=sudo}"
: "${CXX_STANDARD:=14}"
: "${PREFIX:=/usr/local}"

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

optional arguments:
  --nlohmann-json   Build and install nlohmann json
  --spdlog          Build and install spdlog
  --oatpp           Build and install oatpp
  --oatpp-swagger   Build and install oatpp-swagger
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
allowed_opts='nlohmann-json|spdlog|oatpp|oatpp-swagger'
all=true

while [ $# -gt 0 ]; do
  case "$1" in
  -h | --help) usage ;;
  --*) [[ "${1:2}" =~ ^(${allowed_opts})$ ]] || usage "$1" && declare "${1//-/}"=true ;;
  *) usage "$1" ;;
  esac
  shift
  all=false
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
  wget -qO- "$1" | tar -C "${tmp_dir}" --strip=1 -xz
}

function cleanup() {
  rm -rf "${tmp_dir:?}/"*
}

function buildCmake() {
  mkdir "${cmake_build_dir}"
  # -DBUILD_SHARED_LIBS=OFF
  cmake -B "${cmake_build_dir}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=${CXX_STANDARD} -DCMAKE_INSTALL_PREFIX="${PREFIX}" "$@"
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
