#!/bin/ash

set -xeuo pipefail

if (env | grep '^MIRROR=') && [[ -n $MIRROR ]]
then
  sed -i "s|dl-cdn.alpinelinux.org|$MIRROR|" /etc/apk/repositories
fi
apk add alpine-sdk git
abuild-keygen -an
# TODO: use `abuild-keygen --install`
cp ~/.abuild/*.pub /etc/apk/keys/

./packages/alpine/buildapk.sh

mkdir -p dist
cp ~/packages/unsupported/$(uname -m)/redpanda-cpp-git-*.apk dist/
