#!/bin/bash

set -xe

TMP_FOLDER=/build/redpanda-build
DISTRO_ID=$(grep ^ID= /etc/os-release | cut -d= -f2- | tr -d '"')
VERSION_ID=$(grep ^VERSION_ID= /etc/os-release | cut -d= -f2- | tr -d '"')
[[ -z $JOBS ]] && JOBS=$(nproc)

# install deps
default_repositories=(
  deb.debian.org
  security.debian.org
  archive.ubuntu.com
  security.ubuntu.com
  ports.ubuntu.com
)

if [[ -n $MIRROR ]]
then
  for repo in ${default_repositories[@]}
  do
    [[ -f /etc/apt/sources.list ]] && sed -i "s|$repo|$MIRROR|" /etc/apt/sources.list
    for file in $(ls /etc/apt/sources.list.d/)
    do
      # okay for both *.list and *.sources (since Debian 12)
      sed -i "s|$repo|$MIRROR|" /etc/apt/sources.list.d/$file
    done
  done
fi

export DEBIAN_FRONTEND=noninteractive
apt update
apt install -y --no-install-recommends \
  build-essential debhelper g++-mingw-w64 \
  libqt5svg5-dev qtbase5-dev qtbase5-dev-tools qttools5-dev-tools

# prepare source
mkdir -p $TMP_FOLDER

cd $SOURCE_DIR
cp -r packages/debian $TMP_FOLDER
cp -r tools $TMP_FOLDER
cp -r libs $TMP_FOLDER
cp -r RedPandaIDE $TMP_FOLDER
cp README.md $TMP_FOLDER
cp LICENSE $TMP_FOLDER
cp NEWS.md $TMP_FOLDER
cp -r platform $TMP_FOLDER
cp Red_Panda_CPP.pro $TMP_FOLDER

# build
cd $TMP_FOLDER
sed -i '/CONFIG += ENABLE_LUA_ADDON/ { s/^#\s*// }' RedPandaIDE/RedPandaIDE.pro
dpkg-buildpackage -us -uc -j$JOBS

# copy back to host
cd ..
file=$(ls redpanda-cpp_*.deb)
mkdir -p $SOURCE_DIR/dist
cp $file $SOURCE_DIR/dist/${file/.deb/.$DISTRO_ID$VERSION_ID.deb}
