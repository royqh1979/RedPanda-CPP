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
    - `search/` — 文件枚举与批量搜索、已选搜索结果替换。
    - `refactor/` — 语义引用查找、重载/覆盖与模板感知的重命名。
    - `settings/` 和 `settingsdialog/` — 设置模型类与设置对话框控件。
    - `widgets/` — 自定义控件与对话框（通常是 `.cpp`/`.h`[`.ui`] 三件套）。
    - `utils/` — 文件、字体、操作系统、终端、转义等辅助工具。
    - `addon/` — Lua 插件运行时集成（仅在启用 `LUA_ADDON` 时构建）。
    - `vcs/` — Git 集成（仅在启用 `VCS` 时构建）。
  - `test/` — 单元测试（基于 Qt Test）：`test-editor`、`test-widgets`、`test-file-search`、`test-refactor` 和包含全部 Parser 组件及集成 Suite 的 `test-cppparser`。
  - `resources/`、`images/`、`*.qrc` — 嵌入资源（图标、主题、配色方案、字体、代码片段、项目模板）。
  - `translations/` — `.ts` 翻译文件。
- `libs/qsynedit/` — 代码编辑器组件（QSynEdit 的深度改造分支）：文档模型、绘制器、行号栏、代码折叠、搜索器、导出器，以及语法高亮器（cpp、asm/gas/nasm、lua、makefile、glsl）。自带 Qt Test 测试套件，位于 `libs/qsynedit/test/`。
- `libs/redpanda_qt_utils/` — 小型共享 Qt 工具库（字符集检测、杂项工具）。
- `libs/lua/` — 内嵌的 Lua 5.4.6（必须与上游 `lua/lua` v5.4.6 保持完全一致；CI 会强制检查）。
- `tools/consolepauser/` — 辅助程序，用于在用户程序退出后保持控制台窗口不关闭。
- `tools/redpanda-git-askpass/`、`tools/redpanda-win-git-askpass/` — git askpass 辅助程序（仅在启用 `VCS` 时构建）。
- `tools/gdb-scripts/` — gdb pretty-printer / 辅助脚本。
- `addon/` — Lua 插件支持：`defs/` 中的 Teal（`.tl`）API 类型定义、示例插件（`theme/`、`compiler_hint/`），以及用于生成插件代码的 `gen.sh`（CI 会检查其一致性）。
- `assets/` — 打包用的捆绑工具链/资源（astyle、MinGW 发行版）。
- `packages/` — 打包配方与构建脚本：`mingw/`（Windows XP+ NSIS/便携版）、`msvc/`（Windows MSVC 便携版）、`linux/`（静态 AppImage）、`macos/`（bundle），以及各发行版配方（`alpine/`、`archlinux/`、`archlinux-hwcaps/`、`brew/`、`debian/`、`fedora/`、`msys/`、`opensuse/` 和共享的 `_common/`）。
- `platform/` — 各平台支持文件（模板、.desktop 文件、图标、macOS bundle 资源）。
- `tests/` — 手工测试用的零散示例文件（如 `test.asm`、`untitled1.cpp`、`format/`）；不是自动化测试套件。
- `docs/` — 设计/开发笔记：`architecture.md`（整体架构设计，以代码现状为准）、`qsynedit-design.md`（QSynEdit 组件设计）、`qsynedit-wordwrap-plan.md`（全局映射/自动换行重构计划：段数前缀树 + 懒推导锚点）、`cpppreprocessor-optimization.md`（跨边界 replay、Session/Registry 与缓存设计）、`cpppreprocessor-replay-scenarios.md`（replay 场景、正确性规则与测试清单）、`cppparser-design.md`（CppParser、Tokenizer、类型推导与 IDE 查询设计）、`cppnamespace-entity-using-design.md`（唯一命名 namespace、InlineAliasStatement 投影、occurrence 与 StatementFactory 设计）、`cpptemplate-evaluation-design.md`（仅在表达式求值时进行的有限模板实例化设计）、`parser-concurrency-review.md`（CppParser 并发约定与待审查事项）、`search-replace-design.md`（搜索替换、文件搜索与已选结果替换设计）、`qsynedit-incremental-layout-design.md`（增量布局与 max width 更新设计）、`qsynedit-fold-editing-design.md`（折叠显示、光标与编辑规则设计）、`qsynedit-edit-performance-report.md`（Document 与 Document+View 编辑性能热点和优化建议）、`qsynedit-edit-performance-baseline.md`（编辑性能完整基线）、`qsynedit-edit-performance-tools.md`（编辑 profile 配套脚本与使用方法）、`qsynedit-pending-layout-incremental-design.md`（pending 行集合增量维护设计）和 `qsynedit-pending-layout-incremental-result.md`（pending 调度优化实施与收益结果）。

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
- `REDPANDA_PARSER_STDLIB_TEST`（默认 OFF）— 启用标准库 Parser 验收测试；需同时把 `REDPANDA_PARSER_STDLIB_MINGW_ROOT` 设为工具链根目录。本机 GCC 15.3 使用 `D:/mingw`；Clang 23.1.5 + libc++ 使用 `E:/workspaces/contributes/mingw/llvm-mingw-20261104-ucrt-x86_64`。运行时可用 `REDPANDA_PARSER_TEST_CXX_STANDARD` 指定实际编译器的 `-std=` 参数（默认 `c++17`）；测试根据 Parser 从 `__cplusplus` 取得的版本选择场景。启用时 CTest 会设置 `LC_ALL=C`，确保编译器 `-v` 输出使用固定的英文标记。标准库接口验收优先使用 Release 构建，失败或需要断言、栈信息时再切换 Debug，避免 Debug 解析耗时干扰定位。
- `REDPANDA_PARSER_DECLARATION_PROFILE`（默认 OFF）— 编译声明解析的细分计时、self-time scope 栈和计数。普通构建使用空 scope，不引入逐声明插桩成本；需要 `DECLARATION_PROFILE`/`DECLARATION_SELF` 输出时设为 ON，并在运行时设置 `REDPANDA_PARSER_PROFILE=1`。整体 `PARSE_TIMING` 及 preprocess/tokenize/parseDeclarations 等阶段计时不受此开关影响。
- `QSYNEDIT_BUILD_PROFILE_TESTS`（默认 OFF）— 编译 QSynEdit 的 Syntaxer、Document 编辑和 Document+View 编辑 profiling 用例。普通测试构建不包含这些用例；开启后仍需设置各 profile 对应的运行时环境变量才会执行。

