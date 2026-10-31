# Red Panda C++ 整体架构设计

> 本文档以代码现状为唯一事实来源编写（不参考本目录下的旧文档）。文中涉及的类名、文件路径均与仓库代码一一对应。

## 1. 概述

Red Panda C++ 是一款跨平台（Windows / Linux / macOS）的 C/C++/GNU 汇编 IDE，使用 C++17 和 Qt（6.8+ 或 5.15）编写。设计目标是**轻量、快速、开箱即用**：自带代码补全解析器、直接驱动外部编译器/调试器，不依赖语言服务器等重型基础设施。

核心架构特点：

- **单进程、单主窗口**：整个 IDE 运行在一个 Qt 进程中，`MainWindow` 是所有子系统的持有者和协调中心。
- **全局单例**：通过 `pSettings`、`pMainWindow` 等全局指针访问核心对象。
- **异步模型**：编译、运行、调试、代码解析等耗时操作全部放在 `QThread` 子类或异步子进程（`QProcess`）中执行，通过 Qt signals/slots 回主线程通信。
- **条件编译**：Lua 插件（`ENABLE_LUA_ADDON`）、Git 集成（`ENABLE_VCS`）、SDCC 支持（`ENABLE_SDCC`）为可选特性。

## 2. 总体分层

```
┌─────────────────────────────────────────────────────────┐
│  RedPandaIDE（应用层，target: RedPandaIDE）              │
│  MainWindow ──┬── 编辑器封装（Editor / EditorManager）    │
│               ├── C++ 解析器（parser/）                  │
│               ├── 编译运行（compiler/）                  │
│               ├── 调试器（debugger/）                    │
│               ├── 问题集（problems/）                    │
│               ├── 设置系统（settings/）                  │
│               ├── Lua 插件（addon/，可选）               │
│               └── Git 集成（vcs/，可选）                 │
├─────────────────────────────────────────────────────────┤
│  组件库层                                                │
│  libs/qsynedit（代码编辑器组件）                         │
│  libs/redpanda_qt_utils（字符集检测等 Qt 工具）          │
├─────────────────────────────────────────────────────────┤
│  内嵌第三方：libs/lua（Lua 5.4.6，可选编译）             │
├─────────────────────────────────────────────────────────┤
│  外部工具进程（随产品分发）                              │
│  tools/consolepauser、tools/redpanda-git-askpass 等      │
├─────────────────────────────────────────────────────────┤
│  外部进程（用户提供）                                    │
│  gcc/g++/gdb/lldb、make、终端、被调试/运行的用户程序     │
└─────────────────────────────────────────────────────────┘
```

依赖方向自上而下：应用层依赖组件库，组件库不反向依赖应用层；`libs/lua` 仅被 `src/addon/` 使用。

## 3. 启动流程与单实例机制

入口为 `RedPandaIDE/src/main.cpp` 的 `main()`（启用 `ENABLE_GLIBC_HWCAPS` 时导出为 `RedPandaIDE_main`，由 `main_glibc_hwcaps.c` 转发）。启动顺序：

1. **创建 `QApplication`**，设置高 DPI 等属性；macOS 下修正 `PATH`（GUI 应用默认不含 `/usr/local/bin`）。
2. **定位配置文件**（`getSettingFilename`）：便携模式（`PORTABLE_CONFIG`）下使用 `程序目录/config/`，否则使用系统 AppData 目录；据此判断是否为首次运行。
3. **单实例检查**（读旧配置中的 `open_files_in_single_instance`，命令行 `-s`/`-ns` 可覆盖）：
   - 用 `QLockFile`（临时目录下 `RedPandaDevCppStartUp.lock`）做跨进程互斥。
   - Windows 下若已有实例在运行，则把命令行中的文件列表写入 `QSharedMemory("RedPandaCpp/openfiles")`，向旧实例发送 `WM_APP_OPEN_FILE` 消息后退出；旧实例在原生事件过滤器中读取共享内存并打开文件。
