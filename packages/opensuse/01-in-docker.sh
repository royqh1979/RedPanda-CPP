#!/bin/bash

set -xeuo pipefail

DISTRO_ID=$(grep ^ID= /etc/os-release | cut -d= -f2- | tr -d '"')
VERSION_ID=$(grep ^VERSION_ID= /etc/os-release | cut -d= -f2- | tr -d '"')

zypper in -y \
     gcc gcc-c++ rpm-build rpmdevtools git \
     glibc-devel-static \
     libqt5-qtbase-devel libqt5-qtsvg-devel libqt5-qttools-devel
rpmdev-setuptree

cd $SOURCE_DIR
./packages/opensuse/buildrpm.sh

file=$(ls ~/rpmbuild/RPMS/$(uname -m)/redpanda-cpp-git-*.rpm)
basename=$(basename $file)

mkdir -p $SOURCE_DIR/dist
cp $file $SOURCE_DIR/dist/${basename/.rpm/.$DISTRO_ID$VERSION_ID.rpm}
