# 详细构建指南——符合 freedesktop.org（XDG）规范的桌面系统

## 传统 Unix 方式（`./configure`–`make`–`make install`）

- 安装支持 C++17 的 GCC（≥ 7）或 Clang（≥ 6）。
- 安装 Qt 5.15 或 6.7+ Base、SVG、Tools 模块，包括库和开发文件。
- 如果使用静态版本的 Qt 编译，还要安装 fcitx5-qt。
- 安装 astyle 以便在小熊猫 C++ 中对代码进行重新排版。

### 基于 qmake 构建

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
- `PREFIX`：默认值是 `/usr/local`。打包时应该定义为 `/usr`。
- `LIBEXECDIR`：辅助程序的路径，默认值是 `$PREFIX/libexec`。Arch Linux 使用 `/usr/lib`。
- `LINUX_STATIC_IME_PLUGIN=ON`（make 阶段）：静态链接输入法插件。推荐在使用静态版本的 Qt 编译时启用；**不要**在使用动态版本的 Qt 编译时启用。

### 基于 xmake 构建

1. 配置：
   ```bash
   xmake f -p linux -a x86_64 -m release --qt=/usr --prefix=/usr/local
   ```
2. 构建：
   ```bash
   xmake
   ```
3. 安装：
   ```bash
   sudo xmake install --root -o /  # -o ... 模拟了 make install 的 DESTDIR=...
   ```

提示：`xmake f --help` 可以查看更多选项。

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

## 适用于多架构/版本的 Debian 包

可以在容器环境中构建这些包。支持 Linux 宿主和 Windows 宿主。

```bash
podman run --rm -v $PWD:/mnt -w /mnt --platform linux/amd64 docker.io/debian:12 ./packages/debian/01-in-docker.sh
```

平台（`--platform` 参数）可以是 `linux/amd64`、`linux/386`、`linux/arm64/v8`、`linux/arm/v7`、`linux/riscv64` 等。

映像可以是 `docker.io/debian:12`、`docker.io/debian:11`、`docker.io/ubuntu:24.04`、`docker.io/ubuntu:23.10`、`docker.io/ubuntu:22.04` 等。

可选环境变量：
- `-e MIRROR=mirrors.kernel.org`：APT 镜像站。
- `-e JOBS=4`：make 的并行任务数。

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
