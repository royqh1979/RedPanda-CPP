# General Development Notes

Red Panda C++ need Qt 5.15 to build.

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

For Windows 7 or later:

| Library + Toolchain \ Target | x86 | x64 | ARM64 |
| ---------------------------- | --- | --- | ----- |
| MSYS2 + GNU-based MinGW | ✔️ | ✔️ | ❌ |
| MSYS2 + LLVM-based MinGW | ✔️ | ✔️ | ✔️ |
| Qt.io + MinGW | ✔️ | ✔️ | ❌ |
| Qt.io + MSVC | ✔️ | ✔️ | ❌ |
| vcpkg + MSVC | ✔️ | ✔️ | ❌ |

qmake variables:
- `PREFIX`: where `$MAKE install` installs files to.
- `WINDOWS_PREFER_OPENCONSOLE=ON` (make phase): prefer UTF-8 compatible `OpenConsole.exe`.
  - `OpenConsole.exe` is a part of Windows Terminal. UTF-8 input support was added in version 1.18.
  - `OpenConsole.exe` requires ConPTY, which was introduced in Windows 10 1809.

Notes for Windows on ARM:
- Red Panda C++ can be built for ARM64 ABI only on Windows 11 ARM64, while it is supposed (but not tested) to run on Windows 10 ARM64.
  - ARM64EC (“emulation compatible”) host is not supported, i.e., Red Panda C++ cannot be built with ARM64EC toolchain.
  - ARM64EC target is (theoretically) supported, i.e. Red Panda C++ will build ARM64EC binaries if upstream toolchain supports ARM64EC.
