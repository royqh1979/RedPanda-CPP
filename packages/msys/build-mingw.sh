#!/bin/bash

set -euxo pipefail

function fn_print_help() {
  echo " Usage:
   packages/msys/build-mingw.sh [-m|--msystem <MSYSTEM>] [-c|--clean] [-nd|--no-deps] [-t|--target-dir <dir>]
 Options:
   -h, --help               Display this information.
   -m, --msystem <MSYSTEM>  Switch to other MSYS2 environment.
                            (MINGW32, MINGW64, UCRT64, CLANG32, CLANG64, CLANGARM64)
                            MUST be used before other options.
   -c, --clean              Clean build and package directories.
   --mingw                  Alias for --mingw32 (x86 app) or --mingw64 (x64 app).
   --mingw32                Build mingw32 integrated compiler.
   --mingw64                Build mingw64 integrated compiler.
   --ucrt <build>           Include UCRT in the package. Windows SDK required.
                            e.g. '--ucrt 22621' for Windows 11 SDK 22H2.
   -nd, --no-deps           Skip dependency check.
   -t, --target-dir <dir>   Set target directory for the packages."
}
source version.inc
[[ -n "${APP_VERSION_SUFFIX}" ]] && APP_VERSION="${APP_VERSION}${APP_VERSION_SUFFIX}"

if [[ ! -v MSYSTEM ]]; then
  echo "This script must be run in MSYS2 shell"
  exit 1
fi

if [[ $# -gt 1 && ($1 == "-m" || $1 == "--msystem") ]]; then
  msystem=$2
  shift 2
  case "${msystem}" in
    MINGW32|MINGW64|UCRT64|CLANG32|CLANG64|CLANGARM64)
      export MSYSTEM="${msystem}"
      exec /bin/bash --login "$0" "$@"
      ;;
    *)
      echo "Invalid MSYSTEM: ${msystem}"
      exit 1
      ;;
  esac
fi

case "${MSYSTEM}" in
  MINGW32|CLANG32)  # there is no UCRT32
    NSIS_ARCH=x86
    PACKAGE_BASENAME="RedPanda.C++.${APP_VERSION}.win32"
    ;;
  MINGW64|UCRT64|CLANG64)
    NSIS_ARCH=x64
    PACKAGE_BASENAME="RedPanda.C++.${APP_VERSION}.win64"
    ;;
  CLANGARM64)
    NSIS_ARCH=arm64
    PACKAGE_BASENAME="RedPanda.C++.${APP_VERSION}.arm64"
    ;;
  *)
    echo "This script must be run in one of the following MSYS2 shells:"
    echo "  - MINGW32 / CLANG32"
    echo "  - MINGW64 / UCRT64 / CLANG64"
    echo "  - CLANGARM64"
    exit 1
    ;;
esac

CLEAN=0
CHECK_DEPS=1
compilers=()
COMPILER_MINGW32=0
COMPILER_MINGW64=0
TARGET_DIR="$(pwd)/dist"
UCRT=""
while [[ $# -gt 0 ]]; do
  case $1 in
    -h|--help)
      fn_print_help
      exit 0
      ;;
    -c|--clean)
      CLEAN=1
      shift
      ;;
    --mingw)
      case "${NSIS_ARCH}" in
        x86)
          compilers+=("mingw32")
          COMPILER_MINGW32=1
          shift
          ;;
        x64)
          compilers+=("mingw64")
          COMPILER_MINGW64=1
          shift
          ;;
        *)
          echo "ambiguous --mingw, please specify --mingw32 or --mingw64"
          exit 1
          ;;
      esac
      ;;
    --mingw32)
      compilers+=("mingw32")
      COMPILER_MINGW32=1
      shift
      ;;
    --mingw64)
      compilers+=("mingw64")
      COMPILER_MINGW64=1
      shift
      ;;
    --ucrt)
      case "${MSYSTEM}" in
        CLANG32|UCRT64|CLANG64)
          UCRT="$2"
          shift 2
          ;;
        MINGW32|MINGW64)
          echo "Error: Red Panda C++ is not built against UCRT."
          exit 1
          ;;
        CLANGARM64)
          echo "Error: UCRT is a system component on arm64, local deployment is not supported."
          exit 1
          ;;
      esac
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

