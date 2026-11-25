# Parser 设计与实现一致性审查（2026-11-25）

## 审查基线

- 各项以审查时的 `HEAD` 实现为现状基线；当时尚未完成的重解析 WIP 不作为现状。
- 本文只记录已经由代码或测试证实的不一致，不把待分析事项写成实现 bug。
- 每项先讨论应更新文档、调整设计还是修改实现；未讨论前不改变现有行为。
- 行号用于定位，后续修改后应重新核对。

## D01 namespace 专用索引与透明 DFS 已不存在（已确认并修正文档）

**文档声明：**

- `cppparser-design.md` 的 Statement 所有权、重开 namespace 查找和生产接入段落仍描述 namespace fullName 多值索引、透明 DFS 和 `includeInline` 的索引语义。
- 合并后的 `statement-design.md` 第 5 节也保留了这一旧模型。

**实现证据：**

- `StatementModel` 只有 `mStatements` 和 `mGlobalStatements`，没有 namespace 专用索引，见 `statementmodel.h:78-81`。
- `CppParser::doFindNamespace()` 直接通过 Resolver 查找，并明确 `Q_UNUSED(includeInline)`，见 `cppparser.cpp:577-585`。
- Resolver 当前通过 children 和 `InlineAliasStatement::target` 继续查找，见 `cpptyperesolver.cpp:3584-3658`。

**处理结论：**当前设计确认为“命名 namespace 按实际 fullName 全局合并 + InlineAlias 投影”。已删除核心文档中的旧索引/DFS描述；只有出现新的性能证据时才重新评估索引。

## D02 “重开 namespace 由多个 Statement 表示”与唯一实体冲突（已确认并修正文档）

**文档声明：**`cppparser-design.md` 的“重开 namespace 的限定名查找”仍称只有 namespace 会因重开而由多个 Statement 表示同一逻辑作用域。

**实现证据：**Builder 的 namespace 路径复用同一 Statement，并通过 `addNamespaceDeclaration()` 添加 occurrences；现有跨 root 测试也要求 namespace 对象保持唯一。

**处理结论：**已删除多 Statement 前提，明确同一实际 fullName 在文件和 TU 间也只使用一个 NamespaceStatement。

## D03 函数异常规格的文档描述过旧（已确认并修正文档）

**文档声明：**`cppparser-design.md` 的 DeclaratorInfo 段称“当前 Statement 不保存异常说明，因此 noexcept 只需正确消费”。

**实现证据：**

- `TypeDerivation` 和 `Statement` 都保存 `noexceptKind`、`noexceptExpression`，见 `cpptypes.h:147-151`、`parserutils.h:243-249`。
- Builder 在 `cppstatementbuilder.cpp:872-876` 保存它们。
- Resolver 在 `cpptyperesolver.cpp:1200-1210` 对选中调用求值异常规格。

**处理结论：**核心文档已改为声明期保存、合并时同步、求值时读取实际调用；未覆盖路径返回 Unknown。实现无需修改。

## D04 Class Browser 不再按项目收集全部 FileInfo（最终设计已确认）

**文档声明：**`cppparser-design.md` 末尾称“Class Browser 从当前 FileInfo（项目模式为所有项目 FileInfo）的 statements”建树。

**实现证据：**`ClassBrowserModel::fill()` 只把 `mCurrentFile` 对应的 FileInfo 加入列表，见 `classbrowser.cpp:297-303`；`setCurrentFile()` 在文件变化时直接清空，见 `classbrowser.cpp:513-519`。

**处理结论：**只展示当前文件是最终设计，即使文件属于项目也不收集其它项目 FileInfo。核心文档已按此修正。

## D05 项目中的 Rename 不再局限于当前文件（已确认并修正文档）

**文档声明：**`cppparser-design.md` 末尾仍称 `renameSymbol` 只允许声明和定义位于当前文件，不处理未打开文件。

**实现证据：**

- `CppRefactor` 已按 CurrentFile/WholeProject 区分范围，见 `cpprefactor.cpp:910-943`。
- 项目查找逐 TU 枚举源文件并累积结果，见 `cpprefactor.cpp:594-665`。
- Rename 测试覆盖项目文件、open Editor 未保存内容、Undo/Redo 和边界拒绝。

