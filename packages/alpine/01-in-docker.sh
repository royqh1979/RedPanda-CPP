#!/bin/ash

set -xeuo pipefail

if (env | grep '^MIRROR=') && [[ -n $MIRROR ]]
then
  sed -i "s|dl-cdn.alpinelinux.org|$MIRROR|" /etc/apk/repositories
fi
apk add alpine-sdk git
abuild-keygen -an

cd $SOURCE_DIR
./packages/alpine/buildapk.sh

mkdir -p $SOURCE_DIR/dist
cp ~/packages/unsupported/$(uname -m)/redpanda-cpp-git-*.apk $SOURCE_DIR/dist/
