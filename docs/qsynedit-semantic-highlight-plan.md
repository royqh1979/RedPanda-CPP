# QSynEdit 语义高亮与表达式批量求值设计

> 状态：设计稿，尚未实施。目标场景：老机器上的编辑、DND 和快速滚动。

## 1. 背景与问题

当前 painter 逐 token 调用 `Editor::onPreparePaintHighlightToken()`。每个 C++ identifier 都可能经历：

1. `getExpressionAtPosition()` 从当前位置向前扫描并猜测表达式起点；
2. 将 token 文本拼成字符串；
3. 再次 tokenize；
4. 对表达式前缀执行完整类型求值；
5. 将结果转换为 `StatementKind` 和绘制颜色。

链式表达式会重复求值所有前缀。例如 `a.b().c().d()` 中，各 identifier 分别触发 `a`、`a.b()`、`a.b().c()`、`a.b().c().d()` 的求值，总成本接近 O(n²)。DND 和快速滚动还会重复绘制相同表达式，进一步放大成本。

现有 `mIdCache` 已保存 `(line, ch, token) → StatementKind`，但只在 `mParser->parsing()` 时读取；解析完成后仍重新求值再写入缓存，存在直接冗余。

## 2. 目标

1. painter 每行只提交一次语义高亮准备请求，提交范围为行首到当前绘制截止位。
2. 不再执行“token → 拼接字符串 → 再 tokenize”的往返。
3. 使用 syntaxer 的注释、字符串和跨行状态批量收集 token，再从后向前扩展最小求值单元，不为每个 identifier 独立重复扫描。
4. evaluator 一次求值即可输出当前求值单元覆盖的各 identifier 中间结果。
5. 同一文档 revision 下复用表达式结果，避免 DND 每帧重复求值。
6. 不改变现有绘制、选区、word-wrap、IME preedit 和增量重绘行为。

## 3. 非目标

- 不开发第二套 C++ 表达式解析器或类型推导器；
- 不让 QSynEdit 依赖 CppParser；
- 不把表达式分析强行限制在单行；
- 不引入跨 eval、跨文档 revision 的全局求值结果缓存；
- 不在第一阶段改变导出器、代码补全或重构功能的表达式路径；
- 不用任意候选或猜测类型换取显示速度。

## 4. 总体结构

```text
QSynEdit syntaxer
  → painter 批量收集显示 token（行首到绘制截止位）
  → Editor 建立括号匹配并从后向前扩展最小求值单元
  → 带 SourcePosition 的 TokenList
  → 现有 CppExpressionEvaluator 对每个单元求值一次
  → identifier trace
  → 按 SourceRange 回写显示 token 的语义颜色
```

QSynEdit 只负责显示 token 和通用批量回调。表达式分组、语义求值及缓存归 Editor/CppParser 层，不进入通用 painter。

## 5. painter 批量接口

### 5.1 输入

建议每个输入项至少包含：

```cpp
struct HighlightToken {
    int ch;
    QString text;
    PTokenAttribute attr;
    TokenType tokenType;
};
```

`attr` 不足以替代 `tokenType`：当前 `onPreparePaintHighlightToken()` 还读取 `syntaxer()->getTokenType()`。批量接口必须显式传递，避免回调依赖 painter 正在使用的 syntaxer 状态。

每行调用一次批量准备接口，而不是每个 token 调用一次。输入只覆盖本次需要绘制的范围，不要求扫描整行或整个文档。

### 5.2 输出与绘制

接口回写每个 token 的 `FontStyles`、前景色和背景色。painter 继续使用现有 `tokenAccu` 合并及 `paintToken()` 路径，不改变：

- clip 与 word-wrap 段边界；
- 选区颜色及当前行颜色；
- 彩虹括号；
- 空白 glyph、行尾 glyph 和折叠文本；
- IME preedit 的临时 glyph 位置；
- 多视图各自的字体和布局。

painter 自身仍可使用 syntaxer state 处理上述显示规则，无需把所有绘制状态复制到批量接口。

## 6. 批量收集与反向最小求值单元

### 6.1 最小求值单元

求值单元只需要回答“目标 identifier 是什么以及如何解析它”，不必计算所在语句、函数调用或二元表达式的最终结果。仅为语法高亮时，函数不需要做重载决议；operator 也不做重载语义高亮，`operator`、`new`、`delete` 等关键字不参与 statement 查找。

