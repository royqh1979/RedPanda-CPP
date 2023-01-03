#!/bin/bash

set -xe

# build RedPanda C++
mkdir -p /build/redpanda-build
cd /build/redpanda-build
/opt/qt5/bin/qmake PREFIX='/usr' QMAKE_RPATHDIR='/_PlaceHolder' /build/RedPanda-CPP/Red_Panda_CPP.pro
make -j$(nproc)

# install RedPanda C++ to AppDir
make install INSTALL_ROOT=/build/RedPandaIDE.AppDir

# setup AppImage resource
cd /build/RedPandaIDE.AppDir
ln -s usr/bin/RedPandaIDE AppRun
ln -s usr/share/applications/redpandaide.desktop redpandaide.desktop
ln -s usr/share/pixmaps/redpandaide.png redpandaide.png
ln -s usr/share/pixmaps/redpandaide.png .DirIcon

# copy dependency
mkdir -p usr/lib
cp /usr/lib64/libicu{data,i18n,uc}.so.?? usr/lib
patchelf --set-rpath '$ORIGIN' usr/lib/*.so*
patchelf --set-rpath '$ORIGIN/../lib' usr/bin/RedPandaIDE
patchelf --set-rpath '$ORIGIN/../../lib' usr/libexec/RedPandaCPP/*

# create AppImage
cd /build
appimagetool --appimage-extract-and-run RedPandaIDE.AppDir RedPandaIDE-$CARCH.AppImage

# copy back to host
mkdir -p /build/RedPanda-CPP/dist
/bin/cp RedPandaIDE-$CARCH.AppImage /build/RedPanda-CPP/dist
