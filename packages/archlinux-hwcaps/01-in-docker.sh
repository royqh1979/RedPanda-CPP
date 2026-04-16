#!/bin/bash

set -xeuo pipefail

if [[ -v MIRROR && -n $MIRROR ]]
then
  echo Server = "http://$MIRROR/archlinux/\$repo/os/\$arch" >/etc/pacman.d/mirrorlist
fi

pacman -Syu --noconfirm --needed base-devel git

sed -i '/exit $E_ROOT/d' /usr/bin/makepkg
echo "MAKEFLAGS=-j$(($(nproc)+1))" >>/etc/makepkg.conf.d/jobs.conf

export XMAKE_ROOT=y
./packages/archlinux-hwcaps/buildpkg.sh
