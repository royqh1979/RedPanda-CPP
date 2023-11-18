#!/bin/bash

set -xeuo pipefail

dnf install -y dnf-plugins-core git rpm-build rpmdevtools
rpmdev-setuptree

cd $SOURCE_DIR
./packages/fedora/buildrpm.sh

mkdir -p $SOURCE_DIR/dist
cp ~/rpmbuild/RPMS/$(uname -m)/redpanda-cpp-git-*.rpm $SOURCE_DIR/dist/