基本规则：

- 独立 identifier 只解析自身，不附带其调用括号和实参；
- 成员访问必须包含完整接收者链和目标成员名，但不需要目标成员调用自身的实参；
- 只有函数调用的返回类型被后续接收者链使用时，才保留该调用的完整实参列表，以便按现有 evaluator 求出返回类型；
- 模板实例化必须保留必要的模板实参；
- 函数实参中的子表达式独立处理，除非已被某个接收者链求值单元覆盖；
- 二元、三元和赋值运算符两侧可以分别求值，不必为了高亮拼接整个表达式；
- `.`、`->`、`::` 等接收者链不能拆开。

示例：

- `a() + b()`：分别取 `a` 和 `b`；
- `a.b() + c.f()`：分别取 `a.b` 和 `c.f`；
- `f(a())`：分别取 `f` 和 `a`，不为了高亮 `f` 而求值完整 `f(a())`；
- `f(a()).b()`：为 `b` 取 `f(a()).b`，一次求值覆盖 `a`、`f`、`b`；`f(a())` 决定接收者类型，而 `b()` 的实参与 `b` 的高亮无关；
- `f(a()).b(c())`：为 `b` 取 `f(a()).b`，为 `c` 单独取 `c`；
- `a()\n + b()`：两行分别取 `a` 和 `b`；
- `a.\n b()`：必须保留前行的 `a`，形成 `a.b`；
- `a + b * c`、`condition ? x : y`：各 identifier 分别处理，不计算整个二元或三元表达式。

`foo(a.bar())` 在语言上是一条完整表达式，但用于高亮时分别处理 `foo` 和 `a.bar`。`lst.front().append("x")` 中为 `append` 保留 `lst.front().append`，并利用同一次 trace 覆盖 `lst` 和 `front`，但不带入 `append` 的实参。

分段器只负责确定求值单元，不重新解释 C++ 类型语义。接收者链中调用的实参和模板 `<...>` 的识别规则必须与 evaluator 一致，歧义场景保留测试。

### 6.2 批量数据与覆盖状态

painter 先收集 token，不立即求值。批量数据及处理状态包含：

- token 的文本、类型及 `line/ch` 位置，以及当前提取会话的嵌套层数；
- 每个 identifier 所需的接收者链；
- 仅当调用结果作为接收者时需要保留的完整实参范围；
- 已被求值单元覆盖、无需再次作为起点的 token 集合；
- 已确定的求值单元及其嵌套关系。

覆盖以下情况：

- namespace 成员、对象成员、指针成员；
- 函数调用、下标、模板实参及其实参；
- 一元 `*`、`&`、`new`、`delete`、`sizeof` 等前缀；
- 算术、比较、逻辑、位运算、赋值和条件运算符的边界；
- 返回语句、声明、赋值和控制语句；
- 嵌套调用及 lambda。

### 6.3 注释、字符串与续行

直接复用 syntaxer：

