# General Development Notes

Red Panda C++ need Qt 5.15 or 6.7+ to build.

Recommended development environments:
1. Visual Studio Code.
   * Better performance.
2. Qt Creator.
   * (Almost) zero configuration.
   * Built-in UI designer.
   * Debugger integration with Qt.

To setup development environment in Visual Studio Code:
0. (Windows only) Enable Developer Mode in Windows Settings, and enable `core.symlinks` in Git (`git config core.symlinks true`).
1. Install [xmake](https://xmake.io/) and [XMake extension](https://marketplace.visualstudio.com/items?itemName=tboox.xmake-vscode).
2. Install [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) for language and debugging support.
3. Optionally install [clangd](https://clangd.llvm.org/) and [clangd extension](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) for better analysis.
4. Config workspace:
   - Compile commands: `.vscode/compile_commands.json` (“C/C++: Edit Configurations (UI)” from the Command Palette);
   - “Clangd: Arguments”: `--compile-commands-dir=.vscode`;
   - “Xmake: Additional Config Arguments”: `--qt=/usr` for example.
5. Run “XMake: UpdateIntellisense” (Command Palette) to generate compilation database.

\* Note: xmake was introduced for compilation database generation and feature matrix test. It is not fully functional yet.

# Windows

| Library + Toolchain \ Target | x86 | x64 | ARM64 |
| ---------------------------- | --- | --- | ----- |
| MSYS2 + GNU-based MinGW | ✔️ | ✔️ | ❌ |
| MSYS2 + LLVM-based MinGW | ✔️ | ✔️ | ✔️ |
| [Windows NT 5.x](https://github.com/redpanda-cpp/qtbase-xp) + [MinGW Lite](https://github.com/redpanda-cpp/mingw-lite) | ✔️ | ✔️ | ❌ |

See also [more build instructions for Windows](./docs/detailed-build-win.md).

## MSYS2 Qt Library with MinGW Toolchain (Recommended)

Red Panda C++ should work with any MinGW toolchain from MSYS2, including GCCs and Clangs in three GNU-based environments (MINGW32, MINGW64 and UCRT64), and Clangs in 64-bit LLVM-based environments (CLANG64 and CLANGARM64; see also [MSYS2’s document](https://www.msys2.org/docs/environments/)), while the following toolchains are frequently tested:
- MINGW32 GCC,
- MINGW64 GCC,
- UCRT64 GCC (recommended for x64)
- CLANGARM64 Clang (the only and recommended toolchain for ARM64).

Official distributions of Red Panda C++ are built with MINGW32 GCC and MINGW64 GCC.

Prerequisites:

0. Windows 10 x64 or later, or Windows 11 ARM64.
1. Install MSYS2.
2. In selected environment, install toolchain, Qt 5 library, and required utils. For 64-bit:
   ```bash
   pacman -S \
     $MINGW_PACKAGE_PREFIX-{cc,make,qt5-static,7zip,cmake} \
     mingw-w64-i686-nsis \
     git curl
   ```
   And for 32-bit:
   ```bash
   pacman -S \
     $MINGW_PACKAGE_PREFIX-{cc,make,qt5-static,cmake} \
     mingw-w64-i686-nsis \
     mingw-w64-x86_64-7zip \
     git curl
   ```

To build, launch selected MSYS2 environment, run:
```bash
./packages/msys/build-mingw.sh
```
to build Red Panda C++ installer and portable package with MinGW GCC toolchain or without compiler; and
```bash
./packages/msys/build-llvm.sh
```
to build Red Panda C++ installer with LLVM MinGW toolchain.

Common arguments:
- `-h`, `--help`: show help message.
- `-c`, `--clean`: clean build directory.
- `-nd`, `--no-deps`: do not check dependencies.
- `-t <dir>`, `--target-dir <dir>`: set target directory for the packages. Default: `dist/`.

Extra arguments for `build-mingw.sh`:
- `--mingw32`: add `assets/mingw32.7z` to the package.
- `--mingw64`: add `assets/mingw64.7z` to the package.
- `--mingw`: alias for `--mingw32` (x86 app) or `--mingw64` (x64 app).
- `--ucrt <build>`: add UCRT runtime from Windows SDK to the package. e.g. `--ucrt 22621` for Windows 11 SDK 22H2.

## Windows NT 5.x Qt Library with MinGW Lite Toolchain

The scripts `build-xp.sh` are alike `build-mingw.sh`, but the toolchain is provided by Qt library.

Prerequisites for native build:

0. Windows 10 x64 or later.
1. Install MSYS2.

For native build, launch MSYS2 environment, run:
```bash
./packages/msys/build-xp.sh -p 32-msvcrt
```

For cross build, run:
```bash
podman run -it --rm -v $PWD:/mnt -w /mnt docker.io/amd64/ubuntu:24.04

# in container
export MIRROR=mirrors.kernel.org  # optionally set mirror site
./packages/xmingw/build-xp.sh -p 32-msvcrt
```

These scripts accepts the same arguments as `build-mingw.sh`, plus:
- `-p|--profile <profile>`: (REQUIRED) the profile of MinGW Lite as well as Qt library. Available profiles are `64-ucrt`, `64-msvcrt`, `32-ucrt`, `32-msvcrt`.

# Linux

See also [more build instructions for freedesktop.org-conforming (XDG) desktop systems](./docs/detailed-build-xdg.md).

## Alpine Linux, Arch Linux, Debian and Its Derivatives, Fedora, openSUSE

1. Setup build environment (documentation for [Alpine](https://wiki.alpinelinux.org/wiki/Abuild_and_Helpers), [Arch](https://wiki.archlinux.org/title/Makepkg), [Debians](https://wiki.debian.org/BuildingTutorial), [RPM](https://rpm-packaging-guide.github.io/#prerequisites)).
   - For Debians:
     ```sh
     sudo apt install --no-install-recommends build-essential debhelper devscripts equivs
     ```
2. Call build script:
   - Alpine Linux: `./packages/alpine/buildapk.sh`
   - Arch Linux: `./packages/archlinux/buildpkg.sh`
   - Debians: `./packages/debian/builddeb.sh`
   - Fedora: `./packages/fedora/buildrpm.sh`
   - openSUSE: `./packages/opensuse/buildrpm.sh`
3. Install the package:
   - Alpine Linux: `~/packages/unsupported/$(uname -m)/redpanda-cpp-git-*.apk`
   - Arch Linux: `/tmp/redpanda-cpp-git/redpanda-cpp-git-*.pkg.tar.zst`
   - Debians: `/tmp/redpanda-cpp_*.deb`
   - Fedora, openSUSE: `~/rpmbuild/RPMS/$(uname -m)/redpanda-cpp-git-*.rpm`
4. Run Red Panda C++:
   ```bash
   RedPandaIDE
   ```

Note that some of these scripts check out HEAD of the repo, so any changes should be committed before building.

Alternatively, build in container (rootless Podman preferred; Docker may break file permissions):

```sh
podman run --rm -v $PWD:/mnt -w /mnt <image> ./packages/<distro>/01-in-docker.sh

# Arch Linux for example
podman run --rm -v $PWD:/mnt -w /mnt docker.io/archlinux:latest ./packages/archlinux/01-in-docker.sh
```

The package will be placed in `dist/`.

## Statically Linked Binary for Ubuntu 20.04 x86_64 (NOI Linux 2.0)

The package `redpanda-cpp-bin` is roughly “AppImage repack”. The binary is actually built in a container. Thus the build host is not necessarily Ubuntu 20.04; any Linux distribution with Podman and dpkg should work.

1. Install Podman, and dpkg if build host is not Debian or its derivatives:
   ```sh
   sudo apt install podman
   ```
   WARNING: DO NOT install packages with dpkg on non-Debians, or your system will be terminated.
2. Call build script:
   ```sh
   ./packages/debian-static/builddeb.sh
   ```

The package will be placed in `dist/`.

## Linux AppImage

```bash
podman run --rm -v $PWD:/mnt -w /mnt quay.io/redpanda-cpp/appimage-builder-x86_64:20240610.0 ./packages/appimage/01-in-docker.sh
```

Dockerfiles are available in [redpanda-cpp/appimage-builder](https://github.com/redpanda-cpp/appimage-builder). Available architectures: `x86_64`, `aarch64`, `riscv64`.

# macOS

## Qt.io Qt Library

Prerequisites:

0. macOS 10.13 or later.
1. Install Xcode Command Line Tools:
   ```zsh
   xcode-select --install
   ```
2. Install Qt with online installer from [Qt.io](https://www.qt.io/download-qt-installer-oss).
   - Select the library (in _Qt_ group, _Qt 5.15.2_ subgroup, check _macOS_).

Build:

1. Set related variables:
   ```bash
   SRC_DIR="~/redpanda-src"
   BUILD_DIR="~/redpanda-build"
   INSTALL_DIR="~/redpanda-pkg"
   ```
2. Navigate to build directory:
   ```bash
   rm -rf "$BUILD_DIR" # optional for clean build
   mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
   ```
3. Configure, build and install:
   ```bash
   ~/Qt/5.15.2/clang_64/bin/qmake PREFIX="$INSTALL_DIR" "$SRC_DIR/Red_Panda_CPP.pro"
   make -j$(sysctl -n hw.logicalcpu)
   make install
   ~/Qt/5.15.2/clang_64/bin/macdeployqt "$INSTALL_DIR/bin/RedPandaIDE.app"
   ```
