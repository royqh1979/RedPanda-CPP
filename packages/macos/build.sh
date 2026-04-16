#!/bin/bash

set -euxo pipefail

source packages/_common/config.sh

function fn_print_help() {
  cat <<EOF
Usage:
  packages/macos/build.sh [-c|--clean] [--qt-dir <dir>]
Options:
  -h, --help               Display this information.
  -c, --clean              Clean build and package directories.
  --qt-dir <dir>           Specify the Qt directory.
EOF
}

_CLEAN=0
_QT_DIR=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    -c|--clean)
      _CLEAN=1
      ;;
    -h|--help)
      fn_print_help
      exit 0
      ;;
    --qt-dir)
      _QT_DIR="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

_PROJECT_ROOT="$PWD"
_BUILD_DIR="$_PROJECT_ROOT/build/macos"
_PKG_DIR="$_PROJECT_ROOT/build/macos-pkg"

export PATH="$_QT_DIR/bin:$PATH"

function fn_check_qt_install() {
  if [[ ! -x "$_QT_DIR/bin/qmake" ]]; then
    echo "Qt not found at $_QT_DIR. Please specify the correct path with --qt-dir."
    exit 1
  fi
}

function fn_prepare_dirs() {
  if [[ $_CLEAN -eq 1 ]]; then
    rm -rf "$_BUILD_DIR" "$_PKG_DIR"
  fi
  mkdir -p dist
}

function fn_build() {
  cmake -S . -B "$_BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$_QT_DIR" \
    -DCMAKE_INSTALL_PREFIX="$_PKG_DIR" \
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"

  cmake --build "$_BUILD_DIR" --parallel $(sysctl -n hw.logicalcpu)
}

function fn_package() {
  cmake --install "$_BUILD_DIR"

  macdeployqt "$_PKG_DIR/RedPandaIDE.app"
  tar -C "$_PKG_DIR" -cJf dist/RedPandaIDE-$APP_VERSION.tar.xz RedPandaIDE.app
}

fn_check_qt_install
fn_prepare_dirs
fn_build
fn_package
