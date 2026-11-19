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

- token 的文本、位置和 `matchIndex`；
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

### 6.4 反向扩展步骤

不在扫描到 identifier 时立即求值，而是先完成本行 token 收集，再从后往前处理：

1. painter 收集到绘制截止位，并一次性建立括号匹配；
2. 从最后一个尚未覆盖的 identifier 开始，向前扩展其接收者链；仅当某个调用的结果属于接收者链时，才纳入该调用的完整实参；
3. 对最小求值单元执行一次 eval trace；
4. 将该求值单元覆盖的所有 token 标为已完成，trace 只为其中有语义结果的 identifier 生成颜色；继续处理前一个未覆盖 token；
5. 利用 `matchIndex`、已有求值单元和覆盖集合跳过重复工作，不能每个 token 都独立倒退扫描。

例如 `f(a()).b()` 从 `b` 扩展到 `f(a()).b`，一次求值即可覆盖 `a`、`f`、`b`；`f(a()) + b()` 则分别处理 `f`、`a`、`b`，不为了高亮 `f` 或 `b` 进行重载决议。只有 `a.\n b()` 这类连续接收者链才读取前行内容，`a()` 与下一行的 `+ b()` 可分别按行求值。

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

分段器生成的 `matchIndex` 必须与 evaluator 的括号跳过规则一致。独立函数和参数表达式不得被强制并入完整调用；只有接收者链需要调用返回类型时才保留相应实参。

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