4. **加载翻译**：先读 INI 中的语言设置，再依次加载 `RedPandaIDE_<lang>`（内置资源 `:/i18n/`）、`qt_utils_<lang>`、Qt 自身的 `qt_<lang>`/`qtbase_<lang>`。
5. **注册元类型**（`qRegisterMetaType`）并调用 `initParser()` 初始化解析器相关的全局数据。
6. **创建核心全局对象**（生命周期由 `main()` 栈控制）：
   - `SystemConsts` → `pSystemConsts`（系统常量：路径分隔符、可执行后缀等）
   - `CharsetInfoManager` → `pCharsetInfoManager`（编码检测）
   - `Settings`（`std::unique_ptr` 持有）→ `pSettings`；首次运行时自动搜索编译器（`compilerSets().findSets()`）、弹出主题选择对话框、（启用 VCS 时）探测 git。
   - `AutolinkManager` → `pAutolinkManager`（自动链接库配置）
7. **创建并显示 `MainWindow`** → `pMainWindow`；按命令行参数打开文件，或恢复上次打开的文件（`loadLastOpens()`），都没有则新建一个空编辑器。
8. 安装事件过滤器：Windows 的 `WindowLogoutEventFilter`（处理注销 `WM_QUERYENDSESSION`、DPI 变更 `WM_DPICHANGED`、单实例文件转发），以及全局的 `BlockWheelEventFiler`（阻止 QComboBox 意外滚轮改值）。
9. 进入 `app.exec()` 事件循环。退出时若用户选择了"恢复默认设置"，先销毁 `Settings` 再删除配置目录。

## 4. 核心全局对象与协作模式

### 4.1 全局单例

| 全局指针 | 类型 | 定义位置 | 职责 |
|---|---|---|---|
| `pSettings` | `Settings*` | `src/settings.cpp` | 全部设置的聚合入口 |
| `pMainWindow` | `MainWindow*` | `src/mainwindow.cpp` | 主窗口 / 子系统协调中心 |
| `pSystemConsts` | `SystemConsts*` | `src/systemconsts.cpp` | 平台相关常量 |
| `pCharsetInfoManager` | `CharsetInfoManager*` | `libs/redpanda_qt_utils` | 文件编码检测 |
| `pAutolinkManager` | `AutolinkManager*` | `src/autolinkmanager.cpp` | 自动链接库管理 |

这些指针在 `main()` 中赋值，生命周期与进程一致。代码中大量以 `pSettings->editor()`、`pMainWindow->editorManager()` 的形式访问。

### 4.2 MainWindow：协调中心

`MainWindow`（`src/mainwindow.h`）持有并串联所有子系统，主要成员包括：

- 子系统管理器：`EditorManager`、`CompilerManager`、`Debugger`、`ToolsManager`、`SymbolUsageManager`、`CodeSnippetsManager`、`TodoParser`、`VisitHistoryManager`、`IconsManager`、`ColorManager`、`CompetitiveCompanionHandler`；
- 项目：`std::shared_ptr<Project>`（可随时被销毁，故用 `shared_ptr`）；
- 界面模型：`TodoModel`、`SearchResultModel`、`BookmarkModel`、`ClassBrowserModel`、`OJProblemSetModel`、`OJProblemModel` 等（Qt Model/View）；
- 代码补全弹窗：`CodeCompletionPopup`、`HeaderCompletionPopup`、`FunctionTooltipWidget`；
- 其他：`QFileSystemWatcher`（监控外部文件改动）、`QTimer mAutoSaveTimer`（自动保存）、`CaretList`（光标位置历史）。

### 4.3 协作模式

- **signals/slots**：跨模块通信的主机制。子系统（编译、调试、解析）在后台线程完成工作后发信号，由主线程的槽函数更新 UI。跨线程所需的自定义类型在 `main()` 中通过 `qRegisterMetaType` 注册。
- **异常**：项目自定义了 `BaseError` 异常族（`FileError`、`CompileError`、`GitError`、`LuaError`、`DAPMessageError`、`BinaryFileError` 等），用于可预期的失败路径；`main()` 外层捕获 `BaseError` 并弹错。
- **QObject 父子所有权**：管理器和对话框大多以 `MainWindow` 为父对象，随主窗口销毁。

