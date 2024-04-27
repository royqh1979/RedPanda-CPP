#!/bin/bash

set -xeuo pipefail

DISTRO_ID=$(grep ^ID= /etc/os-release | cut -d= -f2- | tr -d '"')
VERSION_ID=$(grep ^VERSION_ID= /etc/os-release | cut -d= -f2- | tr -d '"')

zypper in -y git rpm-build rpmdevtools sudo
rpmdev-setuptree

./packages/opensuse/buildrpm.sh

file=$(ls ~/rpmbuild/RPMS/$(uname -m)/redpanda-cpp-git-*.rpm)
basename=$(basename $file)

mkdir -p dist
cp $file dist/${basename/.rpm/.$DISTRO_ID$VERSION_ID.rpm}
