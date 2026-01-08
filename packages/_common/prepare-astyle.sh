#!/bin/sh

# ATTN: This script can be interpreted by any POSIX shell.

# Usage:
#   prepare-astyle.sh --git-dir <path> --work-dir <path>

set -eux

ASTYLE_VERSION_TAG=3.6.13

_GIT_DIR=""
_WORK_DIR=""

while [ $# -gt 0 ] ; do
  case "$1" in
    --git-dir)
      _GIT_DIR="$2"
      shift 2
      ;;
    --work-dir)
      _WORK_DIR="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done

if [ -z "$_GIT_DIR" -o -z "$_WORK_DIR" ] ; then
  echo "Missing required arguments"
  exit 1
fi

if [ ! -d "$_GIT_DIR" ] ; then
  git clone --bare "https://gitlab.com/saalen/astyle" "$_GIT_DIR"
fi

(
  cd "$_GIT_DIR"
  if [ -z "$(git tag -l "$ASTYLE_VERSION_TAG")" ] ; then
    git fetch --all --tags
  fi
  git --work-tree="$_WORK_DIR" checkout -f "$ASTYLE_VERSION_TAG"
)
