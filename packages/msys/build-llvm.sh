#!/bin/bash

set -euxo pipefail

function fn_print_help() {
  cat <<EOF
Usage:
  packages/msys/build-llvm.sh [-m|--msystem <MSYSTEM>] [-c|--clean] [-nd|--no-deps] [-t|--target-dir <dir>]
Options:
  -h, --help               Display this information.
  -m, --msystem <MSYSTEM>  Switch to other MSYS2 environment.
                           (MINGW32, MINGW64, UCRT64, CLANG64, CLANGARM64)
                           MUST be used before other options.
  -c, --clean              Clean build and package directories.
  -nd, --no-deps           Skip dependency check.
  -t, --target-dir <dir>   Set target directory for the packages.
EOF
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

case $MSYSTEM in
  MINGW32)
    # there is no UCRT32
    # CLANG32 qt5-static removed since 5.15.15
    # https://github.com/msys2/MINGW-packages/commit/ab062c6e5d6e9fff86ee8f88c1d8e9601ea9ab5b
    _NATIVE_ARCH=i686
    _DISPLAY_ARCH=x86
    ;;
  MINGW64|UCRT64|CLANG64)
    _NATIVE_ARCH=x86_64
    _DISPLAY_ARCH=x64
    ;;
  CLANGARM64)
    _NATIVE_ARCH=aarch64
    _DISPLAY_ARCH=arm64
    ;;
  *)
    echo "This script must be run from one of following MSYS2 shells:"
    echo "  - MINGW32"
    echo "  - MINGW64/UCRT64/CLANG64"
    echo "  - CLANGARM64"
    exit 1
    ;;
esac

REDPANDA_LLVM_VERSION="18-r0"
WINDOWS_TERMINAL_VERSION="1.19.10821.0"
ASTYLE_VERSION_TAG="3.4.14"

_QMAKE="$MINGW_PREFIX/qt5-static/bin/qmake"
_NSIS="/mingw32/bin/makensis"

_FINAL_NAME="redpanda-cpp-$APP_VERSION-$_DISPLAY_ARCH-llvm.exe"

_LLVM_DIR="llvm-mingw"
_LLVM_ARCHES=("x86_64" "i686" "aarch64")
_LLVM_ARCHIVE="$_LLVM_DIR-$REDPANDA_LLVM_VERSION-$_NATIVE_ARCH.7z"
_LLVM_URL="https://github.com/redpanda-cpp/toolchain-win32-llvm/releases/download/$REDPANDA_LLVM_VERSION/$_LLVM_ARCHIVE"

_WINDOWS_TERMINAL_DIR="terminal-${WINDOWS_TERMINAL_VERSION}"
_WINDOWS_TERMINAL_ARCHIVE="Microsoft.WindowsTerminal_${WINDOWS_TERMINAL_VERSION}_$_DISPLAY_ARCH.zip"
_WINDOWS_TERMINAL_URL="https://github.com/microsoft/terminal/releases/download/v${WINDOWS_TERMINAL_VERSION}/${_WINDOWS_TERMINAL_ARCHIVE}"

_SRCDIR="$PWD"
_ASSETSDIR="$PWD/assets"
_BUILDDIR="$TEMP/redpanda-llvm-$MSYSTEM-build"
_ASTYLE_BUILD_DIR="${_BUILDDIR}/astyle"
_PKGDIR="$TEMP/redpanda-llvm-$MSYSTEM-pkg"
_DISTDIR="$PWD/dist"

_CLEAN=0
_SKIP_DEPS_CHECK=0
while [[ $# -gt 0 ]]; do
  case $1 in
    -c|--clean)
      _CLEAN=1
      shift
      ;;
    -nd|--no-deps)
      _SKIP_DEPS_CHECK=1
      shift
      ;;
    -t|--target-dir)
      _DISTDIR="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done

function check-deps() {
  # MSYS2’s `pacman -Q` is 100x slower than Arch Linux. Allow skipping the check.
  [[ $_SKIP_DEPS_CHECK -eq 1 ]] && return
  local deps=(
    $MINGW_PACKAGE_PREFIX-{cc,make,qt5-static}
    # always use x86 NSIS to display error message of mismatched architecture
    mingw-w64-i686-nsis
    git
  )
  for dep in "${deps[@]}"; do
    pacman -Q "$dep" >/dev/null 2>&1 || (
      echo "Missing dependency: $dep"
      exit 1
    )
  done
}