## 5. 子系统设计

### 5.1 编辑器组件：QSynEdit（`libs/qsynedit/`）

QSynEdit 是独立组件库，命名空间 `QSynedit`，与应用层解耦：

- `QSynEdit : QAbstractScrollArea`（`qsynedit.h`）——编辑器控件本体，负责事件处理与滚动；
- `Document : QObject`（`document.h`）——文档模型（文本行、修改状态），配 `UndoList`/`RedoList` 实现撤销重做；
- `painter.*` ——文本绘制；`gutter.*` ——行号栏；`codefolding.*` ——代码折叠；`keystrokes.*` ——按键映射；
- `syntaxer/` ——语法高亮器，`Syntaxer` 基类的子类：`CppSyntaxer`、`ASMSyntaxer`（及 `GASSyntaxer`、`NASMSyntaxer`）、`GLSLSyntaxer`、`LuaSyntaxer`、`MakefileSyntaxer`、`TextSyntaxer`；
- `searcher/` ——查找器：`BasicSearcher`、`RegexSearcher`；
- `exporter/` ——导出器：HTML（`HTMLExporter`、`QtSupportedHtmlExporter`）、RTF（`RTFExporter`）；
- `formatter/cppformatter.*` ——简单的 C++ 缩进格式化（与 AStyle 格式化器不同，见 5.9）。

### 5.2 编辑器封装：Editor 与 EditorManager（`src/editor.*`、`src/editormanager.*`）

- `Editor : public QSynedit::QSynEdit`——在编辑器组件上叠加 IDE 语义：关联文件/项目单元、断点、语法检查标记、代码补全触发、符号高亮等。
- 每个 `Editor` 持有一个 `PCppParser`（`std::shared_ptr<CppParser>`）。解析器可以每个编辑器独占，也可以在多个编辑器间共享（设置项 `codeCompletion().shareParser()`，通过 `GetSharedParserrFunc`/`GetCppParserFunc` 回调注入），共享模式省内存，适合小项目。
- `EditorManager : QObject`——管理全部打开的标签页/编辑器，提供新建、打开、保存、关闭、切换等操作，是 MainWindow 与单个 Editor 之间的中间层。

### 5.3 C++ 解析器（`src/parser/`）

自研的 C/C++ 解析器，为代码补全、符号跳转、类浏览器、TODO 列表、符号使用统计、语法检查提供数据：

- `CppTokenizer`（`cpptokenizer.h`）——词法分析；
- `CppPreprocessor`（`cpppreprocessor.h`）——预处理（宏展开、include 处理、条件编译分支）；
- `CppParser : QObject`（`cppparser.h`）——语法解析，产出语句（statement）树；`CppScopes`、`ParsedFileInfo`（`parserutils.h`）维护作用域和文件级缓存；
- `StatementModel : QObject`（`statementmodel.h`）——语句结果的模型封装，供类浏览器等 UI 使用；
- `CppFileParserThread` / `CppFileListParserThread : QThread`——单文件/批量文件的后台解析线程，解析在后台进行、结果回主线程更新。

`initParser()` 在 `main()` 中调用，完成语句类型等全局数据的初始化。

### 5.4 编译与运行（`src/compiler/`）

采用"调度器 + 后台线程"模型：

- `CompilerManager : QObject`（`compilermanager.h`）——由 `MainWindow` 持有，负责调度编译/运行/语法检查任务、汇总编译期错误（`PCompileIssue`）、管理编译后串联任务（`PCompileSuccessionTask`，如"编译成功后运行"）。
- `Compiler : QThread`（`compiler.h`）——编译任务基类，定义 `TargetType`；子类：
  - `FileCompiler`——编译单个源文件；
  - `ProjectCompiler`——编译整个项目（生成并驱动 makefile）；
  - `NASMFileCompiler`——NASM 汇编文件；
  - `SDCCFileCompiler` / `SDCCProjectCompiler`——SDCC 单片机 C 编译（`ENABLE_SDCC`）；
  - `StdinCompiler`——不落地文件、经标准输入编译（用于语法检查等场景）。