**处理结论：**准确语义为“非项目文件的 Rename 限当前文件；项目中的 Rename 可以跨文件，但不得超出项目边界”。核心文档已按此修正。

## D06 “当前语法覆盖缺口”清单已被后续实现修复（已确认并修正文档）

**文档声明：**`cppparser-design.md` 仍把多成员函数 declarator、trailing-return 函数指针、`operator()`、模板类成员函数、namespace 内 extern、if 条件结构化绑定和特定 range-for 形式列为当前缺口。

**实现证据：**

- `test_parse_member_function_declarators()` 已验证两个成员 declarator，见 `test_cppdeclarationparser.cpp:380-386`。
- `test_parse_operator_declaration()` 已验证 `operator()`，见 `test_cppdeclarationparser.cpp:530-538`。
- 语法测试已要求 `fp11`、`M::operator()`、`MemberTemplates::f`、`Q::V::h`、if 中的 `iter/success` 等存在，见 `test_cppparsertyntax.cpp:29-70`、`test_cppparsertyntax.cpp:89-126`。
- `char const c` 位于 `resources/syntax/ranges.cpp:24`，并由 `test_cppparsertyntax.cpp:153` 明确断言。

**处理结论：**旧缺口清单已删除。支持状态、限制和证据统一由 `testing/syntax-coverage.md` 维护；该表不代表完整标准文法覆盖。

## D07 Replay 场景 S3 与当前 definition 失效规则相反（已确认并修正文档）

**文档声明：**`testing/replay-scenarios.md` 的 S3 称失效声明时，若定义仍有效，则“将定义提升为声明位置”。

**实现证据：**

- `cpppreprocessor-design.md` 已明确 definition 失效不得由 declaration 回填，declaration/definition 分别失效。
- `invalidateSingleFile()` 清 definition 时保持其空值，删除 declarations 时不以 definition 回填，见 `cppparser.cpp:267-308`（当前 WIP 行号附近；基线逻辑相同）。

**处理结论：**S3 已改为两侧分别失效；导航查询可以回退到 definition，但不得修改 occurrence 数据。实现无需修改。

## D08 “内部子树文件没有 Registry 记录”术语错误（已确认并修正文档）

**文档声明：**Replay 场景的 F7、S5 等称内部子树文件“没有独立 Registry 记录”。

**实现证据：**Registry 同时包含 `fileInfos` 和 `replayRecords`，见 `cpppreprocessor.h:318-327`。内部子树文件可以有独立 ParsedFileInfo，只是没有独立 ReplayRecord；当前设计文档其它位置已承认逐文件 FileInfo。

**处理结论：**已统一改为“有实际文件的 ParsedFileInfo，但没有独立 ReplayRecord”，并同步修正继承场景的查询表述。实现无需修改。

## D09 隐藏 Editor 时清空 Parser 的设置已删除（已确认并修正文档）

**文档声明：**`cpppreprocessor-design.md` 和 Replay 场景 R1a 仍描述“IDE 默认启用编辑器隐藏时自动清空 Parser”。

**实现证据：**当前设置和隐藏处理中没有该清理选项；隐藏 Editor 只停止当前解析，见 `editor.cpp:1554-1557`，生命周期文档也按停止而非清空描述。

**处理结论：**已删除历史设置说明，明确共享 Parser 保留 Registry，隐藏停止、显示后按需重解析。实现无需修改。

## D10 Eval“绝不回写模型”的绝对表述与现有入口不符

**文档声明：**`cpptype-evaluation-design.md` 的持久声明与一次型求值数据段称，只有 expression eval 才填写的数据“一律不得回写 Statement 或持久 TypeInfo”。

**实现证据：**

- `EvaluationContext` 明确提供 `allowModelChanges`，见 `cpptyperesolver.h:166-192`。
- 声明构建和 Parser 内部求值会设置该标志，见 `cppstatementbuilder.cpp:933-934`、`cppparser.cpp:831-835`。
- Evaluator 在该标志下添加 Lambda/Parameter Statement，见 `cppexpressionevaluator.cpp:677-737`、`cppexpressionevaluator.cpp:791-798`。

