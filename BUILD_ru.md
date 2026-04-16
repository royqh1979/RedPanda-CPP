# Сборка Red Panda C++

There are several options to build Red Panda C++: package manager recipes, standalone application scripts, or manual build commands.

## Package Manager Recipes

> Note: Qt 6.8 or later required. Rolling package repository recommended.

Red Panda C++ built as a package integrates with the package manager ecosystem. It utilizes the toolchain and libraries from the package manager.

Red Panda C++ provides many examples of package manager recipes, including Homebrew, Linux system package managers, and MSYS2.

The steps are similar for each package manager:
1. Setup build environment;
2. Prepare recipe and source;
3. Build package;
4. Install package.

Detailed instructions for **step 1** can be found in the package manager documentation: [Alpine](https://wiki.alpinelinux.org/wiki/Abuild_and_Helpers), [Arch](https://wiki.archlinux.org/title/Makepkg), [Debian family](https://wiki.debian.org/BuildingTutorial), [MSYS2](https://www.msys2.org/dev/new-package/), [RPM](https://rpm-packaging-guide.github.io/#prerequisites).

**Step 2 and 3** can be done with the following scripts:
- Alpine Linux: `./packages/alpine/buildapk.sh`
- Arch Linux: `./packages/archlinux/buildpkg.sh`
- Debian family: `./packages/debian/builddeb.sh`
- Fedora: `./packages/fedora/buildrpm.sh`
- Homebrew: `./packages/brew/buildbottle.sh`
- MSYS2: `./packages/msys/buildpkg.sh`
- openSUSE: `./packages/opensuse/buildrpm.sh`

> Note: These scripts check out HEAD of the repo, so any changes should be committed before building.

Alternatively, build in container for Linux system package manager (rootless Podman preferred; Docker may break file permissions):

```sh
podman run --rm -v $PWD:/mnt -w /mnt <image> ./packages/<distro>/01-in-docker.sh

# Arch Linux for example
podman run --rm -v $PWD:/mnt -w /mnt docker.io/library/archlinux:latest ./packages/archlinux/01-in-docker.sh
```

Then the package for **step 4** will be placed at `dist/`. Use the package manager to install it.

## Standalone Application Scripts

Red Panda C++ built as a standalone application is easy to deploy.

| OS | Toolchain | Qt | Package format | Architectures |
| -- | --------- | -- | -------------- | ------------- |
| Windows XP+ | [MinGW Lite](https://github.com/redpanda-cpp/mingw-lite) | [5.15 with thunks](https://github.com/redpanda-cpp/qtbase-xp) | installer, portable | x86_64, i686 |
| Windows 7+ | MSVC | 5.15, Qt.io | portable | x64, x86 |
| Windows 10 1809+ | MSVC | 6.8+, Qt.io | portable | x64, ARM64 |
| Windows 10 1607+ | MSVC | current 6.x, vcpkg | portable | x64, ARM64, x86 |
| Linux since 2010 | [cross, static, musl libc](https://github.com/redpanda-cpp/appimage-builder) | 6.10 | AppImage, Debian, tarball | x86_64, x86_64.v3, aarch64, riscv64, loong64, i686 |
| recent macOS | Xcode | 6.11+, Qt.io | bundle | universal |

> Warning: Don’t build Red Panda C++ with MinGW-w64 shared Qt. When running as portable app, the default working directory is Red Panda C++’s directory. User programs’s dependencies, which should be loaded from toolchain’s directory, will be overridden by Red Panda C++’s dlls.

### Windows XP

Prerequisites:

0. Windows 10 x64 or later.
1. Install MSYS2.

To build, launch MSYS2 environment, run:
```bash
./packages/mingw/build-xp.sh -p 64-ucrt
```

Available profiles: 64-ucrt, 32-ucrt, 64-msvcrt (deprecated), 32-msvcrt (deprecated).

Arguments:
- `-h`, `--help`: show help message.
- `-c`, `--clean`: clean build directory.
- `--mingw32`: add `assets/mingw32.7z` to the package.
- `--mingw64`: add `assets/mingw64.7z` to the package.
- `--mingw`: alias for `--mingw32` (x86 app) or `--mingw64` (x64 app).
- `-t <dir>`, `--target-dir <dir>`: set target directory for the packages. Default: `dist/`.
- `--ucrt`: include UCRT installer (VC_redist) in the package.

> Note: Windows Server 2003 x64 Edition is deprecated (the installer does not install UCRT for Windows Server 2003 x64 Edition; manual installation should work).

### MSVC Toolchain

Prerequisites:

0. Windows 10 x64 or later, or Windows 11 ARM64.
1. Install Visual Studio 2022 or 2026 (at least “build tools” and “Windows SDK”).
2. Install Qt 6.8+ or 5.15 via Qt.io installer, aqtinstall, or vcpkg.
   - Required components: base, svg, tools, translations.
3. PowerShell (Core, not “Windows PowerShell”).

To build:

1. [Start Visual Studio develop environment](https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell?view=visualstudio).
2. For vcpkg, set toolchain file (required) and target triplet (if you want non-default one):
   ```ps1
   $env:CMAKE_TOOLCHAIN_FILE = "$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
   $env:VCPKG_TARGET_TRIPLET = "x64-windows"
   ```
3. Run build script:
   ```ps1
   ./packages/msvc/build.ps1 -QtDir C:/Qt/6.8.3/msvc2022_64

   # vcpkg (-QtDir is optional; it enables windeployqt)
   ./packages/msvc/build.ps1 -QtDir "$env:VCPKG_ROOT/installed/$env:VCPKG_TARGET_TRIPLET/tools/Qt6"
   ```

Arguments:
- `-c|-Clean`: clean directories before building.
- `-QtDir <dir>`: Qt library directory.

### Linux Static

```bash
podman run --rm -v $PWD:/mnt -w /mnt ghcr.io/redpanda-cpp/appimage-builder-x86_64:20260127.0 ./packages/linux/01-in-docker.sh
```

The packages will be placed in `dist/`.

### macOS

Prerequisites:
0. Recent macOS that satisfies requirements of [Qt 5](https://doc.qt.io/qt-5/macos.html) or [Qt 6](https://doc.qt.io/qt-6/macos.html).
1. Install Xcode Command Line Tools:
   ```zsh
   xcode-select --install
   ```
2. Install Qt 6.11+ via Qt.io installer, or aqtinstall.

To build:
```zsh
./packages/macos/build.sh --qt-dir ~/Qt/6.11.0/macos
```

Arguments:
- `-c`, `--clean`: clean directories before building.
- `--qt-dir <dir>`: Qt library directory.

## Manual Build Commands

Building Red Panda C++ manually is a good idea for learning, developing, and debugging purposes.

Prerequisites:
- Qt 6.8+ or 5.15.
- CMake or Xmake.

> Note: To enable `lupdate` function in Qt Creator, add an external tool in “Edit > Preferences > Environment > External Tools”:
> - Executable: `cmake`;
> - Arguments: `--build . --target update_translations`;
> - Working directory: choose global variable `ActiveProject:BuildConfig:Path`.

### Build with CMake

1. Настройка:
   ```bash
   cmake -S . -B build \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_INSTALL_PREFIX=/usr/local
   ```
2. Сборка:
   ```bash
   cmake --build build -- --parallel
   ```
3. Установка:
   ```bash
   sudo cmake --install build --strip
   ```

Переменные CMake:
- `CMAKE_INSTALL_PREFIX`: where to install.
  - На саму среду Red Panda C++ `CMAKE_INSTALL_PREFIX` не влияет, поскольку она использует относительные пути.
  - На файл `.desktop` влияет `CMAKE_INSTALL_PREFIX`.
- `CMAKE_INSTALL_LIBEXECDIR`: каталог для вспомогательных исполнимых файлов, УКАЗЫВАЕТСЯ ОТНОСИТЕЛЬНО `CMAKE_INSTALL_PREFIX`.
  - Applies to: hierarchy filesystem layout.
  - Arch Linux and MSYS2 use `lib`.
- `FILESYSTEM_LAYOUT`: `hierarchy`, `flat`, or `bundle`.
  - `hierarchy` applies to: all.
    ```
    prefix/
    ├─ bin/
    │  └─ RedPandaIDE
    ├─ libexec/
    │  └─ RedPandaCPP/
    │     ├─ astyle
    │     └─ consolepauser
    └─ share/
       └─ RedPandaCPP/
          └─ templates/
    ```
  - `flat` applies to: Windows.
    ```
    prefix/
    ├─ RedPandaIDE.exe
    ├─ astyle.exe
    ├─ consolepauser.exe
    └─ templates/
    ```
  - `bundle` applies to: macOS.
    ```
    prefix/
    └─ RedPandaIDE.app
       └─ Contents/
          ├─ Frameworks/
          ├─ MacOS/
          │  ├─ RedPandaIDE
          │  ├─ astyle
          │  └─ consolepauser
          └─ Resources/
             └─ templates/
    ```
  - Windows default: `flat`.
  - macOS default: `bundle`.
  - Other platforms force `hierarchy`.
- `FORCE_QT5`: force to use Qt 5.
  - Useful when Qt 5 and Qt 6 are installed to same prefix.
- `LUA_ADDON`: enable Lua add-ons.
- `OVERRIDE_MALLOC`: link specific memory allocation library.
  - e.g. `-DOVERRIDE_MALLOC=mimalloc`.
- `PORTABLE_CONFIG`: `oui`, `non`, or `registry`.
  - Applies to: Windows.
  - `oui`: yes, write config files to Red Panda C++’s directory.
  - `non`: no, write config files to user config directory.
  - `registry`: yes if current instance matches uninstall registry key; no otherwise.
  - Windows default: `registry`.
  - Other platforms force `non`.
- `SDCC`: enable SDCC compiler support.
- `WINDOWS_PREFER_OPENCONSOLE`: prefer UTF-8 compatible `OpenConsole.exe`.
  - Applies to: Windows.
  - `OpenConsole.exe` is a part of Windows Terminal. UTF-8 input support was added in version 1.18.
  - `OpenConsole.exe` requires ConPTY, which was introduced in Windows 10 1809.

### Build with Xmake

1. Настройка:
   ```bash
   xmake f -p linux -a x86_64 -m release --qt=/usr
   ```
2. Сборка:
   ```bash
   xmake
   ```
3. Установка:
   ```bash
   sudo xmake install --root -o /usr/local
   ```

Xmake options:
- `filesystem-layout`: `flat` or `hierarchy`.
  - Applies to: Windows.
  - Windows default: `flat`.
  - Other platforms force `hierarchy`.
- `glibc-hwcaps`: enable dummy `RedPandaIDE` which links to actual `libRedPandaIDE.so`.
  - Applies to: Linux glibc.
  - Glibc will automatically load best version from glibc-hwcaps subdirectories, e.g. `/usr/lib/glibc-hwcaps/x86-64-v3/libRedPandaIDE.so`.
  - There’s no magic in the build script. You need to build multiple times with different `-march` flags and install them to proper directories.
- `libexecdir`: RELATIVE directory for auxiliary executables.
  - Applies to: hierarchy filesystem layout.
  - Arch Linux and MSYS2 use `lib`.
- `lua-addon`: enable Lua add-ons.
- `portable-config`: `oui`, `non`, or `registry`.
  - Applies to: Windows.
  - `oui`: yes, write config files to Red Panda C++’s directory.
  - `non`: no, write config files to user config directory.
  - `registry`: yes if current instance matches uninstall registry key; no otherwise.
  - Windows default: `registry`.
  - Other platforms force `non`.
- `prefix`: where Red Panda C++ starts.
  - Applies to: XDG.
  - Unlike CMake variable `CMAKE_INSTALL_PREFIX`, this option is unrelated to installation directory. It only affects XDG `.desktop` file.
- `sdcc`: enable SDCC compiler support.

## Platform-specific Notes

### Windows on ARM

- Red Panda C++ может быть собран для ARM64 ABI только на Windows 11 ARM64.
  - Запуск на Windows 10 ARM64 больше не поддерживается. Установщики предполагают, что эмуляция x64 всегда доступна. (Родной пакет “Red Panda C++ с инструментарием LLVM MinGW” может работать.)
  - ARM64EC (“совместимый с эмуляцией”) хост не поддерживается, т.е. Red Panda C++ не можер быть собрана с инструментарием ARM64EC.
  - Цель ARM64EC (теоретический) поддерживается, т.е. Red Panda C++ будет создавать двоичные файлы для ARM64EC, если инструментарий сборки поддерживал ARM64EC.
- В связи с [ARM32 deprecation in Windows 11 Insider Preview Build 25905](https://blogs.windows.com/windows-insider/2023/07/12/announcing-windows-11-insider-preview-build-25905/), поддержка ARM32 никогда не будет добавлена.

### Linux Foreign Architectures

There are 2 ways to build Red Panda C++ for foreign architectures:
- Cross build: using cross toolchain.
  - As fast as native build;
  - Building cross Qt is not so easy;
  - QEMU user space emulation is still required if you want to run test cases.
- Emulated native build: используя родные инструменты сборки для целевых архитектур с эмуляцией пользовательского пространства QEMU.
  - As easy as native build;
  - Very slow (~10x build time).

#### Cross Build

Follow [CMake’s general cross compiling instructions](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Cross%20Compiling%20With%20CMake.html). Also set `CMAKE_CROSSCOMPILING_EMULATOR` if you want to run test cases.

The [AppImage build environment](https://github.com/redpanda-cpp/appimage-builder) is an example for bootstrapping musl-based, static cross toolchain and Qt.

#### Эмуляция родной сборки

Примечание: Всегда запускайте эмулируемую родную сборку **в chroot’ed environment, контейнерах или jails**. Смешивание архитектур может привести к сбою в работе вашей системы.

Для машин с Linux или BSD установите статически связанный эмулятор пользовательского пространства QEMU (имя пакета, скорее всего, "qemu-user-static") и убедитесь, что включена поддержка binfmt.


Для Windows-систем, в Docker и Podman должна быть включена эмуляция пользовательского пространства QEMU. В противном случае:
* Для Docker:
  ```ps1
  docker run --rm --privileged multiarch/qemu-user-static:register
  ```
* Для Podman, чья виртуальная машина основана на Fedora WSL, просто включите поддержку binfmt:
  ```ps1
  wsl -d podman-machine-default sudo cp /usr/lib/binfmt.d/qemu-aarch64-static.conf /proc/sys/fs/binfmt_misc/register
  wsl -d podman-machine-default sudo cp /usr/lib/binfmt.d/qemu-riscv64-static.conf /proc/sys/fs/binfmt_misc/register
  ```
