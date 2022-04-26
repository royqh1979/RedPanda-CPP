#!/bin/bash

BUILD_DIR="${TEMP}/redpandacpp-build"
PACKAGE_DIR="${TEMP}/RedPanda-CPP"
GCC_DIR="/mingw64"
PATH="${GCC_DIR}/bin:${PATH}"
QMAKE="${GCC_DIR}/qt5-static/bin/qmake"
NSIS="/d/Program Files (x86)/NSIS/bin/makensis.exe"
SOURCE_DIR=`pwd`
MINGW="/e/Workspaces/contributes/MinGW/Clang64"
MINGW_NAME="Clang64"

rm -rf  "${BUILD_DIR}"
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

#echo "Making no-compiler installer ..."
#pushd .
#cd "${PACKAGE_DIR}"

#cp "${SOURCE_DIR}/windows/installer-scripts/lang.nsh" .
#cp "${SOURCE_DIR}/windows/installer-scripts/redpanda-nocompiler.nsi" .

#"${NSIS}" redpanda-nocompiler.nsi
#rm -f lang.nsh
#rm -f config.nsh
#rm -f config32.nsh
#rm -f redpanda-nocompiler.nsi

#SETUP_NAME=`ls *.Setup.exe`
#PORTABLE_NAME=`echo $SETUP_NAME | sed 's/Setup.exe/Portable.7z/'`
#mv "$SETUP_NAME" "${SOURCE_DIR}"
#popd

#echo "Making no-compiler Portable Package..."
#7z a -mmt8 -mx9  "${PORTABLE_NAME}" "${PACKAGE_DIR}"

# we need reinstall config.nsh
#pushd .
#cd "${BUILD_DIR}"
#make install
#popd

echo "Making installer..."

pushd .
cd "${PACKAGE_DIR}"
ln -s "${MINGW}" $MinGW_NAME

cp "${SOURCE_DIR}/windows/installer-scripts/lang.nsh" .
cp "${SOURCE_DIR}/windows/installer-scripts/redpanda-clang.nsi" .

"${NSIS}" redpanda-clang.nsi
rm -f lang.nsh
rm -f config.nsh
rm -f config32.nsh
rm -f redpanda-clang.nsi

SETUP_NAME=`ls *.Setup.exe`
PORTABLE_NAME=`echo $SETUP_NAME | sed 's/Setup.exe/Portable.7z/'`
mv "$SETUP_NAME" "${SOURCE_DIR}"

popd

echo "Making Portable Package..."
7z a -mmt8 -mx9  "${PORTABLE_NAME}" "${PACKAGE_DIR}"

echo "Clean up..."
rm -rf "${PACKAGE_DIR}"

