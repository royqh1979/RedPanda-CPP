# 通用开发说明

小熊猫 C++ 需要 Qt 5（≥ 5.12）。

推荐开发环境：
1. Visual Studio Code。
   * 性能更好。
2. Qt Creator。
   * （几乎）无需配置。
   * 内建 UI 设计器。
   * 调试器的 Qt 集成。

设置 Visual Studio Code 开发环境的步骤：
1. 安装 [clangd](https://clangd.llvm.org/) 和 [clangd 扩展](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd)。
2. 安装 [Bear](https://github.com/rizsotto/Bear)。
3. 安装 [CodeLLDB extension](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb)。
4. 生成编译数据库：
   ```bash
   mkdir -p /path/to/build && cd /path/to/build
   qmake CONFIG+=debug PREFIX=/path/to/pkg /path/to/src
   bear -- make -j$(nproc)
   ```
5. 在本工作区 “Clangd: Arguments” 中添加 `--compile-commands-dir=/path/to/build` 或直接修改 `.vscode/settings.json`：
   ```json
   {
     "clangd.arguments": [
       "--compile-commands-dir=/path/to/build"
     ]
   }
   ```
6. 在 `.vscode/tasks.json` 中配置构建任务：
   ```json
   {
     "version": "2.0.0",
     "tasks": [
       {
         "label": "build",
         "type": "shell",
         "command": "cd /path/to/build && make -j$(nproc) && make install",
         "group": {
           "kind": "build",
           "isDefault": true
         }
       }
     ]
   }
   ```
   并在 `.vscode/launch.json` 中配置调试任务：
   ```json
   {
     "version": "0.2.0",
     "configurations": [
       {
         "name": "build",
         "type": "lldb",
         "request": "launch",
         "program": "/path/to/pkg/bin/RedPandaIDE",
         "args": [],
         "cwd": "/path/to/build",
         "env": {
           "QT_ASSUME_STDERR_HAS_CONSOLE": "1"
         },
         "preLaunchTask": "build"
       }
     ]
   }
   ```

# Windows

适用于 Windows 7 或更高版本：

| 库 + 工具链 \ 目标 | x86 | x64 | ARM64 |
| ------------------ | --- | --- | ----- |
| MSYS2 + 基于 GNU 的 MinGW | ✔️ | ✔️ | ❌ |
| MSYS2 + 基于 LLVM 的 MinGW | ✔️ | ✔️ | ✔️ |
| Qt.io + MinGW | ✔️ | ✔️ | ❌ |
| Qt.io + MSVC | ✔️ | ✔️ | ❌ |
| vcpkg + MSVC | ✔️ | ✔️ | ❌ |

关于 ARM 上的 Windows 的注记：
- 小熊猫 C++ 只能在 Windows 11 ARM64 上构建 ARM64 版，成品应该能在 Windows 10 ARM64 上运行（但没有测试过）。
  - 不支持 ARM64EC（“仿真兼容”）主机，即不能用 ARM64EC 工具链构建小熊猫 C++。
  - （理论上）支持 ARM64EC 目标，也就是说，如果上游工具链支持 ARM64EC，那么小熊猫 C++ 可以构建 ARM64EC 程序和库。
- 随着 [Windows 11 Insider Preview Build 25905 弃用 ARM32](https://blogs.windows.com/windows-insider/2023/07/12/announcing-windows-11-insider-preview-build-25905/)，小熊猫 C++ 今后也不会添加 ARM32 支持了。

适用于旧版 Windows（NT 5.1 – 6.0）：

| 库 + 工具链 \ 目标 | x86 | x64 |
| ------------------ | --- | --- |
| 从[打过补丁的源代码](https://github.com/redpanda-cpp/qtbase-5.6)构建的 Qt 5.6 + MinGW | ✔️ | ✔️ |

关于旧版 Windows 的注记：
- 支持的 Windows 版本：
  - Windows XP SP3 或更高版本；
  - Windows Server 2003 x64 Edition（也叫 Windows XP x64 Edition）SP2 或更高版本。
- 构建环境需要 Windows 7 x64 或更高版本。
- 从源代码构建 Qt 5.6 并与官方 Qt 安装共存可参考[这个脚本](packages/windows/build-qt5.6-mingw-static.sh)（使用 Qt.io MinGW GCC 8.1.0）。

## MSYS2 的 Qt 库 + MinGW 工具链（推荐）

小熊猫 C++ 应该能在 MSYS2 的 MinGW 工具链上构建，包括 3 个基于 GNU 的环境（MINGW32、MINGW64、UCRT64）中的 GCC 和 Clang，以及 3 个基于 LLVM 的环境（CLANG32、CLANG64、CLANGARM64）中的 Clang，关于环境的详情可参考 [MSYS2 的文档](https://www.msys2.org/docs/environments/)。以下几个工具链测试较充分：
- MINGW32 GCC，
- MINGW64 GCC，
- UCRT64 GCC（x64 推荐），
- CLANGARM64 Clang（ARM64 唯一可用且推荐的工具链）。

小熊猫 C++ 官方版本使用 MINGW32 GCC 和 MINGW64 GCC 构建。

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

0. Windows 7 x64 或更高版本。不支持 ARM64。
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

0. Windows 7 x64 或更高版本。不支持 ARM64。
   - 在全新安装的 Windows 上，依次安装以下组件：
     1. SHA-2 代码签名支持（.NET Framework 4.8 的前置条件），
     2. .NET Framework 4.8（Windows 管理框架 5.1 和 Visual Studio 的前置条件，也是 Git for Windows 的可选依赖），
     3. Windows 管理框架 5.1（vcpkg 自举的前置条件）。
1. 安装 Visual Studio 2017 或更高版本，或 “Visual Studio 构建工具 2017” 或更高版本，带有 “使用 C++ 的桌面开发” 工作负载。
   - 在 “安装详细信息” 面板，“使用 C++ 的桌面开发” 之下，至少选择一个 “MSVC x86/x64 生成工具” 和一个 Windows SDK。
2. 安装 [vcpkg 的独立版本](https://github.com/microsoft/vcpkg/blob/master/README_zh_CN.md#快速开始-windows)。
   - 截至 2023.08.09，Windows 7 需要[一个补丁](./packages/windows/vcpkg-win7-2023.08.09.patch)以使用兼容的 Python 版本。受影响的文件可能会被修改，所以最好手动修改这些文件。
3. 用 vcpkg 安装 Qt。
   ```ps1
   $TARGET = "x64-windows-static" # 或 "x86-windows-static"
   vcpkg install qt5-base:$TARGET qt5-svg:$TARGET qt5-tools:$TARGET
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

# Linux 和其他符合 freedesktop.org（XDG）规范的桌面系统

通用步骤:

- 安装支持 C++17 的 GCC（≥ 7）或 Clang（≥ 6）。
- 安装 Qt 5（≥ 5.12）Base、SVG、Tools 模块，包括库和开发文件。
- 如果使用静态版本的 Qt 编译，还要安装 fcitx5-qt。

构建：

1. 配置：
   ```bash
   qmake PREFIX=/usr/local /path/to/src/Red_Panda_CPP.pro
   ```
2. 构建：
   ```bash
   make -j$(nproc)
   ```
3. 安装：
   ```bash
   sudo make install
   ```

qmake 变量:
- `PREFIX`：默认值是 `/usr/local`。打包时应该定义为 `/usr` 或 `/opt/redpanda-cpp`。
- `LIBEXECDIR`：辅助程序的路径，默认值是 `$PREFIX/libexec`。Arch Linux 使用 `/usr/lib`。
- `XDG_ADAPTIVE_ICON=ON`：遵循 [freedesktop.org 图标主题规范](https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html)安装图标，以适应不同的主题和尺寸。AppImage 需要启用此项；Linux 打包 `PREFIX=/usr` 时推荐启用此项。
- `LINUX_STATIC_IME_PLUGIN=ON`（make 阶段）：静态链接输入法插件。推荐在使用静态版本的 Qt 编译时启用；**不要**在使用动态版本的 Qt 编译时启用。

## Debian 及其衍生版本

### 适用于当前系统的 deb 包

1. 安装依赖包：
   ```bash
   sudo apt install \
     build-essential debhelper \
     libqt5svg5-dev qtbase5-dev qtbase5-dev-tools qttools5-dev-tools
   ```
2. 构建 deb 包：
   ```bash
   ./packages/debian/builddeb.sh
   ```
3. 安装 deb 包：
   ```bash
   sudo apt install /tmp/redpanda-cpp_*.deb
   ```
4. 运行小熊猫 C++：
   ```bash
   RedPandaIDE
   ```

### 在容器中构建适用于多架构/版本的包

Windows 宿主的额外要求：
- Docker 使用基于 WSL 2 的引擎，或者对此项目文件夹启用文件共享（Settings > Resources > File sharing）；
- PowerShell (Core) 或 Windows PowerShell。

* Linux 宿主：
  ```bash
  DOCKER=docker # 或 podman
  SOURCE_DIR=/build/RedPanda-CPP # *容器内*的源代码路径
  JOBS=$(nproc) # 同时构建多个包时可降低并行数

  MIRROR=mirrors.ustc.edu.cn # 留空以使用默认的镜像站
  PLATFORM=linux/amd64 # 或 linux/386、linux/arm64/v8、linux/arm/v7、linux/riscv64
  IMAGE=debian:12 # 或 Ubuntu（如 ubuntu:22.04）

  $DOCKER run --rm -e MIRROR=$MIRROR -e SOURCE_DIR=$SOURCE_DIR -e JOBS=$JOBS -v $PWD:$SOURCE_DIR --platform $PLATFORM $IMAGE $SOURCE_DIR/packages/debian/01-in-docker.sh
  ```
* Windows 宿主：
  ```ps1
  $DOCKER = "docker" # 或 "podman"
  $SOURCE_DIR = "/build/RedPanda-CPP" # *容器内*的源代码路径
  $JOBS = $Env:NUMBER_OF_PROCESSORS # 同时构建多个包时可降低并行数

  $MIRROR = "mirrors.ustc.edu.cn" # 留空以使用默认的镜像站
  $PLATFORM = "linux/amd64" # 或 "linux/386"、"linux/arm64/v8"、"linux/arm/v7"、"linux/riscv64"
  $IMAGE = "debian:12" # 或 Ubuntu（如 "ubuntu:22.04"）

  & $DOCKER run --rm -e MIRROR=$MIRROR -e SOURCE_DIR=$SOURCE_DIR -e JOBS=$JOBS -v "$(Get-Location):$SOURCE_DIR" --platform $PLATFORM $IMAGE $SOURCE_DIR/packages/debian/01-in-docker.sh
  ```

### 手动安装

1. 安装编译器
   ```bash
   apt install gcc g++ make gdb gdbserver
   ```
2. 安装 Qt 5 和依赖包
   ```bash
   apt install qtbase5-dev qttools5-dev-tools libqt5svg5-dev git qterminal
   ```
3. 下载源码
   ```bash
   git clone https://gitee.com/royqh1979/RedPanda-CPP.git
   ```
4. 编译
   ```bash
   cd RedPanda-CPP/
   qmake Red_Panda_CPP.pro
   make -j$(nproc)
   sudo make install
   ```
5. 运行
   ```bash
   RedPandaIDE
   ```

## Alpine Linux, Arch Linux, Fedora, openSUSE

1. 准备构建环境（[Alpine](https://wiki.alpinelinux.org/wiki/Abuild_and_Helpers)、[Arch](https://wiki.archlinux.org/title/Makepkg)、[RPM](https://rpm-packaging-guide.github.io/#prerequisites) 文档）。
2. 调用构建脚本：
   - Alpine Linux：`./packages/alpine/buildapk.sh`
   - Arch Linux：`./packages/archlinux/buildpkg.sh`
   - Fedora：`./packages/fedora/buildrpm.sh`
   - openSUSE：`./packages/opensuse/buildrpm.sh`
3. 安装软件包：
   - Alpine Linux：`~/packages/unsupported/$(uname -m)/redpanda-cpp-git-*.apk`
   - Arch Linux：`/tmp/redpanda-cpp-git/redpanda-cpp-git-*.pkg.tar.zst`
   - Fedora、openSUSE：`~/rpmbuild/RPMS/$(uname -m)/redpanda-cpp-git-*.rpm`
4. 运行小熊猫 C++：
   ```bash
   RedPandaIDE
   ```

注意：这些构建脚本签出此存储库的 HEAD，因此构建之前务必提交所有变更。

## Linux AppImage

1. 安装依赖包：Docker 或 Podman。

   Windows 宿主的额外要求：
   - Docker 使用基于 WSL 2 的引擎，或者对此项目文件夹启用文件共享（Settings > Resources > File sharing）；
   - PowerShell (Core) 或 Windows PowerShell。
2. 准备构建环境。Linux 宿主：
   ```bash
   ARCH=x86_64 # 或 aarch64、riscv64
   DOCKER=docker # 或 podman
   $DOCKER build -t redpanda-builder-$ARCH packages/appimage/dockerfile-$ARCH
   ```
   Windows 宿主：
   ```ps1
   $ARCH = "x86_64" # 或 "aarch64"、"riscv64"
   $DOCKER = "docker" # 或 "podman"
   & $DOCKER build -t redpanda-builder-$ARCH packages/appimage/dockerfile-$ARCH
   ```
3. 构建 AppImage。Linux 宿主：
   ```bash
   ARCH=x86_64
   DOCKER=docker
   $DOCKER run --rm -v $PWD:/build/RedPanda-CPP -e CARCH=$ARCH redpanda-builder-$ARCH /build/RedPanda-CPP/packages/appimage/01-in-docker.sh
   ```
   Windows 宿主：
   ```ps1
   $ARCH = "x86_64"
   $DOCKER = "docker"
   & $DOCKER run --rm -v "$(Get-Location):/build/RedPanda-CPP" -e CARCH=$ARCH redpanda-builder-$ARCH /build/RedPanda-CPP/packages/appimage/01-in-docker.sh
   ```
4. 运行小熊猫 C++.
   ```bash
   ./dist/RedPandaIDE-x86_64.AppImage # 或 *-aarch64.AppImage、*-riscv64.AppImage
   ```

## 异架构的模拟本机构建（emulated native build）

可以借助 QEMU 用户空间模拟，运行目标架构的本机工具链，来构建小熊猫 C++。

注意：始终**在容器或 jail 中**运行模拟本机构建，因为混用不同架构的程序和库可能会损坏系统。

对于 Linux 或 BSD 宿主，需要安装静态链接的 QEMU 用户空间模拟器（包名通常为 `qemu-user-static`）并确认已经启用 binfmt 支持。

对于 Windows 宿主，Docker 和 Podman 应该已经启用了 QEMU 用户空间模拟。如果没有启用，
* Docker：
  ```ps1
  docker run --rm --privileged multiarch/qemu-user-static:register
  ```
* Podman（其虚拟机基于 Fedora WSL）只需要启用 binfmt 支持：
  ```ps1
  wsl -d podman-machine-default sudo cp /usr/lib/binfmt.d/qemu-aarch64-static.conf /proc/sys/fs/binfmt_misc/register
  wsl -d podman-machine-default sudo cp /usr/lib/binfmt.d/qemu-riscv64-static.conf /proc/sys/fs/binfmt_misc/register
  ```

# macOS

## Qt.io 的 Qt 库

前置条件：

0. macOS 10.13 或更高版本。
1. 安装 Xcode 命令行工具：
   ```bash
   xcode-select --install
   ```
2. 用 [Qt.io](https://www.qt.io/download-qt-installer-oss) 或[镜像站](https://mirrors.sjtug.sjtu.edu.cn/docs/qt)的在线安装器安装 Qt。
   - 选中 Qt 库（“Qt” 组下的 “Qt 5.15.2” 小组，勾选 “macOS”）。

构建：

1. 设置相关变量：
   ```bash
   SRC_DIR="~/redpanda-src"
   BUILD_DIR="~/redpanda-build"
   INSTALL_DIR="~/redpanda-pkg"
   ```
2. 定位到构建目录：
   ```bash
   rm -rf "$BUILD_DIR" # 根据需要进行全新构建
   mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
   ```
3. 配置、构建、安装：
   ```bash
   ~/Qt/5.15.2/clang_64/bin/qmake PREFIX="$INSTALL_DIR" "$SRC_DIR/Red_Panda_CPP.pro"
   make -j$(sysctl -n hw.logicalcpu)
   make install
   ~/Qt/5.15.2/clang_64/bin/macdeployqt "$INSTALL_DIR/bin/RedPandaIDE.app"
   ```
