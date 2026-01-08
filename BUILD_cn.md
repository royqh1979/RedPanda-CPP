# 通用开发说明

前置条件：

- Qt 6.8+ 或 5.15。
  - 可以使用 Qt 5.15，但不支持 `make update_translations`。
- 支持 CMake 或 xmake 的 C++ 开发环境，推荐：
  - Visual Studio Code -- 性能更好，与 AI 紧密集成。
  - Qt Creator -- 内建 UI 设计器，调试器的 Qt 集成。
    - `lupdate`：在 “编辑 > Preferences > 环境 > 外部工具” 中添加一个外部工具——执行档：`cmake`；参数：`--build . --target update_translations`；工作目录：选择 Global variables 下的 `ActiveProject:BuildConfig:Path`。

# Windows

| 库 + 工具链 \ 目标 | x86 | x64 | ARM64 |
| ------------------ | --- | --- | ----- |
| [Windows NT 5.x](https://github.com/redpanda-cpp/qtbase-xp) + [MinGW Lite](https://github.com/redpanda-cpp/mingw-lite) | ✔️ | ✔️ | ❌ |

<!--
| MSYS2 + 基于 GNU 的 MinGW | ❌ | ✔️ | ❌ |
| MSYS2 + 基于 LLVM 的 MinGW | ❌ | ✔️ | ✔️ |

另请参阅[详细构建指南——Windows](./docs/detailed-build-win-cn.md)。

## MSYS2 的 Qt 库 + MinGW 工具链（推荐）

小熊猫C++ 应该能在 MSYS2 的 64 位 MinGW 工具链上构建，包括基于 GNU 的环境（MINGW64、UCRT64）中的 GCC 和 Clang，以及基于 LLVM 的环境（CLANG64、CLANGARM64）中的 Clang，关于环境的详情可参考 [MSYS2 的文档](https://www.msys2.org/docs/environments/)。以下几个工具链测试较充分：
- MINGW64 GCC，
- UCRT64 GCC（x64 推荐），
- CLANGARM64 Clang（ARM64 唯一可用且推荐的工具链）。

前置条件：

0. Windows 10 x64 或更高版本，或 Windows 11 ARM64。
1. 安装 MSYS2。
2. 在所选环境中安装工具链、Qt 5 库、其他所需工具：
   ```bash
   pacman -S \
     $MINGW_PACKAGE_PREFIX-{cc,make,qt5-static,7zip,cmake} \
     mingw-w64-i686-nsis \
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
- `--mingw`：`--mingw64`（x64 程序）的别名。
- `--gcc-linux-x86-64`：把 `assets/gcc-linux-x86-64.7z` 和 `assets/alpine-minirootfs-x86_64.tar` 添加到包中。
- `--gcc-linux-aarch64`：把 `assets/gcc-linux-aarch64.7z` 和 `assets/alpine-minirootfs-aarch64.tar` 添加到包中。
- `--ucrt`：把 UCRT 安装程序添加到包中。
-->

## 用于 Windows NT 5.x 的 Qt 库 + MinGW Lite 工具链

`build-xp.sh` 脚本和 `build-mingw.sh` 类似，但是工具链由 Qt 库提供。

本机构建前置条件：

0. Windows 10 x64 或更高版本。
1. 安装 MSYS2。

要进行本机构建，启动 MSYS2 环境，然后运行
```bash
./packages/mingw/build-xp.sh -p 32-msvcrt
```

此脚本除了接受 `build-mingw.sh` 的参数外，还接受以下参数：
- `-p|--profile <profile>`：（必需）MinGW Lite 和 Qt 库的编译配置。可用的配置有 `64-ucrt`、`32-ucrt`、`64-msvcrt`、`32-msvcrt`。

# Linux

另请参阅[详细构建指南——符合 freedesktop.org（XDG）规范的桌面系统](./docs/detailed-build-xdg-cn.md)。

## 用于滚动发行版的软件包

目前有 Alpine Linux (edge)、Arch Linux、Debian (unstable)、Fedora、openSUSE Tumbleweed、Ubuntu (devel)。

1. 准备构建环境（[Alpine](https://wiki.alpinelinux.org/wiki/Abuild_and_Helpers)、[Arch](https://wiki.archlinuxcn.org/wiki/Makepkg)、[Debians](https://wiki.debian.org/BuildingTutorial)、[RPM](https://rpm-packaging-guide.github.io/#prerequisites) 文档）。
   - 对于 Debian 家族：
     ```sh
     sudo apt install --no-install-recommends build-essential debhelper devscripts equivs
     ```
2. 调用构建脚本：
   - Alpine Linux：`./packages/alpine/buildapk.sh`
   - Arch Linux：`./packages/archlinux/buildpkg.sh`
   - Debian 家族：`./packages/debian/builddeb.sh`
   - Fedora：`./packages/fedora/buildrpm.sh`
   - openSUSE：`./packages/opensuse/buildrpm.sh`
3. 安装软件包：
   - Alpine Linux：`~/packages/unsupported/$(uname -m)/redpanda-cpp-git-*.apk`
   - Arch Linux：`/tmp/redpanda-cpp-git/redpanda-cpp-git-*.pkg.tar.zst`
   - Debian 家族：`/tmp/redpanda-cpp_*.deb`
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

## 用于绝大多数 Linux 桌面发行版的静态包

打包格式：AppImage、Debian (`*.deb`)、tar 包 (`.tar.gz`)。

```bash
podman run --rm -v $PWD:/mnt -w /mnt ghcr.io/redpanda-cpp/appimage-builder-x86_64:20260107.0 ./packages/appimage/01-in-docker.sh
```

软件包位于 `dist/` 目录下。

创建构建环境的脚本位于 [redpanda-cpp/appimage-builder](https://github.com/redpanda-cpp/appimage-builder)。可用架构：`x86_64`、`x86_64.v3`、`aarch64`、`riscv64`、`loong64`、`i686`。

<!--
# macOS

## Qt.io 的 Qt 库

前置条件：

0. 近期满足 [Qt 5](https://doc.qt.io/qt-5/macos.html) 或 [Qt 6](https://doc.qt.io/qt-6/macos.html) 要求的 macOS 版本。
1. 安装 Xcode 命令行工具：
   ```zsh
   xcode-select --install
   ```
2. 用 [Qt.io](https://www.qt.io/download-qt-installer-oss) 或[镜像站](https://mirrors.sjtug.sjtu.edu.cn/docs/qt)的在线安装器安装 Qt。
   - 选中 Qt 库（“Qt” 组下的 “Qt 5.15.2” 或 “Qt 6.8.0” 小组，勾选 “macOS”）。

要构建此项目，执行下列命令之一：

```zsh
./packages/macos/build.sh -a x86_64 --qt-version 5.15.2
./packages/macos/build.sh -a x86_64 --qt-version 6.8.0
./packages/macos/build.sh -a arm64 --qt-version 6.8.0
./packages/macos/build.sh -a universal --qt-version 6.8.0
```
-->