function prepare-dirs() {
  if [[ $_CLEAN -eq 1 ]]; then
    [[ -d "$_BUILDDIR" ]] && rm -rf "$_BUILDDIR"
    [[ -d "$_PKGDIR" ]] && rm -rf "$_PKGDIR"
  fi
  mkdir -p "$_ASSETSDIR" "$_BUILDDIR" "$_ASTYLE_BUILD_DIR" "$_PKGDIR" "$_DISTDIR"
}

function download-assets() {
  [[ -f "$_ASSETSDIR/$_LLVM_ARCHIVE" ]] || curl -L -o "$_ASSETSDIR/$_LLVM_ARCHIVE" "$_LLVM_URL"
  [[ -f "$_ASSETSDIR/$_WINDOWS_TERMINAL_ARCHIVE" ]] || curl -L -o "$_ASSETSDIR/$_WINDOWS_TERMINAL_ARCHIVE" "$_WINDOWS_TERMINAL_URL"
  
  if [[ ! -d "$_ASSETSDIR/astyle" ]]; then
    git clone --bare "https://gitlab.com/saalen/astyle" "$_ASSETSDIR/astyle"
  fi
  pushd "$_ASSETSDIR/astyle"
  if [[ -z "$(git tag -l "$ASTYLE_VERSION_TAG")" ]]; then
    git fetch --all --tags
  fi
}

function prepare-openconsole() {
  local windows_terminal_dir="$_BUILDDIR/$_WINDOWS_TERMINAL_DIR"
  if [[ ! -d "$windows_terminal_dir" ]]; then
    bsdtar -C "$_BUILDDIR" -xf "$_ASSETSDIR/$_WINDOWS_TERMINAL_ARCHIVE"
  fi
}

function prepare-src() {
  cp "$_SRCDIR"/RedPandaIDE/RedPandaIDE.pro{,.bak}
  sed -i '/CONFIG += ENABLE_LUA_ADDON/ { s/^#\s*// }' "$_SRCDIR"/RedPandaIDE/RedPandaIDE.pro
}

function restore-src() {
  mv "$_SRCDIR"/RedPandaIDE/RedPandaIDE.pro{.bak,}
}

function build-astyle() {
  pushd "$_ASSETSDIR/astyle"
  git --work-tree="${_ASTYLE_BUILD_DIR}" checkout -f "$ASTYLE_VERSION_TAG"
  popd

  pushd "$_ASTYLE_BUILD_DIR"
  cmake . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-static"
  mingw32-make -j$(nproc)
  cp AStyle/AStyle.exe "$_PKGDIR/astyle.exe"
  popd
}

function build() {
  pushd "$_BUILDDIR"
  "$_QMAKE" PREFIX="$_PKGDIR" "$_SRCDIR"
  time mingw32-make WINDOWS_PREFER_OPENCONSOLE=ON -j$(nproc)
  mingw32-make install

  cp "$_SRCDIR"/packages/msys/compiler_hint.lua "$_PKGDIR"
  cp "$_SRCDIR"/platform/windows/installer-scripts/* "$_PKGDIR"
  cp "$_WINDOWS_TERMINAL_DIR/OpenConsole.exe" "$_PKGDIR"
  [[ -d "$_PKGDIR/llvm-mingw" ]] || bsdtar -C "$_PKGDIR" -xf "$_ASSETSDIR/$_LLVM_ARCHIVE"
  popd
}

function package() {
  pushd "$_PKGDIR"
  nsis_flags=(
    -DAPP_VERSION="$APP_VERSION"
    -DARCH="$_DISPLAY_ARCH"
    -DFINALNAME="$_FINAL_NAME"
    -DREQUIRED_WINDOWS_BUILD=18362
    -DREQUIRED_WINDOWS_NAME="Windows 10 v1903"
    -DUSE_MODERN_FONT
    -DHAVE_LLVM
    -DHAVE_OPENCONSOLE
    -DHAVE_COMPILER_HINT
  )
  "$_NSIS" "${nsis_flags[@]}" redpanda.nsi
  popd
}

function dist() {
  cp "$_PKGDIR/$_FINAL_NAME" "$_DISTDIR"
}

check-deps
prepare-dirs
download-assets
prepare-openconsole
prepare-src
build-astyle
trap restore-src EXIT INT TERM
build
package
dist
