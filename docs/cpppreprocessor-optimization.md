# CppPreprocessor 优化计划（正确性优先）

> 状态：进行中（分支 `parser-preprocess-opt`）。本文是计划与设计结论的记录；具体修改记录见 git 提交信息。

## 背景

RedPandaIDE C++ 解析器（`RedPandaIDE/src/parser/`）分 preprocess → tokenize → analysis 三步。本次先做 preprocessor（`cpppreprocessor.cpp/h`）的整体优化。

**原则：不管任何优化，首先保证处理的正确性，然后才是性能。** 测试集完善是第一阶段、是后续一切改动的前置门禁。

目标（用户确认）：

1. 降低重解析延迟（编辑/保存后 reparse）；
2. 降低首次全量解析耗时（开项目/切编译器）；
3. 控制内存占用（新增缓存必须有界）；
4. 先建性能基准，用数据验收，防退化。

## 现状评估

### 测试覆盖缺口

- 现有 `test_cpppreprocessor.cpp` 13 个测试：续行（1 例）、注释（1 例）、宏展开（4 个资源文件 + 5 个内联用例）、条件表达式求值。
- **include 场景测试为零**（没有多文件测试），而后续缓存与 `openInclude` 剪枝正改这里。
- **`CppParser` 实际运行用 `setFileOnlyIncludeOnce(false)`（cppparser.cpp 构造函数），现有测试全用默认值 `true`**——真实运行模式从未被测。
- 增量路径（`removeScannedFile`→`invalidDefinesInFile`/`mFileUndefines` 恢复、`addDefinesInFile` 重放）完全未测。
- `conditional1.cpp` 是死资源（无测试引用，其注释恰好写明 `#elifdef` 降级语义）。
- `MAX_DEFINE_EXPAND_DEPTH`（cpppreprocessor.h）是死常量，递归防护实际只靠 ignore 集合。

### 热点（代码审查结论）

- `expandMacros()`——每个活动行都走：逐字符 `word += ch`、逐字符 `newLine += ch`；每次宏展开拷贝整个 `QSet`/`QMultiHash`；`text.replace()` 后回退重扫（宏密集行 O(n²)）。
- `expandFunctionLikeMacro()`——循环 `QString::arg()` 逐参数全串扫描。
- `openInclude()`——对 include 栈上每个祖先无条件传播被包含文件的完整传递闭包；祖先已包含该头时闭包必然已建立，可剪枝。
- `getSystemHeaderFilename()`（parserutils.cpp）——每个 `#include` 对全部 include 路径逐个 `QDir::exists()`（文件系统 stat），无缓存。
- 文件内容无通用缓存：`mFileCache` 只覆盖单次运行内"可重复 include"的文件；被级联失效的文件每次重读都重新做编码检测（最多 3 趟完整解码）+ 续行合并 + 注释剥离。

### 结构性结论

- 项目首解析/新增文件的双重预处理（`sortFilesByIncludeRelations` 预扫后丢弃结果）——**推迟到 CppParser 层测试建立后处理**（改的是 cppparser.cpp 的 parse 流程，超出 preprocessor 单元测试保护范围，用户明确不着急做）。
- `CppPreprocessor` 单类混杂"单次运行状态机"与"跨次注册表"两层生命周期——本次拆分。
- `expandMacro` 的 `handleBuffer` 隐式推进 `mIndex`——本次显式化。
- 结果接口的 `#include file:line` 魔数标记——留待 tokenizer 阶段。

## 缓存的正确性分析（三种缓存，约束完全不同）

### 1. 文件内容缓存（新增，缓存"续行合并+注释剥离后的行表"）——与 `#if` 无关，完全安全

它缓存在状态机扫描**之前**，只是文件文本。`#if` 是扫描行表时才求值的，缓存行表不影响条件判断的正确性。含条件判断的文件一样安全。

### 2. 重放缓存（现有机制：`mScannedFiles` + `mFileDefines` + `addDefinesInFile`）——与 `#if` 纠缠，现状比预想更宽

`mFilesCouldRepeatInclude` 在 `clearTempResults()` 里被清空，所以**跨 reparse 时连无守卫文件也走重放**（本运行内第一次包含它时已 scanned → 直接 `addDefinesInFile` 跳过重扫）。

