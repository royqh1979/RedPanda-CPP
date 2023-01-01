FROM quay.io/pypa/manylinux2014_x86_64

COPY appimagetool-x86_64.AppImage /usr/local/bin/appimagetool
RUN chmod +x /usr/local/bin/appimagetool && \
    sed -i 's|^mirrorlist=|#mirrorlist=|g ; s|^#baseurl=http://mirror.centos.org/centos|baseurl=https://mirrors.ustc.edu.cn/centos|g' /etc/yum.repos.d/CentOS-*.repo && \
    sed -i 's|^metalink=|#metalink=|g ; s|^#baseurl=https\?://download.example/pub/epel/|baseurl=https://mirrors.ustc.edu.cn/epel/|g' /etc/yum.repos.d/epel.repo && \
    yum install -y fontconfig-devel freetype-devel libXrender-devel libicu-devel libxcb-devel libxkbcommon-devel patchelf
RUN mkdir -p /build/qt5 && \
    cd /build/qt5 && \
    curl -O 'https://mirrors.ustc.edu.cn/qtproject/archive/qt/5.12/5.12.12/submodules/qt{base,svg,tools}-everywhere-src-5.12.12.tar.xz' && \
    tar xf qtbase-everywhere-src-5.12.12.tar.xz && \
    cd qtbase-everywhere-src-5.12.12 && \
    ./configure \
        -prefix /opt/qt5 \
        -opensource -confirm-license \
        -optimize-size -no-shared -static -platform linux-g++ -no-use-gold-linker \
        -qt-zlib -qt-doubleconversion -qt-pcre -system-freetype -fontconfig -qt-harfbuzz -qt-libjpeg -qt-libpng -qt-xcb -qt-sqlite \
        -nomake examples -nomake tests -nomake tools && \
    make -j$(nproc) && \
    make install && \
    cd /build/qt5 && \
    tar xf qtsvg-everywhere-src-5.12.12.tar.xz && \
    cd qtsvg-everywhere-src-5.12.12 && \
    /opt/qt5/bin/qmake . && \
    make -j$(nproc) && \
    make install && \
    cd /build/qt5 && \
    tar xf qttools-everywhere-src-5.12.12.tar.xz && \
    cd qttools-everywhere-src-5.12.12 && \
    /opt/qt5/bin/qmake . && \
    make -j$(nproc) && \
    make install && \
    cd / && \
    rm -r /build/qt5
