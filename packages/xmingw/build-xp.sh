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
EOF
}

source version.inc
if [[ -n "$APP_VERSION_SUFFIX" ]]; then
  APP_VERSION="$APP_VERSION$APP_VERSION_SUFFIX"
fi

if [[ $# -lt 2 || ($1 != "-p" && $1 != "--profile") ]]; then
  echo "Missing profile argument."
  exit 1
fi

PROFILE=$2
shift 2

case "${PROFILE}" in
  64-ucrt|64-msvcrt)
    NSIS_ARCH=x64
    TARGET=x86_64-w64-mingw32
    PACKAGE_BASENAME="redpanda-cpp-${APP_VERSION}-ws2003_x64"
    ;;
  32-ucrt)
    NSIS_ARCH=x86
    TARGET=i686-w64-mingw32
    PACKAGE_BASENAME="redpanda-cpp-${APP_VERSION}-winxp_x86"
    ;;
  32-msvcrt)
    NSIS_ARCH=x86
    TARGET=i686-w64-mingw32
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
while [[ "$#" -gt 0 ]]; do
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
      case "$PROFILE" in
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
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done

QT_VERSION=5.15.15+redpanda0
MINGW_LITE_BRANCH=14

ASTYLE_VERSION_TAG="3.6.2"
TARGET_DIR="$PWD/dist"
BUILD_DIR="/tmp/build/redpanda-xp-$PROFILE-build"
ASTYLE_BUILD_DIR="$BUILD_DIR/astyle"
PACKAGE_DIR="/tmp/build/redpanda-xp-$PROFILE-pkg"
SOURCE_DIR="$PWD"
ASSETS_DIR="$PWD/assets"

QT_ARCHIVE="x-qt-$QT_VERSION-$PROFILE.tar.zst"
QT_DIR="/opt/x-mingw$PROFILE-$MINGW_LITE_BRANCH"

REDPANDA_MINGW_RELEASE="11.5.0-r0"
MINGW32_ARCHIVE="mingw32-$REDPANDA_MINGW_RELEASE.7z"
MINGW32_COMPILER_NAME="MinGW-w64 i686 GCC"
MINGW32_PACKAGE_SUFFIX="mingw32"

MINGW64_ARCHIVE="mingw64-$REDPANDA_MINGW_RELEASE.7z"
MINGW64_COMPILER_NAME="MinGW-w64 x86_64 GCC"
MINGW64_PACKAGE_SUFFIX="mingw64"

