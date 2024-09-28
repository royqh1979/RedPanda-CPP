#!/bin/bash

set -euxo pipefail

function fn_print_help() {
  cat <<EOF
Usage:
  packages/msys/build-xp.sh -p|--profile <profile> [-c|--clean] [-nd|--no-deps] [-t|--target-dir <dir>]
Options:
  -h, --help               Display this information.
  -p, --profile <profile>  MinGW Lite profile.
                           MUST be used before other options.
  -c, --clean              Clean build and package directories.
  --mingw                  Alias for --mingw32 (x86 app) or --mingw64 (x64 app).
  --mingw32                Build mingw32 integrated compiler.
  --mingw64                Build mingw64 integrated compiler.
  --ucrt <build>           Include UCRT in the package. Windows SDK required.
                           e.g. '--ucrt 22621' for Windows 11 SDK 22H2.
  -t, --target-dir <dir>   Set target directory for the packages.
EOF
}

source version.inc
[[ -n "${APP_VERSION_SUFFIX}" ]] && APP_VERSION="${APP_VERSION}${APP_VERSION_SUFFIX}"

if [[ $# -lt 2 || ($1 != "-p" && $1 != "--profile") ]]; then
  echo "Missing profile argument."
  exit 1
fi

if [[ ! -v MSYSTEM ]]; then
  echo "This script must be run in MSYS2 shell"
  exit 1
fi

PROFILE=$2
shift 2

QT_VERSION="5.15.15+redpanda0"
case "${PROFILE}" in
  64-ucrt|64-msvcrt)
    NSIS_ARCH=x64
    PACKAGE_BASENAME="redpanda-cpp-${APP_VERSION}-ws2003_x64"
    ;;
  32-ucrt)
    NSIS_ARCH=x86
    PACKAGE_BASENAME="redpanda-cpp-${APP_VERSION}-winxp_x86"
    ;;
  32-msvcrt)
    NSIS_ARCH=x86
    PACKAGE_BASENAME="redpanda-cpp-${APP_VERSION}-win2000_x86"
    ;;
  *)
    echo "Invalid profile: ${PROFILE}"
    echo "Available profiles: 64-ucrt, 64-msvcrt, 32-ucrt, 32-msvcrt"
    exit 1
    ;;
esac

CLEAN=0
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
      case "${PROFILE}" in
        64-ucrt|64-msvcrt)
          compilers+=("mingw64")
          COMPILER_MINGW64=1
          shift
          ;;
        32-ucrt|32-msvcrt)
          compilers+=("mingw32")
          COMPILER_MINGW32=1
          shift
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
      case "${PROFILE}" in
        64-ucrt|32-ucrt)
          UCRT="$2"
          shift 2
          ;;
        *)
          echo "Error: Red Panda C++ is not built against UCRT."
          exit 1
          ;;
      esac
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

ASTYLE_VERSION_TAG="3.6.2"
BUILD_DIR="${TEMP}/redpanda-xp-${PROFILE}-build"
ASTYLE_BUILD_DIR="${BUILD_DIR}/astyle"
PACKAGE_DIR="${TEMP}/redpanda-xp-${PROFILE}-pkg"
NSIS="/mingw32/bin/makensis"
_7Z="/mingw64/bin/7z"
CMAKE="/mingw64/bin/cmake"
SOURCE_DIR="$(pwd)"
ASSETS_DIR="${SOURCE_DIR}/assets"
UCRT_DIR="/c/Program Files (x86)/Windows Kits/10/Redist/10.0.${UCRT}.0/ucrt/DLLs/${NSIS_ARCH}"

QT_ARCHIVE="qt-$QT_VERSION-$PROFILE.tar.zst"
QT_DIR="/c/Qt/${QT_VERSION}/${PROFILE}"

REDPANDA_MINGW_RELEASE="11.5.0-r0"
MINGW32_ARCHIVE="mingw32-${REDPANDA_MINGW_RELEASE}.7z"
MINGW32_COMPILER_NAME="MinGW-w64 i686 GCC"
MINGW32_PACKAGE_SUFFIX="mingw32"

MINGW64_ARCHIVE="mingw64-${REDPANDA_MINGW_RELEASE}.7z"
MINGW64_COMPILER_NAME="MinGW-w64 x86_64 GCC"
MINGW64_PACKAGE_SUFFIX="mingw64"

