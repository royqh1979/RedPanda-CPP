# RHEL devtoolset, which provides new version of GCC targetting old libgcc_s and libstdc++, is the key to compatibility.
# EL 7 is modern enough for apps and libs.
# CentOS 7 aarch64 container image is a mess. Use Oracle instead.
FROM docker.io/arm64v8/oraclelinux:7

# System
RUN yum-config-manager --enable ol7_optional_latest && \
    yum install -y oracle-softwarecollection-release-el7 oracle-epel-release-el7 && \
    yum upgrade -y && \
    yum install -y \
        # general devtools
        devtoolset-7-gcc devtoolset-7-gcc-c++ make \
        # squashfs-tools libs
        libzstd-devel \
        # Qt build tools
        file which \
        # Qt libs
        at-spi2-core-devel dbus-devel fontconfig-devel freetype-devel glib2-devel libXrender-devel libxcb-devel libxkbcommon-devel libxkbcommon-x11-devel mesa-libGL-devel xcb-util-devel \
        # fcitx5-qt build tools
        cmake3 extra-cmake-modules && \
    yum clean all

ARG DEVTOOLSET_ROOTPATH=/opt/rh/devtoolset-7/root
ENV PATH=${DEVTOOLSET_ROOTPATH}/usr/bin:${PATH}
ENV LD_LIBRARY_PATH=${DEVTOOLSET_ROOTPATH}/usr/lib64

ARG SQUASHFS_TOOLS_VERSION=4.4
ARG QT_MAJOR_MINOR=5.12
ARG QT_PATCH=12
ARG QT_VERSION=${QT_MAJOR_MINOR}.${QT_PATCH}
ARG FCITX5_QT_VERSION=5.0.17
ARG ALACRITTY_VERSION=0.12.2

# AppImage runtime
RUN curl -L -o /opt/appimage-runtime 'https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-aarch64'

# squashfs-tools
RUN mkdir -p /build/squashfs-tools && \
    cd /build/squashfs-tools && \
    curl -L -o squashfs-tools-${SQUASHFS_TOOLS_VERSION}.tar.gz "https://github.com/plougher/squashfs-tools/archive/refs/tags/${SQUASHFS_TOOLS_VERSION}.tar.gz" && \
    tar xf squashfs-tools-${SQUASHFS_TOOLS_VERSION}.tar.gz && \
    cd squashfs-tools-${SQUASHFS_TOOLS_VERSION}/squashfs-tools && \
    make ZSTD_SUPPORT=1 -j$(nproc) && \
    make install && \
    # cleanup
    cd / && \
    rm -r /build/squashfs-tools

# Qt 5
RUN mkdir -p /build/qt5 && \
    cd /build/qt5 && \
    curl -O "https://download.qt.io/archive/qt/${QT_MAJOR_MINOR}/${QT_VERSION}/submodules/qt{base,svg,tools}-everywhere-src-${QT_VERSION}.tar.xz" && \
    curl -L -o fcitx5-qt-${FCITX5_QT_VERSION}.tar.gz "https://github.com/fcitx/fcitx5-qt/archive/refs/tags/${FCITX5_QT_VERSION}.tar.gz" && \
    tar xf qtbase-everywhere-src-${QT_VERSION}.tar.xz && \
    cd qtbase-everywhere-src-${QT_VERSION} && \
    ./configure \
        -prefix /usr/local \
        -opensource -confirm-license \
        -optimize-size -no-shared -static -platform linux-g++ -no-use-gold-linker \
        -qt-zlib -qt-doubleconversion -iconv -no-icu -qt-pcre -no-openssl -system-freetype -fontconfig -qt-harfbuzz -qt-libjpeg -qt-libpng -qt-xcb -qt-sqlite \
        -nomake examples -nomake tests -nomake tools && \
    make -j$(nproc) && \
    make install && \
    # svg package
    cd /build/qt5 && \
    tar xf qtsvg-everywhere-src-${QT_VERSION}.tar.xz && \
    cd qtsvg-everywhere-src-${QT_VERSION} && \
    qmake . && \
    make -j$(nproc) && \
    make install && \
    # tools package
    cd /build/qt5 && \
    tar xf qttools-everywhere-src-${QT_VERSION}.tar.xz && \
    cd qttools-everywhere-src-${QT_VERSION} && \
    qmake . && \
    make -j$(nproc) && \
    make install && \
    # fcitx5 package
    cd /build/qt5 && \
    tar xf fcitx5-qt-${FCITX5_QT_VERSION}.tar.gz && \
    cd fcitx5-qt-${FCITX5_QT_VERSION} && \
    cmake3 . -Bbuild -DCMAKE_MODULE_PATH=/usr/local/lib/cmake -DCMAKE_PREFIX_PATH=/usr/local -DCMAKE_BUILD_TYPE=Release -DENABLE_QT4=Off -DENABLE_QT5=On -DENABLE_QT6=Off -DBUILD_ONLY_PLUGIN=On -DBUILD_STATIC_PLUGIN=On && \
    cmake3 --build build --parallel && \
    # cmake 3.14 is too old to `--install`
    cmake3 --build build -- install && \
    # cleanup
    cd / && \
    rm -r /build/qt5