- With the [ARM32 deprecation in Windows 11 Insider Preview Build 25905](https://blogs.windows.com/windows-insider/2023/07/12/announcing-windows-11-insider-preview-build-25905/), ARM32 support will never be added.

For legacy Windows (NT 5.1 – 6.0):

| Library + Toolchain \ Target | x86 | x64 |
| ---------------------------- | --- | --- |
| Qt 5.6 from [patched source](https://github.com/redpanda-cpp/qtbase-5.6) + MinGW | ✔️ | ✔️ |

Notes for legacy Windows:
- Supported Windows versions:
  - Windows XP SP3 or later;
  - Windows Server 2003 x64 Edition (a.k.a. Windows XP x64 Edition) SP2 or later.
- Windows 7 x64 or later required as build host.
- Here is [a script](packages/windows/build-qt5.6-mingw-static.sh) for building Qt 5.6 from source alongside official Qt installation (with Qt.io MinGW GCC 8.1.0).

## MSYS2 Qt Library with MinGW Toolchain (Recommended)

Red Panda C++ should work with any MinGW toolchain from MSYS2, including GCCs and Clangs in three GNU-based environments (MINGW32, MINGW64 and UCRT64), and Clangs in three LLVM-based environments (CLANG32, CLANG64 and CLANGARM64; see also [MSYS2’s document](https://www.msys2.org/docs/environments/)), while the following toolchains are frequently tested:
- MINGW32 GCC,
- MINGW64 GCC,
- UCRT64 GCC (recommended for x64)
- CLANGARM64 Clang (the only and recommended toolchain for ARM64).

Official distributions of Red Panda C++ are built with MINGW32 GCC and MINGW64 GCC.

Prerequisites:

0. Windows 8.1 x64 or later, or Windows 11 ARM64.
1. Install MSYS2.
2. In selected environment, install toolchain and Qt 5 library:
   ```bash
   pacman -S $MINGW_PACKAGE_PREFIX-toolchain $MINGW_PACKAGE_PREFIX-qt5-static
   ```

To build:

1. In selected environment, set related variables:
   ```bash
   SRC_DIR="/c/src/redpanda-src" # “C:\src\redpanda-src” for example
   BUILD_DIR="/c/src/redpanda-build" # “C:\src\redpanda-build” for example
   INSTALL_DIR="/c/src/redpanda-pkg" # “C:\src\redpanda-pkg” for example
   ```
2. Navigate to build directory:
   ```bash
   rm -rf "$BUILD_DIR" # optional for clean build
   mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
   ```
3. Configure, build and install:
   ```bash
   $MSYSTEM_PREFIX/qt5-static/bin/qmake PREFIX="$INSTALL_DIR" "$SRC_DIR/Red_Panda_CPP.pro"
   mingw32-make -j$(nproc)
   mingw32-make install
   ```

## Qt.io Qt Library with MinGW Toolchain or MSVC Toolchain

Prerequisites:

0. Windows 7 x64 or later. ARM64 is not supported.
1. Install Qt with online installer from [Qt.io](https://www.qt.io/download-qt-installer-oss).
   - Select the library (in _Qt_ group, _Qt 5.15.2_ subgroup, check at lease one of _MinGW 8.1.0 32-bit_, _MinGW 8.1.0 64-bit_, _MSVC 2019 32-bit_ or _MSVC 2019 64-bit_).
   - For MinGW toolchain, select the toolchain (in _Qt_ group, _Developer and Designer Tools_ subgroup, check _MinGW 8.1.0 32-bit_ or _MinGW 8.1.0 64-bit_, matching the library).
   - Optionally select Qt Creator (in _Qt_ group, _Developer and Designer Tools_ subgroup; recomended for MSVC toolchain for parallel build support).
2. For MSVC toolchain, install Visual Studio 2019 or later, or Visual Studio Build Tools 2019 or later, with _Desktop Development with C++_ workload.
   - In _Installation Details_ panel, under the _Desktop Development with C++_ workload, select at least one _MSVC x86/x64 build tools_ and one _Windows SDK_.

To build:

1. Launch Qt environment from Start Menu.
2. In Qt environment, set related variables:
   ```bat
   rem no quotes even if path contains spaces
   set SRC_DIR=C:\src\redpanda-src
   set BUILD_DIR=C:\src\redpanda-build
   set INSTALL_DIR=C:\src\redpanda-pkg
   rem for MSVC toolchain
   set VS_INSTALL_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community
   rem for MSVC toolchain; or x86
   set VC_ARCH=amd64
   rem for MSVC toolchain; keep unset if Qt Creator is not installed
   set QT_CREATOR_DIR=C:\Qt\Tools\QtCreator
   ```
3. Navigate to build directory:
   ```bat
   rem optional for clean build
   rmdir /s /q "%BUILD_DIR%"
   mkdir "%BUILD_DIR%" && cd /d "%BUILD_DIR%"
   ```
4. Configure, build and install. For MinGW toolchain:
   ```bat
   qmake PREFIX="%INSTALL_DIR%" "%SRC_DIR%\Red_Panda_CPP.pro"
   mingw32-make -j%NUMBER_OF_PROCESSORS%
   mingw32-make install
   windeployqt "%INSTALL_DIR%\RedPandaIDE.exe"
   ```
   For MSVC toolchain:
   ```bat
   call "%VS_INSTALL_PATH%\Common7\Tools\VsDevCmd.bat" -arch=%VC_ARCH%
   qmake PREFIX="%INSTALL_DIR%" "%SRC_DIR%\Red_Panda_CPP.pro"

   set JOM=%QT_CREATOR_DIR%\bin\jom\jom.exe
   if "%QT_CREATOR_DIR%" neq "" (
      "%JOM%" -j%NUMBER_OF_PROCESSORS%
      "%JOM%" install
   ) else (
      nmake
      nmake install
   )
   windeployqt "%INSTALL_DIR%\RedPandaIDE.exe"
   ```

## Advanced Option: vcpkg Qt Static Library with MSVC Toolchain

Prerequisites:

0. Windows 7 x64 or later. ARM64 is not supported.
   - For a fresh installation of Windows 7, install following components in order:
     1. SHA-2 code signing support (prerequisite of .NET Framework 4.8),
     2. .NET Framework 4.8 (prerequisite of Windows Management Framework 5.1 and Visual Studio; also optional dependency of Git for Windows),
     3. Windows Management Framework 5.1 (prerequisite of vcpkg bootstrapping).
1. Install Visual Studio 2017 or later, or Visual Studio Build Tools 2017 or later, with _Desktop Development with C++_ workload.
   - In _Installation Details_ panel, under the _Desktop Development with C++_ workload, select at least one _MSVC x86/x64 build tools_ and one _Windows SDK_.
2. Install [standalone vcpkg](https://vcpkg.io/en/getting-started).
   - As of 2023.08.09, [a patch](./packages/windows/vcpkg-win7-2023.08.09.patch) is required for Windows 7 to use compatible version of Python. Affected files will change over time, so manually edit them to apply the patch.
3. Install Qt with vcpkg.
   ```ps1
   $TARGET = "x64-windows-static" # or "x86-windows-static"
   vcpkg install qt5-base:$TARGET qt5-svg:$TARGET qt5-tools:$TARGET
   ```

To build with VS 2019 or later in PowerShell (Core) or Windows PowerShell:

1. Set related variables:
   ```ps1
   $SRC_DIR = "C:\src\redpanda-src"
   $BUILD_DIR = "C:\src\redpanda-build"
   $INSTALL_DIR = "C:\src\redpanda-pkg"
   $VCPKG_ROOT = "C:\src\vcpkg"
   $VCPKG_TARGET = "x64-windows-static" # or "x86-windows-static"
   $VS_INSTALL_PATH = "C:\Program Files\Microsoft Visual Studio\2022\Community"
   $VC_ARCH = "amd64" # or "x86"
   $JOM = "$VCPKG_ROOT\downloads\tools\jom\jom-1.1.3\jom.exe" # check the version
   ```
2. Navigate to build directory:
   ```ps1
   Remove-Item -Recurse -Force "$BUILD_DIR" # optional for clean build
   (New-Item -ItemType Directory -Force "$BUILD_DIR") -and (Set-Location "$BUILD_DIR")
   ```
3. Configure, build and install:
   ```ps1
   Import-Module "$VS_INSTALL_PATH\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
   Enter-VsDevShell -VsInstallPath "$VS_INSTALL_PATH" -SkipAutomaticLocation -DevCmdArguments "-arch=$VC_ARCH"
   & "$VCPKG_ROOT\installed\$VCPKG_TARGET\tools\qt5\bin\qmake.exe" PREFIX="$INSTALL_DIR" "$SRC_DIR\Red_Panda_CPP.pro"
   & "$JOM" "-j${Env:NUMBER_OF_PROCESSORS}"
   & "$JOM" install
   ```

To build with VS 2017 or later in Command Prompt:

1. Launch proper VC environment from Start Menu.
2. Set related variables:
   ```bat
   rem no quotes even if path contains spaces
   set SRC_DIR=C:\src\redpanda-src
   set BUILD_DIR=C:\src\redpanda-build
   set INSTALL_DIR=C:\src\redpanda-pkg
   set VCPKG_ROOT=C:\src\vcpkg
   rem or x86-windows-static
   set VCPKG_TARGET=x64-windows-static
   rem check the version
   set JOM=%VCPKG_ROOT%\downloads\tools\jom\jom-1.1.3\jom.exe
   ```
3. Navigate to build directory:
   ```bat
   rem optional for clean build
   rmdir /s /q "%BUILD_DIR%"
   mkdir "%BUILD_DIR%" && cd /d "%BUILD_DIR%"
   ```
4. Configure, build and install:
   ```bat
   "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\tools\qt5\bin\qmake.exe" PREFIX="%INSTALL_DIR%" "%SRC_DIR%\Red_Panda_CPP.pro"
   "%JOM%" -j%NUMBER_OF_PROCESSORS%
   "%JOM%" install
   ```

# Linux and Other freedesktop.org-conforming (XDG) Desktop Systems

General steps:

- Install recent version of GCC (≥ 7) or Clang (≥ 6) that supports C++17.
- Install Qt 5.15 Base, SVG and Tools modules, including both libraries and development files.
- Optionally install fcitx5-qt for building with static Qt library.

qmake-based build steps:

1. Configure:
   ```bash
   qmake PREFIX=/usr/local /path/to/src/Red_Panda_CPP.pro
   ```
2. Build:
   ```bash
   make -j$(nproc)
   ```
3. Install:
   ```bash
   sudo make install
   ```

qmake variables:
- `PREFIX`: default to `/usr/local`. It should be set to `/usr` when packaging.
- `LIBEXECDIR`: directory for auxiliary executables, default to `$PREFIX/libexec`. Arch Linux uses `/usr/lib`.
- `LINUX_STATIC_IME_PLUGIN=ON` (make phase): link to static ime plugin. Recommended for building with static version of Qt; **DO NOT** set for dynamic version of Qt.

xmake-based build steps:

1. Configure:
   ```bash
   xmake f -p linux -a x86_64 -m release --qt=/usr --prefix=/usr/local
   ```
2. Build:
   ```bash
   xmake
   ```
3. Install:
   ```bash
   sudo xmake install --root -o /  # `-o ...` imitates `DESTDIR=...` in `make install`
   ```

Hint: `xmake f --help` for more options.

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
  JOBS=$(nproc) # reduce it for multiple builds at same time

  MIRROR=mirrors.kernel.org # leave empty for default mirror
  PLATFORM=linux/amd64 # or linux/386, linux/arm64/v8, linux/arm/v7, linux/riscv64
  IMAGE=debian:12 # or Ubuntu (e.g. ubuntu:22.04)

  $DOCKER run --rm -e MIRROR=$MIRROR -e SOURCE_DIR=$SOURCE_DIR -e JOBS=$JOBS -v $PWD:$SOURCE_DIR --platform $PLATFORM $IMAGE $SOURCE_DIR/packages/debian/01-in-docker.sh
  ```
* Windows host:
  ```ps1
  $DOCKER = "docker" # or "podman"
  $SOURCE_DIR = "/build/RedPanda-CPP" # source directory *in container*
  $JOBS = $Env:NUMBER_OF_PROCESSORS # reduce it for multiple builds at same time

  $MIRROR = "mirrors.kernel.org" # leave empty for default mirror
  $PLATFORM = "linux/amd64" # or "linux/386", "linux/arm64/v8", "linux/arm/v7", "linux/riscv64"
  $IMAGE = "debian:12" # or Ubuntu (e.g. "ubuntu:22.04")

  & $DOCKER run --rm -e MIRROR=$MIRROR -e SOURCE_DIR=$SOURCE_DIR -e JOBS=$JOBS -v "$(Get-Location):$SOURCE_DIR" --platform $PLATFORM $IMAGE $SOURCE_DIR/packages/debian/01-in-docker.sh
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

## Alpine Linux, Arch Linux, Fedora, openSUSE

1. Setup build environment (documentation for [Alpine](https://wiki.alpinelinux.org/wiki/Abuild_and_Helpers), [Arch](https://wiki.archlinux.org/title/Makepkg), [RPM](https://rpm-packaging-guide.github.io/#prerequisites)).
2. Call build script:
   - Alpine Linux: `./packages/alpine/buildapk.sh`
   - Arch Linux: `./packages/archlinux/buildpkg.sh`
   - Fedora: `./packages/fedora/buildrpm.sh`
   - openSUSE: `./packages/opensuse/buildrpm.sh`
3. Install the package:
   - Alpine Linux: `~/packages/unsupported/$(uname -m)/redpanda-cpp-git-*.apk`
   - Arch Linux: `/tmp/redpanda-cpp-git/redpanda-cpp-git-*.pkg.tar.zst`
   - Fedora, openSUSE: `~/rpmbuild/RPMS/$(uname -m)/redpanda-cpp-git-*.rpm`
4. Run Red Panda C++:
   ```bash
   RedPandaIDE
   ```

Note that these build scripts check out HEAD of the repo, so any changes should be committed before building.

## Linux AppImage

Linux host:
```bash
ARCH=x86_64
podman run --rm -v $PWD:/mnt -w /mnt -e CARCH=$ARCH quay.io/redpanda-cpp/appimage-builder-$ARCH:20240304.0 packages/appimage/01-in-docker.sh
```

Windows host:
```ps1
$ARCH = "x86_64"
podman run --rm -v "$(Get-Location):/mnt" -w /mnt -e CARCH=$ARCH redpanda-builder-$ARCH packages/appimage/01-in-docker.sh
```

Dockerfiles are available in [redpanda-cpp/appimage-builder](https://github.com/redpanda-cpp/appimage-builder).

## Emulated Native Build for Foreign Architectures

It is possible to build Red Panda C++ for foreign architectures using targets’ native toolchains with QEMU user space emulation.

Note: Always run emulated native build **in containers or jails**. Mixing architectures may kill your system.

For Linux or BSD host, install statically linked QEMU user space emulator (package name is likely `qemu-user-static`) and make sure that binfmt support is enabled.

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
