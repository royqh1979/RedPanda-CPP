#!/bin/sh

make distclean

TMP_FOLDER=/tmp/redpandaide
mkdir $TMP_FOLDER

cp -r packages/debian $TMP_FOLDER 
cp -r astyle $TMP_FOLDER 
cp -r consolepauser $TMP_FOLDER
cp -r RedPandaIDE $TMP_FOLDER
cp README.md $TMP_FOLDER
cp LICENSE $TMP_FOLDER
cp NEWS.md $TMP_FOLDER
cp redpandaide.desktop.in $TMP_FOLDER
cp -r templates $TMP_FOLDER
cp Red_Panda_CPP.pro $TMP_FOLDER
cp -r linux $TMP_FOLDER

cd $TMP_FOLDER
dpkg-buildpackage -us -uc
