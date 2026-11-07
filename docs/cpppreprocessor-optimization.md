# CppPreprocessor 优化计划（正确性优先）

> 状态：阶段 0（测试集）、0.5（12 项定性修复）、**0.6（宏展开核心重写）已完成**——pp-token 引擎已全面接管预处理管线并删除旧引擎。分支 `parser-preprocess-opt`。后续：阶段 1（Session/Registry 拆分）、阶段 2（性能基线）、阶段 3（缓存等优化）未开始。本文是计划与设计结论的记录；具体修改记录见 git 提交信息。

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

逐个定性（bug / 已知限制），bug 修复独立提交、先于一切优化。

**阶段 0 已确认的发现**（均有钉住测试，测试名见括号）：

1. **重放缓存不安全（两个 XFAIL）**：
   - 环境依赖头文件：分支条件引用包含点环境宏时，重放给出首次包含环境的结果（`test_replay_env_dependent`）——修复方案见上文"缓存的正确性分析"（环境依赖检测）；
   - 外部修改已扫描头文件后仍重放旧 defines（`test_replay_external_modification`）——修复方案：重放前 mtime 校验。
2. **raw string 支持失效**（`test_raw_string_*`、`test_raw_string_pipeline_canary`）：`replaceCommentsBySpaceChar` 的关闭判定要求文本以 `)` + delimiter 结尾，但 delimiter 被错误地塞入了开引号（且自定义 delimiter 字符从未被记录），导致 raw string 状态**永不终止**并跨行泄漏；后果链：raw string 之后的注释不被剥离 → `expandMacros`（不懂注释）会把注释里的宏名展开。
3. **C++23 特性检测失效**（`test_elifdef_cpp23_detection`）：`addDefineByParts` 用 `QString::toLong` 整体转换 `__cplusplus` 的值，真实的带 `L` 后缀值（`202302L`）必然转换失败 → `#elifdef/#elifndef` 在实践中永远走降级空转路径（`conditional1.cpp` 的 legacy golden 即此行为）；表达式求值路径（`evalNumber`）能正确处理后缀，两条路径不一致。另有死代码 `calcUnsupportedElseBranchResult`。
4. **分支可见性映射 off-by-one**（`test_nested_branches`）：`removeCurrentBranch` 用 `mIndex` 而 `setCurrentBranch` 用 `mIndex+1` 记录可见性恢复点，导致假分支最后一个内容行被标为可见（`result()` 内容正确，仅编辑器置灰用的 branch map 错）。
5. **`__has_include` 语义偏差**（`test_has_include`）：返回的是"根文件是否已包含该文件"（`fileInfo->including()`），而非"文件是否存在"。
6. **`#` 字符串化不转义**（`test_stringize_escaping`）：参数内的 `"`、`\` 不转义，产生病态字符串字面量；且参数列表内的引号会破坏参数括号匹配的朴素字符串状态机，导致宏完全不展开。
7. **`tokenizeValue` 吞空格**（`test_empty_varargs`）：`#` 前的空格 token 被 `#` token 覆盖丢弃，展开结果少一个空格。
8. **模板尖括号内逗号拆分宏参数**（`test_args_comma_in_template_angle`）：不跟踪 `<>` 层级，多余参数被静默丢弃（非标准偏差）。
9. **行首空白后的 `#` 不识别为指令**（`test_leading_space_directive`）：C 标准允许 `#` 前有空白。
10. **`#if` 不支持字符常量**（`test_char_constant_in_if`）。
11. **未定义标识符在 `#if` 中不按标准替换为 0**（`test_if_undefined_identifier`）：保留原名导致求值失败按 false 处理；`UNDEFINED == 0` 应为 true 实得 false。
12. **`MAX_DEFINE_EXPAND_DEPTH` 死常量**：递归防护实际只靠 ignore 集合（`test_mutual_recursion_function_macros` 钉住防护有效）。

**已知不支持（记录在案，用户确认暂无计划实现）**：

- **二合符（digraphs）**：`<%`、`%>`、`<:`、`:>`（`{`、`}`、`[`、`]`）、`%:`（`#`）、`%:%:`（`##`）。C/C++ 均有效的替代标点，但真实代码使用率≈0（EBCDIC 时代遗产）；全代码库（preprocessor/cpptokenizer/syntaxer）均不处理。若将来实现：指令行识别接受 `%:` 前缀、token 层映射、字符串/字符字面量内豁免。
- **三字母符（trigraphs）**：`??=`、`??/`、`??'`、`??(`、`??)`、`??!`、`??<`、`??>`、`??-`（`# \ ^ [ ] | { } ~`）。**C++17 与 C23 已从标准删除**，GCC 的 C++ 模式默认忽略；不做。