新增受特性开关控制的源文件时，必须同时登记在 `RedPandaIDE/CMakeLists.txt` 中对应的 `if(...)` 块内，**以及** `RedPandaIDE/xmake.lua` / 根目录 `xmake.lua` 中，以保持两套构建系统同步。

## 测试

测试使用 Qt Test（`Qt::Test`）。测试程序：`test-cppparser`、`test-editor`、`test-widgets`、`test-file-search`、`test-refactor`（均位于 `RedPandaIDE/test/`），以及 qsynedit 测试套件（`libs/qsynedit/test/`）。CTest 已在根目录 `CMakeLists.txt` 中接好。Parser 组件和集成测试统一由 `test-cppparser` 构建执行，不再重复构建独立 tokenizer 目标。

测试必须按被测组件的职责放入对应 Suite 和同名测试文件，不得因为某目标已经链接所需实现、编译方便而随意混放。`test-cppparser` 内使用 `TestCppTokenizer`、`TestCppDeclarationParser`、`TestCppStatementBuilder`、`TestCppTypeResolver`、`TestCppExpressionEvaluator`、`TestCppParser`、`TestCppParserInvalidation`、`TestCppParserSyntax`、`TestCppParserStandardLibrary` 及既有 Preprocessor Suite；可用 `--suite=类名` 单独运行。QSynEdit/Syntaxer 测试属于 `test-qsynedit` 中对应的测试类，Editor 测试属于 `test-editor`，Widget 测试属于 `test-widgets`，文件搜索与已选结果替换测试属于 `test-file-search`，语义引用与重命名测试属于 `test-refactor`。

