#!/bin/bash

set -xeuo pipefail

TMP_FOLDER=/tmp/redpanda-cpp
[[ -d $TMP_FOLDER ]] && rm -rf $TMP_FOLDER
mkdir -p "$TMP_FOLDER"

# git describe: v3.4-56-g789abcd
# sed: 3.4.r56.g789abcd
#   - remove leading 'v'
#   - prepend 'r' to the number before the '-g'
#   - replace '-' with '.'
# fallback: 0.0.r3456.g789abcd
VERSION=$(git describe --long --tags | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g') || VERSION="0.0.r$(git rev-list HEAD --count).g$(git rev-parse --short HEAD)"
sed "s/__VERSION__/$VERSION/g" packages/archlinux/PKGBUILD.in >"$TMP_FOLDER/PKGBUILD"

git archive --prefix="RedPanda-CPP/" -o "$TMP_FOLDER/RedPanda-CPP.tar.gz" HEAD
cp packages/archlinux/compiler_hint.lua "$TMP_FOLDER/"

cd "$TMP_FOLDER"
makepkg -s --noconfirm