重放出错的**充要条件**：文件的分支结果依赖**包含点的宏环境**，且首次扫描时的环境与当前包含点环境不同。典型：

```c
// config.h
#ifdef USE_FAST
#define SPEED 100
#else
#define SPEED 1
#endif
```

`a.c` 先 `#define USE_FAST` 再 include → 首次扫描记录 SPEED=100；`b.c` 不带定义 include → 标准语义应得 SPEED=1，重放给 SPEED=100（错）。

注意：**共享宏表架构**（所有文件一个 `mDefines`）本身已是 per-TU 语义的近似——有 guard 的文件在重扫路径下也会因 guard 宏已定义而跳过文件体，结果同样是"第一次展开的产物"。所以重放的大部分"错误"是架构近似而非缓存引入；重放真正额外引入的错误是上面的环境依赖分支场景。

**安全化方案（属正确性修复，阶段 0.5）：**

1. **环境依赖检测**：分支求值时（`handleIf`/`handleIfdef`/`evaluateIf` 等处）顺手记录该文件条件引用的宏名集合；判定 ⊆ {hard defines（session 内不变）} ∪ {本文件自定义宏（含 guard 宏特判，`mFileIncludeOnceToken` 机制已识别）} → 环境无关才允许重放，否则每次包含重扫。保守原则：拿不准判敏感（漏判=错误缓存，多判=多扫一次）。现实中绝大多数头文件的条件只依赖 `__cplusplus`、编译器预定义宏、自己的 guard——环境敏感的是少数，性能损失小。
2. **重放的 mtime 校验**：重放前 stat 一次，mtime/size 变了 → `removeScannedFile` + 重扫（解决外部修改/git pull 后解析过期）。

### 3. include 解析结果缓存（新增，"名字→路径"映射）——与文件内容无关，存在性校验即可

只缓存"`#include <vector>` 解析到哪个路径"。文件内容变了映射依然有效（内容变化由上面两层负责）。**一次 `exists` stat 校验就够，不需要 mtime。** 负结果不缓存（避免"新建文件后找不到"的陈旧结果）。

### mtime 还是摘要（hash）

**结论：mtime+size，不要 hash。**

- mtime+size：一次 stat，免读盘——这是缓存的核心收益（免 I/O + 免编码检测）。编辑器保存、git 操作必然更新 mtime；漏判（内容变时间不变）在 IDE 工作流里极罕见；误判失效（touch）无害，只是多重扫一次。
- hash：算摘要必须先读整个文件——缓存的免 I/O 收益直接被杀掉，只剩"免文本处理"的小头。且 IDE 内部修改本来就有失效路径（编辑器保存 → `invalidateFile` → `removeScannedFile` → 清条目），mtime 只是兜底外部修改。

## 阶段计划

### 阶段 0 — 测试集完善（先行，后续一切改动的前置门禁）

测试基建：

- 多文件 include 场景：资源文件放 `RedPandaIDE/test/resources/preprocessor/` 子目录（小文件集 + golden，可审查）；
- `fileOnlyIncludeOnce` true/false 两种模式参数化（QTest `_data` 行）——false 是 parser 真实模式；
- 每个场景三个层面的断言：`result()` 行序列（golden 对比）、defines 终态（`getDefine()`）、`fileInfo`（includes/directIncludes/branch 可见性）；
- 补 test-cppparser 的 `--suite=类名` 参数（AGENTS.md 既定要求）。

测试清单：

**A. 行尾 `\` 处理**（`combineLinesEndingWithBackslash`，现有 1 例）

- `\` 后带尾随空白（当前实现宽容——钉住现状）；
- 连续 3+ 行续行；预处理指令名中间续行（`#def\`+`ine`）；
- 字符串内续行与续行中的转义；文件末行以 `\` 结尾（无下一行）；
- CRLF 行尾；续行发生在块注释中间（先合并后剥注释，注释应完整）；
- 合并产生的空行与 result 行号对齐（result 行数 == 源行数）。

**B. 字符串/字符处理**（`replaceCommentsBySpaceChar` 与 `expandMacros` 的状态机）

- 字符串内 `//`、`/*` 不被当注释；`\"` 不终止字符串；
- 字符字面量 `'\''`、`'\\'`、多字符字面量；
- raw string：`R"(...)"`、`R"delim(...)delim"`、跨行、内部孤立的 `)` 不提前终止、内部注释样式文本不剥除；
- 字符串/字符内的宏名不展开；
- `R` 与标识符相邻（`FOO R"(x)"`、以 R 结尾的宏名）；
- `"` 在注释内不开始字符串（状态机交互）。

