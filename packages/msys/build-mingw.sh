#!/bin/bash

set -euxo pipefail

ASTYLE_VERSION_TAG="3.6.9"

function fn_print_help() {
  echo " Usage:
   packages/msys/build-mingw.sh [-m|--msystem <MSYSTEM>] [-c|--clean] [-nd|--no-deps] [-t|--target-dir <dir>]
 Options:
   -h, --help               Display this information.
   -m, --msystem <MSYSTEM>  Switch to other MSYS2 environment.
                            (MINGW32, MINGW64, UCRT64, CLANG64, CLANGARM64)
                            MUST be used before other options.
   -c, --clean              Clean build and package directories.
   --mingw                  Alias for --mingw32 (x86 app) or --mingw64 (x64 app).
   --mingw32                Build mingw32 integrated compiler.
   --mingw64                Build mingw64 integrated compiler.
   --gcc-linux-x86-64       Build x86_64-linux-gnu integrated compiler.
   --gcc-linux-aarch64      Build aarch64-linux-gnu integrated compiler.
   --ucrt <arch>            Include UCRT installer (VC_redist) in the package.
                            <arch> can be x86 or x64.
                            This option can be specified multiple times.
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
    MINGW32|MINGW64|UCRT64|CLANG64|CLANGARM64)
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
  MINGW32)
    # there is no UCRT32
    # CLANG32 qt5-static removed since 5.15.15
    # https://github.com/msys2/MINGW-packages/commit/ab062c6e5d6e9fff86ee8f88c1d8e9601ea9ab5b
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
    echo "  - MINGW32"
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
COMPILER_GCC_LINUX_X8664=0
COMPILER_GCC_LINUX_AARCH64=0
REQUIRED_WINDOWS_BUILD=7600
REQUIRED_WINDOWS_NAME="Windows 7"
TARGET_DIR="$(pwd)/dist"
UCRT_X86=0
UCRT_X64=0
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
    --gcc-linux-x86-64)
      case "${NSIS_ARCH}" in
        x64)
          compilers+=("gcc-linux-x86-64")
          COMPILER_GCC_LINUX_X8664=1
          REQUIRED_WINDOWS_BUILD=17763
          REQUIRED_WINDOWS_NAME="Windows 10 v1809"
          shift
          ;;
        *)
          echo "architecture mismatch, --gcc-linux-x86-64 is only supported on x64"
          exit 1
          ;;
      esac
      ;;
    --gcc-linux-aarch64)
      case "${NSIS_ARCH}" in
        arm64)
          compilers+=("gcc-linux-aarch64")
          COMPILER_GCC_LINUX_AARCH64=1
          REQUIRED_WINDOWS_BUILD=22000
          REQUIRED_WINDOWS_NAME="Windows 11"
          shift
          ;;
        *)
          echo "architecture mismatch, --gcc-linux-aarch64 is only supported on arm64"
          exit 1
          ;;
      esac
      ;;
    --ucrt)
      case "$2" in
        x86)
          UCRT_X86=1
          shift 2
          ;;
        x64)
          UCRT_X64=1
          shift 2
          ;;
        arm64)
          echo "UCRT is always a system component on arm64."
          exit 1
          ;;
        *)
          echo "Invalid UCRT architecture: $2"
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

BUILD_DIR="${TEMP}/redpanda-mingw-${MSYSTEM}-build"
ASTYLE_BUILD_DIR="${BUILD_DIR}/astyle"
PACKAGE_DIR="${TEMP}/redpanda-mingw-${MSYSTEM}-pkg"
QMAKE="${MINGW_PREFIX}/qt5-static/bin/qmake"
NSIS="/mingw32/bin/makensis"
SOURCE_DIR="$(pwd)"
ASSETS_DIR="${SOURCE_DIR}/assets"

# Visual C++ Redistributable for Visual Studio 2019, version 16.7 (14.27)
# This is the last version that supports all Windows versions (on which UCRT is supported).
UCRT_X86_URL="https://download.visualstudio.microsoft.com/download/pr/c168313d-1754-40d4-8928-18632c2e2a71/D305BAA965C9CD1B44EBCD53635EE9ECC6D85B54210E2764C8836F4E9DEFA345/VC_redist.x86.exe"
UCRT_X64_URL="https://download.visualstudio.microsoft.com/download/pr/722d59e4-0671-477e-b9b1-b8da7d4bd60b/591CBE3A269AFBCC025681B968A29CD191DF3C6204712CBDC9BA1CB632BA6068/VC_redist.x64.exe"

