#!/bin/sh

make distclean

TMP_FOLDER=/tmp/redpandaide
test -n $TMP_FOLDER | rm -rf $TMP_FOLDER
test -z $TMP_FOLDER | mkdir $TMP_FOLDER


cp -r packages/debian $TMP_FOLDER 
cp -r astyle $TMP_FOLDER 
cp -r redpanda-git-askpass $TMP_FOLDER 
cp -r consolepauser $TMP_FOLDER
cp -r RedPandaIDE $TMP_FOLDER
cp README.md $TMP_FOLDER
cp LICENSE $TMP_FOLDER
cp NEWS.md $TMP_FOLDER
cp -r linux $TMP_FOLDER
cp Red_Panda_CPP.pro $TMP_FOLDER
cp -r linux $TMP_FOLDER

cd $TMP_FOLDER
dpkg-buildpackage -us -uc
