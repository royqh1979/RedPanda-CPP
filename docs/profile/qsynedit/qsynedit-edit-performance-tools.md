# QSynEdit 编辑性能 Profile 工具

## 1. 用途与位置

`libs/qsynedit/test/resources/analyze_edit_profile.py` 用于汇总 Document 和 Document+View 编辑 profile 的原始日志，按场景分组计算中位数并输出 CSV，避免每次测量后临时重写分析脚本或只保留最终数字。

相关文档：

- [编辑性能基线](qsynedit-edit-performance-baseline.md)：优化前的完整持久基线；
- [编辑性能分析报告](qsynedit-edit-performance-report.md)：场景定义、热点、优化建议及 profile 运行命令。

脚本属于测试配套资源，构建 `test-qsynedit` 时会随 `libs/qsynedit/test/resources/` 的其他内容复制到测试运行目录的 `resources/` 下。分析和对比应优先使用仓库中的脚本，不要依赖 `R:` 中的临时副本。

## 2. 环境依赖

- Python 3；
- 仅使用 Python 标准库：`csv`、`statistics`、`pathlib`、`collections` 和 `sys`；
- 输入日志必须为 UTF-8 文本；
- 不需要安装第三方 Python 包，也不需要 Qt 或 C++ 编译器来执行汇总。

## 3. 输入

脚本接受一个或多个原始日志文件，识别以下前缀：

- `DOCUMENT_EDIT_PROFILE`；
- `VIEW_EDIT_PROFILE`。

每行前缀后的内容必须是当前 profiling 测试输出的 CSV 风格记录。其他日志行会被忽略，因此可以直接输入 Qt Test 的完整日志，无需先手工删除 `QINFO` 等内容。

脚本按以下字段分组：

- Document/View 类型；
- 操作类型；
- 编辑行数；
- `plain`/`fold` 负载；
- 前/中/后位置；
- View 的折叠开关状态。

同一组的多次采样共同计算中位数。可以把同一场景的多个日志文件一次传入，但不得把不同代码版本、构建类型、测试环境或负载定义的日志混合到同一输出中。

当前脚本不处理 `VIEW_OVERHEAD_PROFILE`。该微型 profile 的原始样本和中位数记录在基线文档中；如后续需要自动汇总，应扩展本脚本并保留兼容验证。

## 4. 输出

命令第一个参数是输出 CSV 路径，后续参数都是输入日志。脚本会覆盖已有输出文件，输出目录必须已经存在。

输出字段为：

| 字段 | 含义 |
|---|---|
| `suite` | `document` 或 `view` |
| `operation` | `modify`、`insert`、`delete`、`replace` 或 `move` |
| `count` | 场景编辑行数 |
| `kind` | `plain` 或 `fold` |
| `position` | `begin`、`middle` 或 `end` |
| `folding` | View 的折叠开关；Document 留空 |
| `samples` | 该组参与汇总的样本数 |
| `sync_ms` | 同步编辑耗时的中位数，毫秒 |
| `total_ms` | 包含后台布局完成的总耗时中位数，毫秒；Document 等于 `sync_ms` |
| `reparsed_lines` | 累计重解析行数的中位数 |
| `change_entries` | Document change 数量的中位数 |
| `changed_lines` | 各 change 报告行数累计值的中位数 |
| `fold_rescans` | View fold rescan 次数的中位数；Document 留空 |

纳秒到毫秒的转换以及保留三位小数只发生在输出阶段。计数使用中位数而不是求和；查看优化是否减少重复工作时，应同时核对这些计数和正确性测试。

## 5. 运行方法

以下命令在仓库根目录执行，输入日志和输出 CSV 放在 `R:`，避免临时文件进入仓库。

汇总 Document 全矩阵：

```sh
python libs/qsynedit/test/resources/analyze_edit_profile.py \
  /r/document-edit-profile.csv \
  /r/document-edit-profile.log
```

汇总 View 全矩阵：

```sh
python libs/qsynedit/test/resources/analyze_edit_profile.py \
  /r/view-edit-profile.csv \
  /r/view-edit-profile.log
```

把同一条件下的多份日志合并为一组中位数：

```sh
python libs/qsynedit/test/resources/analyze_edit_profile.py \
  /r/view-edit-selected.csv \
  /r/view-edit-selected-1.log \
  /r/view-edit-selected-2.log \
  /r/view-edit-selected-3.log
```

也可以使用构建时复制到测试目录的版本：

```sh
python resources/analyze_edit_profile.py \
  /r/document-edit-profile.csv \
  /r/document-edit-profile.log
```

最后一条命令的工作目录应为 `test-qsynedit` 的运行目录，例如 `R:/build-redpanda-qt5-release-edit-profile/libs/qsynedit`。

## 6. 结果保存与对比

- 原始日志和生成的 CSV 默认属于临时测量产物，不必提交仓库；可复现脚本、场景定义和关键基线必须保留。
- 正式基线应记录代码版本、日期、编译器、Qt、构建类型、测试负载、采样次数、环境变量和完整结果，不能只记录几个耗时数字。
- 优化前后使用相同命令和场景分别生成 CSV，再比较中位数、样本波动、重解析行数、change 数量和 fold rescan 次数。
- 不覆盖既有基线来表达优化结果；优化数据应作为新结果保存，并注明对应的基线版本。