case "${MSYSTEM}" in
  MINGW32)
    # 32-bit 7zip removed since 24.05
    # https://github.com/msys2/MINGW-packages/commit/de4ea25ca787035cbed50a158bdc200a3776254b
    _7Z="/mingw64/bin/7z"
    _7Z_PACKAGE_PREFIX="mingw-w64-x86_64"
    ;;
  MINGW64|UCRT64|CLANG64|CLANGARM64)
    _7Z="7z"
    _7Z_PACKAGE_PREFIX="${MINGW_PACKAGE_PREFIX}"
    ;;
esac

MINGW32_FOLDER="mingw32"
MINGW32_ARCHIVE="mingw32.7z"
MINGW32_COMPILER_NAME="MinGW-w64 i686 GCC 11.5"
MINGW32_PACKAGE_SUFFIX="MinGW32_11.5"

MINGW64_FOLDER="mingw64"
MINGW64_ARCHIVE="mingw64.7z"
MINGW64_COMPILER_NAME="MinGW-w64 X86_64 GCC 11.4"
MINGW64_PACKAGE_SUFFIX="MinGW64_11.4"

GCC_LINUX_X8664_ARCHIVE="gcc-linux-x86-64.7z"
ALPINE_X8664_ARCHIVE="alpine-minirootfs-x86_64.tar"

GCC_LINUX_AARCH64_ARCHIVE="gcc-linux-aarch64.7z"
ALPINE_AARCH64_ARCHIVE="alpine-minirootfs-aarch64.tar"

