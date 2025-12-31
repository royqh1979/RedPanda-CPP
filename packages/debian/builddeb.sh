#!/bin/bash

set -xeuo pipefail

TMP_FOLDER=/tmp/redpanda-cpp
[[ -d $TMP_FOLDER ]] && rm -rf $TMP_FOLDER
mkdir -p $TMP_FOLDER/debian

(( EUID == 0 )) && SUDO="" || SUDO="sudo"

# git describe: v3.4-56-g789abcd
# sed: 3.4.r56.g789abcd
#   - remove leading 'v'
#   - prepend 'r' to the number before the '-g'
#   - replace '-' with '.'
# fallback: 0.0.r3456.g789abcd
VERSION=$(git describe --long --tags | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g') || VERSION="0.0.r$(git rev-list HEAD --count).g$(git rev-parse --short HEAD)"

cat <<EOF >$TMP_FOLDER/debian/changelog
redpanda-cpp ($VERSION) unstable; urgency=medium
EOF

git archive HEAD | tar -x -C $TMP_FOLDER
cp -r packages/debian $TMP_FOLDER

cd $TMP_FOLDER
$SUDO mk-build-deps -i -t "apt -y --no-install-recommends" debian/control
dpkg-buildpackage -us -uc
