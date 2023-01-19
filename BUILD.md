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
 - Open `Red_Panda_CPP.pro` with Qt Creator

qmake variables:
- `PREFIX`: default to `/usr/local`. It should be set to `/usr` or `/opt/redpanda-cpp` when packaging.
- `LIBEXECDIR`: directory for auxiliary executables, default to `$PREFIX/libexec`. Arch Linux uses `/usr/lib`.
- `XDG_ADAPTIVE_ICON=ON`: install the icon file following [freedesktop.org Icon Theme Specification](https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html) for adaptiveness to themes and sizes. Required by AppImage; recommended for Linux packaging if `PREFIX` set to `/usr`.

## Ubuntu

### 1. Install Compiler

```bash
apt install gcc g++ make gdb gdbserver
```

### 2. Install Qt 5 and Other Dependencies

```bash
apt install qtbase5-dev qttools5-dev-tools libicu-dev libqt5svg5-dev git qterminal
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

1. Install dependency: cURL, Docker.

   Extra requirements for Windows host:
   - Docker uses WSL 2 based engine, or enable file sharing on the project folder (Settings > Resources > File sharing);
   - PowerShell (previously “PowerShell Core”, not “Windows PowerShell”).
2. Prepare build environment. Linux host:
   ```bash
   arch=x86_64 # or aarch64
   curl -L -o packages/appimage/dockerfile-$arch/appimagetool-$arch.AppImage https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-$arch.AppImage
   docker build -t redpanda-builder-$arch packages/appimage/dockerfile-$arch
   ```
   Windows host:
   ```ps1
   $arch = "x86_64" # or "aarch64" someday Docker is available on WoA
   Invoke-WebRequest -OutFile packages/appimage/dockerfile-$arch/appimagetool-$arch.AppImage -Uri https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-$arch.AppImage
   docker build -t redpanda-builder-$arch packages/appimage/dockerfile-$arch
   ```
3. Build AppImage. Linux host:
   ```bash
   ./packages/appimage/build-x86_64.sh # or *-aarch64.sh
   ```
   Windows host:
   ```ps1
   ./packages/appimage/build-x86_64.ps1 # or *-aarch64.ps1 someday Docker is available on WoA
   ```
4. Run Red Panda C++.
   ```bash
   ./dist/RedPandaIDE-x86_64.AppImage # or *-aarch64.AppImage
   ```

Note: AppImage, in which format AppImageKit is shipped, is incompatable with QEMU user space emulator, so you cannot build AArch64 AppImage on x86-64, and vice versa.