if [[ ${#compilers[@]} -eq 0 ]]; then
  PACKAGE_BASENAME="$PACKAGE_BASENAME-none"
else
  [[ "$COMPILER_MINGW32" -eq 1 ]] && PACKAGE_BASENAME="$PACKAGE_BASENAME-$MINGW32_PACKAGE_SUFFIX"
  [[ "$COMPILER_MINGW64" -eq 1 ]] && PACKAGE_BASENAME="$PACKAGE_BASENAME-$MINGW64_PACKAGE_SUFFIX"
fi

function fn_print_progress() {
  echo -e "\e[1;32;44m$1\e[0m"
}

## prepare dirs

if [[ "$CLEAN" -eq 1 ]]; then
  rm -rf "$BUILD_DIR"
  rm -rf "$PACKAGE_DIR"
fi
mkdir -p "$BUILD_DIR" "$PACKAGE_DIR" "$TARGET_DIR" "$ASTYLE_BUILD_DIR" "$ASSETS_DIR"

## install deps

if [[ -v MIRROR && -n $MIRROR ]]
then
  sed -i "s|archive.ubuntu.com|$MIRROR|; s|security.ubuntu.com|$MIRROR|" /etc/apt/sources.list.d/ubuntu.sources
fi

apt update
env DEBIAN_FRONTEND=noninteractive \
  apt install -y --no-install-recommends \
    7zip ca-certificates cmake curl git make nsis qttools5-dev-tools zstd

if [[ ! -f "$ASSETS_DIR/$QT_ARCHIVE" ]]; then
  fn_print_progress "Downloading Qt"
  curl -L "https://github.com/redpanda-cpp/qtbase-xp/releases/download/$QT_VERSION/$QT_ARCHIVE" -o "$ASSETS_DIR/$QT_ARCHIVE"
fi
zstdcat "$ASSETS_DIR/$QT_ARCHIVE" | tar -x -C /

export PATH="$QT_DIR/bin:$PATH"

## prepare assets

fn_print_progress "Updating astyle repo..."
if [[ ! -d "$ASSETS_DIR/astyle" ]]; then
  git clone --bare "https://gitlab.com/saalen/astyle" "$ASSETS_DIR/astyle"
fi
pushd "$ASSETS_DIR/astyle"
if [[ -z "$(git tag -l "$ASTYLE_VERSION_TAG")" ]]; then
  git fetch --all --tags
fi
popd

if [[ "$COMPILER_MINGW32" -eq 1 && ! -f "$ASSETS_DIR/$MINGW32_ARCHIVE" ]]; then
  fn_print_progress "Downloading MinGW32..."
  curl -L "https://github.com/redpanda-cpp/toolchain-win32-mingw-xp/releases/download/$REDPANDA_MINGW_RELEASE/$MINGW32_ARCHIVE" -o "$ASSETS_DIR/$MINGW32_ARCHIVE"
fi
if [[ "$COMPILER_MINGW64" -eq 1 && ! -f "$ASSETS_DIR/$MINGW64_ARCHIVE" ]]; then
  fn_print_progress "Downloading MinGW64..."
  curl -L "https://github.com/redpanda-cpp/toolchain-win32-mingw-xp/releases/download/$REDPANDA_MINGW_RELEASE/$MINGW64_ARCHIVE" -o "$ASSETS_DIR/$MINGW64_ARCHIVE"
fi

## build

fn_print_progress "Building astyle..."
pushd "$ASSETS_DIR/astyle"
git --work-tree="$ASTYLE_BUILD_DIR" checkout -f "$ASTYLE_VERSION_TAG"
popd

pushd "$ASTYLE_BUILD_DIR"
sed -i "s|<Windows.h>|<windows.h>|" AStyle/src/*.cpp

cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME="Windows" -DCMAKE_CC_COMPILER="$TARGET-gcc" -DCMAKE_CXX_COMPILER="$TARGET-g++" -DCMAKE_EXE_LINKER_FLAGS="-static"
make -j$(nproc)
cp AStyle/AStyle.exe "$PACKAGE_DIR/astyle.exe"
popd

fn_print_progress "Building..."
pushd "$BUILD_DIR"
qmake_flags=()
if [[ "$NSIS_ARCH" == x64 ]]; then
  qmake_flags+=("X86_64=ON")
fi
qmake PREFIX="$PACKAGE_DIR" "${qmake_flags[@]}" -o Makefile "$SOURCE_DIR/Red_Panda_CPP.pro" -r
make -j$(nproc)
make install
popd

## prepare packaging resources

pushd "$PACKAGE_DIR"

cp "$SOURCE_DIR/platform/windows/qt.conf" .

cp "$SOURCE_DIR"/platform/windows/installer-scripts/{lang.nsh,utils.nsh,redpanda.nsi} .
popd

## make package

pushd "$PACKAGE_DIR"
SETUP_NAME="$PACKAGE_BASENAME.exe"
PORTABLE_NAME="$PACKAGE_BASENAME.7z"

fn_print_progress "Making installer..."

nsis_flags=(
  -DAPP_VERSION="$APP_VERSION"
  -DARCH="$NSIS_ARCH"
  -DFINALNAME="$SETUP_NAME"
  -DMINGW32_COMPILER_NAME="$MINGW32_COMPILER_NAME"
  -DMINGW64_COMPILER_NAME="$MINGW64_COMPILER_NAME"
)
case "$PROFILE" in
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
if [[ "$COMPILER_MINGW32" -eq 1 ]]; then
  nsis_flags+=(-DHAVE_MINGW32)
  if [[ ! -d mingw32 ]]; then
    7z x "$ASSETS_DIR/$MINGW32_ARCHIVE" -o"$PACKAGE_DIR"
  fi
fi
if [[ "$COMPILER_MINGW64" -eq 1 ]]; then
  nsis_flags+=(-DHAVE_MINGW64)
  if [[ ! -d mingw64 ]]; then
    7z x "$ASSETS_DIR/$MINGW64_ARCHIVE" -o"$PACKAGE_DIR"
  fi
fi
makensis "${nsis_flags[@]}" redpanda.nsi

fn_print_progress "Making Portable Package..."
7z x "$SETUP_NAME" -o"RedPanda-CPP" -xr'!$PLUGINSDIR' -x"!uninstall.exe"
7z a -mmt -mx9 -ms=on -mqs=on -mf=BCJ2 "$PORTABLE_NAME" RedPanda-CPP
rm -rf RedPanda-CPP

mv "$SETUP_NAME" "$TARGET_DIR"
mv "$PORTABLE_NAME" "$TARGET_DIR"
popd
