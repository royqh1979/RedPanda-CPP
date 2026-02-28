# 详细构建指南——Windows

| 库 + 工具链 \ 目标 | x86 | x64 | ARM64 |
| ------------------ | --- | --- | ----- |
| MSYS2 + 基于 GNU 的 MinGW | ❌ | ✔️ | ❌ |
| MSYS2 + 基于 LLVM 的 MinGW | ❌ | ✔️ | ✔️ |
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
- 小熊猫C++ 只能在 Windows 11 ARM64 上构建 ARM64 版。
  - 在 Windows 10 上运行不再受支持。安装程序假设 x64 仿真始终可用。（原生的 “带 LLVM MinGW 工具链的小熊猫C++” 应该能用。）
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
