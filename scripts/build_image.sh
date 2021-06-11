#!/bin/bash
set -e

scripts_path=$(dirname "$(readlink -f "$0")")
image_name=i96751414/libtorrent-cpp
publish_image=false
publish_latest=false

function usage() {
  cat <<EOF
Usage: $(basename "${0}") -t <tag> [OPTIONS]

Script for building and (optionally) push the simple-build dockerfile.

required arguments:
  -t TAG    Tag of the docker image (e.g: 0.0.1)

optional arguments:
  -n        Name of the docker image (default: ${image_name})
  -p        Push the docker image
  -l        Also publish the latest image
  -h        Show this message

EOF
  exit "$1"
}

while getopts "plt:n:h" opt; do
  case "${opt}" in
  p) publish_image=true ;;
  l) publish_latest=true ;;
  t) image_tag="${OPTARG}" ;;
  n) image_name="${OPTARG}" ;;
  h) usage 0 ;;
  *) echo "Invalid option/argument provided: $1" && usage 1 ;;
  esac
done

if [ -z "${image_tag}" ]; then
  echo "Image tag is mandatory!"
  usage 1
fi

docker build --pull -t "${image_name}:${image_tag}" "${scripts_path}"

if [ "${publish_image}" = true ]; then
  docker push "${image_name}:${image_tag}"
  if [ "${publish_latest}" = true ]; then
    docker tag "${image_name}:${image_tag}" "${image_name}:latest"
    docker push "${image_name}:latest"
  fi
fi
