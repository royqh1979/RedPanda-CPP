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

## Debian and Its Derivatives

### “deb” Package for Current OS

1. Install dependency:
   ```bash
   sudo apt install \
     build-essential debhelper \
     libqt5svg5-dev qtbase5-dev qtbase5-dev-tools qttools5-dev-tools
   ```
2. Build the package:
   ```bash
   ./packages/debian/builddeb.sh
   ```
3. Install the package:
   ```bash
   sudo apt install /tmp/redpanda-cpp_*.deb
   ```
4. Run Red Panda C++:
   ```bash
   RedPandaIDE
   ```

### Build Packages for Multiple Architectures and Versions in Containers

Extra requirements for Windows host:
- Docker uses WSL 2 based engine, or enable file sharing on the project folder (Settings > Resources > File sharing);
- PowerShell (Core) or Windows PowerShell.

* Linux host:
  ```bash
  DOCKER=docker # or podman
  SOURCE_DIR=/build/RedPanda-CPP # source directory *in container*

  MIRROR=mirrors.kernel.org # leave empty for default mirror
  PLATFORM=linux/amd64 # or linux/386, linux/arm64/v8, linux/arm/v7, linux/riscv64
  IMAGE=debian:12 # or Ubuntu (e.g. ubuntu:22.04)

  $DOCKER run --rm -e MIRROR=$MIRROR -e SOURCE_DIR=$SOURCE_DIR -v $PWD:$SOURCE_DIR --platform $PLATFORM $IMAGE $SOURCE_DIR/packages/debian/01-in-docker.sh
  ```
* Windows host:
  ```ps1
  $DOCKER = "docker" # or "podman"
  $SOURCE_DIR = "/build/RedPanda-CPP" # source directory *in container*

  $MIRROR = "mirrors.kernel.org" # leave empty for default mirror
  $PLATFORM = "linux/amd64" # or "linux/386", "linux/arm64/v8", "linux/arm/v7", "linux/riscv64"
  $IMAGE = "debian:12" # or Ubuntu (e.g. "ubuntu:22.04")

  & $DOCKER run --rm -e MIRROR=$MIRROR -e SOURCE_DIR=$SOURCE_DIR -v "$(Get-Location):$SOURCE_DIR" --platform $PLATFORM $IMAGE $SOURCE_DIR/packages/debian/01-in-docker.sh
  ```

### Manual Install

1. Install compiler
   ```bash
   apt install gcc g++ make gdb gdbserver
   ```
2. Install Qt 5 and other dependencies
   ```bash
   apt install qtbase5-dev qttools5-dev-tools libqt5svg5-dev git qterminal
   ```
3. Fetch source code
   ```bash
   git clone https://github.com/royqh1979/RedPanda-CPP.git
   ```
4. Build
   ```bash
   cd RedPanda-CPP/
   qmake Red_Panda_CPP.pro
   make -j$(nproc)
   sudo make install
   ```
5. Run
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
   ARCH=x86_64 # or aarch64, riscv64
   DOCKER=docker # or podman
   $DOCKER build -t redpanda-builder-$ARCH packages/appimage/dockerfile-$ARCH
   ```
   Windows host:
   ```ps1
   $ARCH = "x86_64" # or "aarch64", "riscv64"
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
   ./dist/RedPandaIDE-x86_64.AppImage # or *-aarch64.AppImage, *-riscv64.AppImage
   ```

## Emulated Native Build for Foreign Architectures

It is possible to build Red Panda C++ for foreign architectures using targets’ native toolchains with QEMU user space emulation.

Note: Always run emulated native build **in containers**. Mixing architectures may kill your system.

For Linux host, install statically linked QEMU user space emulator (package name is likely `qemu-user-static`) and make sure that binfmt support is enabled.

For Windows host, Docker and Podman should have QEMU user space emulation enabled. If not,
* For Docker:
  ```ps1
  docker run --rm --privileged multiarch/qemu-user-static:register
  ```
* For Podman, whose virtual machine is based on Fedora WSL, simply enable binfmt support:
  ```ps1
  wsl -d podman-machine-default sudo cp /usr/lib/binfmt.d/qemu-aarch64-static.conf /proc/sys/fs/binfmt_misc/register
  wsl -d podman-machine-default sudo cp /usr/lib/binfmt.d/qemu-riscv64-static.conf /proc/sys/fs/binfmt_misc/register
  ```
