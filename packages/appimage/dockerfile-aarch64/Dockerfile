# RHEL would be the best choice, if EL 8 were released on time.
# AppImageKit AArch64 requires Ubuntu 18.04 or later.
FROM docker.io/arm64v8/ubuntu:18.04

# System
RUN sed -i 's|ports.ubuntu.com|mirrors.ustc.edu.cn|g' /etc/apt/sources.list && \
    export DEBIAN_FRONTEND=noninteractive && \
    apt update && \
    apt upgrade -y && \
    apt install --no-install-recommends -y ca-certificates cargo curl cmake file gcc g++ make \
        libatspi2.0-dev libdbus-1-dev libfontconfig1-dev libfreetype6-dev libgl1-mesa-dev libxkbcommon-x11-dev

# AppImageKit
RUN curl -L -o /usr/local/bin/appimagetool 'https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-aarch64.AppImage' && \
    chmod +x /usr/local/bin/appimagetool

# Qt 5
RUN mkdir -p /build/qt5 && \
    cd /build/qt5 && \
    curl -O 'https://mirrors.ustc.edu.cn/qtproject/archive/qt/5.12/5.12.12/submodules/qt{base,svg,tools}-everywhere-src-5.12.12.tar.xz' && \
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
    # cleanup
    cd / && \
    rm -r /build/qt5

# Alacritty
RUN mkdir -p /build/alacritty && \
    cd /build/alacritty && \
    curl -L -o alacritty-0.11.0.tar.gz 'https://github.com/alacritty/alacritty/archive/refs/tags/v0.11.0.tar.gz' && \
    tar xf alacritty-0.11.0.tar.gz && \
    cd alacritty-0.11.0 && \
    /bin/echo -e '[profile.relminsize]\ninherits="release"\ndebug=false\nstrip=true\nopt-level="s"' >>Cargo.toml && \
    cargo build --profile relminsize && \
    cp target/relminsize/alacritty /usr/local/bin && \
    # cleanup
    cd / && \
    rm -r /build/alacritty && \
    rm -r ~/.cargo/registry
