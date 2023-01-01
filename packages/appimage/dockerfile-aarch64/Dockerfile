FROM quay.io/pypa/manylinux_2_28_aarch64

COPY appimagetool-aarch64.AppImage /usr/local/bin/appimagetool
RUN chmod +x /usr/local/bin/appimagetool && \
    sed -i 's|^mirrorlist=|#mirrorlist=|g ; s|^# baseurl=https://repo.almalinux.org/almalinux|baseurl=https://mirror.sjtu.edu.cn/almalinux|g' /etc/yum.repos.d/almalinux*.repo && \
    dnf install -y epel-release && \
    sed -i 's|^metalink=|#metalink=|g ; s|^#baseurl=https\?://download.fedoraproject.org/pub/epel/|baseurl=https://mirrors.ustc.edu.cn/epel/|g' /etc/yum.repos.d/epel*.repo && \
    dnf install -y fontconfig-devel freetype-devel libXrender-devel libicu-devel libxcb-devel libxkbcommon-devel libxkbcommon-x11-devel patchelf xcb-util-devel xcb-util-image-devel xcb-util-keysyms-devel xcb-util-renderutil-devel xcb-util-wm-devel
RUN mkdir -p /build/qt5 && \
    cd /build/qt5 && \
    curl -O 'https://mirrors.ustc.edu.cn/qtproject/official_releases/qt/5.15/5.15.7/submodules/qt{base,svg,tools}-everywhere-opensource-src-5.15.7.tar.xz' && \
    tar xf qtbase-everywhere-opensource-src-5.15.7.tar.xz && \
    cd qtbase-everywhere-src-5.15.7 && \
    ./configure \
        -prefix /opt/qt5 \
        -opensource -confirm-license \
        -optimize-size -no-shared -static -platform linux-g++ -no-use-gold-linker \
        -qt-zlib -qt-doubleconversion -qt-pcre -system-freetype -fontconfig -qt-harfbuzz -qt-libjpeg -qt-libpng -xcb -qt-sqlite \
        -nomake examples -nomake tests -nomake tools && \
    make -j$(nproc) && \
    make install && \
    cd /build/qt5 && \
    tar xf qtsvg-everywhere-opensource-src-5.15.7.tar.xz && \
    cd qtsvg-everywhere-src-5.15.7 && \
    /opt/qt5/bin/qmake . && \
    make -j$(nproc) && \
    make install && \
    cd /build/qt5 && \
    tar xf qttools-everywhere-opensource-src-5.15.7.tar.xz && \
    cd qttools-everywhere-src-5.15.7 && \
    /opt/qt5/bin/qmake . && \
    make -j$(nproc) && \
    make install && \
    cd / && \
    rm -r /build/qt5
