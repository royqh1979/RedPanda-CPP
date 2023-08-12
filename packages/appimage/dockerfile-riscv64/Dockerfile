FROM docker.io/riscv64/ubuntu:20.04

# System
RUN export DEBIAN_FRONTEND=noninteractive && \
    apt update && \
    apt upgrade -y && \
    apt install --no-install-recommends -y \
        # general utils
        ca-certificates curl xz-utils \
        # general devtools
        gcc g++ make \
        # Qt build tools
        file \
        # Qt libs
        libatspi2.0-dev libdbus-1-dev libfontconfig1-dev libfreetype6-dev libgl1-mesa-dev libxkbcommon-x11-dev \
        # fcitx-qt5 build tools
        cmake extra-cmake-modules \
        # appimage tools
        squashfs-tools && \
    apt clean && \
    rm -rf /var/lib/apt/lists/*

ARG QT_MAJOR_MINOR=5.12
ARG QT_PATCH=12
ARG QT_VERSION=${QT_MAJOR_MINOR}.${QT_PATCH}
ARG FCITX5_QT_VERSION=5.0.17
ARG ALACRITTY_VERSION=0.12.2

# AppImage runtime
RUN curl -L -o /opt/appimage-runtime 'https://github.com/cyano-linux/appimage-riscv64/releases/download/20230808.0/runtime-riscv64'

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
    cmake . -Bbuild -DCMAKE_MODULE_PATH=/usr/local/lib/cmake -DCMAKE_PREFIX_PATH=/usr/local -DCMAKE_BUILD_TYPE=Release -DENABLE_QT4=Off -DENABLE_QT5=On -DENABLE_QT6=Off -DBUILD_ONLY_PLUGIN=On -DBUILD_STATIC_PLUGIN=On && \
    cmake --build build --parallel && \
    cmake --install build && \
    # cleanup
    cd / && \
    rm -r /build/qt5

# Alacritty
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal --default-toolchain 1.71.1 && \
    . ~/.cargo/env && \
    mkdir -p /build/alacritty && \
    cd /build/alacritty && \
    curl -L -o alacritty-${ALACRITTY_VERSION}.tar.gz "https://github.com/alacritty/alacritty/archive/refs/tags/v${ALACRITTY_VERSION}.tar.gz" && \
    tar xf alacritty-${ALACRITTY_VERSION}.tar.gz && \
    cd alacritty-${ALACRITTY_VERSION} && \
    /bin/echo -e '[profile.minsize]\ninherits="release"\ndebug=false\nstrip=true\nopt-level="s"' >>Cargo.toml && \
    cargo build --profile minsize && \
    cp target/minsize/alacritty /usr/local/bin && \
    # cleanup
    cd / && \
    rm -r /build/alacritty && \
    rustup self uninstall -y
