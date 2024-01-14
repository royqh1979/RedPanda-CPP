# for Git Bash

# Usage:
#   ./build-qt5.6-mingw-static.sh --arch <32|64> [--debug] [--official-qt-dir <path>]

set -euo pipefail

_ARCH=""
_CLEAN=0
_DEBUG=0
_OFFICAL_QT_DIR="/c/Qt"
while [[ $# -gt 0 ]] ; do
    case "$1" in
        --arch)
            _ARCH="$2"
            shift
            shift
            ;;
        --clean)
            _CLEAN=1
            shift
            ;;
        --debug)
            _DEBUG=1
            shift
            ;;
        --official-qt-dir)
            _OFFICIAL_QT_DIR="$2"
            shift
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

case "$_ARCH" in
    32|64)
        _MINGW_TOOLCHAIN="$_OFFICAL_QT_DIR/Tools/mingw810_$_ARCH"
        _QT_INSTALL_PREFIX="$_OFFICAL_QT_DIR/5.6.4/mingw81_$_ARCH-redpanda"
        ;;
    *)
        echo "Please specify --arch 32 or --arch 64"
        exit 1
        ;;
esac

if [[ $_DEBUG -eq 1 ]] ; then
    _QT_CONFIGURE_DEBUG_OR_RELEASE=-debug-and-release
else
    _QT_CONFIGURE_DEBUG_OR_RELEASE=-release
fi

clean() {
    rm -rf build-qt{base,svg,tools}-"$_ARCH" || true
    rm -rf "$_QT_INSTALL_PREFIX" || true
}

check-toolchain() {
    if ! [[ -x "$_MINGW_TOOLCHAIN/bin/g++.exe" ]] ; then
        echo "Please install MinGW 8.1 from Qt Maintenance Tool or download from https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/tools_mingw/ or https://mirrors.ustc.edu.cn/qtproject/online/qtsdkrepository/windows_x86/desktop/tools_mingw/ and extract to $_OFFICAL_QT_DIR."
        exit 1
    fi
}

check-qt-sources() {
    while ! [[ -d qtbase ]] ; do
        echo "Please clone or link qtbase into this directory. e.g."
        echo "    git clone https://github.com/redpanda-cpp/qtbase-5.6.git --branch=5.6-redpanda --depth=1 qtbase"
        echo "    MSYS=winsymlinks:nativestrict ln -s /path/to/qtbase-5.6 qtbase"
        echo "Press enter to continue..."
        read
    done
    while ! [[ -d qtsvg ]] ; do
        echo "Please clone or link qtsvg into this directory. e.g."
        echo "    git clone https://github.com/qt/qtsvg.git --branch=5.6 --depth=1"
        echo "Press enter to continue..."
        read
    done
    while ! [[ -d qttools ]] ; do
        echo "Please clone or link qttools into this directory. e.g."
        echo "    git clone https://github.com/qt/qttools.git --branch=5.6 --depth=1"
        echo "Press enter to continue..."
        read
    done
}

build-qt-base() {
    local build_dir="build-qtbase-$_ARCH"
    mkdir -p "$build_dir"
    pushd "$build_dir"
    {
        "../qtbase/configure.bat" \
            -prefix "$_QT_INSTALL_PREFIX" "$_QT_CONFIGURE_DEBUG_OR_RELEASE" \
            -opensource -confirm-license \
            -no-use-gold-linker -static -static-runtime -platform win32-g++ -target xp \
            -opengl desktop -no-angle -iconv -gnu-iconv -no-icu -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -qt-freetype -no-fontconfig -qt-harfbuzz -no-ssl -no-openssl \
            -nomake examples -nomake tests -nomake tools
        mingw32-make "-j$(nproc)"
        mingw32-make install
    }
    popd
}

build-qt-module() {
    local module_name="$1"
    local build_dir="build-qt$module_name-$_ARCH"
    mkdir -p "$build_dir"
    pushd "$build_dir"
    {
        qmake "../qt$module_name"
        mingw32-make "-j$(nproc)"
        mingw32-make install
    }
    popd
}

export PATH="$_QT_INSTALL_PREFIX/bin:$_MINGW_TOOLCHAIN/bin:$PATH"

[[ $_CLEAN -eq 1 ]] && clean
check-toolchain
check-qt-sources
build-qt-base
build-qt-module svg
build-qt-module tools
