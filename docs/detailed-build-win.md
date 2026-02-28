# More Build Instructions for Windows

| Library + Toolchain \ Target | x86 | x64 | ARM64 |
| ---------------------------- | --- | --- | ----- |
| MSYS2 + GNU-based MinGW | ❌ | ✔️ | ❌ |
| MSYS2 + LLVM-based MinGW | ❌ | ✔️ | ✔️ |
| [Windows XP](https://github.com/redpanda-cpp/qtbase-xp) + [MinGW UCRT](https://github.com/redpanda-cpp/mingw-lite) | ✔️ | ✔️ | ❌ |
| Qt.io + MinGW | ✔️ | ✔️ | ❌ |
| Qt.io + MSVC | ✔️ | ✔️ | ❌ |
| vcpkg + MSVC | ✔️ | ✔️ | ❌ |

qmake variables:
- `PREFIX`: where `$MAKE install` installs files to.
- `WINDOWS_PREFER_OPENCONSOLE=ON` (make phase): prefer UTF-8 compatible `OpenConsole.exe`.
  - `OpenConsole.exe` is a part of Windows Terminal. UTF-8 input support was added in version 1.18.
  - `OpenConsole.exe` requires ConPTY, which was introduced in Windows 10 1809.

Notes for Windows on ARM:
- Red Panda C++ can be built for ARM64 ABI only on Windows 11 ARM64.
  - Running on Windows 10 ARM64 is no longer supported. Installers assume x64 emulation is always available. (The native package “Red Panda C++ with LLVM MinGW toolchain” may work.)
  - ARM64EC (“emulation compatible”) host is not supported, i.e., Red Panda C++ cannot be built with ARM64EC toolchain.
  - ARM64EC target is (theoretically) supported, i.e. Red Panda C++ will build ARM64EC binaries if upstream toolchain supports ARM64EC.
- With the [ARM32 deprecation in Windows 11 Insider Preview Build 25905](https://blogs.windows.com/windows-insider/2023/07/12/announcing-windows-11-insider-preview-build-25905/), ARM32 support will never be added.

## Maunally Build in MSYS2

Prerequisites:

0. Windows 8.1 x64 or later, or Windows 11 ARM64.
1. Install MSYS2.
2. In selected environment, install toolchain and Qt 5 library:
   ```bash
   pacman -S \
     $MINGW_PACKAGE_PREFIX-{toolchain,qt5-static} \
     git
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
