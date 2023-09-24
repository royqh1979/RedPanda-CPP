#!/bin/bash

set -xe

VERSION=$(sed -nr -e '/APP_VERSION\s*=/ s/APP_VERSION\s*=\s*(([0-9]+\.)*[0-9]+)\s*/\1/p' /build/RedPanda-CPP/Red_Panda_CPP.pro)
APPIMAGE_FILE=RedPandaIDE-$VERSION-$CARCH.AppImage
RUNTIME_FILE=/opt/appimage-runtime
RUNTIME_SIZE=$(wc -c <$RUNTIME_FILE)

# build RedPanda C++
mkdir -p /build/redpanda-build
cd /build/redpanda-build
qmake PREFIX='/usr' XDG_ADAPTIVE_ICON=ON /build/RedPanda-CPP/Red_Panda_CPP.pro
make LINUX_STATIC_IME_PLUGIN=ON -j$(nproc)

# install RedPanda C++ to AppDir
make install INSTALL_ROOT=/build/RedPandaIDE.AppDir

# setup AppImage resource
cd /build/RedPandaIDE.AppDir
ln -s usr/share/applications/redpandaide.desktop redpandaide.desktop
ln -s usr/share/icons/hicolor/scalable/apps/redpandaide.svg redpandaide.svg
# following files may come from Windows filesystem, use `install` to preseve file permission
install -m755 /build/RedPanda-CPP/packages/appimage/AppRun.sh AppRun
install -m644 /build/RedPanda-CPP/platform/linux/redpandaide.png .DirIcon

# create AppImage
cd /build
mksquashfs RedPandaIDE.AppDir $APPIMAGE_FILE -offset $RUNTIME_SIZE -comp zstd -root-owned -noappend -b 1M -mkfs-time 0
dd if=$RUNTIME_FILE of=$APPIMAGE_FILE conv=notrunc
chmod +x $APPIMAGE_FILE

# copy back to host
mkdir -p /build/RedPanda-CPP/dist
cp $APPIMAGE_FILE /build/RedPanda-CPP/dist
