#!/bin/bash

set -xeuo pipefail

SPECDIR=$(rpm --eval %{_specdir})
SOURCEDIR=$(rpm --eval %{_sourcedir})
BUILDDIR=$(rpm --eval %{_builddir})/redpanda-cpp
[[ -d $BUILDDIR ]] && rm -rf $BUILDDIR

(( EUID == 0 )) && SUDO="" || SUDO="sudo"

# git describe: v3.4-56-g789abcd
# sed: 3.4.r56.g789abcd
#   - remove leading 'v'
#   - prepend 'r' to the number before the '-g'
#   - replace '-' with '.'
# fallback: 0.0.r3456.g789abcd
VERSION=$(git describe --long --tags | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g') || VERSION="0.0.r$(git rev-list HEAD --count).g$(git rev-parse --short HEAD)"
sed "s/__VERSION__/$VERSION/g" packages/fedora/redpanda-cpp.spec.in >$SPECDIR/redpanda-cpp.spec

git archive --prefix="RedPanda-CPP/" -o "$SOURCEDIR/RedPanda-CPP.tar.gz" HEAD

$SUDO dnf builddep -y $SPECDIR/redpanda-cpp.spec
rpmbuild --nodebuginfo -bb $SPECDIR/redpanda-cpp.spec
