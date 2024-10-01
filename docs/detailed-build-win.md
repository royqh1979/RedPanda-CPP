# More Build Instructions for Windows

| Library + Toolchain \ Target | x86 | x64 | ARM64 |
| ---------------------------- | --- | --- | ----- |
| MSYS2 + GNU-based MinGW | ✔️ | ✔️ | ❌ |
| MSYS2 + LLVM-based MinGW | ✔️ | ✔️ | ✔️ |
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
- Red Panda C++ can be built for ARM64 ABI only on Windows 11 ARM64, while it is supposed (but not tested) to run on Windows 10 ARM64.
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

## Qt.io Qt Library with MinGW Toolchain or MSVC Toolchain

Prerequisites:

0. Windows 10 x64 or later. ARM64 is not supported.
   - For MSVC toolchain, Windows has to use Unicode UTF-8 for worldwide language support.
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

0. Windows 10 x64 or later. ARM64 is not supported.
   - For MSVC toolchain, Windows has to use Unicode UTF-8 for worldwide language support.
1. Install Visual Studio 2017 or later, or Visual Studio Build Tools 2017 or later, with _Desktop Development with C++_ workload.
   - In _Installation Details_ panel, under the _Desktop Development with C++_ workload, select at least one _MSVC x86/x64 build tools_ and one _Windows SDK_.
2. Install [standalone vcpkg](https://vcpkg.io/en/getting-started).
3. Install Qt with vcpkg.
   ```ps1
   $TARGET = "x64-windows-static" # or "x86-windows-static"
   vcpkg install qt5-base:$TARGET qt5-svg:$TARGET qt5-tools:$TARGET qt5-translations:$TARGET
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
