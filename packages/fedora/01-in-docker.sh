#!/bin/bash

set -xeuo pipefail

dnf install -y \
     gcc gcc-c++ rpm-build rpmdevtools git \
     glibc-static libstdc++-static libasan \
     qt5-qtbase-devel qt5-qtsvg-devel qt5-qttools-devel
rpmdev-setuptree

cd $SOURCE_DIR
./packages/fedora/buildrpm.sh

mkdir -p $SOURCE_DIR/dist
cp ~/rpmbuild/RPMS/$(uname -m)/redpanda-cpp-git-*.rpm $SOURCE_DIR/dist/
