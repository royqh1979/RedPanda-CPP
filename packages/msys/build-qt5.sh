#!/bin/bash

SCRIPT_DIR=$( dirname ${BASH_SOURCE[0]} )
source "${SCRIPT_DIR}/config.sh"

function fn_print_help() {
  echo " Usage:
   packages/msys/build-qt5.sh [-32|64] [-c|--clean] [-nd|--no-deps] [-m|--mingw] -d|--distribution-dir <dir> -t|--tools-dir <dir> 
 Options:
   -h, --help               	Display this information.
   -c, --clean              	Clean build and package directories.
   -32							Build 32bit version
   -64							Build 64bit version(default)
   -m, --mingw                  	Build mingwintegrated compiler.
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
    -m|--mingw)
	  INTEGRATE_MINGW=1
	  shift
      ;;
    -nd|--no-deps)
      CHECK_DEPS=0
      shift
      ;;
    -d|--distribution-dir)
      TARGET_DIR="$2"
      shift 2
      ;;
    -t|--tools-dir)
      TOOLS_DIR="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done

if [ ! -d "${TARGET_DIR}" ]; then
	echo " Error: Target directory not set"
	fn_print_help
	exit 1
fi

if [ ! -d "${TOOLS_DIR}" ]; then
	echo " Error: Tools directory doesn't exist:${TOOLS_DIR}"
	fn_print_help
	exit 1
fi

QT_DIR="${TOOLS_DIR}/qt5-static-64"
MINGW_DIR="${TOOLS_DIR}/mingw64"
NSIS_ARCH=x64
PACKAGE_BASENAME="RedPanda.C++.${APP_VERSION}.win64"
MSYS_MINGW_DIR="/mingw64"
if [[ "${MSYSTEM}" == "MINGW32" ]]; then
	QT_DIR="${TOOLS_DIR}/qt5-static-32"
	MINGW_DIR="${TOOLS_DIR}/mingw32"
    NSIS_ARCH=x86
    PACKAGE_BASENAME="RedPanda.C++.${APP_VERSION}.win32"	  
    MSYS_MINGW_DIR="/mingw32"
fi

QMAKE="${QT_DIR}/bin/qmake.exe"
MAKE="${MINGW_DIR}/bin/mingw32-make.exe"
if [ ! -d "${QT_DIR}" ]; then
	echo " Error: QT directory doesn't exist:${QT_DIR}"
	fn_print_help
	exit 1
fi

if [ ! -f "${QMAKE}" ]; then
	echo " Error: qmake doesn't exist:${QMAKE}"
	fn_print_help
	exit 1
fi

if [ ! -d "${MINGW_DIR}" ]; then
	echo " Error: MINGW directory doesn't exist:${QT_DIR}"
	fn_print_help
	exit 1
fi

if [ ! -f "${MAKE}" ]; then
	echo " Error: make tool doesn't exist:${MAKE}"
	fn_print_help
	exit 1
fi

OLD_PATH=$PATH
BUILD_DIR="${TEMP}/redpanda-mingw-${MSYSTEM}-build"
ASTYLE_BUILD_DIR="${BUILD_DIR}/astyle"
PACKAGE_DIR="${TEMP}/redpanda-mingw-${MSYSTEM}-pkg"
NSIS="/mingw32/bin/makensis"
SOURCE_DIR="$(pwd)"
ASSETS_DIR="${SOURCE_DIR}/assets"
_7Z="/mingw64/bin/7z"
MINGW32_FOLDER="mingw32"
MINGW32_ARCHIVE="mingw32.7z"

MINGW64_FOLDER="mingw64"
MINGW64_ARCHIVE="mingw64.7z"
COMPILER_MINGW32=0
COMPILER_MINGW64=0 
CMAKE="/mingw64/bin/cmake"
CMAKE_MAKE_PROGRAM="${MAKE}"
CMAKE_CXX_COMPILER="${MINGW_DIR}/bin/g++.exe"