- Comment token 不参与表达式，但其源码位置仍保留；
- 跨行注释和字符串是否结束由 syntaxer state 判断，不自行解释 `//`、`/* */`；
- raw string 和普通字符串作为原子 token，不分析内部文本；
- 行尾 `\` 只表示物理续行，不加入表达式；
- 注释、空白和续行不会清空当前表达式状态。

不再需要 `CppTokenizer` 来判断注释或表达式起点。分段器直接向 evaluator 输出带位置的 token 列表，不重新拼接源码文本。

### 6.4 P1 核心算法：同层反向 postfix 链

不沿用旧 `LastSymbolType` 状态机，也不逐步增加运算符特判。核心只识别 C++ primary/postfix 结构，不解析完整表达式，不判断类型或重载。

#### 数据形式

调用方传入的 syntaxer 经 `createInstance()` 创建会话实例，保留自定义设置；token 保留文本、类型及 `line/ch`。空白和 Comment 不进入语义 token 流；目标位于 Comment 或 String 时直接返回空。raw string、普通字符串和续行由 syntaxer/state 处理。

提取使用单一 back-to-front 游标，不记录 token id 或 match 指针。每个提取会话只保存当前 `()[]{}` 和模板 `<...>` 的嵌套层数。提取结果首先表示为 token 范围，需要时再生成 `QStringList`；token 的 `line/ch` 同时供 P2 标记已覆盖位置，避免重复提取。

#### 跨行文本与匹配

SyntaxState 只用于其可靠的词法职责：识别注释、普通字符串、raw string、转义和 `mergeWithNextLine` 续行，排除其中的伪括号。P1 不使用 `lineSeq` 或括号 level 辅助表达式匹配。

从目标位置向前读取必要的 Document line；一旦某行属于可能纳入结果的表达式，该行及起始行到目标行之间的内容都正常 token 化。反向扫描只用当前嵌套层数验证括号配对，不回看已经处理的 token，到达表达式边界立即停止。只有后续 profile 证明跨行扫描是热点时，才另行评估索引或缓存。

P1 向现有 evaluator 提供完整 token 流，因此 lambda 的 `{...}` 也不能作为不透明整体省略：extractor 不知道后续求值是否需要函数体，而现有 evaluator 在没有 trailing return type 时会扫描顶层 `return` 推导返回类型。目标位于函数体外时，组内各行仍需保留；目标位于函数体内时，则在该组自己的边界内正常提取。本方案不引入 opaque group、惰性 token 化或按需回填函数体的接口。

#### 唯一扩展规则

1. 目标 identifier 自身是初始结果，不读取它后面的调用或下标。
2. 仅当目标右侧是其模板实参时，额外保留完整的 `<...>`。
3. 从目标向左读取一个 primary/postfix atom：
   - identifier、`this` 等原子直接结束该 atom；
   - 遇到 `)`、`]`、`}` 时，按当前嵌套层数读入整个组，再继续读取它左侧的 callable/base；如果左侧没有 callable/base，则该组自身就是 parenthesized primary；
   - 接收者需要的模板 `<...>` 与调用 `(...)`、下标 `[...]` 一样完整保留。
4. atom 左侧如果是 `.`、`->`、`::`、`.*`、`->*`，则保留该运算符并继续读取左侧接收者 atom；否则提取结束。
5. 遇到当前子表达式边界、逗号、二元/赋值运算符、语句关键字或其它非接收者 token 时结束，不把一元或二元表达式整体纳入。

目标自身的 `()`/`[]` 不读，因此独立函数不做重载决议；接收者的 `()`/`[]` 必须读，因为其结果决定后续成员类型。`new` 等会改变接收者结果类型的前缀只在接收者链确有需要时保留；普通一元 `*`、`&` 不跨越 postfix 链额外吸收，位于括号组内时则随组自然保留。

#### 同层边界

先确定目标所在的最内层括号组，反向扫描不得越过该组的起始位置。这样不需要针对每个嵌套调用编写特判：

- `f(a())` 中的 `a` 以最内层 `(` 为边界，只能得到 `a`；
- `f(a()).b()` 中的 `b` 位于外层，左读时把 `f(a())` 作为完整接收者；
- `f(a()).b(c())` 中的 `c` 同样受自己的参数组边界约束，只得到 `c`。

模板 `<...>` 不能全局当作括号。只有左右结构都符合模板上下文时才建立匹配：左侧能附着于 identifier/模板结果，右侧能继续 postfix、类型或目标结尾。否则 `<`、`>` 就是当前层的表达式边界。例如 `a < b` 中的 `b` 只取 `b`，`a > foo().b` 中的 `b` 只取 `foo().b`，不能误吸左侧的 `a`。

#### 失败与复杂度

不能组成完整接收者链时返回失败，由调用方走现有未知 identifier 回退，不能把缺少接收者的成员名当作全局 identifier 猜测。合法但未声明的 identifier 仍返回自身，由 evaluator 得出未知结果。

单次提取只移动索引并跳转匹配组，不递归扫描嵌套实参，工作量与实际纳入表达式的 token 数成正比。P1 不引入 AST、缓存或第二套类型系统。

典型结果：

| 源码与目标 | 提取结果 |
| --- | --- |
| `f(a())` 中的 `f` | `f` |
| `f(a())` 中的 `a` | `a` |
| `f(a()).b()` 中的 `b` | `f(a()).b` |
| `f(a()).b(c())` 中的 `c` | `c` |
| `foo(a.bar())` 中的 `bar` | `a.bar` |
| `a[i].b` 中的 `b` | `a[i].b` |
| `a[i].b` 中的 `i` | `i` |
| `std::vector<int>::iterator` 中的 `iterator` | `std::vector<int>::iterator` |
| `a./* comment */b` 中的 `b` | `a.b` |
| `a.` 后换行的 `b` | `a.b` |

### 6.5 P2 整行反向调度

P1 验证后，整行处理才增加调度：

1. painter 收集到绘制截止位，复用同一份反向 token 流；每个提取会话独立维护嵌套层数；
2. 从最后一个尚未覆盖的 identifier 开始，调用 P1 的同一提取实现取得最小范围；
3. 对该范围执行一次 eval trace；
4. 将该范围覆盖的 token 标为已完成，trace 只为其中有语义结果的 identifier 生成颜色；
5. 继续处理前一个未覆盖 identifier，不能重新扫描已被覆盖的接收者链。

例如 `f(a()).b()` 从 `b` 得到 `f(a()).b`，一次求值覆盖 `a`、`f`、`b`；`f(a()) + b()` 则分别处理 `f`、`a`、`b`。只有 `a.\n b()` 这类连续接收者链才读取前行内容，`a()` 与下一行的 `+ b()` 可分别按行求值。

括号匹配和覆盖状态可按文档 revision 缓存，DND 继续复用。目标是每个求值单元只扫描一次，而不是每个 identifier 扫描一次。

## 7. evaluator trace

### 7.1 接口

在现有 evaluator 上增加可选 trace，不另建并行解析路径：

```cpp
struct EvalTraceEntry {
    SourceRange identifierRange;
    EvalResult result;
};
```

保留现有 `evaluate()` 包装，并新增接受带位置 `TokenList` 和 trace 输出的接口。旧 `QStringList` 调用可保留为兼容包装，但新路径不再 `join(' ')` 或重新 tokenize。

### 7.2 记录点

trace 只记录源码表达式中的 identifier 结果：

- primary identifier 解析完成；
- `::` 成员解析完成；
- `.` / `->` 成员解析完成；
- 最终 `EvalResult` 仍按原方式返回。

独立函数 identifier 不触发重载决议；只有其返回类型用于后续接收者链时，才按现有 evaluator 对完整调用求值。

trace 不放进 resolver，避免模板展开、默认参数和内部类型解析混入非源码 identifier。lambda 内部生成的临时 statement 不得写入 StatementModel，沿用现有只读查询约束。

例如，为 `lst.front().append("x")` 中的 `append` 求值时使用：

```cpp
lst.front().append
```

一次求值输出：

- `lst` → 变量 statement；
- `front` → 函数 statement，返回 `basic_string&`；
- `append` → 函数 statement，返回 `basic_string&`。

链长为 n 时，由约 n 次前缀求值降为一次接收者链求值，消除链式 O(n²) 的主要来源。

### 7.3 结果后处理

每条 trace 需要沿用现有单 token 路径的后处理：

- `visibleStatement()` 的文件可见性；
- alias 解引用；
- 构造函数后没有 `(` 或 `{` 时回退到父类 statement；
- 未知 identifier 的函数/变量回退规则；
- `StatementKind` 到颜色及字体样式的映射。

这些规则应集中复用，避免 trace 与旧接口产生不同高亮语义。

## 8. 缓存与失效

### 8.1 缓存层级

建议区分：

1. **token 级颜色缓存**：现有 `mIdCache`，保存 `(line, ch, token) → StatementKind`；
2. **表达式级 trace 缓存**：以表达式文本、作用域和文档 revision 为 key；
3. **批量扫描状态缓存**：保存括号匹配和 token 覆盖状态，按 revision 失效。

首先修正 `mIdCache`：在现有失效规则允许时也供 parser 完成后的绘制使用，而不是只在 parsing 时读取。

### 8.2 生命周期

所有语义缓存都受模型和文档 revision 约束：

- `onParseFinished()` 继续执行现有失效处理；
- 文档内容变化后，只失效受影响表达式、批量扫描状态及依赖变化内容的跨行接收者链；
- parsing 期间可沿用旧缓存以避免闪烁，完成后按新模型重建；
- 不允许把旧 statement 指针用于已失效模型；
- evaluator 在查询期不得修改 StatementModel。

多视图可共享同一 Editor/Parser 的语义结果，但布局、字体和 painter 缓存仍按视图独立。

## 9. 正确性要求

新旧路径必须逐 identifier 对照，不只比较最终表达式类型。至少覆盖：

- 变量、函数、类型、namespace 和 alias；
- 构造函数、析构函数和 overloaded operator；
- 独立重载函数不做重载决议，以及重载函数返回类型用于接收者链时保留完整调用；
- `std::vector<std::string>` 等模板与嵌套模板；
- `foo(a.bar())`、`f(a())` 和 `f(a()).b()` 等嵌套表达式；
- `return`、声明、赋值和控制语句；
- 注释位于标识符之间、注释跨行、行尾 `\`；
- raw string、普通字符串和转义；
- 跨行表达式及 lambda；
- 不可见预处理行和 `#include` 特殊着色；
- 未知 identifier 的现有回退；
- IME preedit、word-wrap、选区和多视图。

extractor 自身不生成 `matchIndex`；P4 构造 evaluator TokenList 时，其 `matchIndex` 必须与 evaluator 的括号跳过规则一致。独立函数和参数表达式不得被强制并入完整调用；只有接收者链需要调用返回类型时才保留相应实参。

## 10. 性能验收

先在 GCC 15.3/C++17、Release 下建立基线，再比较优化结果：

- 每个绘制帧的 `findStatementOf` 和 eval 调用次数；
- 表达式扫描、tokenize、eval 和 painter 各阶段耗时；
- 不同长度链式表达式的调用次数与耗时；
- 大文件连续 DND、快速滚动和光标移动的总绘制耗时；
- `miniaudio.h` 的实际绘制压力场景。

长链测试应验证：表达式求值次数从“每个 identifier 一次”降为“每个最小求值单元一次”，并核对 trace 与旧结果一致。

不能仅以单帧时间或完整 CTest 时间推断收益。需要分别记录冷缓存和同 revision 热缓存结果；老机器目标以实际 DND 帧耗时和主观流畅度共同验收，不预设所有机器统一毫秒阈值。

## 11. 实施阶段

### P0：基线

- 记录现有调用次数和 DND 基线；
- 不改变生产代码行为。

### P1：单 identifier 的最小求值单元

- 输入一个目标 identifier 及 syntaxer token/state，输出该 identifier 所需的最小 TokenList 和源码范围；
- 完整处理独立 identifier、成员接收者链、接收者所需调用实参、模板、注释、字符串和续行；
- 独立函数不带调用实参，`f(a()).b()` 中的 `b` 提取为 `f(a()).b`；
- 暂不修改 painter 或 evaluator 接口，也不接入整行批量调度；
- 先用单 identifier 测试把提取规则和边界情况做完善，再进入 P2。

### P2：整行表达式拆分

- 在 P1 之上增加批量收集、括号匹配和覆盖状态；
- 从后向前选择尚未覆盖的 identifier，并调用 P1 的同一提取实现，禁止另写一套起点推断规则；
- 利用一次求值单元的覆盖范围跳过内部 identifier，同时核对每个 identifier 的提取结果仍与 P1 单独调用一致；
- 覆盖嵌套调用、长接收者链、跨行接收者链、二元表达式及未知 identifier；
- 此阶段仍不修改 painter 或 evaluator 接口。

### P3：painter 批量接口

- 增加每行批量准备接口，接入已经验证的整行拆分；
- 初期保持逐 identifier 求值语义，验证 clip、IME、选区、word-wrap 和多视图；
- 保留旧路径用于逐 token 对照。

### P4：evaluator TokenList、trace 与缓存

- evaluator 接受带位置的 TokenList，移除新路径的拼字符串和重新 tokenize；
- 输出 identifier trace，并按最小求值单元批量回写颜色；
- 修正 `mIdCache` 使用，按 revision 缓存表达式 trace 和批量扫描状态；
- 新旧逐 identifier 结果对照，并分别测量冷缓存和热缓存。

### P5：回归与性能验收

- 先 Release 后 Debug 跑全量 CTest；
- 对比长短链和 DND 基线；
- 如仍有热点，再根据 profile 决定下一步，不预先扩大到完整语句或函数体分析。

## 12. 回退策略

各阶段都应能保留旧路径作为对照。若批量接口、表达式分段或 trace 出现语义差异，可以仅关闭对应新路径，不影响 painter 的基本绘制和现有逐 token 求值逻辑。不得通过删除旧测试或放宽 identifier 结果断言来接受新实现。
