# 通用开发说明

小熊猫C++ 需要 Qt 5.15。

推荐开发环境：
1. Visual Studio Code。
   * 性能更好。
2. Qt Creator。
   * （几乎）无需配置。
   * 内建 UI 设计器。
   * 调试器的 Qt 集成。

设置 Visual Studio Code 开发环境的步骤：
0. 在 Windows 设置中，启用 “开发人员模式”。启用 Git 的 `core.symlinks` 选项（`git config core.symlinks true`）。
1. 安装 [xmake](https://xmake.io/) 和 [XMake 扩展](https://marketplace.visualstudio.com/items?itemName=tboox.xmake-vscode)。
2. 安装 [C/C++ 扩展](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) 以支持语言和调试功能。
3. 根据需要安装 [clangd](https://clangd.llvm.org/) 和 [clangd 扩展](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd)以获得更好的代码分析能力。
4. 配置工作区：
   - 编译命令：`.vscode/compile_commands.json`（命令面板中的 “C/C++: 编辑配置(UI)”）；
   - “Clangd: Arguments”：`--compile-commands-dir=.vscode`；
   - “Xmake: Additional Config Arguments”：如 `--qt=/usr`。
5. 在命令面板中执行 “XMake: UpdateIntellisense” 以生成编译数据库。

\* 提示：xmake 的引入是为了支持编译数据库的生成和功能测试矩阵，目前并不完备。

# Windows

适用于 Windows 7 或更高版本：

| 库 + 工具链 \ 目标 | x86 | x64 | ARM64 |
| ------------------ | --- | --- | ----- |
| MSYS2 + 基于 GNU 的 MinGW | ✔️ | ✔️ | ❌ |
| MSYS2 + 基于 LLVM 的 MinGW | ✔️ | ✔️ | ✔️ |
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

小熊猫C++ 应该能在 MSYS2 的 MinGW 工具链上构建，包括 3 个基于 GNU 的环境（MINGW32、MINGW64、UCRT64）中的 GCC 和 Clang，以及 3 个基于 LLVM 的环境（CLANG32、CLANG64、CLANGARM64）中的 Clang，关于环境的详情可参考 [MSYS2 的文档](https://www.msys2.org/docs/environments/)。以下几个工具链测试较充分：
- MINGW32 GCC，
- MINGW64 GCC，
- UCRT64 GCC（x64 推荐），
- CLANGARM64 Clang（ARM64 唯一可用且推荐的工具链）。

小熊猫C++ 官方版本使用 MINGW32 GCC 和 MINGW64 GCC 构建。

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

# Linux

另请参阅[详细构建指南——符合 freedesktop.org（XDG）规范的桌面系统](./docs/detailed-build-xdg-cn.md)。

## Alpine Linux、Arch Linux、Debian 及其衍生版本、Fedora、openSUSE

1. 准备构建环境（[Alpine](https://wiki.alpinelinux.org/wiki/Abuild_and_Helpers)、[Arch](https://wiki.archlinuxcn.org/wiki/Makepkg)、[Debians](https://wiki.debian.org/BuildingTutorial)、[RPM](https://rpm-packaging-guide.github.io/#prerequisites) 文档）。
   - 对于 Debian 系：
     ```sh
     sudo apt install --no-install-recommends build-essential debhelper devscripts equivs
     ```
2. 调用构建脚本：
   - Alpine Linux：`./packages/alpine/buildapk.sh`
   - Arch Linux：`./packages/archlinux/buildpkg.sh`
   - Debian 系：`./packages/debian/builddeb.sh`
   - Fedora：`./packages/fedora/buildrpm.sh`
   - openSUSE：`./packages/opensuse/buildrpm.sh`
3. 安装软件包：
   - Alpine Linux：`~/packages/unsupported/$(uname -m)/redpanda-cpp-git-*.apk`
   - Arch Linux：`/tmp/redpanda-cpp-git/redpanda-cpp-git-*.pkg.tar.zst`
   - Debian 系：`/tmp/redpanda-cpp_*.deb`
   - Fedora、openSUSE：`~/rpmbuild/RPMS/$(uname -m)/redpanda-cpp-git-*.rpm`
4. 运行小熊猫C++：
   ```bash
   RedPandaIDE
   ```

注意：这些构建脚本签出此存储库的 HEAD，因此构建之前务必提交所有变更。

此外，也可以在容器环境中构建（推荐使用 rootless Podman；Docker 可能搞乱文件权限）：

```bash
podman run --rm -v $PWD:/mnt -w /mnt <image> ./packages/<distro>/01-in-docker.sh

# 以 Arch Linux 为例
podman run --rm -v $PWD:/mnt -w /mnt docker.io/archlinux:latest ./packages/archlinux/01-in-docker.sh
```

软件包位于 `dist/` 目录下。

## Ubuntu 20.04 x86_64（NOI Linux 2.0）静态链接包

`redpanda-cpp-bin` 包大体上就是 “AppImage 重新打包”。真正的构建过程在容器中进行，因此构建主机不一定要 Ubuntu 20.04，任何 Linux 发行版只要有 Podman 和 dpkg 就行。

1. 安装 Podman，如果不是 Debian 及其衍生版还要安装 dpkg。
   ```sh
   sudo apt install podman
   ```
   **警告**：**不要**在非 Debian 系上使用 dpkg 安装软件包，否则将会破坏系统。
2. 调用构建脚本：
   ```sh
   ./packages/debian-static/builddeb.sh
   ```

软件包位于 `dist/` 目录下。

## Linux AppImage

```bash
podman run --rm -v $PWD:/mnt -w /mnt quay.io/redpanda-cpp/appimage-builder-x86_64:20240304.0 ./packages/appimage/01-in-docker.sh
```

Dockerfile 位于 [redpanda-cpp/appimage-builder](https://github.com/redpanda-cpp/appimage-builder)。可用架构：`x86_64`、`aarch64`、`riscv64`。

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