**验证结果：**独立 `parser-eval-model-writes` 分支在 Eval 写入点插桩，GCC 15.3/libstdc++ 标准库 Suite 为 154 passed、0 failed、4 skipped。共发现 10 个 Lambda 的写入，每个均包括 Lambda、operator()、Parameter 和 declaration 操作；写入前没有同 fullName 的现有 Statement。

根因已经定位：Builder 已按原位置创建 Lambda，但 Eval 使用规范化 initializer 重新 Tokenize，内部空格丢失，而 `relocateTokens()` 只增加 initializer 起始列。`find_if` Lambda 在 Builder/Eval 中分别为 `32:54` 与 `32:52`，`transform` 分别为 `34:70` 与 `34:67`，导致复用查找失败并再次创建。这不是 Lambda 未识别，而是 Parse/Eval 位置映射错误。

**处理结论：**已按“Builder 唯一创建、Eval 只复用”修复。Lambda 使用 Parser 内递增 ID 作为 command，含 Lambda 的 initializer 在声明期生成求值专用占位文本，不再通过规范化后的 `[` 位置识别身份。原始 initializer 文本和 occurrence 位置保留，Eval 的 Lambda/参数/operator() 创建回退已删除。`test_lambda_evaluation_reuses_parse_statement()` 覆盖位置漂移、同作用域多个 Lambda、类型复用及求值前后 Statement 数量不变。

## D11 通用声明查找直接用 `split("::")` 分段（已修复）

**文档声明：**`cpptype-evaluation-design.md` 明确要求完整限定名不能通过 `QString::split("::")` 分段，因为模板实参和 `decltype(...)` 内部也可能包含 `::`。

**实现证据：**原 `CppTypeResolver::findDeclaration()` 使用 `name.split("::", Qt::SkipEmptyParts)`，会误拆模板实参和 `decltype(...)` 内的作用域符。

**处理结论：**已增加轻量 `splitQualifiedName()`，直接扫描原始字符串，只在非引号、非括号且非模板嵌套层的 `::` 处分段，不创建 Tokenizer 或持久 token 列表。Resolver 的候选筛选、特化选择和 Eval 规则均不变。`test_qualified_name_splitter()` 覆盖普通名称、嵌套模板、decltype 和运算符，`test_qualified_name_lookup_preserves_template_scope()` 验证真实特化 Statement 查询。

## D12 Replay 级联失效遗漏 boundaryPrefix 依赖（已修复）

**设计规则：**失效文件 H 时，除 H 自身及 include 闭包包含 H 的 ReplayRecord 外，`boundaryPrefix` 包含 H 的记录也必须立即失效。后者表达有序生效前提，不要求存在 include 关系。

**实现证据：**原 `CppPreprocessor::calculateAffectedFiles()` 只检查 `rootInfo->including(fileName)`，没有检查 `ReplayRecord::boundaryPrefix()`。后续扫描中的 `allBoundaryEntriesReplayed` 只能提供惰性保护，不能代替显式失效和多 TU/关闭路径中的立即清理。

**处理结论：**`calculateAffectedFiles()` 现在一次遍历同时检查 include 闭包和完整 boundaryPrefix；Replay 校验、延迟失效、Statement 清理、Editor 生命周期和 Resolver 均未改变。原有后续记录惰性失效及跨 TU replay 预期已按规则修正，前序无关记录保留场景不变。测试同时验证后续记录的 FileInfo、ReplayRecord 和 Statement 在重扫前清理，以及另一 TU 重扫后的正确恢复。

前缀依赖修复后，原 11.2 生命周期疑问只可能涉及与失效文件无关的前序缓存。Registry 明确允许保留这类跨 TU 缓存且不做全局 orphan GC，因此不构成未决正确性问题。

## 文档结构整理结果

- 重写候选决策、旧现状分析和历史实施内容已分流到 `history/`。
- 语法支持状态统一由 `testing/syntax-coverage.md` 维护。
- 核心文档不再复制逐轮 profile 数字；正式数据统一由 `docs/profile/README.md` 索引。
- 重解析方案已实施并归档为 `parser-reparse-unification-2026-11-25.md`；`proposals/` 中的 C++17 计划仍是后续计划，不作为当前能力清单。