```sh
# CMake：构建全部测试目标，然后运行（CTest 自动设置无头运行环境）
cmake --build build --target all-test-targets --parallel
cd build && ctest --output-on-failure

# Xmake：构建并运行测试
xmake test
```

完整回归必须先用 Release 构建运行全部 CTest，再用 Debug 构建运行全部 CTest，顺序不得反过来。Release 先验证功能并快速覆盖标准库场景，Debug 再检查断言和调试期不变量；两种构建均通过才算完整回归通过。

CI（`.github/workflows/unit.yml`）在 Ubuntu + Qt 5 上运行的正是上述步骤。它还强制检查：
- `addon/gen.sh` 的输出已提交（运行后 `git diff --exit-code` 必须干净）。
- `libs/lua/lua/` 与上游 Lua v5.4.6 完全一致。

## Windows 本机构建与测试（开发机）

以下为本机（Windows + Git Bash）的既定做法，新会话直接照用：

- 工具链：编译器 `d:/qt/mingw64`；Qt 用 `d:/qt/qt5-debug-64`（Qt 5.15 调试版）。clangd 在 `d:/tools/` 下，代码导航时可用。
- 构建目录放内存虚拟盘 `R:`（减少固态硬盘损耗；**重启后 R: 内容清空，需重新配置构建**）：

```sh
mkdir -p /r/build-redpanda-qt5-debug && cd /r/build-redpanda-qt5-debug
PATH="/d/qt/mingw64/bin:$PATH" cmake -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/d/qt/qt5-debug-64 \
  -DFORCE_QT5=ON -DQSYNEDIT_BUILD_MULTIVIEW_DEMO=ON \
  /e/workspaces/contributes/RedPanda-CPP
```

- CTest 自动为测试设置 `QT_QPA_PLATFORM=offscreen`；检测到 Windows 字体目录时，还会将 `QT_QPA_FONTDIR` 指向该目录，避免本机 Qt 安装缺少 `lib/fonts` 时反复报告字体目录 warning。这些设置只影响测试进程，不修改 Qt 安装。直接运行测试 exe 时仍需自行设置这些环境变量。

- **test-qsynedit 优先直接运行**，并设置 `QT_QPA_PLATFORM=offscreen`、`QT_QPA_FONTDIR=C:/Windows/Fonts` 和 `QT_FORCE_STDERR_LOGGING=1`。当前构建已验证可直接运行并通过；只有出现无输出退出、崩溃或需要栈信息时才使用 gdb：

```sh
cd /r/build-redpanda-qt5-debug/libs/qsynedit
PATH="/d/qt/mingw64/bin:/d/qt/qt5-debug-64/bin:$PATH" \
  QT_QPA_PLATFORM=offscreen QT_QPA_FONTDIR=C:/Windows/Fonts \
  QT_FORCE_STDERR_LOGGING=1 ./test-qsynedit.exe
```

  本机调试版 Qt 没有 `lib/fonts`；offscreen 后端否则会反复报告字体目录 warning。
  `QT_QPA_FONTDIR=C:/Windows/Fonts` 只影响测试进程的字体查找，不修改 Qt 安装。

- **QSynEdit profiling 编译开关**：以下 Syntaxer、增量布局和编辑矩阵用例只在配置时启用 `QSYNEDIT_BUILD_PROFILE_TESTS=ON` 后编译；普通 `all-test-targets` 构建保持 OFF。

