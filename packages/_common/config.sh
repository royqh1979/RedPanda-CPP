#!/bin/bash

ASTYLE_VERSION_TAG="3.6.13"

function read_version_json() {
  local version_major
  local version_minor
  local version_patch
  local version_preRelease

  if command -v jq
  then
    version_major=$(jq -r .major version.json)
    version_minor=$(jq -r .minor version.json)
    version_patch=$(jq -r .patch version.json)
    version_preRelease=$(jq -r .preRelease version.json)
  else
    echo -e "\e[33m[Warning] version.json: jq is not installed, falling back to grep and sed.\e[0m"
    version_major=$(cat version.json | grep '"major":' | sed -E 's/.*"major": ([0-9]+).*/\1/')
    version_minor=$(cat version.json | grep '"minor":' | sed -E 's/.*"minor": ([0-9]+).*/\1/')
    version_patch=$(cat version.json | grep '"patch":' | sed -E 's/.*"patch": ([0-9]+).*/\1/')
    version_preRelease=$(cat version.json | grep '"preRelease":' | sed -E 's/.*"preRelease": "(.*)".*/\1/')
  fi

  APP_VERSION="$version_major.$version_minor.$version_patch"
  [[ -n $version_preRelease ]] && APP_VERSION="$APP_VERSION-$version_preRelease" || true
}

read_version_json
