#!/bin/bash

set -xeuo pipefail

DISTRO_ID=$(grep ^ID= /etc/os-release | cut -d= -f2- | tr -d '"')

SPECDIR=$(rpm --eval %{_specdir})
SOURCEDIR=$(rpm --eval %{_sourcedir})
BUILDDIR=$(rpm --eval %{_builddir})/redpanda-cpp
[[ -d $BUILDDIR ]] && rm -rf $BUILDDIR

(( EUID == 0 )) && SUDO="" || SUDO="sudo"

VERSION=$(git describe --long --tags | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g') || VERSION="0.0.r$(git rev-list HEAD --count).g$(git rev-parse --short HEAD)"
sed "s/__VERSION__/$VERSION/g" packages/opensuse/redpanda-cpp.spec.in >$SPECDIR/redpanda-cpp.spec

git archive --prefix="RedPanda-CPP/" -o "$SOURCEDIR/RedPanda-CPP.tar.gz" HEAD

$SUDO zypper in -y $(rpmspec -q --buildrequires $SPECDIR/redpanda-cpp.spec)
rpmbuild --nodebuginfo -bb $SPECDIR/redpanda-cpp.spec

file=$(ls ~/rpmbuild/RPMS/$(uname -m)/redpanda-cpp-*.rpm)
basename=$(basename $file)

mkdir -p dist
cp $file dist/${basename/.rpm/.$DISTRO_ID.rpm}
