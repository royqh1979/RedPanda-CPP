#!/bin/bash

set -euxo pipefail

# Usage:
#   packages/msys/build-x64.sh [-c|--clean] [-nd|--no-deps] [-t|--target-dir <dir>]

source version.inc
[[ -n "${APP_VERSION_SUFFIX}" ]] && APP_VERSION="${APP_VERSION}${APP_VERSION_SUFFIX}"

if [[ ! -v MSYSTEM ]]; then
  echo "This script must be run in MSYS2 shell"
  exit 1
fi

case "${MSYSTEM}" in
  MINGW32|CLANG32)  # there is no UCRT32
    NSIS_ARCH=x86
    BITNESS=32
    COMPILER_NAME="MinGW-w64 i686 GCC 8.1"
    ARCHIVE_MINGW_COMPILER_BASENAME="RedPanda.C++.${APP_VERSION}.win32.${COMPILER_NAME}"
    ARCHIVE_NO_COMPILER_BASENAME="RedPanda.C++.${APP_VERSION}.win32.No.Compiler"
    ;;
  MINGW64|UCRT64|CLANG64)
    NSIS_ARCH=x64
    BITNESS=64
    COMPILER_NAME="MinGW-w64 X86_64 GCC 11.4"
    ARCHIVE_MINGW_COMPILER_BASENAME="RedPanda.C++.${APP_VERSION}.win64.${COMPILER_NAME}"
    ARCHIVE_NO_COMPILER_BASENAME="RedPanda.C++.${APP_VERSION}.win64.No.Compiler"
    ;;
  *)
    echo "This script must be run in one of the following MSYS2 shells:"
    echo "  - MINGW32 / CLANG32"
    echo "  - MINGW64 / UCRT64 / CLANG64"
    exit 1
    ;;
esac

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
MINGW_ARCHIVE="mingw${BITNESS}.7z"

function fn_print_progress() {
  echo -e "\e[1;32;44m$1\e[0m"
}

## check deps

if [[ ${CHECK_DEPS} -eq 1 ]]; then
  case "${MSYSTEM}" in
    MINGW32|MINGW64|UCRT64)
      compiler=gcc
      ;;
    CLANG32|CLANG64)
      compiler=clang
      ;;
  esac
  deps=(
    ${MINGW_PACKAGE_PREFIX}-{$compiler,make,qt5-static,7zip}
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
  echo "Missing MinGW archive: assets/${MINGW_ARCHIVE}"
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
qmake_flags=()
[[ NSIS_ARCH == x64 ]] && qmake_flags+=("X86_64=ON")
"$QMAKE" PREFIX="${PACKAGE_DIR}" ${qmake_flags[@]} -o Makefile "${SOURCE_DIR}/Red_Panda_Cpp.pro" -r
mingw32-make -j$(nproc)
mingw32-make install
popd

## prepare packaging resources

pushd .
cd "${PACKAGE_DIR}"

cp "${SOURCE_DIR}/platform/windows/qt.conf" .

cp "${SOURCE_DIR}/platform/windows/installer-scripts/lang.nsh" .
cp "${SOURCE_DIR}/platform/windows/installer-scripts/redpanda.nsi" .
popd

## make no-compiler package

pushd .
cd "${PACKAGE_DIR}"
SETUP_NAME="${ARCHIVE_NO_COMPILER_BASENAME}.Setup.exe"
PORTABLE_NAME="${ARCHIVE_NO_COMPILER_BASENAME}.Portable.7z"

fn_print_progress "Making no-compiler installer ..."
"${NSIS}" \
  -DAPP_VERSION="${APP_VERSION}" \
  -DARCH="${NSIS_ARCH}" \
  -DFINALNAME="${SETUP_NAME}" \
  redpanda.nsi

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
SETUP_NAME="${ARCHIVE_MINGW_COMPILER_BASENAME}.Setup.exe"
PORTABLE_NAME="${ARCHIVE_MINGW_COMPILER_BASENAME}.Portable.7z"

fn_print_progress "Making installer..."
[[ ! -d "mingw${BITNESS}" ]] && 7z x "${SOURCE_DIR}/assets/${MINGW_ARCHIVE}" -o"${PACKAGE_DIR}"

"${NSIS}" \
  -DAPP_VERSION="${APP_VERSION}" \
  -DARCH="${NSIS_ARCH}" \
  -DFINALNAME="${SETUP_NAME}" \
  -DHAVE_MINGW -DCOMPILERNAME="${COMPILER_NAME}" -DCOMPILERFOLDER="mingw${BITNESS}" \
  redpanda.nsi

fn_print_progress "Making Portable Package..."
7z x "${SETUP_NAME}" -o"RedPanda-CPP" -xr'!$PLUGINSDIR' -x"!uninstall.exe"
7z a -mmt -mx9 -ms=on -mqs=on -mf=BCJ2 "${PORTABLE_NAME}" "RedPanda-CPP"
rm -rf "RedPanda-CPP"

mv "${SETUP_NAME}" "${TARGET_DIR}"
mv "${PORTABLE_NAME}" "${TARGET_DIR}"
popd
