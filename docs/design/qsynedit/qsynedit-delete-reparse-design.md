# QSynEdit 删除后的重解析范围设计

## 1. 目标与范围

消除 `Document::deleteLines()` 在删除 N 行后，仍把删除点后的 N 行作为强制重解析范围的冗余工作。

本项只修改 `deleteLines()` 在 `parseToEnd=true` 时传给 `reparseLines()` 的结束行：

- 从删除点后的当前行开始重解析；
- 只强制解析这一行，之后由现有状态相等检查决定是否继续；
- 跨行注释、字符串、预处理或语言指令状态未收敛时，必须继续到状态稳定或 EOF。

不修改 `reparseLines()` 的状态收敛算法、syntaxer、自动缩进、折叠、View、Insert/Replace/Move、Undo/Redo 或公共接口。

## 2. 现状与根因

当前删除后调用：

```cpp
reparseLines(index, index + numLines, true);
```

删除的行已经不存在，`numLines` 只表示旧文档中的删除数量，不表示删除后仍需强制验证的行数。普通文本删除因此会解析删除点后的 N 行。

`reparseLines()` 已具备完整的收敛协议：

1. 从 `startLine-1` 的 state 开始解析当前行；
2. 到达请求的 `endLine` 后，如果新 state 与该行保存的旧 state 相等，则停止；
3. 如果不相等，则继续向后解析，直到状态收敛或 EOF；
4. EOF 没有可解析行时，仍发送通知并更新 freeze line。

因此调用方只需给出“删除点后当前一行”的强制验证范围，无需自行预测传播长度。

## 3. 方案选择

### 方案 A：强制当前一行并沿用收敛（采用）

改为：

```cpp
reparseLines(index, index + 1, true);
```

当 `index < count()` 时，恰好强制验证删除后的当前一行。若新 state 与该行旧 state 一致，后续行无需重扫；若不一致，现有循环会继续向后传播。

当 `index == count()` 时，删除发生在 EOF，沿用现有空范围处理，不访问不存在的行。

### 方案 B：完全交给 `reparseLines(index, index, true)`

该写法在普通行上通常也会在 do-while 中解析一行，但 `endLine` 不再直接表达“验证当前一行”的意图，增加阅读和边界理解成本。方案 A 的接口语义更清楚，无需另建实验分支。

### 方案 C：根据 fold-dirty 或文本类型跳过重解析

没有 fold-dirty 不代表注释、字符串或语言状态不变。普通括号配平也不能证明逐行 state 相同。自动缩进和后续显示仍依赖这些 state，因此不能跳过删除点验证，也不能只按 `{}` 判断。

## 4. 必须保持的语义

- `parseToEnd=false` 仍不主动重解析，保留“先删除、再由后续 `putLine()` 修复状态”的批处理协议；
- `parseToEnd=true` 时必须验证删除点后的当前行，不能仅通知而不解析；
- 删除点前行不变，其 state 是重解析起点；
- 删除后当前行的旧 state 只用于收敛比较，不能代替重新计算；
- 未闭合块注释、字符串或语言指令造成的 state 变化必须传播到收敛；
- EOF 删除继续发送 `linesReparsed(startLine, INT_MAX)` 并更新 freeze line；
- `mFoldStateDirty`、contentChanged、Undo/Redo 和自动缩进使用的逐行 state 不得改变；
- `deleteLine()` 本来就只验证一行，不在本项范围内。

## 5. 测试保护

在 `TestDocument` 中补充删除范围与状态测试：

1. 普通文本删除 N 行，`linesReparsed` 只报告删除点后一行，而不是 N 行；
2. 删除 1 行保持原行为；
3. EOF 删除没有当前行，仍发送原 EOF 通知和 freeze line；
4. 删除块注释结束符后，后续普通行必须保持 comment-not-finished 状态并传播到 EOF；
5. 多行删除 GAS 语言模式指令后，后续行恢复正确的语法模式与寄存器前缀状态；
6. 删除后继续 `putLine()` 的既有修复协议、fold-dirty、contentChanged、Undo/Redo 和自动缩进测试不得回归。

测试同时检查重解析行数和最终 state，避免通过少解析必要行获得虚假收益。

## 6. 验证与收益评估

先运行 `TestDocument`、QSynEdit 测试，再依次运行 Release、Debug 全量 CTest。

使用既有 `TestDocumentProfile` 对比普通删除 1/10/100/1000/10000 行的 Document sync 和重解析行数；折叠边界或块注释场景用于确认必要传播没有被缩短。此前 `d77823fa` 基线与本项父提交的 Document 实现相同，可作为直接基线；如实现或环境有歧义，再用父提交独立 worktree 重测。

预期普通删除由强制 N 行降为 1 行，耗时随删除规模下降；需要尾部传播的场景不承诺收益。

## 7. KISS 与范围复审

本项只需修改一个调用参数，正确性由现有收敛算法和新增 state 测试保证。无需新状态、缓存或编辑协议，也不应通过修改 `reparseLines()` 来适配调用方。

若测试证明一行不足以恢复正确 state，应检查收敛算法本身并重新评审，而不是恢复“按删除数量扫描”的粗略近似。
