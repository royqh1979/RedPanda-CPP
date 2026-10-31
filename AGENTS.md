# Red Panda C++ — Agent 指南

## 项目概览

Red Panda C++（旧名：Red Panda Dev-C++ 7）是一款快速、轻量、开源的跨平台 C/C++/GNU 汇编 IDE，使用 C++17 和 Qt 框架（Qt 6.8+ 或 Qt 5.15）编写，运行于 Windows、Linux 和 macOS。许可证：GPLv3+（见 `LICENSE`）。作者：Roy Qu（royqh1979@gmail.com）。

主要功能：基于自研 C++ 解析器的代码编辑/补全、编译/运行/调试（GDB/MI 和 DAP）、OJ 题目的问题集与 Competitive Companion 支持、汇编（GAS/NASM）与 SDCC 支持、可选的 Lua 插件、可选的 Git 集成。

版本号定义在 `version.json`（当前为 3.5.0-RC1）；构建号在配置阶段由 `git rev-list HEAD --count` 生成。翻译维护在 `RedPandaIDE/translations/`（pt_BR、ru_RU、zh_CN、zh_TW），并以 Qt 资源形式嵌入二进制。

## 仓库结构

- `RedPandaIDE/` — 主 IDE 应用（目标名 `RedPandaIDE`）。
  - `src/` — 应用源码，按功能分区：
    - `parser/` — C++ 词法分析器、预处理器和解析器（代码补全引擎）。
    - `compiler/` — 编译/运行流水线（`Compiler`、`Runner`、文件/项目编译器、NASM/SDCC 变体、OJ 题目用例运行器）。
    - `debugger/` — GDB/MI 和 DAP 调试器后端。
    - `problems/` — 问题集 / Competitive Companion 支持。
    - `reformatter/` — 代码格式化（AStyle）。
    - `settings/` 和 `settingsdialog/` — 设置模型类与设置对话框控件。
    - `widgets/` — 自定义控件与对话框（通常是 `.cpp`/`.h`[`.ui`] 三件套）。
    - `utils/` — 文件、字体、操作系统、终端、转义等辅助工具。
    - `addon/` — Lua 插件运行时集成（仅在启用 `LUA_ADDON` 时构建）。
    - `vcs/` — Git 集成（仅在启用 `VCS` 时构建）。
  - `test/` — 单元测试（基于 Qt Test）：`test-editor` 和 `test-cppparser` 两个测试程序。
  - `resources/`、`images/`、`*.qrc` — 嵌入资源（图标、主题、配色方案、字体、代码片段、项目模板）。
  - `translations/` — `.ts` 翻译文件。
- `libs/qsynedit/` — 代码编辑器组件（QSynEdit 的深度改造分支）：文档模型、绘制器、行号栏、代码折叠、搜索器、导出器，以及语法高亮器（cpp、asm/gas/nasm、lua、makefile、glsl）。自带 Qt Test 测试套件，位于 `libs/qsynedit/test/`。
- `libs/redpanda_qt_utils/` — 小型共享 Qt 工具库（字符集检测、杂项工具）。
- `libs/lua/` — 内嵌的 Lua 5.4.6（必须与上游 `lua/lua` v5.4.6 保持完全一致；CI 会强制检查）。
- `tools/consolepauser/` — 辅助程序，用于在用户程序退出后保持控制台窗口不关闭。
- `tools/redpanda-git-askpass/`、`tools/redpanda-win-git-askpass/` — git askpass 辅助程序（仅在启用 `VCS` 时构建）。
- `tools/gdb-scripts/` — gdb pretty-printer / 辅助脚本。
- `addon/` — Lua 插件支持：`defs/` 中的 Teal（`.tl`）API 类型定义（见 `docs/addon.md`）、示例插件（`theme/`、`compiler_hint/`），以及用于生成插件代码的 `gen.sh`（CI 会检查其一致性）。
- `assets/` — 打包用的捆绑工具链/资源（astyle、MinGW 发行版）。
- `packages/` — 打包配方与构建脚本：`mingw/`（Windows XP+ NSIS/便携版）、`msvc/`（Windows MSVC 便携版）、`linux/`（静态 AppImage）、`macos/`（bundle），以及各发行版配方（`alpine/`、`archlinux/`、`archlinux-hwcaps/`、`brew/`、`debian/`、`fedora/`、`msys/`、`opensuse/` 和共享的 `_common/`）。
- `platform/` — 各平台支持文件（模板、.desktop 文件、图标、macOS bundle 资源）。
- `tests/` — 手工测试用的零散示例文件（如 `test.asm`、`untitled1.cpp`、`format/`）；不是自动化测试套件。
- `docs/` — 设计/开发笔记：`architecture.md`（整体架构设计，以代码现状为准）；`addon.md`、`qsynedit-redesign.txt` 已过期，仅供参考。

## 构建

支持两套构建系统，且都在 CI 中执行：**CMake**（主，根目录 `CMakeLists.txt`）和 **Xmake**（根目录 `xmake.lua`）。qmake 工程文件（`*.pro`、`.qmake.conf`）仍然存在但属于遗留。完整指南见 `BUILD.md`；要点如下：

```sh
# CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Xmake
xmake f -p linux -a x86_64 -m release --qt=/usr
xmake
```