### 阶段 0.6 — 宏展开核心重写（pp-token 流式展开器）

**动机**：宏展开实现的别扭是结构性的，补丁无法根治——`formatValue` 模板 + `QString::arg`（衍生出 9 参数上限、宏体内字面 `%` 误替换、`%%` 收尾补丁、`##` 靠占位符相邻拼接等整类问题）；普通行与 `#if` 两套展开引擎行为分叉；防自递归"蓝漆"按位置记录（每次展开整表拷贝+位置平移）；rescan 用字符串替换+回退（O(n²) 且位置簿记脆弱）；`handleBuffer` 跨行突变 `mIndex`。

**总体步骤**（只换宏展开引擎，其余骨架不动）：

```
读文件（readFileToLines / 回调）
   → ① 续行合并（不变）
   → ② 注释剥离（不变）
   → ③ 主扫描循环（原 preprocessBuffer，仍按行驱动——指令以行为单位）
        ├─ 指令行：#define → parseArgs 预编译【替换列表】存入 Define（换掉 formatValue）
        │         #undef / include / pragma 等（不变，含 guard 检测、重放、mtime、环境依赖判定）
        │         #if/#elif → 同一个 token 展开器展开表达式 → token 流求值（引擎合一）
        └─ 非指令行：喂给【token 展开器】，输出按行写 mResult；
              跨行函数式宏由展开器通过显式游标超前消费，被消费行补空行（与现状逐字节一致）
   → ④ closeInclude / 文件栈恢复（不变）
   → mResult (QStringList)（接口不变，留给将来结构化）
```

**展开器设计**：

- **PpTokenizer（拉式）**：按 token 拉动才前进。token 类型：标识符、数字、字符串、字符、raw string（可跨行）、标点（`#`/`##` 单列）、空白（只记录"前面是否有空白"标志，不发射）。raw string 状态可跨行，普通字符串/字符不跨行（与现状态机一致）。
- **替换列表（核心数据结构，杀 formatValue）**：宏定义时一次性预编译——
  ```cpp
  struct ReplToken {
      enum Kind { Text, Argument } kind;
      QString text;        // Kind==Text: 源文本切片
      int argIndex;        // Kind==Argument
      bool stringize;      // 前导 #
      bool pasteBefore;    // 前导 ##
  };
  // Define::replacementList: QVector<ReplToken>
  ```
  展开 = 单趟遍历发射；`#` 现场转义拼串；`##` 与前一发射 token 合并重 lex（合并出的新标识符经 rescan 帧可再参与展开，语义天然正确）。
- **输入上下文栈**（rescan 与跨行）：`[文件游标 (buffer, line, col)]` 之上压"替换片段帧"。读 token 先弹栈顶帧，弹完回落游标——ISO 6.10.3.4 直译。跨行参数收集 = 游标前进（替代 handleBuffer 的 mIndex 突变，原计划"handleBuffer 显式化"并入本阶段）。
- **蓝漆（防自递归）**：放弃位置表，改**展开帧内名字集合**——展开宏 M 时压帧 {替换序列， M}，帧存活期间 M 不可再展开（帧栈通常个位数深度，线性查即可）。
- **参数收集**：token 级深度计数，depth-1 逗号切分；每个参数是收集序列上的区间视图（零拷贝）；`#`/`##` 用处的参数不预展开，其余用处的参数先展开（标准语义）。
- **输出与空白还原**：Text 段保留源文本切片；参数间空白由"前面是否有空白"标志 + 防粘连检查（相邻 token 拼接会改变 lex 时补空格）决定；golden 测试逐字节验收。
- **`#if` 求值**：条件表达式喂给同一展开器（`defined`/`__has_include` 作为 token 级运算符处理），展开后文本送现有递归求值器；条件引用记录（环境依赖判定）成为 token 扫描的副产品。

**顺带解决的问题**（重写自然覆盖，不需单独补丁）：宏参数 9 上限、宏体内字面 `%` 误替换、`#define` 参数列表 `split(',')` 粗切（参数内带逗号如 `void(*)(int)` 会切错）、双引擎分叉、位置蓝漆拷贝、handleBuffer 隐式推进、`expandMacros() const` 的 `const_cast`。

**兼容层（对外接口不变）**：`expandMacros(QString)`、`expandMacrosInConditioningExpression(QString)`、`evaluateIf(QString)` 保持签名，内部改走新引擎；`tokenizeValue`（cppparser 用于宏提示）独立保留。

