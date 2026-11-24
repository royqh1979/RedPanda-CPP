# Red Panda C++ 性能文档总览

## 1. 文档职责

本文是性能基线、分析报告、优化结果和配套工具的总入口。`AGENTS.md` 只链接本文，不再逐项罗列性能文档。

性能数据应记录测试环境、场景、采样方法和限制；不同提交和机器的数值不得直接混为同一基线。

## 2. QSynEdit 编辑性能

### 基线与分析

- `qsynedit/qsynedit-edit-performance-baseline.md` — Document 与 Document+View 完整基线。
- `qsynedit/qsynedit-edit-performance-report.md` — 热点分析与优化建议。
- `qsynedit/qsynedit-edit-performance-tools.md` — profile 脚本、输入输出与使用方法。

### 导出

- `qsynedit/qsynedit-exporter-baseline.md` — HTML/RTF 输出尺寸与导出耗时基线。
- `qsynedit/qsynedit-exporter-result.md` — HTML 尺寸优化、RTF 兼容处理、逐行语义高亮统一及复测结果。

### 优化结果

- `qsynedit/qsynedit-pending-layout-incremental-result.md` — pending 调度优化。
- `qsynedit/qsynedit-bulk-setline-layout-result.md` — 大批量 SetLine 布局优化。
- `qsynedit/qsynedit-delete-reparse-result.md` — 删除后的重解析范围优化。
- `qsynedit/qsynedit-bulk-insert-deferred-result.md` — 批量插入 deferred 重解析优化。
- `qsynedit/qsynedit-bulk-fold-delete-result.md` — 折叠块批量删除优化。
- `qsynedit/qsynedit-reparse-loop-result.md` — 重解析内循环优化。

## 3. C++ Parser

- `parser/std-function-eval-baseline.md` — `std::function` 调用与赋值的求值基线、热点和复现方法。
- `parser/bits-stdcpp-parse-baseline.md` — GCC 15.3 `bits/stdc++.h` 完整解析基线、阶段成本与头文件热点。

## 4. 试题集

- `problems/problemset-validator-baseline.md` — 大期望输出文件比对基线、场景和运行方法。
- `problems/problemset-validator-result.md` — 无分配逐行比对的优化结果与限制。

## 5. 关联文档

- 总体设计：`../design/architecture.md`。
- QSynEdit 设计：`../design/qsynedit/qsynedit-design.md`。
- 环境模板：`../env/template/local-development.md`。
