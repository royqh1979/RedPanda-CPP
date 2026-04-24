# 构建 Red Panda C++

有多种方式构建 Red Panda C++：包管理器、独立应用、手动构建。

## 包管理器配方

> 注意：需要 Qt 6.8 或更高版本。推荐使用滚动更新的软件包仓库。

以软件包形式构建的 Red Panda C++ 会与包管理器生态系统集成，使用包管理器提供的工具链和库。

Red Panda C++ 提供了许多包管理器配方的示例，包括 Homebrew、Linux 系统包管理器、MSYS2。

每个包管理器的步骤类似：
1. 准备构建环境；
2. 准备配方和源码；
3. 构建软件包；
4. 安装软件包。

**第 1 步**的详细说明请参阅包管理器文档：[Alpine](https://wiki.alpinelinux.org/wiki/Abuild_and_Helpers)、[Arch](https://wiki.archlinux.org/title/Makepkg)、[Debian 家族](https://wiki.debian.org/BuildingTutorial)、[MSYS2](https://www.msys2.org/dev/new-package/)、[RPM](https://rpm-packaging-guide.github.io/#prerequisites)。

**第 2 步和第 3 步**可以通过以下脚本完成：
- Alpine Linux：`./packages/alpine/buildapk.sh`
- Arch Linux：`./packages/archlinux/buildpkg.sh`
- Debian 家族：`./packages/debian/builddeb.sh`
- Fedora：`./packages/fedora/buildrpm.sh`
- Homebrew：`./packages/brew/buildbottle.sh`
- MSYS2：`./packages/msys/buildpkg.sh`
- openSUSE：`./packages/opensuse/buildrpm.sh`

> 注意：这些构建脚本签出此存储库的 HEAD，因此构建之前务必提交所有变更。

此外，也可以在容器环境中构建 Linux 系统包（推荐使用 rootless Podman；Docker 可能搞乱文件权限）：

```sh
podman run --rm -v $PWD:/mnt -w /mnt <image> ./packages/<distro>/01-in-docker.sh

# 以 Arch Linux 为例
podman run --rm -v $PWD:/mnt -w /mnt docker.io/library/archlinux:latest ./packages/archlinux/01-in-docker.sh
```

**第 4 步**的软件包将放在 `dist/` 目录下。使用包管理器进行安装。

## 独立应用脚本

以独立应用形式构建的 Red Panda C++ 易于部署。

| 操作系统 | 工具链 | Qt | 打包格式 | 架构 |
| -------- | ------ | -- | -------- | ---- |
| Windows XP+ | [MinGW Lite](https://github.com/redpanda-cpp/mingw-lite) | [5.15 with thunks](https://github.com/redpanda-cpp/qtbase-xp) | 安装程序、便携版 | x86_64, i686 |
| Windows 7+ | MSVC | 5.15, Qt.io | 便携版 | x64, x86 |
| Windows 10 1809+ | MSVC | 6.8+, Qt.io | 便携版 | x64, ARM64 |
| Windows 10 1607+ | MSVC | 当前 6.x, vcpkg | 便携版 | x64, ARM64, x86 |
| 2010 年以来的 Linux | [交叉、静态、musl libc](https://github.com/redpanda-cpp/appimage-builder) | 6.10 | AppImage, Debian, tar 包 | x86_64, x86_64.v3, aarch64, riscv64, loong64, i686 |
| 近期的 macOS | Xcode | 6.11+, Qt.io | bundle | universal |

> 警告：不要使用 MinGW-w64 动态链接的 Qt 构建 Red Panda C++。作为绿色版运行时，默认工作目录是 Red Panda C++ 的目录。用户程序的依赖本应从工具链目录加载，但会被 Red Panda C++ 的 dll 覆盖。

### Windows XP

前置条件：

0. Windows 10 x64 或更高版本。
1. 安装 MSYS2。

要构建此项目，启动 MSYS2 环境，然后运行
```bash
./packages/mingw/build-xp.sh -p 64-ucrt
```

可用的配置：64-ucrt、32-ucrt、64-msvcrt（已弃用）、32-msvcrt（已弃用）。

参数：
- `-h`、`--help`：显示帮助信息。
- `-c`、`--clean`：清理构建目录。
- `--mingw32`：把 `assets/mingw32.7z` 添加到包中。
- `--mingw64`：把 `assets/mingw64.7z` 添加到包中。
- `--mingw`：`--mingw32`（x86 程序）或 `--mingw64`（x64 程序）的别名。
- `-t <dir>`、`--target-dir <dir>`：指定输出目录。默认值为 `dist/`。
- `--ucrt`：把 UCRT 安装程序（VC_redist）添加到包中。

> 注意：Windows Server 2003 x64 Edition 已弃用（安装程序不会为 Windows Server 2003 x64 Edition 安装 UCRT；手动安装应该可以）。

### MSVC 工具链

前置条件：

0. Windows 10 x64 或更高版本，或 Windows 11 ARM64。
1. 安装 Visual Studio 2022 或 2026（至少要有 “build tools” 和 “Windows SDK”）。
2. 通过 Qt.io 安装器、aqtinstall 或 vcpkg 安装 Qt 6.8+ 或 5.15。
   - 必需组件：base、svg、tools、translations。
3. PowerShell (Core，不是 Windows PowerShell)。

构建：

1. [启动 Visual Studio 开发环境](https://learn.microsoft.com/zh-cn/visualstudio/ide/reference/command-prompt-powershell?view=visualstudio)：
2. 对于 vcpkg，设置工具链文件（必需）和目标三元组（如果非默认）：
   ```ps1
   $env:CMAKE_TOOLCHAIN_FILE = "$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
   $env:VCPKG_TARGET_TRIPLET = "x64-windows"
   ```
3. 运行构建脚本：
   ```ps1
   ./packages/msvc/build.ps1 -QtDir C:/Qt/6.8.3/msvc2022_64

   # vcpkg (-QtDir 可选，用于 windeployqt)
   ./packages/msvc/build.ps1 -QtDir "$env:VCPKG_ROOT/installed/$env:VCPKG_TARGET_TRIPLET/tools/Qt6"
   ```

参数：
- `-c|-Clean`：清理目录后再构建。
- `-QtDir <dir>`：Qt 库目录。

### Linux 静态包

```bash
podman run --rm -v $PWD:/mnt -w /mnt ghcr.io/redpanda-cpp/appimage-builder-x86_64:20260127.0 ./packages/linux/01-in-docker.sh
```

软件包位于 `dist/` 目录下。

### macOS

前置条件：

0. 近期满足 [Qt 5](https://doc.qt.io/qt-5/macos.html) 或 [Qt 6](https://doc.qt.io/qt-6/macos.html) 要求的 macOS 版本。
1. 安装 Xcode 命令行工具：
   ```zsh
   xcode-select --install
   ```
2. 通过 Qt.io 安装器、aqtinstall 安装 Qt 6.11+。

构建：
```zsh
./packages/macos/build.sh --qt-dir ~/Qt/6.11.0/macos
```

参数：
- `-c`、`--clean`：清理目录后再构建。
- `--qt-dir <dir>`：Qt 库目录。

## 手动构建命令

手动构建 Red Panda C++ 适合用于学习、开发、调试。

前置条件：
- Qt 6.8+ 或 5.15。
- CMake 或 Xmake。

> 提示：要在 Qt Creator 中启用 `lupdate` 功能，在 "Edit > Preferences > Environment > External Tools" 中添加一个外部工具：
> - 执行档：`cmake`；
> - 参数：`--build . --target update_translations`；
> - 工作目录：选择 Global variables 下的 `ActiveProject:BuildConfig:Path`。

### 基于 CMake 构建

1. 配置：
   ```bash
   cmake -S . -B build \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_INSTALL_PREFIX=/usr/local
   ```
2. 构建：
   ```bash
   cmake --build build -- --parallel
   ```
3. 安装：
   ```bash
   sudo cmake --install build --strip
   ```

CMake 变量：
- `CMAKE_INSTALL_PREFIX`：安装路径。
  - 小熊猫C++ 内部使用相对路径，不受影响。
  - XDG `.desktop` 文件受影响。
- `CMAKE_INSTALL_LIBEXECDIR`：辅助程序的路径，**相对于** `CMAKE_INSTALL_PREFIX`。
  - 适用于：层级文件系统布局 (hierarchy)。
  - Arch Linux 和 MSYS2 使用 `lib`。
- `FILESYSTEM_LAYOUT`：`hierarchy`、`flat` 或 `bundle`。
  - `hierarchy` 适用于：所有平台。
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
  - `flat` 适用于：Windows。
    ```
    prefix/
    ├─ RedPandaIDE.exe
    ├─ astyle.exe
    ├─ consolepauser.exe
    └─ templates/
    ```
  - `bundle` 适用于：macOS。
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
  - Windows 默认：`flat`。
  - macOS 默认：`bundle`。
  - 其他平台强制 `hierarchy`。
- `FORCE_QT5`：强制使用 Qt 5。
  - 当 Qt 5 和 Qt 6 安装在相同前缀下时有用。
- `LUA_ADDON`：启用 Lua 附加组件。
- `OVERRIDE_MALLOC`：链接指定的内存分配库。
  - 如 `-DOVERRIDE_MALLOC=mimalloc`。
- `PORTABLE_CONFIG`：`oui`、`non` 或 `registry`。
  - 适用于：Windows。
  - `oui`：是，将配置文件写入小熊猫C++ 的目录。
  - `non`：否，将配置文件写入用户配置目录。
  - `registry`：如果当前实例匹配卸载注册表项则为是，否则为否。
  - Windows 默认：`registry`。
  - 其他平台强制 `non`。
- `SDCC`：启用 SDCC 编译器支持。
- `WINDOWS_PREFER_OPENCONSOLE`：首选兼容 UTF-8 的 `OpenConsole.exe`。
  - 适用于：Windows。
  - `OpenConsole.exe` 是 Windows 终端的组件，在 1.18 版本加入了 UTF-8 输出支持。
  - `OpenConsole.exe` 需要 Windows 10 1809 加入的 ConPTY 接口。

### 基于 xmake 构建

1. 配置：
   ```bash
   xmake f -p linux -a x86_64 -m release --qt=/usr
   ```
2. 构建：
   ```bash
   xmake
   ```
3. 安装：
   ```bash
   sudo xmake install --root -o /usr/local
   ```

xmake 选项：
- `filesystem-layout`：`flat` 或 `hierarchy`。
  - 适用于：Windows。
  - Windows 默认：`flat`。
  - 其他平台强制 `hierarchy`。
- `glibc-hwcaps`：启用伪 `RedPandaIDE`，链接到实际的 `libRedPandaIDE.so`。
  - 适用于：Linux glibc。
  - Glibc 会自动从 glibc-hwcaps 子目录加载最佳版本，如 `/usr/lib/glibc-hwcaps/x86-64-v3/libRedPandaIDE.so`。
  - 构建脚本没有特别处理。需要使用不同的 `-march` 标志构建多次并安装到相应目录。
- `libexecdir`：辅助程序的**相对**路径。
  - 适用于：层级文件系统布局 (hierarchy)。
  - Arch Linux 和 MSYS2 使用 `lib`。
- `lua-addon`：启用 Lua 附加组件。
- `portable-config`：`oui`、`non` 或 `registry`。
  - 适用于：Windows。
  - `oui`：是，将配置文件写入小熊猫C++ 的目录。
  - `non`：否，将配置文件写入用户配置目录。
  - `registry`：如果当前实例匹配卸载注册表项则为是，否则为否。
  - Windows 默认：`registry`。
  - 其他平台强制 `non`。
- `prefix`：小熊猫C++ 的启动位置。
  - 适用于：XDG。
  - 与 CMake 变量 `CMAKE_INSTALL_PREFIX` 不同，此选项与安装目录无关，只影响 XDG `.desktop` 文件。
- `sdcc`：启用 SDCC 编译器支持。

## 平台特定说明

### Windows ARM

- 小熊猫C++ 只能在 Windows 11 ARM64 上构建 ARM64 版。
  - 在 Windows 10 上运行不再受支持。安装程序假设 x64 仿真始终可用。（原生的 “带 LLVM MinGW 工具链的小熊猫C++” 应该能用。）
  - 不支持 ARM64EC（“仿真兼容”）主机，即不能用 ARM64EC 工具链构建小熊猫 C++。
  - （理论上）支持 ARM64EC 目标，也就是说，如果上游工具链支持 ARM64EC，那么小熊猫C++ 可以构建 ARM64EC 程序和库。
- 随着 [Windows 11 Insider Preview Build 25905 弃用 ARM32](https://blogs.windows.com/windows-insider/2023/07/12/announcing-windows-11-insider-preview-build-25905/)，小熊猫 C++ 今后也不会添加 ARM32 支持了。

### Linux 异架构

有两种方式编译异架构的小熊猫C++：
- 交叉构建：用交叉工具链。
  - 和本机构建一样快；
  - 构建 Qt 并不容易；
  - 如果要运行测试，仍要安装 QEMU 用户空间模拟器。
- 模拟本机构建 (emulated native build)：借助 QEMU 用户空间模拟，运行目标架构的本机工具链。
  - 和本机构建一样简单；
  - 非常慢（耗时可能高达 10 倍）。

#### 交叉构建

遵循 [CMake 的交叉编译指南](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Cross%20Compiling%20With%20CMake.html)。如果要运行测试用例，可以设置 `CMAKE_CROSSCOMPILING_EMULATOR`。

[AppImage 构建环境](https://github.com/redpanda-cpp/appimage-builder)是一个案例，展示了如何自举基于 musl 的静态交叉工具链和 Qt。

#### 模拟本机构建

除了要安装 QEMU 用户空间模拟器外，和本机构建一样。

注意：始终**在 chroot 环境、容器或 jail 中**运行模拟本机构建，因为混用不同架构的程序和库可能会损坏系统。

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