- **Syntaxer profiling**：Release 下设置 `QSYNEDIT_PROFILE_FILE`，直接运行 `test-qsynedit` 的 `TestCppSyntaxer`、`TestASMSyntaxer` 或 `TestTextSyntaxer` 中 `test_profile_file`，例如 `QSYNEDIT_PROFILE_FILE=resources/miniaudio.h ./test-qsynedit.exe --suite=TestCppSyntaxer test_profile_file`（大文件已收入 `libs/qsynedit/test/resources/`，构建时复制到测试目录，也可供后续 QSynEdit 编辑性能检查复用）。文件读取不计时；用例分别跑三轮纯扫描和三轮包含 `getToken()` 的遍历，以 `SYNTAXER_PROFILE` 行输出纳秒、token 数和字符数。短文件可用 `QSYNEDIT_PROFILE_REPEAT=10000` 在计时前展开；未设置文件变量时正常测试自动跳过。

- **Document / Document+View 编辑 profiling**：Release 下设置 `QSYNEDIT_EDIT_PROFILE=1`，分别运行 `test-qsynedit --suite=TestDocumentProfile test_edit_matrix` 和 `test-qsynedit --suite=TestQSyneditDisplay test_miniaudio_edit_profile`。两者基于 `miniaudio.h`，覆盖修改、插入、删除、替换、连续行块移动 × 1/10/100/1000/10000 行 × 普通/折叠边界变化，并额外比较修改的前/中/后位置。普通与折叠负载只有末尾分隔符不同，token 主体和显示宽度一致。`QSYNEDIT_EDIT_PROFILE_REPEAT` 控制每场景采样数（默认 3）；View 可用 `QSYNEDIT_EDIT_PROFILE_FOLDING=0` 做关闭折叠维护的对照实验。输出 CSV 风格的 `DOCUMENT_EDIT_PROFILE` 和 `VIEW_EDIT_PROFILE` 行；View 同时记录同步编辑及后台布局完成后的总耗时。

- **Search profiling**：Release 下设置 `QSYNEDIT_SEARCH_PROFILE=1`，直接运行 `test-file-search --suite=TestFileSearchEngine test_profile`；输出普通、整词、regex、长行、目录枚举及文件读取匹配的纳秒耗时和匹配数，未设置开关时正常测试自动跳过。

- **Incremental layout profiling**：Release 下设置 `QSYNEDIT_INCREMENTAL_LAYOUT_PROFILE=1`，直接运行 `test-qsynedit --suite=TestQSyneditDisplay test_miniaudio_single_line_edit_profile`；在 `miniaudio.h` 上执行 20 次单行插入/修改/删除，逐轮断言不创建后台 Job，并核对最终索引及滚动范围通知，未设置开关时正常测试自动跳过。

- **STL 单屏绘制 profiling**：`REDPANDA_PARSER_STDLIB_TEST` 也会为 `test-editor` 提供标准库配置。Release 下设置 `REDPANDA_STL_PAINT_PROFILE=1`，直接运行 `test-editor --suite=TestEditorDisplay test_stl_paint_profile`；`REDPANDA_STL_PAINT_PROFILE_REPEAT` 控制采样轮数（默认 9）。测试固定每屏 80 行、关闭 word-wrap，分别输出逐 identifier 表达式提取、整行拆分、statement 求值、求值单元数、冷 paint 及同 revision 热 paint 的纳秒中位数；未设置开关时正常测试自动跳过。

- **Painter self-time profiling**：在上述测试中额外设置 `QSYNEDIT_PAINT_PROFILE=1`，进程结束时以 `PAINTER_SELF` 输出各阶段的累计 self-time（毫秒）和调用次数；嵌套阶段扣除直接子阶段耗时。该数据用于定位热点，最终帧耗时应关闭此开关后测量。

