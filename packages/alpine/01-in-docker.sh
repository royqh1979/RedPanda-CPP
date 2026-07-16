#!/bin/ash

set -xeuo pipefail

if (env | grep '^MIRROR=') && [[ -n $MIRROR ]]
then
  sed -i "s|dl-cdn.alpinelinux.org|$MIRROR|" /etc/apk/repositories
fi

apk add alpine-sdk doas git
# why is root not permitted to doas?
echo 'permit nopass root' >>/etc/doas.conf

abuild-keygen -ain

./packages/alpine/buildapk.sh
