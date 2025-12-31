# 详细构建指南——符合 freedesktop.org（XDG）规范的桌面系统

## 传统 Unix 方式（`./configure`–`make`–`make install`）

- 安装支持 C++17 的 GCC 或 Clang。
- 安装 Qt 6.8+ Base、SVG、Tools 模块，包括库和开发文件。
- 如果使用静态版本的 Qt 编译，还要安装 fcitx5-qt。
- 安装 astyle 以便在小熊猫 C++ 中对代码进行重新排版。

### 基于 CMake 构建

1. 配置：
   ```bash
   cmake -S /path/to/src -B /path/to/build \
     -G "Unix Makefiles" \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_INSTALL_PREFIX=/usr/local
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
- `CMAKE_INSTALL_PREFIX`：`$MAKE install` 的安装路径。
  - 小熊猫C++ 内部使用相对路径，不受影响。
  - `.desktop` 文件受影响。
- `LIBEXECDIR`：辅助程序的路径，**相对于 `CMAKE_INSTALL_PREFIX`**。
  - Arch Linux 使用 `lib`。

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

<!--
### Debian/Ubuntu 的傻瓜式指南

```bash
# 准备工作
apt install gcc g++ make git gdb gdbserver astyle qterminal # 安装构建工具和运行时工具
apt install qtbase5-dev qttools5-dev-tools libqt5svg5-dev   # 安装开发头文件和库
git clone https://gitee.com/royqh1979/RedPanda-CPP.git      # 获取源代码

# 构建
mkdir -p RedPanda-CPP/build && cd RedPanda-CPP/build        # 创建构建目录
qmake ../Red_Panda_CPP.pro                                  # 配置
make -j$(nproc)                                             # 构建
sudo make install                                           # 安装

# 运行
RedPandaIDE
```
-->

## 异架构的模拟本机构建（emulated native build）

可以借助 QEMU 用户空间模拟，运行目标架构的本机工具链，来构建小熊猫C++。

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