- **崩溃时取栈**：`gdb -batch -ex run -ex bt --args ./test-qsynedit.exe [参数]`（`-ex` 顺序必须是先 run 后 bt；对非主线程的崩溃，停止后用 `-ex "thread apply all bt"` 打印所有线程）。注意栈地址异常（如数据指针为野值）通常意味着更早前已发生堆/栈破坏，崩溃只是案发点，应向前追查写坏内存的代码，而不是只修崩溃处。

- **测试中 connect 的 lambda 不得既捕获局部变量、又以长命对象（如 `mEdit.get()`）作 context**：测试函数返回后连接仍然存活，死 lambda 会在之后每次信号触发时往已释放的栈地址写数据，造成堆/栈破坏——崩溃点往往远在肇事代码之后、且看似随机（真实案例见提交 `63e90e30`）。正确做法：以一个随测试函数销毁的局部 `QObject` 作 context（连接随之自动断开），或在测试结束前显式 `disconnect`。

- **按测试类筛选**：test-qsynedit、test-editor 和 test-cppparser 支持 `--suite=类名`（如 `--suite=TestDocument`、`--suite=TestCppParser`）只跑指定测试类，不带参数则跑全部。
- **调试产生的有效验证场景必须保留**：修复问题后不得连同临时日志一起删除这些场景。将调试输出整理为正式断言，按职责放入合适的测试目标；通用根因用最小声明覆盖，真实工具链差异用对应真实头文件覆盖。中间类型、作用域归属和声明绑定等关键分层检查也应保留，不能只保留最终端到端结果。
- **不得因测试不符合当前实现或既有预设而丢弃场景**：测试失败时先判断它是否暴露了新的合法使用场景、语义缺陷或实现 bug。即使夹具或断言本身有误，也只能修正错误部分，并保留原场景或另加等价的最小回归测试；需要改变场景语义时，应新增测试而不是悄悄替换原测试。
- **Windows 直接运行测试时取得 Qt 日志**：若 `qWarning()`/`qDebug()` 只发往调试器，给测试进程设置 `QT_FORCE_STDERR_LOGGING=1` 即可输出到 Git Bash 终端。性能测量必须直接运行 Release 可执行文件，不能在 gdb 下计时；gdb 只用于崩溃、断言和栈信息。例如：

```sh
cd /r/build-redpanda-qt5-static-release-parser-stdlib-gcc81/RedPandaIDE
PATH="/d/qt/mingw64/bin:/d/qt/qt5-static-64/bin:$PATH" \
  QT_QPA_PLATFORM=offscreen QT_FORCE_STDERR_LOGGING=1 \
  ./test-cppparser.exe --suite=TestCppParserStandardLibrary test_standard_library_public_api_complete
```

  需要 `DECLARATION_PROFILE`、`DECLARATION_SELF`、`MERGE_DECLARATION_PROFILE` 等详细计时输出时，先在配置阶段启用 `REDPANDA_PARSER_DECLARATION_PROFILE=ON`（声明细分开关默认 OFF），再额外设置 `REDPANDA_PARSER_PROFILE=1`。详细 profile 默认关闭，避免计时器干扰生产性能；普通构建的声明 scope 为空操作，整体阶段计时仍可使用。嵌套的细项计时会增加开销，优化前后对比必须以未启用详细 profile 的 Direct Release 结果为准。测量 preprocessor 或完整 parse 时，以独立新进程连续运行两次相同测试并采用第二次结果，排除系统文件缓存 miss；不能用同一进程中的第二次 parse 代替，因为那还会混入 Parser 自身的 replay 缓存收益。定位热点时优先使用 `RESOLVER_SELF`：它已扣除直接子操作耗时；`RESOLVER_PROFILE` 和 `RESOLVER_DETAIL` 的 inclusive 时间只能用于理解调用结构，不能相加或当作独立成本。
