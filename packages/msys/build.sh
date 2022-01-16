#!/bin/bash

BUILD_DIR="${TEMP}/redpandacpp-build"
PACKAGE_DIR="${TEMP}/RedPanda-CPP"
GCC_DIR="/mingw64"
PATH="${GCC_DIR}/bin:${PATH}"
QMAKE="${GCC_DIR}/qt5-static/bin/qmake"
NSIS="/d/Program Files (x86)/NSIS/bin/makensis.exe"
SOURCE_DIR=`pwd`
MINGW64="/d/Program Files/RedPanda-CPP/MinGW64"
MINGW32="/d/Program Files/RedPanda-CPP/MinGW32"

test -z "${BUILD_DIR}" | mkdir "${BUILD_DIR}"
test -z "${PACKAGE_DIR}" | mkdir "${PACKAGE_DIR}"
pushd .

cd "${BUILD_DIR}"

"$QMAKE" PREFIX="${PACKAGE_DIR}" -o Makefile "${SOURCE_DIR}\Red_Panda_Cpp.pro" -r -spec win32-g++ 
make -j16
make install
popd

#build install package
cp "${PACKAGE_DIR}/config.nsh" .
cp "${SOURCE_DIR}/windows/installer-scripts/redpanda-x64.nsi" .
cp "${SOURCE_DIR}/windows/installer-scripts/lang.nsh" .

pushd .
cd "${PACKAGE_DIR}"
rm MinGW64
ln -s "${MINGW64}" 

"${NSIS}" /NOCD "${SOURCE_DIR}/redpanda-x64.nsi"
popd

rm -f lang.nsh
rm -f config.nsh
rm -f redpanda-x64.nsi




