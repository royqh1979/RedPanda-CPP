#!/bin/bash

BUILD_DIR="${TEMP}/redpandacpp-build"
PACKAGE_DIR="${TEMP}/RedPanda-CPP"
GCC_DIR="/mingw32"
PATH="${GCC_DIR}/bin:${PATH}"
QMAKE="${GCC_DIR}/qt5-static/bin/qmake"
NSIS="/d/Program Files (x86)/NSIS/bin/makensis.exe"
SOURCE_DIR=`pwd`
MINGW="/e/Workspaces/contributes/MinGW/MinGW32"
MINGW_NAME="MinGW32"

test -z "${BUILD_DIR}" | mkdir "${BUILD_DIR}"
rm -rf  "${PACKAGE_DIR}"
mkdir "${PACKAGE_DIR}"

echo "Building..."
pushd .
cd "${BUILD_DIR}"
make distclean
"$QMAKE" PREFIX="${PACKAGE_DIR}" -o Makefile "${SOURCE_DIR}\Red_Panda_Cpp.pro" -r -spec win32-g++ 
make -j16
make install
popd

echo "Making no-compiler installer ..."
pushd .
cd "${PACKAGE_DIR}"

cp "${SOURCE_DIR}/windows/installer-scripts/lang.nsh" .
cp "${SOURCE_DIR}/windows/installer-scripts/redpanda-i686-nocompiler.nsi" .

"${NSIS}" redpanda-i686-nocompiler.nsi
rm -f lang.nsh
rm -f config32.nsh
rm -f config.nsh
rm -f redpanda-i686-nocompiler.nsi

SETUP_NAME=`ls *.Setup.exe`
PORTABLE_NAME=`echo $SETUP_NAME | sed 's/Setup.exe/Portable.7z/'`
mv "$SETUP_NAME" "${SOURCE_DIR}"
popd

echo "Making no-compiler Portable Package..."
7z a -mmt8 -mx9  "${PORTABLE_NAME}" "${PACKAGE_DIR}"

# we need reinstall config32.nsh
pushd .
cd "${BUILD_DIR}"
make install
popd

echo "Making installer..."

pushd .
cd "${PACKAGE_DIR}"
ln -s "${MINGW}" $MinGW_NAME

cp "${SOURCE_DIR}/windows/installer-scripts/lang.nsh" .
cp "${SOURCE_DIR}/windows/installer-scripts/redpanda-i686.nsi" .

"${NSIS}" redpanda-i686.nsi
rm -f lang.nsh
rm -f config32.nsh
rm -f config.nsh
rm -f redpanda-i686.nsi

SETUP_NAME=`ls *.Setup.exe`
PORTABLE_NAME=`echo $SETUP_NAME | sed 's/Setup.exe/Portable.7z/'`
mv "$SETUP_NAME" "${SOURCE_DIR}"

popd

echo "Making Portable Package..."
7z a -mmt8 -mx9  "${PORTABLE_NAME}" "${PACKAGE_DIR}"

echo "Clean up..."
rm -rf "${PACKAGE_DIR}"

