#!/bin/ash

set -xeuo pipefail

TMP_FOLDER=/tmp/unsupported/redpanda-cpp-git
[[ -d $TMP_FOLDER ]] && rm -rf $TMP_FOLDER
mkdir -p "$TMP_FOLDER"

COMMIT_DATE=$(git log -1 --format=%cd --date=format:%Y%m%d)
VERSION=$(git describe --long --tags | sed 's/^v//;s/-\([^-]*\)-g\([0-9a-f]*\)/_git'$COMMIT_DATE'/') || VERSION="0.0_git$COMMIT_DATE"
sed "s/__VERSION__/$VERSION/g" packages/alpine/APKBUILD.in >"$TMP_FOLDER/APKBUILD"

git archive --prefix="RedPanda-CPP/" -o "$TMP_FOLDER/RedPanda-CPP.tar.gz" HEAD

cd "$TMP_FOLDER"
abuild -F checksum
abuild -Fr