- `Runner : QThread`（`runner.h`）——运行任务基类；子类 `ExecutableRunner`（运行可执行文件，配合 `consolepauser` 保持控制台窗口）、`OJProblemCasesRunner`（批量运行 OJ 题目的测试用例并比对输出）。
- `CompilerInfo` 族（`compilerinfo.h`）：`GCCCompilerInfo`、`ClangCompilerInfo`、`SDCCCompilerInfo`，由 `CompilerInfoManager` 管理，描述不同编译器的诊断信息格式（用于解析错误输出、定位到编辑器行）。

编译/运行均通过启动外部进程（gcc、make、用户程序）完成，线程负责喂命令、读输出、发信号。

### 5.5 调试器（`src/debugger/`）

双后端架构：

- `Debugger : QObject`（`debugger.h`）——调试功能对上层（MainWindow/Editor）的统一接口，由 `MainWindow` 持有；
- `DebugTarget : QThread`——被调试进程的生命周期管理；
- `DebuggerClient : QThread`——调试器后端客户端基类，两个实现：
  - `GDBMIDebuggerClient`（`gdbmidebugger.h`）——GDB/MI 机器接口，`GDBMIResultParser`（`gdbmiresultparser.h`）解析 GDB/MI 输出记录；
  - `DAPDebuggerClient`（`dapdebugger.h`）——DAP（Debug Adapter Protocol），`dapprotocol.*` 处理 DAP 消息编解码（`DAPMessageError`）。
- `debuggermodels.h`——调试界面的 Qt 模型：`BreakpointModel`、`BacktraceModel`（调用栈）、`WatchModel`（监视变量）、`RegisterModel`（寄存器）、`MemoryModel`（内存查看）。

### 5.6 问题集与 Competitive Companion（`src/problems/`）

面向算法竞赛/OJ 刷题场景：

- `OJProblemSet : QObject`、`OJProblem : QObject`（`ojproblemset.h`）——问题集与题目（含测试用例）的数据模型；`freeprojectsetformat.*` 处理问题集文件格式；
- `ProblemCaseValidator`（`problemcasevalidator.h`）——比对程序输出与期望输出；
- `CompetitiveCompanionThread : QThread` + `CompetitiveCompanionHandler : QObject`（`competitivecompenionhandler.h`）——监听 Competitive Companion 浏览器插件（经 `QTcpSocket`）推送的题目，自动建题；
- 运行用例由 `compiler/` 下的 `OJProblemCasesRunner` 执行。

### 5.7 设置系统（`src/settings.*`、`src/settings/`）

- `Settings`（`src/settings.h`）是聚合根，按功能分组持有设置子对象：`dirs()`、`editor()`、`compilerSets()`、`environment()`、`executor()`、`debugger()`、`codeCompletion()`、`codeFormatter()`、`compile()`、`ui()`、`languages()`，以及（`ENABLE_VCS` 时）`vcs()`。
- 每个分组对应 `src/settings/` 下一个类（`EditorSettings`、`CompilerSets`、`DirSettings`……），多数继承 `BaseSettings`（`basesettings.h`），统一提供 load/save。
- 持久化用 INI（`QSettings::IniFormat`），经 `SettingsPersistor` 写盘；配置位置由 `PORTABLE_CONFIG` 决定（见第 3 节）。
- 首次运行时的编译器搜索、主题选择等初始化逻辑在 `main()` 中完成。

### 5.8 顶层管理器（`src/` 根目录）