- **tokenize/profile 的目的首先是发现并消除不合理操作，不是让计数或耗时好看**：profile 本身不以预期收益决定是否进行；每个不同 entry 都必须核对触发来源和必要性，不能仅凭总次数下降判断优化完成。cache/memo 可以使用，但统计必须同时保留解析尝试、命中情况和调用链，不能用 cache 命中隐藏冗余查找或重复处理。先查清并消除无意义操作，再用 cache 复用必要工作的结果；是否修改设计和代码时，才根据证据评估收益、复杂度和风险。
- **测试/profile 配套工具必须持久保存**：Python 脚本、数据汇总与对比工具等要和对应测试的其他 resource 一起存入仓库，不得只放在 `R:` 等临时目录或用完即弃。必须在专门文档中登记其用途、环境依赖、输入输出、运行命令及对应基线，确保后续能够复现和继续扩展。

- **多视图 demo 仅在需要时编译**：CMake 选项 `QSYNEDIT_BUILD_MULTIVIEW_DEMO` 默认 OFF，只在验证多视图行为时显式打开。

- 剪贴板相关测试（列模式 paste/cut/copy-paste 等）偶发失败，多为外部程序（QQ、百度网盘等）抢占剪贴板所致，与代码无关；先重跑或排查占用程序，不要据此改代码。
- 仓库文件多为 CRLF 行尾；用 Python 等脚本批量替换时先转 LF 再匹配，否则可能匹配失败。
- **Git 操作会改变工作区行尾状态**：Windows 下受 `core.autocrlf` 等配置影响，提交、检出或切换分支后，工作区文件可能在 LF/CRLF 间转换。不得沿用之前读到的行尾状态；操作后需重新 Read 再 Edit。Write 追加可能造成混合行尾，必须立即按该文件的原有主导风格修正；不要反复凭旧状态构造匹配串。
- **Edit 工具插入新函数的陷阱（已踩 6 次）**：`old_string` 若以"下一个函数的签名行"结尾而 `new_string` 不含它，该函数头会被吞掉。插入新函数时，`old_string` 应以目标函数**完整结尾的 `}`** 为止，或在 `new_string` 中原样保留被锚住的后续行；改完立刻 grep 确认相邻函数头完好。

打包工作流：`.github/workflows/build.yml`（发行版包）、`build-app.yml`（独立应用）、`feature-matrix.yml`、`winget.yml`。

## 代码风格与约定

- C++17、C11；全面使用 Qt 惯用法（signals/slots、用户可见字符串用 `tr()`、`QObject` 父子对象所有权）。
- 仓库**没有** `.clang-format`/`.editorconfig`；请与所在文件的现有风格保持一致。项目内部附带了自己的格式化配置（AStyle 捆绑在 `assets/astyle`），但仓库范围内没有统一的格式化配置。
- 文件使用 GPL v3+ 版权头（模板见任意现有 `src/*.cpp`）。
- `RedPandaIDE/src` 中的新类通过根目录 `CMakeLists.txt` 定义的辅助函数注册到 `RedPandaIDE/CMakeLists.txt`：`target_qt_plain_cpp`（纯 `.cpp/.h`）、`target_moc_classes`（需要 moc 的 QObject 类）、`target_ui_classes`（带 `.ui` 文件）。每个只列不含扩展名的源文件基名。
- 翻译：用户可见字符串走 `tr()`；`.ts` 文件位于 `RedPandaIDE/translations/`。Qt 5 下通过 `update_translations` 目标更新（`cmake --build . --target update_translations`）；Qt 6 下由 `qt_add_translations` 处理。
- 资源通过 `target_embed_resources`/`target_embed_translations` CMake 辅助函数嵌入，这些集合不手写 `.qrc`。
- 注释与文档使用英文；新写的注释和文档也请使用英文。
- 设计文档（`docs/architecture.md`、`docs/qsynedit-design.md`）只描述**现状与约定**，以代码为准；具体修改记录（改名、bug 修复等）写入 git 提交信息，不放进设计文档。提交信息使用英文。
- **NEWS.md 维护**：完成功能/修复工作时，必须在 `NEWS.md` 当前版本节中面向用户记录变更——新增功能（`enhancement:`）、修复的 bug（`fix:`）、行为变化（`change:`）。只记录已发布/既有版本中就存在的问题修复与新能力；**不记录**本次开发/调试/重构过程中引入又修复的临时问题（那些从未发布）。
- 特性代码用 `ENABLE_LUA_ADDON`、`ENABLE_SDCC`、`ENABLE_VCS` 宏做条件编译——相应的 `#include` 也要加保护。
- **lambda/functor 槽的寿命不变式：连接的寿命必须 ≤ 它捕获的每个对象的寿命**。落实为两条机械规则：
  1. `connect()`/`QTimer::singleShot()` 等以 lambda 作槽时必须传入 QObject context 对象（context 析构即断开连接），它约束按值捕获的 `this`/裸指针。无 context 的连接只随 sender 存亡，sender 可能比捕获的对象长寿——案例：todoparser 的 `connect(mThread, finished, [this]{...})`，TodoParser 先析构则 `this` 悬空。
  2. connected lambda 不得用 `&` 引用捕获局部变量——context 保护不了比它早死的栈对象（真实崩溃案例见提交 `63e90e30`：测试 lambda 以长命的 `mEdit.get()` 为 context、引用捕获局部变量，函数返回后信号触发写已释放栈地址）。确需写回局部变量时，用一个在函数开头声明的局部 `QObject` 作 context（作用域结束时断开，与局部变量同生共死）。
  残余注意：按值捕获的裸指针不在两条规则覆盖内，要求指针指向的对象明显比 context 长寿；优先捕获值类型或 shared_ptr。
