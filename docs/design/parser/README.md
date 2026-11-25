# Parser 设计文档

## 阅读顺序

1. `cppparser-design.md`：声明解析、Statement、作用域、导航和项目引用查找。
2. `cpppreprocessor-design.md`：预处理、边界、ReplayRecord、文件变化和缓存。
3. `cpptype-evaluation-design.md`：类型解析、模板绑定、表达式求值和重载选择。
4. `statement-design.md`：Statement 类系、所有权、occurrence、失效、namespace 和可见性。
5. `parser-lifecycle-design.md`：Configuration/Session/Registry、并发、Editor 关闭和应用退出。

这些文档描述当前设计和实现约束，不按提交记录重复堆砌修改过程。

`implementation-review.md` 单独记录已经核实的文档/实现不一致及待讨论项；审查结论未确认前，不直接改写对应设计语义。

## 核心解析

### CppParser

`cppparser-design.md` 覆盖：

- Token、TokenCursor 和声明级递归下降；
- DeclarationInfo、StatementBuilder 和声明/定义合并；
- StatementModel、作用域和源码位置；
- 普通表达式中的 lambda 导航；
- 当前主文件正确性与逐 TU Refactor 边界；
- 基本 profile 约定。

### CppPreprocessor

`cpppreprocessor-design.md` 覆盖：

- Session 与跨轮 Registry 的职责；
- 边界内外、跨边界祖先和 ReplayRecord；
- include 处理、宏环境、有序前缀和文件变化；
- Statement 的声明/定义归属及失效；
- 缓存与性能约束。

### 类型与表达式求值

`cpptype-evaluation-design.md` 覆盖：

- 有限模板实例化、别名和模板绑定；
- 主模板与特化选择；
- 类型转换、函数调用和重载候选；
- Eval 上下文、缓存、终止和失败语义。

### Statement 与 namespace

`statement-design.md` 统一原来的 namespace 唯一实体与透明性文档，并扩展为 Statement 通用设计，覆盖：

- Statement 类系、StatementModel 所有权和 StatementFactory；
- StatementOccurrence、declaration/definition 来源与部分失效；
- 命名/匿名/inline namespace；
- InlineAliasStatement 和直接父级投影；
- 直接 children、InlineAlias、补全和视图；不另设 namespace 专用索引。

### 状态与生命周期

`parser-lifecycle-design.md` 统一原来的状态分类、并发审查和关闭/退出文档，覆盖：

- Configuration、Session、Registry；
- clear/reset/reconfigure/invalidate 契约；
- Parser 的 shared_ptr/weak_ptr 所有权；
- public 查询与 private `do*` 的并发分工；
- Editor 隐藏、批量打开关闭、Close All 和应用退出。

## 测试与验收

- `testing/syntax-coverage.md`：按语法类别记录支持、部分支持、不支持、未覆盖及测试证据。
- `testing/replay-scenarios.md`：预处理边界、Replay、失效、Statement 生命周期和缓存命中的场景清单。
- `testing/public-api-baseline.md`：C/C++ 公开接口独立基准、生成来源、编译开关和 eval profile。

性能基线与结果不放设计文档，统一由 `docs/profile/README.md` 索引。

## 方案与历史材料

- `proposals/parser-reparse-unification.md`：当前统一重解析、多 TU 和 include paths 的方案及审核记录；不属于已确认的现状实现。
- `proposals/cpp17-support-plan.md`：按能力分层扩展 C++17 的后续计划，不代表所有列举能力已经实现。
- `history/cppparser-rewrite-design.md`：声明级递归下降重写前的方案比较、现状问题与历史实施记录。
- `history/parse-time-declaration-merge.md`：Parse 阶段声明/定义合并实验及验收过程。
- `history/parser-rewrite-handoff.md`：Parser 重写阶段交接与历史性能记录。

proposal 和历史材料不得覆盖核心文档中已经核实的现状；新方案被接受并实施后，再合并到对应核心文档。
