#!/bin/bash

set -euxo pipefail

function fn_print_help() {
  cat <<EOF
Usage:
  packages/macos/build.sh [-a|--arch <arch>] [-c|--clean] [--qt-install-dir <dir>] [--qt-version <version>]
Options:
  -h, --help               Display this information.
  -a, --arch <arch>        Set target architecture. Available options: x86_64, arm64, universal.
  -c, --clean              Clean build and package directories.
  --qt-install-dir <dir>   Set Qt installation directory. Default: $HOME/Qt.
  --qt-version <version>   Set Qt version. Default: 6.8.1.
EOF
}

. version.inc
_TEST_VERSION=$(git rev-list HEAD --count)
if [[ -n "$APP_VERSION_SUFFIX" ]]; then
  _VERSION="$APP_VERSION.$_TEST_VERSION.$APP_VERSION_SUFFIX"
else
  _VERSION="$APP_VERSION.$_TEST_VERSION"
fi

_ARCH="universal"
_CLEAN=0
_QT_INSTALL_DIR="$HOME/Qt"
_QT_VERSION="6.8.1"
while [[ $# -gt 0 ]]; do
  case "$1" in
    -a|--arch)
      _ARCH="$2"
      shift 2
      ;;
    -c|--clean)
      _CLEAN=1
      ;;
    -h|--help)
      fn_print_help
      exit 0
      ;;
    --qt-install-dir)
      _QT_INSTALL_DIR="$2"
      shift 2
      ;;
    --qt-version)
      _QT_VERSION="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

case "$_ARCH" in
  x86_64)
    _DEVICE_ARCH="x86_64"
    ;;
  arm64)
    _DEVICE_ARCH="arm64"
    ;;
  universal)
    _DEVICE_ARCH="x86_64 arm64"
    ;;
  *)
    echo "Invalid architecture: $_ARCH. Available options: x86_64, arm64, universal."
    exit 1
    ;;
esac

_PROJECT_ROOT="$PWD"
case "$_QT_VERSION" in
  6.*.*)
    _QT_DIR="$_QT_INSTALL_DIR/$_QT_VERSION/macos"
    ;;
  *)
    echo "Invalid Qt version schemes: $_QT_VERSION. Available schemes: 6.x.y."
    exit 1
    ;;
esac

export PATH="$_QT_DIR/bin:$PATH"

function fn_check_qt_install() {
  if [[ ! -x "$_QT_DIR/bin/qmake" ]]; then
    echo "Qt not found at $_QT_DIR. Please specify the correct path with --qt-install-dir."
    exit 1
  fi
}

function fn_prepare_dirs() {
  if [[ $_CLEAN -eq 1 ]]; then
    rm -rf build/$_ARCH pkg/$_ARCH
  fi
  mkdir -p build/$_ARCH dist
}

function fn_build() {
  pushd build/$_ARCH
  {
    qmake \
      PREFIX=$_PROJECT_ROOT/pkg/$_ARCH \
      QMAKE_APPLE_DEVICE_ARCHS="$_DEVICE_ARCH" \
      $_PROJECT_ROOT/Red_Panda_CPP.pro
    make -j$(sysctl -n hw.logicalcpu)
  }
  popd
}

function fn_package() {
  pushd build/$_ARCH
  {
    make install
  }
  popd

  macdeployqt pkg/$_ARCH/bin/RedPandaIDE.app
  tar -C pkg/$_ARCH/bin -cJf dist/RedPandaIDE-$_VERSION-$_ARCH.tar.xz RedPandaIDE.app
}

fn_check_qt_install
fn_prepare_dirs
fn_build
fn_package
