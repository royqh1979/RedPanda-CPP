#!/bin/bash

set -euxo pipefail

. version.inc
TEST_VERSION="$(git rev-list HEAD --count)"
if [[ -n "$APP_VERSION_SUFFIX" ]]; then
  VERSION="$APP_VERSION.$TEST_VERSION.$APP_VERSION_SUFFIX"
else
  VERSION="$APP_VERSION.$TEST_VERSION"
fi
DEB_FILE="redpanda-cpp-bin_${VERSION}_amd64.deb"

TMP_FOLDER="$(mktemp -d)"

podman run -it --rm -v "$PWD:/mnt" -w /mnt -v "$TMP_FOLDER:/out" quay.io/redpanda-cpp/appimage-builder-x86_64:20240610.0 packages/debian-static/01-in-docker.sh

mkdir -p dist
mkdir -m 755 -p "$TMP_FOLDER/DEBIAN"
sed "s/__VERSION__/$VERSION/" packages/debian-static/control.in >"$TMP_FOLDER/DEBIAN/control"
dpkg-deb --build "$TMP_FOLDER" "dist/$DEB_FILE"
