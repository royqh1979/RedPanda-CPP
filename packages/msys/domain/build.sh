#!/bin/bash

case $MSYSTEM in
  MINGW32|CLANG32)
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
    echo "  - MINGW32/CLANG32"
    echo "  - MINGW64/UCRT64/CLANG64"
    echo "  - CLANGARM64"
    exit 1
    ;;
esac

set -euxo pipefail

GCC_VERSION="13.2.0"
MINGW_VERSION="rt_v11-rev1"
REDPANDA_LLVM_VERSION="17-r0"
WINDOWS_TERMINAL_VERSION="1.18.3181.0"

_QMAKE="$MINGW_PREFIX/qt5-static/bin/qmake"
_NSIS="/mingw32/bin/makensis"

_MINGW32_DIR="mingw32"
_MINGW32_TRIPLET="i686-w64-mingw32"
_MINGW32_ARCHIVE="i686-$GCC_VERSION-release-posix-dwarf-ucrt-$MINGW_VERSION.7z"
_MINGW32_URL="https://github.com/niXman/mingw-builds-binaries/releases/download/$GCC_VERSION-$MINGW_VERSION/$_MINGW32_ARCHIVE"

_MINGW64_DIR="mingw64"
_MINGW64_TRIPLET="x86_64-w64-mingw32"
_MINGW64_ARCHIVE="x86_64-$GCC_VERSION-release-posix-seh-ucrt-$MINGW_VERSION.7z"
_MINGW64_URL="https://github.com/niXman/mingw-builds-binaries/releases/download/$GCC_VERSION-$MINGW_VERSION/$_MINGW64_ARCHIVE"

_LLVM_DIR="llvm-mingw"
_LLVM_ARCHES=("x86_64" "i686" "aarch64")
_LLVM_ARCHIVE="$_LLVM_DIR-$REDPANDA_LLVM_VERSION-$_NATIVE_ARCH.7z"
_LLVM_URL="https://github.com/redpanda-cpp/toolchain-win32-llvm/releases/download/$REDPANDA_LLVM_VERSION/$_LLVM_ARCHIVE"

_WINDOWS_TERMINAL_DIR="terminal-${WINDOWS_TERMINAL_VERSION}"
_WINDOWS_TERMINAL_ARCHIVE="Microsoft.WindowsTerminal_${WINDOWS_TERMINAL_VERSION}_$_DISPLAY_ARCH.zip"
_WINDOWS_TERMINAL_URL="https://github.com/microsoft/terminal/releases/download/v${WINDOWS_TERMINAL_VERSION}/${_WINDOWS_TERMINAL_ARCHIVE}"

_SRCDIR="$PWD"
_ASSETSDIR="$PWD/assets"
_BUILDDIR="$TEMP/$MINGW_PACKAGE_PREFIX-redpanda-build"
_PKGDIR="$TEMP/$MINGW_PACKAGE_PREFIX-redpanda-pkg"
_DISTDIR="$PWD/dist"

_REDPANDA_VERSION=$(sed -nr -e '/APP_VERSION\s*=/ s/APP_VERSION\s*=\s*(([0-9]+\.)*[0-9]+)\s*/\1/p' "$_SRCDIR"/Red_Panda_CPP.pro)
_REDPANDA_TESTVERSION=$(sed -nr -e '/TEST_VERSION\s*=/ s/TEST_VERSION\s*=\s*([A-Za-z0-9]*)\s*/\1/p' "$_SRCDIR"/Red_Panda_CPP.pro)
if [[ -n $_REDPANDA_TESTVERSION ]]; then
  _REDPANDA_VERSION="$_REDPANDA_VERSION.$_REDPANDA_TESTVERSION"
fi

_CLEAN=0
_SKIP_DEPS_CHECK=0
_7Z_REPACK=0
while [[ $# -gt 0 ]]; do
  case $1 in
    --clean)
      _CLEAN=1
      shift
      ;;
    --skip-deps-check)
      _SKIP_DEPS_CHECK=1
      shift
      ;;
    --7z)
      _7Z_REPACK=1
      shift
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
  case $MSYSTEM in
    MINGW32|MINGW64|UCRT64)
      local compiler=gcc
      ;;
    CLANG32|CLANG64|CLANGARM64)
      local compiler=clang
      ;;
  esac
  local deps=(
    $MINGW_PACKAGE_PREFIX-{$compiler,make,qt5-static}
    mingw-w64-i686-nsis
  )
  [[ _7Z_REPACK -eq 1 ]] && deps+=("$MINGW_PACKAGE_PREFIX-7zip")
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
  mkdir -p "$_ASSETSDIR" "$_BUILDDIR" "$_PKGDIR" "$_DISTDIR"
}