| 类 | 文件 | 职责 |
|---|---|---|
| `ThemeManager` | `thememanager.*` | 应用主题（含默认配色/图标集） |
| `ColorManager` / `ColorScheme` | `colorscheme.*` | 编辑器配色方案 |
| `IconsManager` | `iconsmanager.*` | 图标集管理与加载 |
| `ShortcutManager` | `shortcutmanager.*` | 快捷键自定义与持久化 |
| `CodeSnippetsManager` | `codesnippetsmanager.*` | 代码片段 |
| `SymbolUsageManager` | `symbolusagemanager.*` | 符号引用统计（重命名重构的数据来源） |
| `TodoParser` | `todoparser.*` | 扫描源码中的 TODO/FIXME 注释 |
| `VisitHistoryManager` | `visithistorymanager.*` | 编辑位置前进/后退历史 |
| `AutoLinkManager` | `autolinkmanager.*` | 自动链接库（按头文件关联 -l 参数） |
| `Project` / `ProjectTemplate` | `project.*`、`projecttemplate.*` | 项目（.dev 文件）与项目模板 |
| `CaretList` | `caretlist.*` | 光标历史 |

### 5.9 重构与格式化

- `CppRefacter : QObject`（`src/cpprefacter.*`）——基于解析器和 `SymbolUsageManager` 的符号级重构（重命名等）。
- `reformatter/`：`BaseReformatter : QObject` 基类 + `AStyleReformatter` 实现——调用捆绑的 AStyle 做代码格式化；格式化选项由 `pSettings->codeFormatter()` 提供。

### 5.10 Lua 插件（`src/addon/`，`ENABLE_LUA_ADDON`）

- 命名空间 `AddOn`；`RaiiLuaState`（`luaruntime.h`）以 RAII 封装 `lua_State`；`LuaError : BaseError` 报告脚本错误。
- `luaapi.*`——向 Lua 暴露的 API 组实现。
- `luaexecutor.h`——`SimpleExecutor` 基类及特化：`ThemeExecutor`（主题插件）、`CompilerHintExecutor`（编译器探测提示插件）。
- API 的 Teal 类型定义在仓库根的 `addon/defs/`（`global_env.d.tl`、`theme.d.tl`、`compiler_hint.d.tl`），生成脚本 `addon/gen.sh`。

### 5.11 Git 集成（`src/vcs/`，`ENABLE_VCS`）

- `GitManager : QObject`（`gitmanager.h`）——git 操作入口，错误用 `GitError : BaseError` 报告；
- `GitRepository`（`gitrepository.*`）——仓库状态抽象；
- 一组操作对话框：branch/fetch/log/merge/pull/push/remote/reset/用户配置（`git*dialog.*`）；
- 认证通过外部 `tools/redpanda-git-askpass` / `tools/redpanda-win-git-askpass` 辅助进程完成。

### 5.12 通用工具（`src/utils/`）

`escape.*`（命令行/路径转义）、`file.*`（文件读写与编码）、`font.*`、`os.*`（平台判别、进程）、`terminal.*`（各平台终端启动）、`parsearg.*`（命令行解析）、`parsemacros.*`（宏变量替换）、`pe.*`（Windows PE 信息）等。

## 6. 关键交互流程

### 6.1 打开文件 → 代码补全

1. `MainWindow` 经 `EditorManager` 创建 `Editor`，加载文件到 QSynEdit 的 `Document`（经 `CharsetInfoManager` 检测编码）。
2. `Editor` 获得（独占或共享的）`CppParser`，触发后台解析（`CppFileParserThread`）。
3. 解析完成信号回到主线程，更新 `StatementModel`（类浏览器）、TODO、语法检查标记。
4. 用户输入触发补全时，`Editor` 查询 parser 的语句/作用域数据，由 `MainWindow` 持有的 `CodeCompletionPopup` 展示候选。

### 6.2 编译 → 运行