- Lua 插件 API 用 Teal 标注类型；API 定义位于 `addon/defs/*.d.tl`。插件 API 的版本规则很严格，修改接口前须核对现有定义和调用方。

## 安全注意事项

- 绝不提交密钥；`.gitignore` 已排除构建产物（`build/`、`dist/`、`.xmake/`）。
- IDE 会执行外部工具（编译器、gdb、用户程序）并运行 Lua 插件——对路径和命令行要格外小心；使用 `src/utils/escape.*` 中现有的转义工具。
- `libs/lua/` 是内嵌的上游代码——不要在本地打补丁；CI 会将其与上游 v5.4.6 做 diff。
- 本仓库构建出的二进制会运行不可信的用户代码（这正是本产品的用途）；新增网络或执行能力时须考虑沙箱/信任问题。

## 给 Agent 的备注

- **代码注释和 git 提交信息一律使用英文**（设计文档语言随已有文档）。
- **修复 bug 后先报告、确认后提交**：定位并修复问题后，先把发现的问题、根因与修复方案报告给用户，得到确认后再提交；不得修复后直接提交。
- 大的计划性改动（多批次重构/新功能）**必须在独立分支上进行**，不要直接在 master 上工作。
- **性能优化必须逐项实施，并为每一项遵循完整流程**：先建独立分支；编写设计文档；按 KISS 原则审核设计并仔细评估修改范围；检查现有测试覆盖并先完善测试保护；严格控制范围编码实施；执行专项测试和 Release→Debug 全量回归；使用相同场景做优化前后 profile 收益评估；审核差异并迭代；编写实施结果总结文档；最后用英文提交信息提交。同一问题存在多个值得验证的改进方案时，必须分别建分支试验，再依据正确性、复杂度、风险和实测收益综合比较，不得只凭推测选择。合并和推送仍须用户另行明确授权。
- 分支合并回 master 前**必须先询问用户**，得到确认后再合并；不得擅自 push。
- `BUILD.md`（以及 `BUILD_cn.md`/`BUILD_ru.md`）是权威的构建参考——在自创构建步骤之前请先参考它。
- `NEWS.md` 是更新日志；查询功能历史时可参考。
- 当修改了上文描述的任何内容（构建选项、测试命令、目录职责）时，请同步更新本 `AGENTS.md`。
