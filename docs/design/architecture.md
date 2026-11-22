# Red Panda C++ 总体设计与模块文档

## 1. 文档职责

本文是项目设计总入口，说明主要模块及各自设计文档。`AGENTS.md` 只链接本文，不再逐项罗列模块文档。

各模块文档应描述当前设计、职责边界、关键约定和必要的实施留痕；具体修改历史以 git 提交为准。

## 2. 模块划分

- `RedPandaIDE/`：IDE 应用、项目、编译运行、调试、设置和界面。
- `RedPandaIDE/src/parser/`：C/C++ 预处理、声明解析、语句模型、类型推导和表达式求值。
- `libs/qsynedit/`：文档模型、编辑、布局、绘制、语法高亮、折叠和搜索。
- `RedPandaIDE/src/search/`：文件搜索与已选结果替换。
- `RedPandaIDE/src/refactor/`：语义引用查找与重命名。
- `libs/redpanda_qt_utils/`：共享 Qt 工具。
- `libs/lua/`：上游 Lua 运行时，保持与上游一致。
- `addon/`、`tools/`、`packages/`、`platform/`：插件、辅助工具、打包与平台支持。

## 3. 模块设计文档

### QSynEdit

- `qsynedit/qsynedit-design.md` — 组件职责、Document、View、Syntaxer 与编辑约定。
- `qsynedit/qsynedit-wordwrap-plan.md` — 自动换行与全局映射。
- `qsynedit/qsynedit-multiview-analysis.md` — 多视图分析。
- `qsynedit/qsynedit-incremental-layout-design.md` — 增量布局与 max width。
- `qsynedit/qsynedit-pending-layout-incremental-design.md` — pending 行集合增量维护。
- `qsynedit/qsynedit-bulk-setline-layout-design.md` — 大批量 SetLine 布局。
- `qsynedit/qsynedit-fold-editing-design.md` — 折叠显示、光标与编辑规则。
- `qsynedit/qsynedit-bulk-fold-delete-design.md` — 折叠块批量删除。
- `qsynedit/qsynedit-delete-reparse-design.md` — 删除后的重解析范围。
- `qsynedit/qsynedit-bulk-insert-deferred-design.md` — 批量插入 deferred 重解析。
- `qsynedit/qsynedit-reparse-loop-design.md` — 重解析内循环与内外接口分层。
- `qsynedit/qsynedit-semantic-highlight-plan.md` — 语义高亮计划。
- `qsynedit/qsynedit-indent-tab-design.md`、`qsynedit-indent-tab-size-plan.md`、`qsynedit-indent-always-use-spaces-plan.md` — 缩进与 Tab 设计留痕。

### Parser / C++ 语义

- `parser/cppparser-design.md` — CppParser、Tokenizer、类型推导与 IDE 查询。
- `parser/parser-concurrency-review.md` — Parser 并发约定。
- `parser/cpptemplate-evaluation-design.md` — 有限模板实例化。
- `parser/cpppreprocessor-optimization.md` — 预处理 replay、Session/Registry 与缓存。
- `parser/cpppreprocessor-replay-scenarios.md` — replay 场景和测试清单。
- `parser/cpp17-support-plan.md` — C++17 支持计划。
- `parser/cppnamespace-entity-using-design.md` — namespace、InlineAliasStatement 与 StatementFactory。
- `parser/cppnamespace-transparency-design.md` — namespace transparency。
- `parser/parser-rewrite-handoff.md` — Parser 重写交接说明。

### 搜索与重构

- `search/search-replace-design.md` — 搜索替换、文件搜索与已选结果替换。

## 4. 相关文档

- 性能基线、报告、结果与工具：`../profile/README.md`。
- 环境模板：`../env/template/local-development.md`。
- 本机配置：`../env/local/AGENTS.local.md`（默认不提交）。
- 更新日志：`../reference/NEWS.md`。
