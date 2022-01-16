#!/bin/bash

BUILD_DIR="${TEMP}/redpandacpp-build"
PACKAGE_DIR="${TEMP}/RedPanda-CPP"
GCC_DIR="/mingw64"
PATH="${GCC_DIR}/bin:${PATH}"
QMAKE="${GCC_DIR}/qt5-static/bin/qmake"
NSIS="/d/Program Files (x86)/NSIS/bin/makensis.exe"
SOURCE_DIR=`pwd`
MINGW64="/d/Program Files/RedPanda-CPP/MINGW64"

test -z "${BUILD_DIR}" | mkdir "${BUILD_DIR}"
test -z "${PACKAGE_DIR}" | mkdir "${PACKAGE_DIR}"
pushd .

cd "${BUILD_DIR}"

echo `pwd`

"$QMAKE" PREFIX="${PACKAGE_DIR}" BUILD_MSYS=1 -o Makefile "${SOURCE_DIR}\Red_Panda_Cpp.pro" -r -spec win32-g++ 
make -j16
make install
popd

pushd .
cd "${PACKAGE_DIR}"
mklink /j MinGW64 "${MINGW64}"
cp "${SOURCE_DIR}\installer\devcpp-x64.nsi" build.nsi
cp "${SOURCE_DIR}\installer\lang.nsh" .
"${NSIS}" build.nsi
rm -f lang.nsi
rm -f build.nsi

popd



