# 通用开发说明

小熊猫C++ 需要 Qt 5.15 或 6.7+。

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

| 库 + 工具链 \ 目标 | x86 | x64 | ARM64 |
| ------------------ | --- | --- | ----- |
| MSYS2 + 基于 GNU 的 MinGW | ✔️ | ✔️ | ❌ |
| MSYS2 + 基于 LLVM 的 MinGW | ✔️ | ✔️ | ✔️ |
| [Windows NT 5.x](https://github.com/redpanda-cpp/qtbase-xp) + [MinGW Lite](https://github.com/redpanda-cpp/mingw-lite) | ✔️ | ✔️ | ❌ |

另请参阅[详细构建指南——Windows](./docs/detailed-build-win-cn.md)。

## MSYS2 的 Qt 库 + MinGW 工具链（推荐）

小熊猫C++ 应该能在 MSYS2 的 MinGW 工具链上构建，包括 3 个基于 GNU 的环境（MINGW32、MINGW64、UCRT64）中的 GCC 和 Clang，以及基于 LLVM 的 64 位环境（CLANG64、CLANGARM64）中的 Clang，关于环境的详情可参考 [MSYS2 的文档](https://www.msys2.org/docs/environments/)。以下几个工具链测试较充分：
- MINGW32 GCC，
- MINGW64 GCC，
- UCRT64 GCC（x64 推荐），
- CLANGARM64 Clang（ARM64 唯一可用且推荐的工具链）。

小熊猫C++ 官方版本使用 MINGW32 GCC 和 MINGW64 GCC 构建。

前置条件：

0. Windows 10 x64 或更高版本，或 Windows 11 ARM64。
1. 安装 MSYS2。
2. 在所选环境中安装工具链、Qt 5 库、其他所需工具，64 位：
   ```bash
   pacman -S \
     $MINGW_PACKAGE_PREFIX-{cc,make,qt5-static,7zip,cmake} \
     mingw-w64-i686-nsis \
     git curl
   ```
   32 位：
   ```bash
   pacman -S \
     $MINGW_PACKAGE_PREFIX-{cc,make,qt5-static,cmake} \
     mingw-w64-i686-nsis \
     mingw-w64-x86_64-7zip \
     git curl
   ```

要构建此项目，启动所选的 MSYS2 环境，然后运行
```bash
./packages/msys/build-mingw.sh
```
以构建带 MinGW GCC 工具链或不带编译器的小熊猫C++ 安装程序和绿色包；或者运行
```bash
./packages/msys/build-llvm.sh
```
以构建带 LLVM MinGW 工具链的小熊猫C++ 安装程序。

通用参数：
- `-h`、`--help`：显示帮助信息。
- `-c`、`--clean`：清理构建目录。
- `-nd`、`--no-deps`：不检查依赖项。
- `-t <dir>`、`--target-dir <dir>`：指定输出目录。默认值为 `dist/`。

`build-mingw.sh` 的额外参数：
- `--mingw32`：把 `assets/mingw32.7z` 添加到包中。
- `--mingw64`：把 `assets/mingw64.7z` 添加到包中。
- `--mingw`：`--mingw32`（x86 程序）或 `--mingw64`（x64 程序）的别名。
- `--ucrt <build>`：把 Windows SDK 附带的 UCRT 运行时添加到包中。例如 `--ucrt 22621` 表示 Windows 11 SDK 22H2。

## 用于 Windows NT 5.x 的 Qt 库 + MinGW Lite 工具链

`build-xp.sh` 脚本和 `build-mingw.sh` 类似，但是工具链由 Qt 库提供。

本机构建前置条件：

0. Windows 10 x64 或更高版本。
1. 安装 MSYS2。

要进行本机构建，启动 MSYS2 环境，然后运行
```bash
./packages/msys/build-xp.sh -p 32-msvcrt
```

要进行交叉构建，运行
```bash
podman run -it --rm -v $PWD:/mnt -w /mnt docker.io/amd64/ubuntu:24.04

# 在容器内
export MIRROR=mirrors.ustc.edu.cn  # 根据需要设置镜像站
./packages/xmingw/build-xp.sh -p 32-msvcrt
```

此脚本除了接受 `build-mingw.sh` 的参数外，还接受以下参数：
- `-p|--profile <profile>`：（必需）MinGW Lite 和 Qt 库的编译配置。可用的配置有 `64-ucrt`、`32-ucrt`、`64-msvcrt`、`32-msvcrt`。

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
podman run --rm -v $PWD:/mnt -w /mnt quay.io/redpanda-cpp/appimage-builder-x86_64:20240610.0 ./packages/appimage/01-in-docker.sh
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
