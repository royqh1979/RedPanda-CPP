#!/bin/bash

set -xeuo pipefail

dnf install -y dnf-plugins-core git rpm-build rpmdevtools
rpmdev-setuptree

./packages/fedora/buildrpm.sh

mkdir -p dist
cp ~/rpmbuild/RPMS/$(uname -m)/redpanda-cpp-git-*.rpm dist/
