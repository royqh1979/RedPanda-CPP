#!/bin/bash

set -xe

# build RedPanda C++
mkdir -p /build/redpanda-build
cd /build/redpanda-build
qmake PREFIX='/usr' XDG_ADAPTIVE_ICON=ON /build/RedPanda-CPP/Red_Panda_CPP.pro
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

# create AppImage
cd /build
appimagetool --appimage-extract-and-run RedPandaIDE.AppDir RedPandaIDE-$CARCH.AppImage

# copy back to host
mkdir -p /build/RedPanda-CPP/dist
cp RedPandaIDE-$CARCH.AppImage /build/RedPanda-CPP/dist
