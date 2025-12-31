#!/bin/sh

set -euo pipefail

if command -v jq >/dev/null 2>&1
then
  version_major=$(jq -r .major ../version.json)
  version_minor=$(jq -r .minor ../version.json)
  version_patch=$(jq -r .patch ../version.json)
  version_preRelease=$(jq -r .preRelease ../version.json)
else
  # jq is not installed, fall back to grep and sed
  version_major=$(cat ../version.json | grep '"major":' | sed -E 's/.*"major": ([0-9]+).*/\1/')
  version_minor=$(cat ../version.json | grep '"minor":' | sed -E 's/.*"minor": ([0-9]+).*/\1/')
  version_patch=$(cat ../version.json | grep '"patch":' | sed -E 's/.*"patch": ([0-9]+).*/\1/')
  version_preRelease=$(cat ../version.json | grep '"preRelease":' | sed -E 's/.*"preRelease": "(.*)".*/\1/')
fi

APP_VERSION="$version_major.$version_minor.$version_patch"

if git rev-list HEAD --count >/dev/null 2>&1
then
  git_count=$(git rev-list HEAD --count)
  APP_VERSION="$APP_VERSION.$git_count"
fi

if test -n "$version_preRelease"
then
  APP_VERSION="$APP_VERSION-$version_preRelease"
fi

echo $APP_VERSION
