# Dependancy
 
 Red Panda C++ need Qt 5 (>=5.12) to build.

# Windows
 I build Red Panda Cpp with the latest gcc and mingw-w64, distributed by msys2 mingw-w64. Visual C++  and other version of gcc may not work.

 - Install msys2 (https://www.msys2.org)
 - Use msys2's pacman to install mingw-w64-x86_64-qt5 and mingw-w64-x86_64-gcc
 - Install qtcreator
 - Use qtcreator to open Red_Panda_CPP.pro

# Linux

 - Install gcc and qt5
 - Optionally install fcitx5-qt for building with static version of Qt
 - Open `Red_Panda_CPP.pro` with Qt Creator

qmake variables:
- `PREFIX`: default to `/usr/local`. It should be set to `/usr` or `/opt/redpanda-cpp` when packaging.
- `LIBEXECDIR`: directory for auxiliary executables, default to `$PREFIX/libexec`. Arch Linux uses `/usr/lib`.
- `XDG_ADAPTIVE_ICON=ON`: install the icon file following [freedesktop.org Icon Theme Specification](https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html) for adaptiveness to themes and sizes. Required by AppImage; recommended for Linux packaging if `PREFIX` set to `/usr`.
- `LINUX_STATIC_IME_PLUGIN=ON` (make phase): link to static ime plugin. Recommended for building with static version of Qt; **DO NOT** set for dynamic version of Qt.

## Ubuntu

### 1. Install Compiler

```bash
apt install gcc g++ make gdb gdbserver
```

### 2. Install Qt 5 and Other Dependencies

```bash
apt install qtbase5-dev qttools5-dev-tools libqt5svg5-dev git qterminal
```

### 3. Fetch Source Code

```bash
git clone https://github.com/royqh1979/RedPanda-CPP.git
```

### 4. Build

```bash
cd RedPanda-CPP/
qmake Red_Panda_CPP.pro
make -j$(nproc)
sudo make install
```

### 5. Run

```bash
RedPandaIDE
```

## Arch Linux

A reference PKGBUILD is available at `packages/archlinux`. Build RedPanda C++ with [makepkg](https://wiki.archlinux.org/title/Makepkg) and then install.

Enter `RedPandaIDE` to launch RedPanda C++.

Note that makepkg checks out HEAD of the repo, so any change should be committed before building.

## AppImage

1. Install dependency: Docker or Podman.

   Extra requirements for Windows host:
   - Docker uses WSL 2 based engine, or enable file sharing on the project folder (Settings > Resources > File sharing);
   - PowerShell (Core) or Windows PowerShell.
2. Prepare build environment. Linux host:
   ```bash
   ARCH=x86_64 # or aarch64
   DOCKER=docker # or podman
   $DOCKER build -t redpanda-builder-$ARCH packages/appimage/dockerfile-$ARCH
   ```
   Windows host:
   ```ps1
   $ARCH = "x86_64" # or "aarch64" someday Docker or Podman is available on WoA
   $DOCKER = "docker" # or "podman"
   & $DOCKER build -t redpanda-builder-$ARCH packages/appimage/dockerfile-$ARCH
   ```
3. Build AppImage. Linux host:
   ```bash
   ARCH=x86_64
   DOCKER=docker
   $DOCKER run --rm -v $PWD:/build/RedPanda-CPP -e CARCH=$ARCH redpanda-builder-$ARCH /build/RedPanda-CPP/packages/appimage/01-in-docker.sh
   ```
   Windows host:
   ```ps1
   $ARCH = "x86_64"
   $DOCKER = "docker"
   & $DOCKER run --rm -v "$(Get-Location):/build/RedPanda-CPP" -e CARCH=$ARCH redpanda-builder-$ARCH /build/RedPanda-CPP/packages/appimage/01-in-docker.sh
   ```
4. Run Red Panda C++.
   ```bash
   ./dist/RedPandaIDE-x86_64.AppImage # or *-aarch64.AppImage
   ```

Note: AppImage, in which format AppImageKit is shipped, is incompatable with QEMU user space emulator, so you cannot build AArch64 AppImage on x86-64, and vice versa.
