# RHEL devtoolset, which provides new version of GCC targetting old libgcc_s and libstdc++, is the key to compatibility.
# AppImageKit x86-64 requires EL 7 or later.
FROM docker.io/amd64/centos:7

# System
RUN sed -i 's|^mirrorlist=|#mirrorlist=|g; s|^#baseurl=http://mirror.centos.org/centos|baseurl=https://mirrors.ustc.edu.cn/centos|g' /etc/yum.repos.d/CentOS-Base.repo && \
    yum install -y centos-release-scl-rh epel-release && \
    sed -i 's|^mirrorlist=|#mirrorlist=|g; s|^#baseurl=http://mirror.centos.org/centos|baseurl=https://mirrors.ustc.edu.cn/centos|g' /etc/yum.repos.d/CentOS-SCLo-scl-rh.repo && \
    sed -i 's|^metalink=|#metalink=|g; s|^#baseurl=https\?://download.fedoraproject.org/pub/epel/|baseurl=https://mirrors.ustc.edu.cn/epel/|g' /etc/yum.repos.d/epel.repo && \
    yum upgrade -y && \
    yum install -y cargo cmake3 extra-cmake-modules file make which \
        at-spi2-core-devel dbus-devel fontconfig-devel freetype-devel glib2-devel libXrender-devel libxcb-devel libxkbcommon-devel libxkbcommon-x11-devel mesa-libGL-devel xcb-util-devel \
        devtoolset-7-gcc devtoolset-7-gcc-c++

ARG DEVTOOLSET_ROOTPATH=/opt/rh/devtoolset-7/root
ENV PATH=${DEVTOOLSET_ROOTPATH}/usr/bin:${PATH}
ENV LD_LIBRARY_PATH=${DEVTOOLSET_ROOTPATH}/usr/lib64

# AppImageKit
RUN curl -L -o /usr/local/bin/appimagetool 'https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-x86_64.AppImage' && \
    chmod +x /usr/local/bin/appimagetool

# Qt 5
RUN mkdir -p /build/qt5 && \
    cd /build/qt5 && \
    curl -O 'https://download.qt.io/archive/qt/5.12/5.12.12/submodules/qt{base,svg,tools}-everywhere-src-5.12.12.tar.xz' && \
    curl -L -o fcitx5-qt-5.0.17.tar.gz 'https://github.com/fcitx/fcitx5-qt/archive/refs/tags/5.0.17.tar.gz' && \
    tar xf qtbase-everywhere-src-5.12.12.tar.xz && \
    cd qtbase-everywhere-src-5.12.12 && \
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
    tar xf qtsvg-everywhere-src-5.12.12.tar.xz && \
    cd qtsvg-everywhere-src-5.12.12 && \
    qmake . && \
    make -j$(nproc) && \
    make install && \
    # tools package
    cd /build/qt5 && \
    tar xf qttools-everywhere-src-5.12.12.tar.xz && \
    cd qttools-everywhere-src-5.12.12 && \
    qmake . && \
    make -j$(nproc) && \
    make install && \
    # fcitx5 package
    cd /build/qt5 && \
    tar xf fcitx5-qt-5.0.17.tar.gz && \
    cd fcitx5-qt-5.0.17 && \
    cmake3 . -Bbuild -DCMAKE_MODULE_PATH=/usr/local/lib/cmake -DCMAKE_PREFIX_PATH=/usr/local -DCMAKE_BUILD_TYPE=Release -DENABLE_QT4=Off -DENABLE_QT5=On -DENABLE_QT6=Off -DBUILD_ONLY_PLUGIN=On -DBUILD_STATIC_PLUGIN=On && \
    cmake3 --build build --parallel && \
    cmake3 --install build && \
    # cleanup
    cd / && \
    rm -r /build/qt5

# Alacritty
RUN mkdir -p /build/alacritty && \
    cd /build/alacritty && \
    curl -L -o alacritty-0.11.0.tar.gz 'https://github.com/alacritty/alacritty/archive/refs/tags/v0.11.0.tar.gz' && \
    tar xf alacritty-0.11.0.tar.gz && \
    cd alacritty-0.11.0 && \
    echo -e '[profile.relminsize]\ninherits="release"\ndebug=false\nstrip=true\nopt-level="s"' >>Cargo.toml && \
    cargo build --profile relminsize && \
    cp target/relminsize/alacritty /usr/local/bin && \
    # cleanup
    cd / && \
    rm -r /build/alacritty && \
    rm -r ~/.cargo/registry
