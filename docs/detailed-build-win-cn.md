# 详细构建指南——Windows

| 库 + 工具链 \ 目标 | x86 | x64 | ARM64 |
| ------------------ | --- | --- | ----- |
| MSYS2 + 基于 GNU 的 MinGW | ✔️ | ✔️ | ❌ |
| MSYS2 + 基于 LLVM 的 MinGW | ✔️ | ✔️ | ✔️ |
| [Windows XP](https://github.com/redpanda-cpp/qtbase-xp) + [MinGW UCRT](https://github.com/redpanda-cpp/mingw-lite) | ✔️ | ✔️ | ❌ |
| Qt.io + MinGW | ✔️ | ✔️ | ❌ |
| Qt.io + MSVC | ✔️ | ✔️ | ❌ |
| vcpkg + MSVC | ✔️ | ✔️ | ❌ |

qmake 变量：
- `PREFIX`：`$MAKE install` 的安装路径。
- `WINDOWS_PREFER_OPENCONSOLE=ON`（make 阶段）：首选兼容 UTF-8 的 `OpenConsole.exe`。
  - `OpenConsole.exe` 是 Windows 终端的组件，在 1.18 版本加入了 UTF-8 输出支持。
  - `OpenConsole.exe` 需要 Windows 10 1809 加入的 ConPTY 接口。

关于 ARM 上的 Windows 的注记：
- 小熊猫C++ 只能在 Windows 11 ARM64 上构建 ARM64 版，成品应该能在 Windows 10 ARM64 上运行（但没有测试过）。
  - 不支持 ARM64EC（“仿真兼容”）主机，即不能用 ARM64EC 工具链构建小熊猫 C++。
  - （理论上）支持 ARM64EC 目标，也就是说，如果上游工具链支持 ARM64EC，那么小熊猫C++ 可以构建 ARM64EC 程序和库。
- 随着 [Windows 11 Insider Preview Build 25905 弃用 ARM32](https://blogs.windows.com/windows-insider/2023/07/12/announcing-windows-11-insider-preview-build-25905/)，小熊猫 C++ 今后也不会添加 ARM32 支持了。

## 在 MSYS2 环境中手动构建

前置条件：

0. Windows 8.1 x64 或更高版本，或 Windows 11 ARM64。
1. 安装 MSYS2。
2. 在选定的环境中，安装工具链和 Qt 5 库：
   ```bash
   pacman -S $MINGW_PACKAGE_PREFIX-toolchain $MINGW_PACKAGE_PREFIX-qt5-static
   ```

构建：

1. 在选定的环境中，设置相关变量：
   ```bash
   SRC_DIR="/c/src/redpanda-src" # 以 “C:\src\redpanda-src” 为例
   BUILD_DIR="/c/src/redpanda-build" # 以 “C:\src\redpanda-build” 为例
   INSTALL_DIR="/c/src/redpanda-pkg" # 以 “C:\src\redpanda-pkg” 为例
   ```
2. 定位到构建目录：
   ```bash
   rm -rf "$BUILD_DIR" # 根据需要进行全新构建
   mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
   ```
3. 配置、构建、安装：
   ```bash
   $MSYSTEM_PREFIX/qt5-static/bin/qmake PREFIX="$INSTALL_DIR" "$SRC_DIR/Red_Panda_CPP.pro"
   mingw32-make -j$(nproc)
   mingw32-make install
   ```

## Qt.io 的 Qt 库 + MinGW 工具链或 MSVC 工具链

前置条件：

0. Windows 10 x64 或更高版本。不支持 ARM64。
   - 对于 MSVC 工具链，Windows 必须使用 Unicode UTF-8 提供全球语言支持。
1. 用 [Qt.io](https://www.qt.io/download-qt-installer-oss) 或[镜像站](https://mirrors.sjtug.sjtu.edu.cn/docs/qt)的在线安装器安装 Qt。
   - 选中 Qt 库（“Qt” 组下的 “Qt 5.15.2” 小组，勾选 “MinGW 8.1.0 32-bit” “MinGW 8.1.0 64-bit” “MSVC 2019 32-bit” “MSVC 2019 64-bit” 中的至少一个）。
   - 对于 MinGW 工具链，选中相应的工具链（“Qt” 组下的 “Developer and Designer Tools” 小组，“MinGW 8.1.0 32-bit” 或 “MinGW 8.1.0 64-bit”，匹配库的版本）。
   - 根据需要，选中 Qt Creator（“Qt” 组下的 “Developer and Designer Tools” 小组，推荐在使用 MSVC 工具链时选中以支持并行构建）。
2. 对于 MSVC 工具链，安装 Visual Studio 2019 或更高版本，或 “Visual Studio 构建工具 2019” 或更高版本，附带 “使用 C++ 的桌面开发” 工作负载。
   - 在 “安装详细信息” 面板，“使用 C++ 的桌面开发” 之下，至少选择一个 “MSVC x86/x64 生成工具” 和一个 Windows SDK。

构建：

1. 从开始菜单中打开 Qt 环境。
2. 在 Qt 环境中，设置相关变量：
   ```bat
   rem 即使路径含空格也不加引号
   set SRC_DIR=C:\src\redpanda-src
   set BUILD_DIR=C:\src\redpanda-build
   set INSTALL_DIR=C:\src\redpanda-pkg
   rem 仅 MSVC 工具链需要设置
   set VS_INSTALL_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community
   rem 仅 MSVC 工具链需要设置；或 x86
   set VC_ARCH=amd64
   rem 仅 MSVC 工具链需要；如果未安装 Qt Creator 则不要设置
   set QT_CREATOR_DIR=C:\Qt\Tools\QtCreator
   ```
3. 定位到构建目录：
   ```bat
   rem 根据需要进行全新构建
   rmdir /s /q "%BUILD_DIR%"
   mkdir "%BUILD_DIR%" && cd /d "%BUILD_DIR%"
   ```
4. 配置、构建、安装。对于 MinGW 工具链：
   ```bat
   qmake PREFIX="%INSTALL_DIR%" "%SRC_DIR%\Red_Panda_CPP.pro"
   mingw32-make -j%NUMBER_OF_PROCESSORS%
   mingw32-make install
   windeployqt "%INSTALL_DIR%\RedPandaIDE.exe"
   ```
   对于 MSVC 工具链：
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

## 高级选项：vcpkg 的 Qt 静态库 + MSVC 工具链

前置条件：

0. Windows 10 x64 或更高版本。不支持 ARM64。
   - Windows 必须使用 Unicode UTF-8 提供全球语言支持。
1. 安装 Visual Studio 2017 或更高版本，或 “Visual Studio 构建工具 2017” 或更高版本，带有 “使用 C++ 的桌面开发” 工作负载。
   - 在 “安装详细信息” 面板，“使用 C++ 的桌面开发” 之下，至少选择一个 “MSVC x86/x64 生成工具” 和一个 Windows SDK。
2. 安装 [vcpkg 的独立版本](https://github.com/microsoft/vcpkg/blob/master/README_zh_CN.md#快速开始-windows)。
3. 用 vcpkg 安装 Qt。
   ```ps1
   $TARGET = "x64-windows-static" # 或 "x86-windows-static"
   vcpkg install qt5-base:$TARGET qt5-svg:$TARGET qt5-tools:$TARGET qt5-translations:$TARGET
   ```

在 PowerShell (Core) 或 Windows PowerShell 中用 VS 2019 或更高版本构建：

1. 设置相关变量：
   ```ps1
   $SRC_DIR = "C:\src\redpanda-src"
   $BUILD_DIR = "C:\src\redpanda-build"
   $INSTALL_DIR = "C:\src\redpanda-pkg"
   $VCPKG_ROOT = "C:\src\vcpkg"
   $VCPKG_TARGET = "x64-windows-static" # 或 "x86-windows-static"
   $VS_INSTALL_PATH = "C:\Program Files\Microsoft Visual Studio\2022\Community"
   $VC_ARCH = "amd64" # 或 "x86"
   $JOM = "$VCPKG_ROOT\downloads\tools\jom\jom-1.1.3\jom.exe" # 检查版本号
   ```
2. 定位到构建目录：
   ```ps1
   Remove-Item -Recurse -Force "$BUILD_DIR" # 根据需要进行全新构建
   (New-Item -ItemType Directory -Force "$BUILD_DIR") -and (Set-Location "$BUILD_DIR")
   ```
3. 配置、构建、安装：
   ```ps1
   Import-Module "$VS_INSTALL_PATH\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
   Enter-VsDevShell -VsInstallPath "$VS_INSTALL_PATH" -SkipAutomaticLocation -DevCmdArguments "-arch=$VC_ARCH"
   & "$VCPKG_ROOT\installed\$VCPKG_TARGET\tools\qt5\bin\qmake.exe" PREFIX="$INSTALL_DIR" "$SRC_DIR\Red_Panda_CPP.pro"
   & "$JOM" "-j${Env:NUMBER_OF_PROCESSORS}"
   & "$JOM" install
   ```

在命令提示符中用 VS 2017 或更高版本构建：

1. 从开始菜单中打开合适的 VC 环境。
2. 设置相关变量：
   ```bat
   rem 即使路径含空格也不加引号
   set SRC_DIR=C:\src\redpanda-src
   set BUILD_DIR=C:\src\redpanda-build
   set INSTALL_DIR=C:\src\redpanda-pkg
   set VCPKG_ROOT=C:\src\vcpkg
   rem 或 x86-windows-static
   set VCPKG_TARGET=x64-windows-static
   rem 检查版本号
   set JOM=%VCPKG_ROOT%\downloads\tools\jom\jom-1.1.3\jom.exe
   ```
3. 定位到构建目录：
   ```bat
   rem 根据需要进行全新构建
   rmdir /s /q "%BUILD_DIR%"
   mkdir "%BUILD_DIR%" && cd /d "%BUILD_DIR%"
   ```
4. 配置、构建、安装：
   ```bat
   "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\tools\qt5\bin\qmake.exe" PREFIX="%INSTALL_DIR%" "%SRC_DIR%\Red_Panda_CPP.pro"
   "%JOM%" -j%NUMBER_OF_PROCESSORS%
   "%JOM%" install
   ```
