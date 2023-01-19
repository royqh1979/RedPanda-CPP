# 依赖
 
 小熊猫C++需要Qt 5(>=5.12)

# Windows

 我使用msys2打包的最新版的GCC和MinGW-w64工具链来编译小熊猫C++。VC和其他版本的gcc不一定能够正常编译。

 编译步骤：
 - 安装msys2 (https://www.msys2.org)
 - 使用msys2的pacman程序安装mingw-w64-x86_64-qt5和mingw-w64-x86_64-gcc
 - 安装qtcreator
 - 使用qtcreator打开Red_Panda_CPP.pro文件

# Linux

步骤:
 - 安装 gcc 和 qt5开发相关包
 - 使用qtcreator打开Red_Panda_CPP.pro文件

qmake 变量:
- `PREFIX`：默认值是 `/usr/local`。打包时应该定义为 `/usr` 或 `/opt/redpanda-cpp`。
- `LIBEXECDIR`：辅助程序的路径，默认值是 `$PREFIX/libexec`。Arch Linux 使用 `/usr/lib`。
- `XDG_ADAPTIVE_ICON=ON`：遵循 [freedesktop.org 图标主题规范](https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html)安装图标，以适应不同的主题和尺寸。AppImage 需要启用此项；Linux 打包 `PREFIX=/usr` 时推荐启用此项。

## Ubuntu

### 1.安装编译器

```bash
apt install gcc g++ make gdb gdbserver
```

### 2.安装QT5和依赖包

```bash
apt install qtbase5-dev qttools5-dev-tools libicu-dev libqt5svg5-dev git qterminal
```

### 3.下载源码

```bash
git clone https://gitee.com/royqh1979/RedPanda-CPP.git
```

### 4.编译

```bash
cd RedPanda-CPP/
qmake Red_Panda_CPP.pro
make -j$(nproc)
sudo make install
```

### 5.运行

```bash
RedPandaIDE
```

## Arch Linux 及衍生版本

`packages/archlinux` 目录下提供了一个参考 PKGBUILD，使用 [makepkg](https://wiki.archlinuxcn.org/wiki/Makepkg) 构建小熊猫 C++ 并安装。

小熊猫 C++ 可以通过 `RedPandaIDE` 命令启动。

注意：makepkg 签出此存储库的 HEAD，因此构建之前务必提交所有变更。

## AppImage

1. 安装依赖包：cURL、Docker。

   Windows 宿主的额外要求：
   - Docker 使用基于 WSL 2 的引擎，或者对此项目文件夹启用文件共享（Settings > Resources > File sharing）；
   - PowerShell（曾用名 “PowerShell Core”，不是 “Windows PowerShell”）。
2. 准备构建环境。Linux 宿主：
   ```bash
   arch=x86_64 # 或 aarch64
   curl -L -o packages/appimage/dockerfile-$arch/appimagetool-$arch.AppImage https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-$arch.AppImage
   docker build -t redpanda-builder-$arch packages/appimage/dockerfile-$arch
   ```
   Windows 宿主：
   ```ps1
   $arch = "x86_64" # 或 "aarch64"（如果将来 Docker 支持 WoA）
   Invoke-WebRequest -OutFile packages/appimage/dockerfile-$arch/appimagetool-$arch.AppImage -Uri https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-$arch.AppImage
   docker build -t redpanda-builder-$arch packages/appimage/dockerfile-$arch
   ```
3. 构建 AppImage。Linux 宿主：
   ```bash
   ./packages/appimage/build-x86_64.sh # 或 *-aarch64.sh
   ```
   Windows 宿主：
   ```ps1
   ./packages/appimage/build-x86_64.ps1 # 或 *-aarch64.ps1（如果将来 Docker 支持 WoA）
   ```
4. 运行小熊猫 C++.
   ```bash
   ./dist/RedPandaIDE-x86_64.AppImage # 或 *-aarch64.AppImage
   ```

注意：AppImage 与 QEMU 用户态模拟不兼容，使用此格式的 AppImageKit 工具自然不能用 QEMU 用户态模拟来运行。因此不能在 x86-64 系统上构建 AArch64 AppImage，反之亦然。
