#!/bin/bash

set -xe

# build RedPanda C++
mkdir -p /build/redpanda-build
cd /build/redpanda-build
qmake PREFIX='/usr' XDG_ADAPTIVE_ICON=ON QMAKE_RPATHDIR='/_PlaceHolder' /build/RedPanda-CPP/Red_Panda_CPP.pro
make -j$(nproc)

# install RedPanda C++ to AppDir
make install INSTALL_ROOT=/build/RedPandaIDE.AppDir

# setup AppImage resource
cd /build/RedPandaIDE.AppDir
ln -s usr/bin/RedPandaIDE AppRun
ln -s usr/share/applications/redpandaide.desktop redpandaide.desktop
ln -s usr/share/icons/hicolor/scalable/apps/redpandaide.svg redpandaide.svg
cp /build/RedPanda-CPP/platform/linux/redpandaide.png .DirIcon

# copy dependency
cp /usr/local/bin/alacritty usr/bin
mkdir -p usr/lib
cp /usr/lib64/libicu{data,i18n,uc}.so.?? usr/lib || cp /usr/lib/aarch64-linux-gnu/libicu{data,i18n,uc}.so.?? usr/lib
patchelf --set-rpath '$ORIGIN' usr/lib/*.so*
patchelf --set-rpath '$ORIGIN/../lib' usr/bin/RedPandaIDE
patchelf --set-rpath '$ORIGIN/../../lib' usr/libexec/RedPandaCPP/*

# create AppImage
cd /build
appimagetool --appimage-extract-and-run RedPandaIDE.AppDir RedPandaIDE-$CARCH.AppImage

# copy back to host
mkdir -p /build/RedPanda-CPP/dist
cp RedPandaIDE-$CARCH.AppImage /build/RedPanda-CPP/dist
