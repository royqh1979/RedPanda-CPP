#!/bin/bash

set -euxo pipefail

SRC_DIR="$PWD"

# build RedPanda C++
mkdir -p /build
cd /build
qmake PREFIX=/usr "$SRC_DIR/Red_Panda_CPP.pro"
make LINUX_STATIC_IME_PLUGIN=ON -j$(nproc)

# install RedPanda C++ to AppDir
make INSTALL_ROOT=/out install
