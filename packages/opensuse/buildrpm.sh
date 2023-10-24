#!/bin/bash

set -xeuo pipefail

SPECDIR=$(rpm --eval %{_specdir})
BUILDDIR=$(rpm --eval %{_builddir})/redpanda-cpp-git
VERSION=$(git describe --long --tags --always | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g')

sed -s "s/__VERSION__/$VERSION/g" packages/opensuse/redpanda-cpp-git.spec.in >$SPECDIR/redpanda-cpp-git.spec

[[ -d $BUILDDIR ]] && rm -rf $BUILDDIR

rpmbuild --define "_sourcedir $(pwd)" -bb $SPECDIR/redpanda-cpp-git.spec
