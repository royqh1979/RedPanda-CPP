# for Git Bash

set -xe

QT_CONFIGURE_DEBUG_OR_RELEASE="-release"
OFFICIAL_QT_DIRECTORY="/c/Qt"
QT_INSTALL_PREFIX="/c/Qt/5.6.3"

qt-module-directory() {
    moduleName="$1"
    echo "qt$moduleName-opensource-src-5.6.3"
}

prepare-qt-base-source() {
    moduleDirectory="$(qt-module-directory base)"
    fileName="$moduleDirectory.tar.xz"

    if ! [[ -d "$moduleDirectory" ]] ; then
        patchFileName="qtbase-5.6.3-redpanda.patch"
        if ! [[ -f "$patchFileName" ]] ; then
            echo -n "Patch file not found. Please copy it to this directory and press enter to continue..."
            read -r
        fi
        if ! [[ -f "$fileName" ]] ; then
            downloadUrl="https://download.qt.io/new_archive/qt/5.6/5.6.3/submodules/$fileName"
            curl -L -o "$fileName" "$downloadUrl"
        fi
        tar xf "$fileName"
        tar xf "$fileName" "$moduleDirectory/configure.exe"  # workaround for MSYS2 tar bug: https://github.com/msys2/MSYS2-packages/issues/4103
        pushd "$moduleDirectory"
        patch --forward --strip=1 --input="../$patchFileName"
        popd
    fi
}

prepare-qt-module-source() {
    moduleName="$1"
    moduleDirectory="$(qt-module-directory $moduleName)"
    fileName="$moduleDirectory.tar.xz"

    if ! [[ -d "$moduleDirectory" ]] ; then
        if ! [[ -f "$fileName" ]] ; then
            downloadUrl="https://download.qt.io/new_archive/qt/5.6/5.6.3/submodules/$fileName"
            curl -L -o "$fileName" "$downloadUrl"
        fi
        tar xf "$fileName"
    fi
}

prepare-qt-sources() {
    prepare-qt-base-source
    prepare-qt-module-source svg
    prepare-qt-module-source tools
}

build-qt-base() {
    configuration="$1"

    buildDir="build-qtbase-$configuration"
    mkdir -p "$buildDir"
    pushd "$buildDir"

    prefix="$QT_INSTALL_PREFIX/$configuration"
    "../$(qt-module-directory base)/configure.bat" \
        -prefix $prefix $QT_CONFIGURE_DEBUG_OR_RELEASE \
        -opensource -confirm-license \
        -no-use-gold-linker -static -static-runtime -platform win32-g++ -target xp \
        -opengl desktop -no-angle -iconv -gnu-iconv -no-icu -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -qt-freetype -no-fontconfig -qt-harfbuzz -no-ssl -no-openssl \
        -nomake examples -nomake tests -nomake tools
    mingw32-make "-j$(nproc)"
    mingw32-make install
    export PATH="$prefix/bin:$PATH"

    popd
}

build-qt-module() {
    configuration="$1"
    moduleName="$2"

    buildDir="build-qt$moduleName-$configuration"
    mkdir -p $buildDir
    pushd $buildDir

    qmake "../$(qt-module-directory $moduleName)"
    mingw32-make "-j$(nproc)"
    mingw32-make install

    popd
}

build-qt() {
    configuration="$1"

    build-qt-base $configuration
    build-qt-module $configuration svg
    build-qt-module $configuration tools
}

main() {
    basePath="$PATH"
    prepare-qt-sources

    ## 32-bit
    export PATH="$OFFICIAL_QT_DIRECTORY/Tools/mingw810_32/bin:$basePath"
    build-qt mingw81_32-redpanda

    ## 64-bit
    export PATH="$OFFICIAL_QT_DIRECTORY/Tools/mingw810_64/bin:$basePath"
    build-qt mingw81_64-redpanda
}

main
