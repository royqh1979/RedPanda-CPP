#!/bin/bash

SCRIPT_DIR=$( dirname ${BASH_SOURCE[0]} )
source "${SCRIPT_DIR}/build-config.sh"
function fn_print_help() {
  echo " Usage:
   packages/msys/build-qt5.sh [-32|64] [-c|--clean] [-nd|--no-deps] -d|--distribution-dir <dir> -t|--tools-dir <dir> 
 Options:
   -h, --help               	Display this information.
   -c, --clean              	Clean build and package directories.
   -32							Build 32bit version
   -64							Build 64bit version(default)
   --mingw                  	Build mingwintegrated compiler.
   -nd, --no-deps           	Skip dependency check.
   -d, --distribution-dir <dir>	Set directory for the packages.
   -t, --tools-dir <dir> 		Set parent directory for mingw and qt"
}

CLEAN=0
CHECK_DEPS=1
compilers=()
INTEGRATE_MINGW=0
TARGET_DIR=""
UCRT=""
MSYSTEM="MINGW64"
TOOLS_DIR=""
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
    -32)
	  MSYSTEM="MINGW32"
      shift
      ;;
    -64)
	  MSYSTEM="MINGW64"
      shift
      ;;     
    --mingw)
	  INTEGRATE_MINGW=1
      ;;
    -nd|--no-deps)
      CHECK_DEPS=0
      shift
      ;;
    -d|--distribution-dir)
      TARGET_DIR="$2"
      shift 2
      ;;
    -q|--
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done

if [ -z "${TARGET}" ]; then
	echo " Error:target directory not set"
	fn_print_help
	exit 1
fi

if [ $MSYTSTEM="MINGW64" ]; then

fi

BUILD_DIR="${TEMP}/redpanda-mingw-${MSYSTEM}-build"
ASTYLE_BUILD_DIR="${BUILD_DIR}/astyle"
PACKAGE_DIR="${TEMP}/redpanda-mingw-${MSYSTEM}-pkg"
QMAKE="${MINGW_PREFIX}/qt5-static/bin/qmake"
NSIS="/mingw32/bin/makensis"
SOURCE_DIR="$(pwd)"
ASSETS_DIR="${SOURCE_DIR}/assets"
UCRT_DIR="/c/Program Files (x86)/Windows Kits/10/Redist/10.0.${UCRT}.0/ucrt/DLLs/${NSIS_ARCH}"
