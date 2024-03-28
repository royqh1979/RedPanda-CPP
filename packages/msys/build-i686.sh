#!/bin/bash

set -euxo pipefail

# Usage:
#   packages/msys/build-i686.sh [-c|--clean] [-nd|--no-deps] [-t|--target-dir <dir>]

if [[ ! -v MSYSTEM || ${MSYSTEM} != "MINGW32" ]]; then
  echo "This script must be run in a MinGW32 shell"
  exit 1
fi

CLEAN=0
CHECK_DEPS=1
TARGET_DIR="$(pwd)/dist"
while [[ $# -gt 0 ]]; do
  case $1 in
    -c|--clean)
      CLEAN=1
      shift
      ;;
    -nd|--no-deps)
      CHECK_DEPS=0
      shift
      ;;
    -t|--target-dir)
      TARGET_DIR="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done

BUILD_DIR="${TEMP}/redpanda-mingw-${MSYSTEM}-build"
PACKAGE_DIR="${TEMP}/redpanda-mingw-${MSYSTEM}-pkg"
QMAKE="${MINGW_PREFIX}/qt5-static/bin/qmake"
NSIS="/mingw32/bin/makensis"
SOURCE_DIR="$(pwd)"
MINGW_ARCHIVE="mingw32.7z"

function fn_print_progress() {
  echo -e "\e[1;32;44m$1\e[0m"
}

## check deps

if [[ ${CHECK_DEPS} -eq 1 ]]; then
  deps=(
    ${MINGW_PACKAGE_PREFIX}-{gcc,make,qt5-static,7zip}
    mingw-w64-i686-nsis
    git
  )
  for dep in ${deps[@]}; do
    pacman -Q ${dep} &>/dev/null || {
      echo "Missing dependency: ${dep}"
      exit 1
    }
  done
fi

if [[ ! -f "${SOURCE_DIR}/assets/${MINGW_ARCHIVE}" ]]; then
  echo "Missing ${MINGW_ARCHIVE}"
  exit 1
fi

## prepare dirs

if [[ ${CLEAN} -eq 1 ]]; then
  rm -rf "${BUILD_DIR}"
  rm -rf "${PACKAGE_DIR}"
fi
mkdir -p "${BUILD_DIR}" "${PACKAGE_DIR}" "${TARGET_DIR}"

## build

fn_print_progress "Building..."
pushd .
cd "${BUILD_DIR}"
"$QMAKE" PREFIX="${PACKAGE_DIR}" -o Makefile "${SOURCE_DIR}\Red_Panda_Cpp.pro" -r -spec win32-g++ 
mingw32-make -j$(nproc)
mingw32-make install
popd

## prepare packaging resources

echo "Making no-compiler installer ..."
pushd .
cd "${PACKAGE_DIR}"

cp "${SOURCE_DIR}/platform/windows/qt.conf" .

cp "${SOURCE_DIR}/platform/windows/installer-scripts/lang.nsh" .
cp "${SOURCE_DIR}/platform/windows/installer-scripts/redpanda-i686-nocompiler.nsi" .
cp "${SOURCE_DIR}/platform/windows/installer-scripts/redpanda-i686.nsi" .
popd

## make no-compiler package

pushd .
cd "${PACKAGE_DIR}"
fn_print_progress "Making no-compiler installer ..."
"${NSIS}" redpanda-i686-nocompiler.nsi

SETUP_NAME="$(ls *.Setup.exe)"
PORTABLE_NAME="$(echo $SETUP_NAME | sed 's/Setup.exe/Portable.7z/')"

fn_print_progress "Making no-compiler Portable Package..."
7z x "${SETUP_NAME}" -o"RedPanda-CPP" -xr'!$PLUGINSDIR' -x"!uninstall.exe"
7z a -mmt -mx9 -ms=on -mqs=on -mf=BCJ2 "${PORTABLE_NAME}" "RedPanda-CPP"
rm -rf "RedPanda-CPP"

mv "${SETUP_NAME}" "${TARGET_DIR}"
mv "${PORTABLE_NAME}" "${TARGET_DIR}"
popd

## make mingw package

pushd .
cd "${PACKAGE_DIR}"
fn_print_progress "Making installer..."
[[ ! -d mingw32 ]] && 7z x "${SOURCE_DIR}/assets/${MINGW_ARCHIVE}" -o"${PACKAGE_DIR}"

"${NSIS}" redpanda-i686.nsi

SETUP_NAME="$(ls *.Setup.exe)"
PORTABLE_NAME="$(echo $SETUP_NAME | sed 's/Setup.exe/Portable.7z/')"

fn_print_progress "Making Portable Package..."
7z x "${SETUP_NAME}" -o"RedPanda-CPP" -xr'!$PLUGINSDIR' -x"!uninstall.exe"
7z a -mmt -mx9 -ms=on -mqs=on -mf=BCJ2 "${PORTABLE_NAME}" "RedPanda-CPP"
rm -rf "RedPanda-CPP"

mv "${SETUP_NAME}" "${TARGET_DIR}"
mv "${PORTABLE_NAME}" "${TARGET_DIR}"
popd