function fn_print_progress() {
  echo -e "\e[1;32;44m$1\e[0m"
}

## check deps

if [[ ${CHECK_DEPS} -eq 1 ]]; then
  deps=(
    mingw-w64-x86_64-{cmake,7zip,gcc}
    # always use x86 NSIS to display error message of mismatched architecture
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

if [[ ${COMPILER_MINGW32} -eq 1 && ! -d "${ASSETS_DIR}/${MINGW32_FOLDER}" ]]; then
  echo "Missing integrated MinGW folder: ${ASSETS_DIR}/${MINGW32_FOLDER}"
  exit 1
fi
if [[ ${COMPILER_MINGW64} -eq 1 && ! -d "${ASSETS_DIR}/${MINGW64_FOLDER}" ]]; then
  echo "Missing integrated MinGW folder: ${ASSETS_DIR}/${MINGW64_FOLDER}"
  exit 1
fi

GCC_VERSION=""
if [[ ${COMPILER_MINGW32} -eq 1 && ! -d "${ASSETS_DIR}/${MINGW32_FOLDER}" ]]; then
  GCC_VERSION=$( "${ASSETS_DIR}/${MINGW32_FOLDER}/bin/gcc" "-dumpversion" )
else
  GCC_VERSION=$( "${ASSETS_DIR}/${MINGW64_FOLDER}/bin/gcc" "-dumpversion")
fi

MINGW32_COMPILER_NAME="MinGW-w64 i686 GCC ${GCC_VERSION}"
MINGW32_PACKAGE_SUFFIX="MinGW32_${GCC_VERSION}"

MINGW64_COMPILER_NAME="MinGW-w64 X86_64 GCC ${GCC_VERSION}"
MINGW64_PACKAGE_SUFFIX="MinGW64_${GCC_VERSION}"
if [[ INTEGRATE_MINGW -eq 0 ]]; then
  PACKAGE_BASENAME="${PACKAGE_BASENAME}.NoCompiler"
else
  if [[ "${MSYSTEM}" == "MINGW32" ]]; then
    COMPILER_MINGW32=1
    PACKAGE_BASENAME="${PACKAGE_BASENAME}.${MINGW32_PACKAGE_SUFFIX}"
  else
    COMPILER_MINGW64=1
    PACKAGE_BASENAME="${PACKAGE_BASENAME}.${MINGW64_PACKAGE_SUFFIX}"
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
PATH="${MSYS_MINGW_DIR}/bin:${PATH}"
cd "${ASTYLE_BUILD_DIR}"
${CMAKE} . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release  -DCMAKE_EXE_LINKER_FLAGS="-static"
${MAKE} -j$(nproc)
cp AStyle/AStyle.exe "${PACKAGE_DIR}/astyle.exe"
popd

PATH="${MINGW_DIR}/bin:${OLD_PATH}"
fn_print_progress "Building..."
pushd .
cd "${BUILD_DIR}"
qmake_flags=()
[[ ${NSIS_ARCH} == x64 ]] && qmake_flags+=("X86_64=ON")
"$QMAKE" PREFIX="${PACKAGE_DIR}" ${qmake_flags[@]} -o Makefile "${SOURCE_DIR}/Red_Panda_Cpp.pro" -r
${MAKE} -j$(nproc)
${MAKE} install
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
  if [[ ! -d "mingw32" ]]; then
	cp -a --dereference "${SOURCE_DIR}/assets/${MINGW32_FOLDER}" "${PACKAGE_DIR}"
  fi 
fi
if [[ ${COMPILER_MINGW64} -eq 1 ]]; then
  nsis_flags+=(-DHAVE_MINGW64)
  if [[ ! -d "mingw64" ]]; then  
	cp -a --dereference "${SOURCE_DIR}/assets/${MINGW64_FOLDER}" "${PACKAGE_DIR}"
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