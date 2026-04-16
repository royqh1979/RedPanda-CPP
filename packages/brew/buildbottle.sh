#!/bin/bash

set -euxo pipefail

TMP_FOLDER=/tmp/redpanda-cpp
[[ -d $TMP_FOLDER ]] && rm -rf $TMP_FOLDER
mkdir -p "$TMP_FOLDER"

# git describe: v3.4-56-g789abcd
# sed: 3.4.r56.g789abcd
#   - remove leading 'v'
#   - prepend 'r' to the number before the '-g'
#   - replace '-' with '.'
# fallback: 0.0.r3456.g789abcd
VERSION=$(git describe --long --tags | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g') || VERSION="0.0.r$(git rev-list HEAD --count).g$(git rev-parse --short HEAD)"

sed \
  -e "s/__VERSION__/$VERSION/" \
  -e "s|__URL__|file://$TMP_FOLDER/RedPanda-CPP.tar.gz|" \
  packages/brew/redpanda-cpp.rb.in >"$TMP_FOLDER/redpanda-cpp.rb"

git archive --prefix="RedPanda-CPP/" -o "$TMP_FOLDER/RedPanda-CPP.tar.gz" HEAD

(
  cd "$TMP_FOLDER"
  if brew list --formulae | grep -q redpanda-cpp; then
    env HOMEBREW_NO_AUTOREMOVE=1 \
      brew remove redpanda-cpp
  fi
  env HOMEBREW_DEVELOPER=1 \
    brew install -v --build-bottle --formula ./redpanda-cpp.rb
  brew bottle -v --force-core-tap --no-rebuild redpanda-cpp
)

mkdir -p dist
cp "$TMP_FOLDER"/redpanda-cpp-*.bottle.tar.gz dist/
