# QSynEdit 批量插入 Deferred 重解析设计

## 1. 目标与范围

消除普通多行插入/替换中“先解析 N 个空行，再逐行解析 N 行正式内容”的重复工作。

本项采用显式批处理契约：

- 新增只插入空行、不执行重解析的 Document 入口；
- 调用方必须在同一批处理中逐行 `putLine()` 写入并解析所有正式内容；
- 普通 `insertLines()` 的独立调用行为不变；
- 普通粘贴继续按原顺序计算自动缩进和逐行 syntax state。

不修改收敛算法、自动缩进算法、Undo/Redo、公共粘贴行为、View 布局或折叠实现。

## 2. 现状与根因

`doInsertTextByNormalMode()` 先调用 `properInsertLines()` 插入空行，再逐行 `properSetLine()` 写入正式文本。

当前 `Document::insertLines(index, count, false)` 即使 `parseToEnd=false`，仍会 `reparseLines(index, index + count, false)`，因此先解析全部空行。后续 `putLine()` 又逐行解析正式内容，形成约 2N 行工作量。

空行只是编辑过程的临时状态，最终不会保留。但是，不能简单让现有 `insertLines(..., false)` 全部停止解析：其它调用方可能依赖该 primitive 返回时已有局部 state。

## 3. 自动缩进约束

普通粘贴的每一行可能执行 `calcIndentSpaces()`。该计算会读取前一行文本和 syntax state，包括未结束字符串、行 continuation、括号缩进等。下一行正式内容又必须在前一行正式内容解析后继续处理。

因此以下方案不可接受：

- 一次性写入所有正式内容后只解析最后一行；
- 先统一计算全部缩进，再一次性插入；
- 让最后一行 `putLine()` 从尚未建立的前行 state 开始修复；
- 仅因 fold-dirty 为假就跳过逐行 state。

本项只能消除空行扫描，不能消除正式内容的逐行扫描。

## 4. 方案选择

### 方案 A：显式 deferred 插入入口（采用）

新增 `Document::insertLinesDeferred(int index, int count)`：

1. 与普通 `insertLines()` 一样创建空 DocumentLine、登记 lineSeq 和 Insert change；
2. 不调用 `reparseLines()`，不发送空行的 `linesReparsed`；
3. 要求调用方在同一批次中随后对每个插入行执行 `putLine()`，由这些调用建立正式 state 和必要的尾部传播。

`doInsertTextByNormalMode()` 改用它，现有首行处理和逐行正式内容循环不变。profile 中对应的批量插入/替换也使用同一 deferred 入口，避免测到已经淘汰的空行工作。

### 方案 B：预计算缩进/state 后一次性插入

新增接收 `QStringList` 的插入接口并在创建行时写入文本，看似更直接，但普通 C++ 粘贴需要逐行自动缩进和前一行 state，不能在不预演语法状态的情况下统一替代现有路径。

预先逐行计算缩进后再插入有两种实现：若正式插入仍重解析，预计算与解析合计仍约为 2N，不如 deferred 的 N；若同时保存预计算的每行 state 并跳过正式解析，则约为 N，与 deferred 同阶，但必须预先分配最终 lineSeq、保存整批 state，并另行保证 fold-dirty、尾部收敛、change 和 Undo/Redo 一致，实质是重写一套批处理解析流程。它没有渐进复杂度收益且状态一致性风险更高，不建立实验分支。为非缩进场景再增加快速路径也会形成两套行为，不采用。

### 方案 C：修改所有 `insertLines(..., false)` 跳过重解析

该方式修改面最小，但破坏独立 primitive 返回时局部 state 已建立的既有契约，无法区分随后是否真的会写入所有行，不采用。

## 5. Deferred 契约与边界

`insertLinesDeferred()` 只用于已确定后续写入流程的批处理：

- `count > 0`，索引约束与普通 `insertLines()` 相同；
- 插入后不保证任何新行已有 syntax state；
- 调用方必须逐行写入所有新行，不能跳过空行文本；
- 最后一次 `putLine()` 的 `parseToEnd` 仍决定是否传播到后续文档；
- Insert/SetLine change 的记录顺序和数量不变；
- 最终文本、lineSeq、foldStateDirty、reparseToLine 和 contentChanged 不变；
- 不用于没有后续 `putLine()` 的独立插入操作。

空行文本也必须经过 `putLine()`，因为正式内容就是空字符串，仍需建立对应 state。

## 6. 测试保护

### Document 层

- deferred 插入后逐行写入，累计重解析行数等于正式行数，而不是 2N；
- 所有正式行 state 正确，后续行在需要时继续收敛；
- 正式内容为空的行同样经过 `putLine()` 并建立 state；
- Insert/SetLine change、lineSeq、最终文本和 freeze line 正确；
- 普通 `insertLines()` 的既有测试保持，不改变独立 primitive 语义。

### View/自动缩进层

通过 `doInsertTextByNormalMode()` 覆盖：

- C++ 多行粘贴的逐行缩进结果；
- 括号层级和最终文本；
- Undo/Redo 后文本与 state；
- 未结束字符串、行 continuation 等现有自动缩进测试；
- 普通单行输入和既有粘贴测试不回归。

### Profile

普通 Insert/Replace 使用 deferred 入口，检查重解析行数从约 2N 降为 N，并比较 Document 和 Document+View 的 sync/total。

## 7. 验证与收益评估

先运行 `TestDocument`、粘贴/自动缩进相关 Suite 和 QSynEdit 测试，再依次运行 Release、Debug 全量 CTest。

以 `c375eae6` 为基线，对比普通 Insert/Replace 1000、10000 行：

- Document sync 与重解析行数；
- View sync 与 total；
- 单行插入不回归；
- fold 场景的必要状态传播不缩短。

结果必须同时证明正式行没有漏解析，不能只报告次数减半。

## 8. KISS 与范围复审

实现只需一个语义明确的 deferred 插入入口，并替换普通粘贴中的空行插入调用。它不改变正式内容处理顺序，也不需要内容列表接口或两套自动缩进路径。

生产修改应限于 `document.h/.cpp`、`doInsertTextByNormalMode()` 和对应 profile；测试限于 Document 与编辑/粘贴测试。若需要修改自动缩进或收敛算法才能通过测试，应停止并重新评审。