function download-assets() {
  if [[ $_NATIVE_ARCH == i686 ]]; then
    [[ -f "$_ASSETSDIR/$_MINGW32_ARCHIVE" ]] || curl -L -o "$_ASSETSDIR/$_MINGW32_ARCHIVE" "$_MINGW32_URL"
  fi
  if [[ $_NATIVE_ARCH == x86_64 ]]; then
    [[ -f "$_ASSETSDIR/$_MINGW64_ARCHIVE" ]] || curl -L -o "$_ASSETSDIR/$_MINGW64_ARCHIVE" "$_MINGW64_URL"
  fi
  [[ -f "$_ASSETSDIR/$_LLVM_ARCHIVE" ]] || curl -L -o "$_ASSETSDIR/$_LLVM_ARCHIVE" "$_LLVM_URL"
  [[ -f "$_ASSETSDIR/$_WINDOWS_TERMINAL_ARCHIVE" ]] || curl -L -o "$_ASSETSDIR/$_WINDOWS_TERMINAL_ARCHIVE" "$_WINDOWS_TERMINAL_URL"
}

function prepare-mingw() {
  local bit="$1"
  eval local mingw_dir="\$_MINGW${bit}_DIR"
  eval local mingw_archive="\$_MINGW${bit}_ARCHIVE"
  eval local mingw_triplet="\$_MINGW${bit}_TRIPLET"
  local mingw_dir="$_BUILDDIR/$mingw_dir"
  local mingw_lib_dir="$mingw_dir/$mingw_triplet/lib"
  local mingw_shared_dir="$mingw_dir/$mingw_triplet/bin"
  local mingw_include_dir="$mingw_dir/$mingw_triplet/include"
  if [[ ! -d "$mingw_dir" ]]; then
    bsdtar -C "$_BUILDDIR" -xf "$_ASSETSDIR/$mingw_archive"
    local old_path="$PATH"
    export PATH="$mingw_dir/bin:$PATH"
    {
      gcc -Os -fno-exceptions -nodefaultlibs -nostdlib -c -o "$mingw_lib_dir/utf8init.o" "$_SRCDIR/platform/windows/utf8/utf8init.cpp"
      windres -O coff -o "$mingw_lib_dir/utf8manifest.o" "$_SRCDIR/platform/windows/utf8/utf8manifest.rc"
    }
    export PATH="$old_path"
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

function build() {
  pushd "$_BUILDDIR"
  "$_QMAKE" PREFIX="$_PKGDIR" "$_SRCDIR"
  time mingw32-make WINDOWS_PREFER_OPENCONSOLE=ON -j$(nproc)
  mingw32-make install

  cp "$_SRCDIR"/packages/msys/domain/{main.nsi,lang.nsh,compiler_hint.lua} "$_PKGDIR"
  cp "$_WINDOWS_TERMINAL_DIR/OpenConsole.exe" "$_PKGDIR"
  if [[ $_NATIVE_ARCH == i686 ]]; then
    [[ -d "$_PKGDIR/mingw32" ]] || cp -r "mingw32" "$_PKGDIR"
  fi
  if [[ $_NATIVE_ARCH == x86_64 ]]; then
    [[ -d "$_PKGDIR/mingw64" ]] || cp -r "mingw64" "$_PKGDIR"
  fi
  [[ -d "$_PKGDIR/llvm-mingw" ]] || bsdtar -C "$_PKGDIR" -xf "$_ASSETSDIR/$_LLVM_ARCHIVE"
  popd
}

function package() {
  pushd "$_PKGDIR"
  "$_NSIS" -DVERSION="$_REDPANDA_VERSION" -DARCH="$_DISPLAY_ARCH" main.nsi &
  "$_NSIS" -DVERSION="$_REDPANDA_VERSION" -DARCH="$_DISPLAY_ARCH" -DUSER_MODE main.nsi &
  wait
  if [[ _7Z_REPACK -eq 1 ]]; then
    7z x "redpanda-cpp-$_REDPANDA_VERSION-$_DISPLAY_ARCH-user.exe" -o"RedPanda-CPP" -xr'!$PLUGINSDIR' -x"!uninstall.exe"
    7z a -t7z -mx=9 -ms=on -mqs=on -mf=BCJ2 -m0="LZMA2:d=128m:fb=273:c=2g" "redpanda-cpp-$_REDPANDA_VERSION-$_DISPLAY_ARCH.7z" "RedPanda-CPP"
    rm -rf "RedPanda-CPP"
  fi
  popd
}

function dist() {
  cp "$_PKGDIR/redpanda-cpp-$_REDPANDA_VERSION-$_DISPLAY_ARCH-system.exe" "$_DISTDIR"
  cp "$_PKGDIR/redpanda-cpp-$_REDPANDA_VERSION-$_DISPLAY_ARCH-user.exe" "$_DISTDIR"
  [[ _7Z_REPACK -eq 1 ]] && cp "$_PKGDIR/redpanda-cpp-$_REDPANDA_VERSION-$_DISPLAY_ARCH.7z" "$_DISTDIR"
}

check-deps
prepare-dirs
download-assets
[[ $_NATIVE_ARCH == i686 ]] && prepare-mingw 32
[[ $_NATIVE_ARCH == x86_64 ]] && prepare-mingw 64
prepare-openconsole
prepare-src
trap restore-src EXIT INT TERM
build
package
dist
