#!/bin/sh

TMP_FOLDER=/tmp/redpandaide
version=0.12.5
make install
mkdir $TMP_FOLDER
mkdir $TMP_FOLDER/DEBIAN
mkdir $TMP_FOLDER/opt
mkdir $TMP_FOLDER/usr
mkdir $TMP_FOLDER/usr/share
mkdir $TMP_FOLDER/usr/share/applications

cp packages/deb/control $TMP_FOLDER/DEBIAN
cp -r /opt/RedPandaIDE $TMP_FOLDER/opt
cp -r /opt/RedPandaIDE/*.desktop $TMP_FOLDER/usr/share/applications

dpkg-deb --build /tmp/redpandaide

mv $TMP_FOLDER/../redpandaide.deb redpanda-ide-${version}_amd64.deb

rm -rf $TMP_FOLDER