**迁移步骤（旁路实现，测试驱动切换，不一次性断代）**：

1. PpTokenizer（含跨行 raw string）+ 单测；
2. 替换列表预编译（`parseArgs` 产出 `replacementList`）+ 单测；
3. PpExpander（上下文栈、蓝漆帧、参数收集、发射与空白还原）+ 单测（复用阶段 0 宏展开用例）；
4. 接入主循环（显式游标、行对齐输出）——阶段 0 全部 golden 为门禁；
5. `#if` 求值同引擎；`#define` 参数列表 token 化切分（顺带修 `split(',')` 问题）；
6. 兼容层 + 旧代码（expandMacros 全家、formatValue、argNotExpand、tempIngoreMacros）清理。

**实现完成时的要点记录**（与设计稿的差异/补充）：

- **`mPopFloor` 水位线**：帧的惰性弹出不得越过子泵自身的帧基——外层已耗尽帧仍承载着界定嵌套参数预展开作用域的蓝漆（互递归测试 hang 暴露，已入引擎注释）。
- **Text 条目保持每 token 一条**（不合并）：合并会把宏名藏进字符串导致 rescan 无法再次展开。
- **空白保留为精确文本**（`whitespaceBefore: QString`，含 tab），非布尔标志。
- **可变参数完整支持**：`...`、`__VA_ARGS__`、GNU `, ## __VA_ARGS__`（空则丢逗号、非空不粘合）、`__VA_OPT__`（C++20）——旧引擎前两者半残、后两者没有。
- **宏名与 `(` 之间允许换行**（peek 跳过 EOL 并计入消耗行；未找到则回推全部 peek 过的 token）。
- **condition 模式全套 iso646 替代关键字**（11 个，旧实现只有 and/or）。
- **EOL token 携带空白**：纯空白行与行尾空白不丢失。
- **统一后行为差异（已记录并更新钉住预期）**：旧"条件展开器"在 `SCAN(EXAMPLE(5))` 场景产出双空格，普通路径产出单空格；新引擎统一为单空格。
- **已完成清理**：expandMacros 全部旧重载、expandMacro、expandFunctionLikeMacro、parseArgs、formatValue/argUsed/argNotExpand/argStringized/varArgIndex 字段、calcUnsupportedElseBranchResult、无用字符分类静态函数均已删除；`tokenizeValue`（cppparser 宏提示用）与 `eval*` 求值家族保留。

### 阶段 1 — 结构重构（仅限 preprocessor 内部，有阶段 0 测试保护）

1. **拆 Session/Registry**：`CppPreprocessor` 拆为 Registry（持久：defines/scanned/fileInfos/路径/缓存——阶段 3 缓存的归宿）与 Session（每次 preprocess 新建：include 栈/mIndex/mBuffer/分支栈/mProcessed/mFileCache）。对外接口保持不变。
2. ~~**`handleBuffer` 显式化**~~：**并入阶段 0.6**（token 展开器的显式游标即此问题的落地）；消除 `expandMacros() const` 的 `const_cast`（随 0.6 兼容层解决）。
3. 小项：`mPreprocessorHandlers` 的 QString map 查找改 switch。

**推迟到 CppParser 层测试建立后（本次不做）**：

- 消除双重预处理（`sortFilesByIncludeRelations` 预扫改完整选项并保留结果）；
- `internalParse` 中 `mResult` 浅拷贝后 `clearTempResults` 触发 detach。

### 阶段 2 — 性能基线（优化动手前）

- QBENCHMARK 基准：宏展开吞吐（人造宏密集输入）、完整 preprocess（即时生成的深 include 链 + 宏密集头文件，CI 可重复）；
- 记录基线数据到提交信息；本机另用真实 MinGW 头文件手动对照。

### 阶段 3 — 性能优化（基线驱动，逐项提交）

1. ~~`expandMacros()` 内循环重写~~、~~`expandFunctionLikeMacro()` 单次扫描替换~~：**已由阶段 0.6 的 pp-token 重写取代**（本两项删除）。
2. `openInclude()` 传播剪枝：祖先已 `including(fileName)` 时跳过闭包传播。
3. include 解析结果缓存：正结果缓存 + 存在性校验（一次 `exists` stat）；负结果不缓存；路径列表变化清缓存；`removeScannedFile` 清条目。
4. 文件内容缓存扩大：处理后行表 + mtime/size 校验，LRU 字节上限（内存有界），挂在 Registry 上。

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