ASTYLE_VERSION_TAG="3.4.14"
BUILD_DIR="${TEMP}/redpanda-mingw-${MSYSTEM}-build"
ASTYLE_BUILD_DIR="${BUILD_DIR}/astyle"
PACKAGE_DIR="${TEMP}/redpanda-mingw-${MSYSTEM}-pkg"
QMAKE="${MINGW_PREFIX}/qt5-static/bin/qmake"
NSIS="/mingw32/bin/makensis"
SOURCE_DIR="$(pwd)"
ASSETS_DIR="${SOURCE_DIR}/assets"
UCRT_DIR="/c/Program Files (x86)/Windows Kits/10/Redist/10.0.${UCRT}.0/ucrt/DLLs/${NSIS_ARCH}"

MINGW32_ARCHIVE="mingw32.7z"
MINGW32_COMPILER_NAME="MinGW-w64 i686 GCC 8.1"
MINGW32_PACKAGE_SUFFIX="MinGW32_8.1"

MINGW64_ARCHIVE="mingw64.7z"
MINGW64_COMPILER_NAME="MinGW-w64 X86_64 GCC 11.4"
MINGW64_PACKAGE_SUFFIX="MinGW64_11.4"

if [[ ${#compilers[@]} -eq 0 ]]; then
  PACKAGE_BASENAME="${PACKAGE_BASENAME}.NoCompiler"
else
  [[ ${COMPILER_MINGW32} -eq 1 ]] && PACKAGE_BASENAME="${PACKAGE_BASENAME}.${MINGW32_PACKAGE_SUFFIX}"
  [[ ${COMPILER_MINGW64} -eq 1 ]] && PACKAGE_BASENAME="${PACKAGE_BASENAME}.${MINGW64_PACKAGE_SUFFIX}"
fi

function fn_print_progress() {
  echo -e "\e[1;32;44m$1\e[0m"
}

## check deps

if [[ ${CHECK_DEPS} -eq 1 ]]; then
  case "${MSYSTEM}" in
    MINGW32|MINGW64|UCRT64)
      compiler=gcc
      ;;
    CLANG32|CLANG64|CLANGARM64)
      compiler=clang
      ;;
  esac
  deps=(
    ${MINGW_PACKAGE_PREFIX}-{$compiler,make,qt5-static,7zip,cmake}
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

if [[ ${COMPILER_MINGW32} -eq 1 && ! -f "${SOURCE_DIR}/assets/${MINGW32_ARCHIVE}" ]]; then
  echo "Missing MinGW archive: assets/${MINGW32_ARCHIVE}"
  exit 1
fi
if [[ ${COMPILER_MINGW64} -eq 1 && ! -f "${SOURCE_DIR}/assets/${MINGW64_ARCHIVE}" ]]; then
  echo "Missing MinGW archive: assets/${MINGW64_ARCHIVE}"
  exit 1
fi
if [[ -n "${UCRT}" && ! -f "${UCRT_DIR}/ucrtbase.dll" ]]; then
  echo "Missing Windows SDK, UCRT cannot be included."
  exit 1
fi

## prepare dirs

if [[ ${CLEAN} -eq 1 ]]; then
  rm -rf "${BUILD_DIR}"
  rm -rf "${PACKAGE_DIR}"
fi
mkdir -p "${BUILD_DIR}" "${PACKAGE_DIR}" "${TARGET_DIR}" "${ASTYLE_BUILD_DIR}" "${ASSETS_DIR}"

## prepare assets

fn_print_progress "Updating astyle repo..."
if [[ ! -d "${ASSETS_DIR}/astyle" ]]; then
  git clone --bare "https://gitlab.com/saalen/astyle" "${ASSETS_DIR}/astyle"
fi
pushd "${ASSETS_DIR}/astyle"
if [[ -z "$(git tag -l ${ASTYLE_VERSION_TAG})" ]]; then
  git fetch --all --tags
fi
popd

## build
fn_print_progress "Building astyle..."
pushd "${ASSETS_DIR}/astyle"
git --work-tree="${ASTYLE_BUILD_DIR}" checkout -f "${ASTYLE_VERSION_TAG}"
popd

pushd .
cd "${ASTYLE_BUILD_DIR}"
cmake . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release  -DCMAKE_EXE_LINKER_FLAGS="-static"
mingw32-make -j$(nproc)
cp AStyle/AStyle.exe "${PACKAGE_DIR}/astyle.exe"
popd

fn_print_progress "Building..."
pushd .
cd "${BUILD_DIR}"
qmake_flags=()
[[ ${NSIS_ARCH} == x64 ]] && qmake_flags+=("X86_64=ON")
"$QMAKE" PREFIX="${PACKAGE_DIR}" ${qmake_flags[@]} -o Makefile "${SOURCE_DIR}/Red_Panda_Cpp.pro" -r
mingw32-make -j$(nproc)
mingw32-make install
popd

## prepare packaging resources

pushd .
cd "${PACKAGE_DIR}"

cp "${SOURCE_DIR}/platform/windows/qt.conf" .

cp "${SOURCE_DIR}/platform/windows/installer-scripts/lang.nsh" .
cp "${SOURCE_DIR}/platform/windows/installer-scripts/utils.nsh" .
cp "${SOURCE_DIR}/platform/windows/installer-scripts/redpanda.nsi" .
popd

## make package

pushd .
cd "${PACKAGE_DIR}"
SETUP_NAME="${PACKAGE_BASENAME}.Setup.exe"
PORTABLE_NAME="${PACKAGE_BASENAME}.Portable.7z"

fn_print_progress "Making installer..."

nsis_flags=(
  -DAPP_VERSION="${APP_VERSION}"
  -DARCH="${NSIS_ARCH}"
  -DFINALNAME="${SETUP_NAME}"
  -DMINGW32_COMPILER_NAME="${MINGW32_COMPILER_NAME}"
  -DMINGW64_COMPILER_NAME="${MINGW64_COMPILER_NAME}"
  -DREQUIRED_WINDOWS_BUILD=7600
  -DREQUIRED_WINDOWS_NAME="Windows 7"
  -DUSE_MODERN_FONT
)
if [[ ${COMPILER_MINGW32} -eq 1 ]]; then
  nsis_flags+=(-DHAVE_MINGW32)
  [[ -d "mingw32" ]] || 7z x "${SOURCE_DIR}/assets/${MINGW32_ARCHIVE}" -o"${PACKAGE_DIR}"
fi
if [[ ${COMPILER_MINGW64} -eq 1 ]]; then
  nsis_flags+=(-DHAVE_MINGW64)
  [[ -d "mingw64" ]] || 7z x "${SOURCE_DIR}/assets/${MINGW64_ARCHIVE}" -o"${PACKAGE_DIR}"
fi
if [[ -n "${UCRT}" ]]; then
  nsis_flags+=(-DHAVE_UCRT)
  if [[ ! -f ucrt/ucrtbase.dll ]]; then
    mkdir -p ucrt
    cp "${UCRT_DIR}"/*.dll ucrt
  fi
fi
"${NSIS}" "${nsis_flags[@]}" redpanda.nsi

fn_print_progress "Making Portable Package..."
7z x "${SETUP_NAME}" -o"RedPanda-CPP" -xr'!$PLUGINSDIR' -x"!uninstall.exe"
7z a -mmt -mx9 -ms=on -mqs=on -mf=BCJ2 "${PORTABLE_NAME}" "RedPanda-CPP"
rm -rf "RedPanda-CPP"

mv "${SETUP_NAME}" "${TARGET_DIR}"
mv "${PORTABLE_NAME}" "${TARGET_DIR}"
popd
