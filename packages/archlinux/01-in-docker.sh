#!/bin/bash

set -xeuo pipefail

if [[ -v MIRROR && -n $MIRROR ]]
then
  echo Server = "http://$MIRROR/archlinux/\$repo/os/\$arch" >/etc/pacman.d/mirrorlist
fi

pacman -Syu --noconfirm --needed base-devel git

useradd -m builduser
echo 'builduser ALL=(ALL) NOPASSWD: ALL' > /etc/sudoers.d/builduser
echo "MAKEFLAGS=-j$(($(nproc)+1))" >>/etc/makepkg.conf.d/jobs.conf

su builduser -c "git config --global --add safe.directory $PWD"
su builduser -c ./packages/archlinux/buildpkg.sh

mkdir -p dist
cp /tmp/redpanda-cpp-git/redpanda-cpp-git-*.pkg.tar.zst dist/
