#!/bin/bash

set -xeuo pipefail

SPECDIR=$(rpm --eval %{_specdir})
SOURCEDIR=$(rpm --eval %{_sourcedir})
BUILDDIR=$(rpm --eval %{_builddir})/redpanda-cpp-git
[[ -d $BUILDDIR ]] && rm -rf $BUILDDIR

VERSION=$(git describe --long --tags | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g') || VERSION="0.0.r$(git rev-list HEAD --count).g$(git rev-parse --short HEAD)"
sed "s/__VERSION__/$VERSION/g" packages/fedora/redpanda-cpp-git.spec.in >$SPECDIR/redpanda-cpp-git.spec

git archive --prefix="RedPanda-CPP/" -o "$SOURCEDIR/RedPanda-CPP.tar.gz" HEAD

sudo dnf builddep -y $SPECDIR/redpanda-cpp-git.spec
rpmbuild --nodebuginfo -bb $SPECDIR/redpanda-cpp-git.spec