**C. 宏展开**（现有覆盖不错，补边界）

- `#` 字符串化：含空格的参数、含引号/反斜杠的参数（当前实现疑似不转义——钉住现状后定性）；
- `##` 在宏体首/尾、连续 `##`；
- 参数内逗号：括号内不拆（已有）、**模板尖括号内逗号会拆参数**（非标准偏差，钉住）；
- 空实参 `F(,)`、空 `__VA_ARGS__`；命名参数与 `__VA_ARGS__` 混合；
- 对象宏→函数宏的链式 rescan；互相递归调用的函数宏（防护仅靠 ignore 集，钉住防护有效）；
- 同名重定义（覆盖）；空体宏 `#define E`；
- 展开结果与相邻词的粘合/隔离边界。

**D. 条件判断**（`handleIf` 族 + `evaluateIf` 家族）

- 未定义标识符在 `#if` 中的行为（现状：保留名字→求值失败→false，钉住）；
- 3+ 层嵌套分支真假组合；假分支内 `#define`/`#include` 不生效；
- `#elif` 链（首真后后续分支不求值——借 defines 状态断言）；
- 假分支内函数式宏跨行调用（`mIndex` 不被错误推进）；
- `#else`/`#endif` 不配对的健壮性（钉住现状）；
- `and`/`or` 替代关键字（代码已有、无测试）；`__has_include`（代码已有、无测试）；
- 字符常量在 `#if`（现状不支持——钉住并定性）；
- `#elifdef/#elifndef`：CPP23 支持与不支持两条路径——启用死资源 `conditional1.cpp`。

**E. 预处理指令与 include（当前测试为零，最大黑洞）**

- 单级/三层嵌套 include：插入位置、`#include file:line` 标记行、行号映射、返回后继续正确行；
- 重复 include 三种形态：include guard（`#ifndef` 模式，guard 检测 `mFileIncludeOnceToken`）、`#pragma once`、无守卫重扫（defines 重放、内容重复）；
- guard 检测边界：guard 前有空行/注释/`#include` 时 `mFileJustOpenned` 的翻转行为（钉住）；
- `#include` 宏计算（`#include PATH_H`）；递归 include A→B→A（栈查重不死循环）；文件不存在静默跳过；`#include_next`；
- `#undef`：本文件宏、外部文件宏（`mFileUndefines` 机制）；`#error`/`#warning`/`#line`/其它 `#pragma` 忽略不炸；
- `<>` 与 `""` 的解析顺序（project 路径优先于 compiler 路径）；
- 行首空白后的 `#`（现状不识别——钉住并定性）。

**F. 增量/注册表行为（为重构与缓存铺路）**

- 同实例 preprocess 两次：第二次走 `addDefinesInFile` 重放路径，结果一致；
- `removeScannedFile` 后重预处理：defines 清除与 hard define 恢复（`invalidDefinesInFile`）、undef 恢复（`mFileUndefines`）正确；
- `mOnGetFileStream` 回调路径；
- `mFileCache` 单次运行内对无守卫重复包含文件的命中（`fileOnlyIncludeOnce=false` 模式）。

**G. 重放正确性（钉住现状中的错误语义，阶段 0.5 修复的依据）**

- **环境依赖头文件**：分支条件引用包含点环境宏的头文件（如 `#ifdef USE_FAST` 的 config.h），在两个不同环境的包含者先后包含时，第二个包含者得到的分支结果——按正确语义写期望（现状重放会给出第一个包含者的结果，测试预期失败，驱动阶段 0.5 修复）；
- **环境无关头文件**（条件只依赖 hard defines 和自身定义宏，含 guard 宏）：重放结果与重扫一致，长期安全；
- 无守卫文件跨 reparse 的重放行为（钉住后定性）；
- 外部修改：文件 mtime 变化后（模拟 git pull/外部编辑器），重放应让位重扫（现状无校验——钉住正确语义，驱动 mtime 校验）。

