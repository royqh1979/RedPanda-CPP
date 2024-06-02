#!/bin/bash

set -xeuo pipefail

TMP_FOLDER=/tmp/redpandaide
[[ -d $TMP_FOLDER ]] && rm -rf $TMP_FOLDER
mkdir -p "$TMP_FOLDER"

cp -r packages/debian $TMP_FOLDER 
cp -r tools $TMP_FOLDER 
cp -r libs $TMP_FOLDER 
cp -r RedPandaIDE $TMP_FOLDER
cp README.md $TMP_FOLDER
cp LICENSE $TMP_FOLDER
cp NEWS.md $TMP_FOLDER
cp version.inc $TMP_FOLDER
cp -r platform $TMP_FOLDER
cp Red_Panda_CPP.pro $TMP_FOLDER

cd $TMP_FOLDER
rm control
cp control.deepin control
command -v mk-build-deps && mk-build-deps -i -t "apt -y --no-install-recommends" debian/control
sed -i '/CONFIG += ENABLE_LUA_ADDON/ { s/^#\s*// }' RedPandaIDE/RedPandaIDE.pro
dpkg-buildpackage -us -uc
