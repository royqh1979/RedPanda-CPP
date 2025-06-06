#!/bin/bash

set -xeuo pipefail

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
