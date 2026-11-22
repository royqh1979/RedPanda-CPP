# 试题集测试与期望输出比对设计

## 目标与范围

`test-problemset` 覆盖试题集核心数据、持久化、Free Problem Set 导入导出和测试点输出验证，并提供大期望输出文件的可重复性能测试。

比对优化不改变运行器、设置界面或 MainWindow 的职责，也不改变用户可见的内容比对规则。

## 组件设计

- `OJProblemSet`、`OJProblem`、`OJProblemCase` 保存层级数据、修改状态和运行结果。
- `OJProblemSetModel`、`OJProblemModel` 提供 Qt 模型、JSON 持久化、顺序调整和 `%ProblemSetPath%` 路径转换。保存到 JSON 时使用 `/` 作为可移植分隔符，加载后同样返回规范化的 `/` 路径。
- `importFreeProblemSet()`、`exportFreeProblemSet()` 处理 Free Problem Set XML。导入保留测试点名称，拼接相邻字符/CDATA 分片，在 `test_output` 结束时完成测试点，并对 XML 语法错误抛出 `FileError`。
- 外部期望输出文件存在时优先于内联 `expected`；文件不存在时回退到内联文本。外部文件明确按 UTF-8 解码，不承担系统编码自动探测。

## 输出比对与结果接口

`ProblemCaseValidator::validate()` 支持精确、忽略行首尾空白、忽略所有空白三种规则，返回 `ProblemCaseValidationResult`：

- `Passed`、`Mismatch`、`Error` 区分通过、内容不一致和文件错误；
- 双方行数和 0 起始的 `firstDiffLine` 描述差异；
- `errorMessage` 包含明确的文件错误；打开失败或 UTF-8 解码失败均属于 `Error`；
- 验证器不把结果写回输入对象，MainWindow 将结果同步到现有显示字段并提示错误。

比对通过按行游标同步扫描实际输出和期望输出，不为完整文本构造 `QStringList`，也不为每行分配 `QString`。`QStringView` 提供无分配行视图；忽略所有空白时直接逐字符比较 token。

发现内容差异后停止内容比较，但仍扫描剩余行以记录完整行数。末尾换行不产生额外空行，CRLF 与 LF 等价；忽略空白不等于忽略行数差异。

验证仍由 MainWindow 在测试点结束时调用。是否移到工作线程，以及如何避免 MainWindow 为展示而再次完整读取期望文件，属于后续独立优化。

## 测试脚手架

`test-problemset` 是独立可执行文件，不并入 `test-widgets`。它直接编译被测实现，链接 Qt Test 和 `redpanda_qt_utils`，支持 `--suite=类名` 和 Qt Test 原生参数。

Suite 按职责划分：

- `TestProblemSet`：层级数据、增删移动、修改状态传播、信号和无效索引。
- `TestProblemSetModel`：JSON 往返、当前项、相对路径、内外部路径、模型操作、异常输入和错误处理。
- `TestFreeProblemSetFormat`：XML 导入导出往返、多个试题和测试点、单位、CDATA、特殊字符及错误输入。
- `TestProblemCaseValidator`：三种比对规则、空文本、Unicode 空白、CRLF/LF、末尾换行、缺失/额外行、差异位置、外部文件优先级、内联回退及 UTF-8 错误。
- `TestProblemCasesRunner`：需要独立子进程辅助模式时再加入；不得为了链接方便把运行器测试混入验证器 Suite。

模型测试只使用 IconsManager 测试 stub，不构造主窗口和完整设置系统。stub 只替代图标读取，不替代被测模型或业务逻辑。

## 大期望输出测试与 Profile

大样本由测试在 `QTemporaryDir` 中生成，不提交大文件资源。样本覆盖：

- 5000 行旧特殊分支边界的两侧；
- 10,000、100,000、500,000 行；
- 三种比对规则；
- 完全一致、首个差异位于末尾、实际输出缺失或多出末尾行；
- 外部文件与内联文本；
- UTF-8 纯 ASCII、非 ASCII 和非法字节序列。

正确性测试不设置时间上限。性能测试由 `REDPANDA_BUILD_PROBLEMSET_PROFILE_TESTS` 控制，仅在 Release 直接运行，使用 Qt Test `QBENCHMARK` 测量 `validate()`，不包含样本生成和文件写入。Debug 或调试器中的数字不作为基线。

## 验证

每项实现依次执行专项测试、同场景 Release profile 和完整 Release/Debug 回归。测试不得为了迎合新实现删除旧场景；发现不同但合法的语义时，应增加明确的新场景并先讨论行为变化。