### 阶段 0.5 — 测试中暴露问题的定性与修复

逐个定性（bug / 已知限制），bug 修复独立提交、先于一切优化。候选清单（先由测试钉住再定）：

- **重放缓存安全化（确认的正确性修复，两项）**：环境依赖检测；重放的 mtime 校验（见上文"缓存的正确性分析"）；
- `#elifdef/#elifndef` 低版本 `__cplusplus` 下 handler 空转的语义（含死代码 `calcUnsupportedElseBranchResult`）；
- 行首空白 `#` 不识别；
- `MAX_DEFINE_EXPAND_DEPTH` 死常量（递归防护是否足够）；
- `#` 字符串化的转义处理；`#if` 字符常量；模板尖括号内逗号拆参数。

### 阶段 1 — 结构重构（仅限 preprocessor 内部，有阶段 0 测试保护）

1. **拆 Session/Registry**：`CppPreprocessor` 拆为 Registry（持久：defines/scanned/fileInfos/路径/缓存——阶段 3 缓存的归宿）与 Session（每次 preprocess 新建：include 栈/mIndex/mBuffer/分支栈/mProcessed/mFileCache）。对外接口保持不变。
2. **`handleBuffer` 显式化**：宏展开跨行消耗的行数显式返回，替代隐式推进 `mIndex`；消除 `expandMacros() const` 的 `const_cast`。
3. 小项：`mPreprocessorHandlers` 的 QString map 查找改 switch。

**推迟到 CppParser 层测试建立后（本次不做）**：

- 消除双重预处理（`sortFilesByIncludeRelations` 预扫改完整选项并保留结果）；
- `internalParse` 中 `mResult` 浅拷贝后 `clearTempResults` 触发 detach。

### 阶段 2 — 性能基线（优化动手前）

- QBENCHMARK 基准：宏展开吞吐（人造宏密集输入）、完整 preprocess（即时生成的深 include 链 + 宏密集头文件，CI 可重复）；
- 记录基线数据到提交信息；本机另用真实 MinGW 头文件手动对照。

### 阶段 3 — 性能优化（基线驱动，逐项提交）

1. `expandMacros()` 内循环重写：索引区间代替逐字符构词、reserve、`QMultiHash`→小 `QVector`、消除整表拷贝；保持 ISO 6.10.3.4 重扫语义。
2. `expandFunctionLikeMacro()`：单次扫描替换 `%1..%9`，替代循环 `QString::arg()`。
3. `openInclude()` 传播剪枝：祖先已 `including(fileName)` 时跳过闭包传播。
4. include 解析结果缓存：正结果缓存 + 存在性校验（一次 `exists` stat）；负结果不缓存；路径列表变化清缓存；`removeScannedFile` 清条目。
5. 文件内容缓存扩大：处理后行表 + mtime/size 校验，LRU 字节上限（内存有界），挂在 Registry 上。

### 文档同步

- `docs/architecture.md` parser 章节：同步 Session/Registry 拆分与缓存设计（相应阶段完成后）。

## 验证

1. 每个子项提交前构建并跑 test-cppparser 全量（本机若无输出走 gdb，与 test-qsynedit 既定做法一致）；
2. 阶段 0 完成 = 清单全部场景有测试且通过（暴露的问题进阶段 0.5）；
3. 阶段 1 完成 = 全部测试在不改断言的情况下通过（重构纯行为保持）；
4. 阶段 3 每项优化：测试全绿 + 基准对比记录；
5. 最终用户在真实 IDE 手动验证。

## 假设

- include 测试资源用仓库内小文件集（可审查 golden），不用临时目录即时生成；性能基准用即时生成（CI 可重复）；
- "钉住现状"的测试用例若后续定性为 bug，修复时同步改断言，提交信息记录定性理由；
- include 解析缓存接受"同名文件后加入更高优先级路径时命中旧缓存"的极小概率风险；
- 文件内容缓存 LRU 上限先取固定常量（如 64MB），基准后再调。