1. 用户触发编译，`MainWindow` 调 `CompilerManager`，后者按目标类型创建相应的 `Compiler` 子类线程。
2. 编译线程启动外部编译器进程，逐行解析诊断输出为 `PCompileIssue`，发信号给主线程填入问题列表并标注到编辑器。
3. 若存在串联任务（如"编译并运行"），编译成功信号触发 `CompilerManager` 创建 `Runner`（`ExecutableRunner` 或 `OJProblemCasesRunner`）线程运行程序；控制台程序经 `consolepauser` 包装以保持窗口。

### 6.3 调试会话

1. `MainWindow` 指示 `Debugger` 启动会话；`Debugger` 按配置创建 `GDBMIDebuggerClient` 或 `DAPDebuggerClient` 线程和 `DebugTarget`。
2. 客户端线程与 gdb/适配器通信（GDB/MI 文本记录或 DAP 消息），事件经信号回到主线程。
3. 主线程更新断点/调用栈/监视/寄存器/内存模型（`debuggermodels.h`），并在 `Editor` 中同步当前执行行与断点显示。

## 7. 资源、翻译与构建

- **资源**：图标、主题、配色、字体、代码片段、项目模板经 Qt 资源系统（`*.qrc` 与 CMake 的 `target_embed_resources`/`target_embed_translations` 辅助函数）嵌入二进制，运行时以 `:/...` 路径访问。
- **翻译**：`tr()` + `RedPandaIDE/translations/*.ts`；启动时按设置加载（见第 3 节）。
- **构建**：CMake（主，`CMakeLists.txt`）与 Xmake（`xmake.lua`）双系统；特性开关 `LUA_ADDON`、`VCS`、`SDCC`、`FORCE_QT5`、`FILESYSTEM_LAYOUT`、`PORTABLE_CONFIG`、`MAKE_INTERFACE` 等控制条件编译，对应代码中的 `ENABLE_LUA_ADDON`、`ENABLE_VCS`、`ENABLE_SDCC` 宏。
- **测试**：Qt Test。`RedPandaIDE/test/`（`test-cppparser`、`test-editor`）与 `libs/qsynedit/test/`；CTest 统一驱动，GUI 测试用 `QT_QPA_PLATFORM=offscreen` 运行。

## 8. 目录结构速查

| 目录 | 职责 | 关键类 |
|---|---|---|
| `RedPandaIDE/src/` | 应用主代码、顶层管理器 | `MainWindow`、`Editor`、`EditorManager`、`Project` |
| `src/parser/` | C++ 解析器 | `CppTokenizer`、`CppPreprocessor`、`CppParser`、`StatementModel` |
| `src/compiler/` | 编译/运行 | `CompilerManager`、`Compiler`、`Runner` 及子类 |
| `src/debugger/` | 调试 | `Debugger`、`DebuggerClient`（GDBMI/DAP）、调试模型 |
| `src/problems/` | OJ 问题集 | `OJProblemSet`、`OJProblem`、`CompetitiveCompanionHandler` |
| `src/settings/` | 分组设置类 | `EditorSettings`、`CompilerSets` 等 |
| `src/reformatter/` | 代码格式化 | `BaseReformatter`、`AStyleReformatter` |
| `src/widgets/` | 自定义控件/对话框 | — |
| `src/utils/` | 通用工具 | escape、file、os、terminal 等 |
| `src/addon/` | Lua 插件（可选） | `RaiiLuaState`、`SimpleExecutor` 等 |
| `src/vcs/` | Git 集成（可选） | `GitManager`、`GitRepository` |
| `libs/qsynedit/` | 编辑器组件库 | `QSynEdit`、`Document`、`Syntaxer` 族 |
| `libs/redpanda_qt_utils/` | Qt 工具库 | `CharsetInfoManager` |
| `libs/lua/` | 内嵌 Lua 5.4.6 | （上游代码，不可修改） |
| `tools/` | 辅助程序 | consolepauser、git-askpass |
| `addon/` | 插件 API 定义/示例 | `defs/*.d.tl`、`gen.sh` |
| `packages/`、`platform/` | 打包与平台支持 | — |