前置条件：Qt 6.8+ 或 Qt 5.15（组件：Core、Gui、LinguistTools、Network、PrintSupport、Svg、Test、Widgets、Xml；macOS 上还需 GuiPrivate）、CMake ≥ 3.19 或 Xmake，以及 C++17 编译器。

重要的 CMake 选项（见根目录 `CMakeLists.txt`）：
- `LUA_ADDON`（默认 OFF）— Lua 插件支持；会引入 `libs/lua`。
- `VCS`（默认 OFF）— Git 集成；增加 `src/vcs/*` 和 askpass 工具。
- `SDCC`（默认 ON）— SDCC 编译器支持。
- `FORCE_QT5` — 同时安装了 Qt 5 和 Qt 6 时强制使用 Qt 5。
- `FILESYSTEM_LAYOUT` — `hierarchy`（Linux 默认）、`flat`（Windows 默认）、`bundle`（macOS 默认）。
- `PORTABLE_CONFIG` — `oui`/`non`/`registry`（Windows 配置位置行为）。
- `MAKE_INTERFACE` — `unix` 与 `mingw32`（IDE 调用哪个 make）。
- `OVERRIDE_MALLOC` — 链接如 mimalloc 之类的分配器。
- `WINDOWS_PREFER_OPENCONSOLE` — Windows 上优先使用 OpenConsole.exe。

新增受特性开关控制的源文件时，必须同时登记在 `RedPandaIDE/CMakeLists.txt` 中对应的 `if(...)` 块内，**以及** `RedPandaIDE/xmake.lua` / 根目录 `xmake.lua` 中，以保持两套构建系统同步。

## 测试

测试使用 Qt Test（`Qt::Test`）。测试程序：`test-cppparser`、`test-editor`（均位于 `RedPandaIDE/test/`），以及 qsynedit 测试套件（`libs/qsynedit/test/`）。CTest 已在根目录 `CMakeLists.txt` 中接好。

```sh
# CMake：构建全部测试目标，然后运行（GUI 测试使用 offscreen）
cmake --build build --target all-test-targets --parallel
cd build && env CTEST_OUTPUT_ON_FAILURE=1 QT_QPA_PLATFORM=offscreen make test

# Xmake：构建并运行测试
xmake test
```

CI（`.github/workflows/unit.yml`）在 Ubuntu + Qt 5 上运行的正是上述步骤。它还强制检查：
- `addon/gen.sh` 的输出已提交（运行后 `git diff --exit-code` 必须干净）。
- `libs/lua/lua/` 与上游 Lua v5.4.6 完全一致。

打包工作流：`.github/workflows/build.yml`（发行版包）、`build-app.yml`（独立应用）、`feature-matrix.yml`、`winget.yml`。

## 代码风格与约定

- C++17、C11；全面使用 Qt 惯用法（signals/slots、用户可见字符串用 `tr()`、`QObject` 父子对象所有权）。
- 仓库**没有** `.clang-format`/`.editorconfig`；请与所在文件的现有风格保持一致。项目内部附带了自己的格式化配置（AStyle 捆绑在 `assets/astyle`），但仓库范围内没有统一的格式化配置。
- 文件使用 GPL v3+ 版权头（模板见任意现有 `src/*.cpp`）。
- `RedPandaIDE/src` 中的新类通过根目录 `CMakeLists.txt` 定义的辅助函数注册到 `RedPandaIDE/CMakeLists.txt`：`target_qt_plain_cpp`（纯 `.cpp/.h`）、`target_moc_classes`（需要 moc 的 QObject 类）、`target_ui_classes`（带 `.ui` 文件）。每个只列不含扩展名的源文件基名。
- 翻译：用户可见字符串走 `tr()`；`.ts` 文件位于 `RedPandaIDE/translations/`。Qt 5 下通过 `update_translations` 目标更新（`cmake --build . --target update_translations`）；Qt 6 下由 `qt_add_translations` 处理。
- 资源通过 `target_embed_resources`/`target_embed_translations` CMake 辅助函数嵌入，这些集合不手写 `.qrc`。
- 注释与文档使用英文；新写的注释和文档也请使用英文。
- 特性代码用 `ENABLE_LUA_ADDON`、`ENABLE_SDCC`、`ENABLE_VCS` 宏做条件编译——相应的 `#include` 也要加保护。
- Lua 插件 API 用 Teal 标注类型；API 定义位于 `addon/defs/*.d.tl`，文档见 `docs/addon.md`。插件 API 的版本规则很严格——修改接口前请先阅读 `docs/addon.md`。

## 安全注意事项

- 绝不提交密钥；`.gitignore` 已排除构建产物（`build/`、`dist/`、`.xmake/`）。
- IDE 会执行外部工具（编译器、gdb、用户程序）并运行 Lua 插件——对路径和命令行要格外小心；使用 `src/utils/escape.*` 中现有的转义工具。
- `libs/lua/` 是内嵌的上游代码——不要在本地打补丁；CI 会将其与上游 v5.4.6 做 diff。
- 本仓库构建出的二进制会运行不可信的用户代码（这正是本产品的用途）；新增网络或执行能力时须考虑沙箱/信任问题。

## 给 Agent 的备注

- `BUILD.md`（以及 `BUILD_cn.md`/`BUILD_ru.md`）是权威的构建参考——在自创构建步骤之前请先参考它。
- `NEWS.md` 是更新日志；查询功能历史时可参考。
- 当修改了上文描述的任何内容（构建选项、测试命令、目录职责）时，请同步更新本 `AGENTS.md`。