if [[ ${#compilers[@]} -eq 0 ]]; then
  PACKAGE_BASENAME="${PACKAGE_BASENAME}.NoCompiler"
else
  [[ ${COMPILER_MINGW32} -eq 1 ]] && PACKAGE_BASENAME="${PACKAGE_BASENAME}.${MINGW32_PACKAGE_SUFFIX}"
  [[ ${COMPILER_MINGW64} -eq 1 ]] && PACKAGE_BASENAME="${PACKAGE_BASENAME}.${MINGW64_PACKAGE_SUFFIX}"
  [[ ${COMPILER_GCC_LINUX_X8664} -eq 1 || ${COMPILER_GCC_LINUX_AARCH64} -eq 1 ]] && PACKAGE_BASENAME="${PACKAGE_BASENAME}.Linux_GCC"
fi

function fn_print_progress() {
  echo -e "\e[1;32;44m$1\e[0m"
}

## check deps

if [[ ${CHECK_DEPS} -eq 1 ]]; then
  deps=(
    ${MINGW_PACKAGE_PREFIX}-{cc,make,qt5-static,cmake}
    # always use x86 NSIS to display error message of mismatched architecture
    mingw-w64-i686-nsis
    ${_7Z_PACKAGE_PREFIX}-7zip
    git
  )

  for dep in ${deps[@]}; do
    pacman -Q ${dep} &>/dev/null || {
      echo "Missing dependency: ${dep}"
      exit 1
    }
  done
fi

if [[ ${COMPILER_MINGW32} -eq 1 && ! -f "${SOURCE_DIR}/assets/${MINGW32_ARCHIVE}" && ! -d "${SOURCE_DIR}/assets/${MINGW32_FOLDER}" ]]; then
  echo "Missing MinGW archive: assets/${MINGW32_ARCHIVE} or MinGW folder: assets/${MINGW32_FOLDER}"
  exit 1
fi
if [[ ${COMPILER_MINGW64} -eq 1 && ! -f "${SOURCE_DIR}/assets/${MINGW64_ARCHIVE}" && ! -d "${SOURCE_DIR}/assets/${MINGW64_FOLDER}" ]]; then
  echo "Missing MinGW archive: assets/${MINGW64_ARCHIVE} or MinGW folder: assets/${MINGW64_FOLDER}"
  exit 1
fi
if [[ ${COMPILER_GCC_LINUX_X8664} -eq 1 ]]; then
  if [[ ! -f "${SOURCE_DIR}/assets/${GCC_LINUX_X8664_ARCHIVE}" ]]; then
    echo "Missing GCC archive: assets/${GCC_LINUX_X8664_ARCHIVE}"
  fi
  if [[ ! -f "${SOURCE_DIR}/assets/${ALPINE_X8664_ARCHIVE}" ]]; then
    echo "Missing Alpine rootfs: assets/${ALPINE_X8664_ARCHIVE}"
  fi
fi
if [[ ${COMPILER_GCC_LINUX_AARCH64} -eq 1 ]]; then
  if [[ ! -f "${SOURCE_DIR}/assets/${GCC_LINUX_AARCH64_ARCHIVE}" ]]; then
    echo "Missing GCC archive: assets/${GCC_LINUX_AARCH64_ARCHIVE}"
  fi
  if [[ ! -f "${SOURCE_DIR}/assets/${ALPINE_AARCH64_ARCHIVE}" ]]; then
    echo "Missing Alpine rootfs: assets/${ALPINE_AARCH64_ARCHIVE}"
  fi
fi

if [[ ${UCRT_X86} -eq 1 ]] ; then
  if [[ ! -f "${SOURCE_DIR}/assets/VC_redist.x86.exe" ]]; then
    curl -L -o "${SOURCE_DIR}/assets/VC_redist.x86.exe" "${UCRT_X86_URL}"
  fi
fi
if [[ ${UCRT_X64} -eq 1 ]] ; then
  if [[ ! -f "${SOURCE_DIR}/assets/VC_redist.x64.exe" ]]; then
    curl -L -o "${SOURCE_DIR}/assets/VC_redist.x64.exe" "${UCRT_X64_URL}"
  fi
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
  -DREQUIRED_WINDOWS_BUILD="${REQUIRED_WINDOWS_BUILD}"
  -DREQUIRED_WINDOWS_NAME="${REQUIRED_WINDOWS_NAME}"
  -DUSE_MODERN_FONT
)
if [[ ${COMPILER_MINGW32} -eq 1 ]]; then
  nsis_flags+=(-DHAVE_MINGW32)
  if [[ ! -d "mingw32" ]]; then
	[[ -f "${SOURCE_DIR}/assets/${MINGW32_ARCHIVE}" ]] && "${_7Z}" x "${SOURCE_DIR}/assets/${MINGW32_ARCHIVE}" -o"${PACKAGE_DIR}"
	[[ -d "${SOURCE_DIR}/assets/${MINGW32_FOLDER}" ]] && cp -a --dereference "${SOURCE_DIR}/assets/${MINGW32_FOLDER}" "${PACKAGE_DIR}"
  fi 
fi
if [[ ${COMPILER_MINGW64} -eq 1 ]]; then
  nsis_flags+=(-DHAVE_MINGW64)
  if [[ ! -d "mingw64" ]]; then  
	[[ -f "${SOURCE_DIR}/assets/${MINGW64_ARCHIVE}" ]] && "${_7Z}" x "${SOURCE_DIR}/assets/${MINGW64_ARCHIVE}" -o"${PACKAGE_DIR}"
	[[ -d "${SOURCE_DIR}/assets/${MINGW64_FOLDER}" ]] && cp -a --dereference "${SOURCE_DIR}/assets/${MINGW64_FOLDER}" "${PACKAGE_DIR}"
  fi
fi
if [[ ${COMPILER_GCC_LINUX_X8664} -eq 1 ]]; then
  nsis_flags+=(-DHAVE_GCC_LINUX_X8664 -DSTRICT_ARCH_CHECK)
  if [[ ! -d "gcc-linux-x86_64" ]]; then
    "${_7Z}" x "${SOURCE_DIR}/assets/${GCC_LINUX_X8664_ARCHIVE}" -o"${PACKAGE_DIR}"
  fi
  if [[ ! -d "alpine-minirootfs.tar" ]]; then
    cp "${SOURCE_DIR}/assets/${ALPINE_X8664_ARCHIVE}" alpine-minirootfs.tar
  fi
fi
if [[ ${COMPILER_GCC_LINUX_AARCH64} -eq 1 ]]; then
  nsis_flags+=(-DHAVE_GCC_LINUX_AARCH64 -DSTRICT_ARCH_CHECK)
  if [[ ! -d "gcc-linux-aarch64" ]]; then
    "${_7Z}" x "${SOURCE_DIR}/assets/${GCC_LINUX_AARCH64_ARCHIVE}" -o"${PACKAGE_DIR}"
  fi
  if [[ ! -d "alpine-minirootfs.tar" ]]; then
    cp "${SOURCE_DIR}/assets/${ALPINE_AARCH64_ARCHIVE}" alpine-minirootfs.tar
  fi
fi
if [[ ${UCRT_X86} -eq 1 ]]; then
  nsis_flags+=(-DHAVE_UCRT_X86)
  cp "${SOURCE_DIR}/assets/VC_redist.x86.exe" VC_redist.x86.exe
fi
if [[ ${UCRT_X64} -eq 1 ]]; then
  nsis_flags+=(-DHAVE_UCRT_X64)
  cp "${SOURCE_DIR}/assets/VC_redist.x64.exe" VC_redist.x64.exe
fi
"${NSIS}" "${nsis_flags[@]}" redpanda.nsi

fn_print_progress "Making Portable Package..."
"${_7Z}" x "${SETUP_NAME}" -o"RedPanda-CPP" -xr'!$PLUGINSDIR' -x"!uninstall.exe"
"${_7Z}" a -mmt -mx9 -ms=on -mqs=on -mf=BCJ2 "${PORTABLE_NAME}" "RedPanda-CPP"
rm -rf "RedPanda-CPP"

mv "${SETUP_NAME}" "${TARGET_DIR}"
mv "${PORTABLE_NAME}" "${TARGET_DIR}"
popd