if [[ ${#compilers[@]} -eq 0 ]]; then
  PACKAGE_BASENAME="${PACKAGE_BASENAME}-none"
else
  [[ ${COMPILER_MINGW32} -eq 1 ]] && PACKAGE_BASENAME="${PACKAGE_BASENAME}-${MINGW32_PACKAGE_SUFFIX}"
  [[ ${COMPILER_MINGW64} -eq 1 ]] && PACKAGE_BASENAME="${PACKAGE_BASENAME}-${MINGW64_PACKAGE_SUFFIX}"
fi

function fn_print_progress() {
  echo -e "\e[1;32;44m$1\e[0m"
}

## check deps

if [[ -n "${UCRT}" && ! -f "${UCRT_DIR}/ucrtbase.dll" ]]; then
  echo "Missing Windows SDK, UCRT cannot be included."
  exit 1
fi

## prepare dirs

if [[ ${CLEAN} -eq 1 ]]; then
  rm -rf "${BUILD_DIR}"
  rm -rf "${PACKAGE_DIR}"
fi
mkdir -p /c/Qt "${BUILD_DIR}" "${PACKAGE_DIR}" "${TARGET_DIR}" "${ASTYLE_BUILD_DIR}" "${ASSETS_DIR}"

## install deps

pacman -S --noconfirm --needed \
  mingw-w64-x86_64-7zip mingw-w64-x86_64-cmake \
  mingw-w64-i686-nsis \
  curl git

if [[ ! -f "$ASSETS_DIR/$QT_ARCHIVE" ]]; then
  fn_print_progress "Downloading Qt"
  curl -L "https://github.com/redpanda-cpp/qtbase-xp/releases/download/$QT_VERSION/$QT_ARCHIVE" -o "$ASSETS_DIR/$QT_ARCHIVE"
fi
zstdcat "$ASSETS_DIR/$QT_ARCHIVE" | tar -x -C /c/Qt

export PATH="${QT_DIR}/bin:${PATH}"

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

if [[ ${COMPILER_MINGW32} -eq 1 && ! -f "${ASSETS_DIR}/${MINGW32_ARCHIVE}" ]]; then
  curl -L "https://github.com/redpanda-cpp/toolchain-win32-mingw-xp/releases/download/${REDPANDA_MINGW_RELEASE}/${MINGW32_ARCHIVE}" -o "${ASSETS_DIR}/${MINGW32_ARCHIVE}"
fi
if [[ ${COMPILER_MINGW64} -eq 1 && ! -f "${ASSETS_DIR}/${MINGW64_ARCHIVE}" ]]; then
  curl -L "https://github.com/redpanda-cpp/toolchain-win32-mingw-xp/releases/download/${REDPANDA_MINGW_RELEASE}/${MINGW64_ARCHIVE}" -o "${ASSETS_DIR}/${MINGW64_ARCHIVE}"
fi

## build
fn_print_progress "Building astyle..."
pushd "${ASSETS_DIR}/astyle"
git --work-tree="${ASTYLE_BUILD_DIR}" checkout -f "${ASTYLE_VERSION_TAG}"
popd

pushd .
cd "${ASTYLE_BUILD_DIR}"
"${CMAKE}" . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release  -DCMAKE_EXE_LINKER_FLAGS="-static"
mingw32-make -j$(nproc)
cp AStyle/AStyle.exe "${PACKAGE_DIR}/astyle.exe"
popd

fn_print_progress "Building..."
pushd .
cd "${BUILD_DIR}"
qmake_flags=()
[[ ${NSIS_ARCH} == x64 ]] && qmake_flags+=("X86_64=ON")
qmake PREFIX="${PACKAGE_DIR}" ${qmake_flags[@]} -o Makefile "${SOURCE_DIR}/Red_Panda_CPP.pro" -r
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
SETUP_NAME="${PACKAGE_BASENAME}.exe"
PORTABLE_NAME="${PACKAGE_BASENAME}.7z"

fn_print_progress "Making installer..."

nsis_flags=(
  -DAPP_VERSION="${APP_VERSION}"
  -DARCH="${NSIS_ARCH}"
  -DFINALNAME="${SETUP_NAME}"
  -DMINGW32_COMPILER_NAME="${MINGW32_COMPILER_NAME}"
  -DMINGW64_COMPILER_NAME="${MINGW64_COMPILER_NAME}"
)
case "${PROFILE}" in
  64-ucrt|64-msvcrt)
    nsis_flags+=(
      -DREQUIRED_WINDOWS_BUILD=3790
      -DREQUIRED_WINDOWS_NAME="Windows Server 2003"
    )
    ;;
  32-ucrt)
    nsis_flags+=(
      -DREQUIRED_WINDOWS_BUILD=2600
      -DREQUIRED_WINDOWS_NAME="Windows XP"
    )
    ;;
  32-msvcrt)
    nsis_flags+=(
      -DREQUIRED_WINDOWS_BUILD=2195
      -DREQUIRED_WINDOWS_NAME="Windows 2000"
    )
    ;;
esac
if [[ ${COMPILER_MINGW32} -eq 1 ]]; then
  nsis_flags+=(-DHAVE_MINGW32)
  [[ -d "mingw32" ]] || "${_7Z}" x "$ASSETS_DIR/${MINGW32_ARCHIVE}" -o"${PACKAGE_DIR}"
fi
if [[ ${COMPILER_MINGW64} -eq 1 ]]; then
  nsis_flags+=(-DHAVE_MINGW64)
  [[ -d "mingw64" ]] || "${_7Z}" x "$ASSETS_DIR/${MINGW64_ARCHIVE}" -o"${PACKAGE_DIR}"
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
"${_7Z}" x "${SETUP_NAME}" -o"RedPanda-CPP" -xr'!$PLUGINSDIR' -x"!uninstall.exe"
"${_7Z}" a -mmt -mx9 -ms=on -mqs=on -mf=BCJ2 "${PORTABLE_NAME}" "RedPanda-CPP"
rm -rf "RedPanda-CPP"

mv "${SETUP_NAME}" "${TARGET_DIR}"
mv "${PORTABLE_NAME}" "${TARGET_DIR}"
popd
