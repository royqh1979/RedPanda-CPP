#!/bin/bash

set -euxo pipefail

if [[ ! -v ARCH || ! -v DEB_ARCH || ! -v APPIMAGE_RUNTIME ]]; then
  echo 'This script should be run in a given container.'
  exit 1
fi

# git describe: v3.4-56-g789abcd
# sed: 3.4.r56.g789abcd
#   - remove leading 'v'
#   - prepend 'r' to the number before the '-g'
#   - replace '-' with '.'
# fallback: 0.0.r3456.g789abcd
VERSION=$(git describe --long --tags | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g') || VERSION="0.0.r$(git rev-list HEAD --count).g$(git rev-parse --short HEAD)"

SRC_DIR="$PWD"
mkdir -p "$SRC_DIR/dist"

APPIMAGE_FILE=RedPandaIDE-$VERSION-$ARCH.AppImage
RUNTIME_SIZE=$(wc -c <$APPIMAGE_RUNTIME)

if [[ -v DEB_ARCH_VARIANT ]] ; then
  DEB_FILE=redpanda-cpp-bin_${VERSION}_${DEB_ARCH_VARIANT}.deb
else
  DEB_FILE=redpanda-cpp-bin_${VERSION}_${DEB_ARCH}.deb
fi

TARBALL_FILE=redpanda-cpp-bin-$VERSION-$ARCH.tar.gz

# build astyle
(
  mkdir -p "$SRC_DIR/assets" /build/astyle
  "$SRC_DIR/packages/_common/prepare-astyle.sh" --git-dir "$SRC_DIR/assets/astyle" --work-dir /build/astyle
  cd /build/astyle
  cmake -S . -B . \
    -G "Unix Makefiles" \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    -DCMAKE_EXE_LINKER_FLAGS="-static-pie"
  make -j$(nproc)
  make DESTDIR=/pkg/astyle install/strip
)

# build
(
  mkdir -p /build/redpanda
  cd /build/redpanda
  cmake -S "$SRC_DIR" -B . \
    -G "Unix Makefiles" \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    -DCMAKE_EXE_LINKER_FLAGS="-static-pie" \
    -DOVERRIDE_MALLOC="mimalloc"
  make -j$(nproc)
)

# test
(
  cd /build/redpanda
  QT_QPA_PLATFORM=offscreen make test
)

# package tarball
(
  (
    cd /build/redpanda
    make DESTDIR=/pkg/tarball install/strip
  )

  mkdir -p /pkg/tarball/usr/libexec/RedPandaCPP
  cp /pkg/astyle/usr/bin/astyle /pkg/tarball/usr/libexec/RedPandaCPP/astyle

  tar -C /pkg/tarball -czf "$SRC_DIR/dist/$TARBALL_FILE" .
)

# package deb
(
  (
    cd /build/redpanda
    make DESTDIR=/pkg/deb install/strip
  )

  mkdir -p /pkg/deb/usr/libexec/RedPandaCPP
  cp /pkg/astyle/usr/bin/astyle /pkg/deb/usr/libexec/RedPandaCPP/astyle

  mkdir -p /pkg/deb/DEBIAN

  if [[ -v DEB_ARCH_VARIANT ]] ; then
    sed \
      -e "s/__VERSION__/$VERSION/" \
      -e "s/__ARCH__/$DEB_ARCH/" \
      -e "s/__ARCH_VARIANT__/$DEB_ARCH_VARIANT/" \
      packages/linux/control.in >/pkg/deb/DEBIAN/control
  else
    sed \
      -e "s/__VERSION__/$VERSION/" \
      -e "s/__ARCH__/$DEB_ARCH/" \
      -e "/__ARCH_VARIANT__/d" \
      packages/linux/control.in >/pkg/deb/DEBIAN/control
  fi
  dpkg-deb --build /pkg/deb "$SRC_DIR/dist/$DEB_FILE"
)

# package AppImage
(
  (
    cd /build/redpanda
    make DESTDIR=/pkg/appimage install/strip
  )

  mkdir -p /pkg/appimage/usr/libexec/RedPandaCPP
  cp /pkg/astyle/usr/bin/astyle /pkg/appimage/usr/libexec/RedPandaCPP/astyle

  ln -s usr/share/applications/RedPandaIDE.desktop /pkg/appimage/RedPandaIDE.desktop
  ln -s usr/share/icons/hicolor/scalable/apps/redpandaide.svg /pkg/appimage/redpandaide.svg

  # the following files may come from Windows filesystem, use `install` to preseve file permission

  # AppImage runtime set `argv[0]` to AppImage file, which is not reliable.
  # Qt framework expects reliable `argv[0]` to locate configuration files.
  # The wrapper fixes `argv[0]`.
  install -m755 "$SRC_DIR/packages/linux/appimage-wrapper.sh" /pkg/appimage/AppRun
  install -m644 "$SRC_DIR/platform/linux/redpandaide.png" /pkg/appimage/.DirIcon

  # AppImage is appimage-runtime + squashfs
  cd /pkg
  mksquashfs /pkg/appimage $APPIMAGE_FILE -offset $RUNTIME_SIZE -comp zstd -root-owned -noappend -b 1M -mkfs-time 0
  dd if=$APPIMAGE_RUNTIME of=$APPIMAGE_FILE conv=notrunc
  chmod +x $APPIMAGE_FILE
  cp $APPIMAGE_FILE "$SRC_DIR/dist"
)
