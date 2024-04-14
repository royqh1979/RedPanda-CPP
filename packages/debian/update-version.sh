#!/bin/bash

DEBIAN_DIR=packages/debian

minorver=`git rev-list HEAD --count`
majorver=`head version.inc -n 1 | sed -r 's/^APP_VERSION=\"(.*)\".*$/\1/g'`
ver="${majorver}.${minorver}"

pushd .
cd $DEBIAN_DIR

pwd
oldver=`head changelog -n 1 | sed -r 's/^redpanda-cpp\s\((.*)-(.*)\)\s.*$/\1/g'`
count=`head changelog -n 1 | sed -r 's/^redpanda-cpp\s\((.*)-(.*)\)\s.*$/\2/g'`
echo "Old version: $oldver"


if [ "$oldver" != "$ver" ]; then
  echo "Upgrade to $ver"
  tmpfile=$(mktemp)
  now=`date -R`
  echo "redpanda-cpp ($ver-1) unstable; urgency=medium" >> $tmpfile
  echo "" >>  $tmpfile
  echo "  * Update to $ver" >> $tmpfile
  echo "" >>  $tmpfile
  echo " -- Roy Qu (瞿华) <royqh1979@gmail.com>  $now" >> $tmpfile
  echo "" >>  $tmpfile
  cat changelog >> $tmpfile
  mv $tmpfile changelog
fi
