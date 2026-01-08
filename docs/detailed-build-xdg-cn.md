# 详细构建指南——符合 freedesktop.org（XDG）规范的桌面系统

## 传统 Unix 方式（`./configure`–`make`–`make install`）

- 安装支持 C++17 的 GCC 或 Clang。
- 安装 Qt 6.8+ Base、SVG、Tools 模块，包括库和开发文件。
- 如果使用静态版本的 Qt 编译，还要安装 fcitx5-qt。
- 安装 astyle 以便在小熊猫 C++ 中对代码进行重新排版。

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

qmake 变量:
- `CMAKE_INSTALL_PREFIX`：`$MAKE install` 的安装路径。
  - 小熊猫C++ 内部使用相对路径，不受影响。
  - `.desktop` 文件受影响。
- `LIBEXECDIR`：辅助程序的路径，**相对于 `CMAKE_INSTALL_PREFIX`**。
  - Arch Linux 使用 `lib`。
- `OVERRIDE_MALLOC`：链接指定的内存分配库。如 `-DOVERRIDE_MALLOC=mimalloc`。

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

提示：`xmake f --help` 可以查看更多选项。

## 异架构

有两种方式编译异架构的小熊猫C++：
- 交叉构建：用交叉工具链。
  - 和本机构建一样快；
  - 构建 Qt 并不容易；
  - 如果要运行测试，仍要安装 QEMU 用户空间模拟器。
- 模拟本机构建 (emulated native build)：借助 QEMU 用户空间模拟，运行目标架构的本机工具链。
  - 和本机构建一样简单；
  - 非常慢（耗时可能高达 10 倍）。

### 交叉构建

遵循 [CMake 的交叉编译指南](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Cross%20Compiling%20With%20CMake.html)。如果要运行测试用例，可以设置 `CMAKE_CROSSCOMPILING_EMULATOR`。

[AppImage 构建环境](https://github.com/redpanda-cpp/appimage-builder)是一个案例，展示了如何自举基于 musl 的静态交叉工具链和 Qt。

### 模拟本机构建

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
