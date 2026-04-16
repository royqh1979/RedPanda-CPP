#!/bin/bash

set -xeuo pipefail

# install deps
default_repositories=(
  deb.debian.org
  security.debian.org
  archive.ubuntu.com
  security.ubuntu.com
  ports.ubuntu.com
)

if [[ -v MIRROR ]]
then
  for repo in ${default_repositories[@]}
  do
    [[ -f /etc/apt/sources.list ]] && sed -i "s|$repo|$MIRROR|" /etc/apt/sources.list
    for file in $(ls /etc/apt/sources.list.d/)
    do
      # okay for both *.list and *.sources (since Debian 12)
      sed -i "s|$repo|$MIRROR|" /etc/apt/sources.list.d/$file
    done
  done
fi

export DEBIAN_FRONTEND=noninteractive
apt update
apt install -y --no-install-recommends build-essential debhelper devscripts equivs git

./packages/debian/builddeb.sh
