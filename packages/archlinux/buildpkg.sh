#!/bin/bash

set -xeuo pipefail

TMP_FOLDER=/tmp/redpanda-cpp-git
[[ -d $TMP_FOLDER ]] && rm -rf $TMP_FOLDER
mkdir -p "$TMP_FOLDER"

VERSION=$(git describe --long --tags | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g') || VERSION="0.0.r$(git rev-list HEAD --count).g$(git rev-parse --short HEAD)"
sed "s/__VERSION__/$VERSION/g" packages/archlinux/PKGBUILD.in >"$TMP_FOLDER/PKGBUILD"

git archive --prefix="RedPanda-CPP/" -o "$TMP_FOLDER/RedPanda-CPP.tar.gz" HEAD
cp packages/archlinux/compiler_hint.lua "$TMP_FOLDER/"

cd "$TMP_FOLDER"
makepkg -s --noconfirm
