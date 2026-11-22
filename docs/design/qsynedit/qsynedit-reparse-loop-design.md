# QSynEdit 重解析内循环优化设计

## 1. 目标与范围

减少 `Document::reparseLines()` 在已经持有 `mMutex` 时，反复通过带锁 accessor 访问同一行产生的递归锁开销。

本项采用明确的内外接口分层，不在内循环中直接访问 `DocumentLine`：

- 现有 `getLine()`、`getLineSeq()`、`getSyntaxState()`、`setSyntaxState()` 保持对外接口及加锁语义；
- 新增内部 `doGetLine()`、`doGetLineSeq()`、`doGetSyntaxState()`、`doSetSyntaxState()`，表达 Document 内部已经受锁保护的访问，不再重复加锁；
- `reparseLines()` 只调用这些内部接口。

不修改起止范围、状态恢复、收敛比较、fold-dirty、通知、自动缩进、Syntaxer::getState() 或锁的对外语义。

## 2. 现状与证据

`reparseLines()` 入口已经 `QMutexLocker locker(&mMutex)`，但每行仍多次调用 Document 自身的带锁 accessor，形成重复加锁。

已有 Document profile 基线覆盖：

- 普通修改 10000 行的大量内循环；
- 未配对分隔符造成的前/中/尾必要状态传播；
- GAS、块注释和 fold 状态收敛。

`Syntaxer::getState()` 每行产生新状态，是语义所需工作，不能用共享状态对象替代。

## 3. 接口分层与语义

内部接口放在 Document 的 private 区域，统一使用 `do + 对外接口名`，让对应关系一眼可见：

- `doGetLine(line)`：返回当前行文本；
- `doGetLineSeq(line)`：返回当前行 lineSeq；
- `doGetSyntaxState(line)`：返回当前行状态；
- `doSetSyntaxState(line,state)`：写入当前行状态。

对外 `get*` 只负责持有 `mMutex` 并委托给内部接口。这样调用点可以明确区分“外部需要 Document 自己加锁”和“内部已经处于锁保护上下文”，同时保持接口实现简单。

内部接口必须保留原 accessor 的边界行为：

- 非法行号时 `doGetLine()` 返回空字符串、`doGetLineSeq()` 返回 0、`doGetSyntaxState()` 返回新的空 `SyntaxState`；
- 行存在但尚未解析时，`doGetSyntaxState()` 同样返回新的空 `SyntaxState`，不能返回 nullptr；
- `doSetSyntaxState()` 保留原有行号断言。

不能简单返回 `mLines[line]->syntaxState()` 而丢失空状态约定。

## 4. 方案选择

### 方案 A：对外加锁、对内语义接口（采用候选）

对外接口保留签名和行为，内部接口供 `reparseLines()` 等已经持锁的 Document 方法使用。递归锁开销减少，接口边界和空状态语义不变。

### 方案 B：在 reparseLines 中直接访问 DocumentLine

虽然 Document 是 friend，但会把空状态处理、边界规则和锁上下文假设散落在算法中，降低可读性并容易误用，不采用。

### 方案 C：移除整个 reparseLines 锁

后台 wrap Job 和共享 View 会读取 Document，直接去掉锁会破坏线程安全，不能采用。

### 方案 D：缓存或复用每行 getState 结果

每行 state 独立且用于后续比较、存储和自动缩进。复用会引入别名和状态污染，不能采用。

方案 A 是否最终保留由直接 A/B profile 决定；若收益低于测量噪声或出现正确性问题，不强行合并。

## 5. 测试保护

复用现有测试，无需为新行为增加不同语义的场景：

- `TestDocument` 的 reparse、删除范围、deferred 插入、GAS cascade、fold-dirty、freeze line；
- 自动缩进、Undo/Redo、折叠、多视图和全量编辑测试；
- 普通/折叠 profile 同时检查重解析行数和最终状态。

必须特别保持：

- 状态相等时提前停止；
- 未相等时传播到收敛或 EOF；
- `linesReparsed` 范围不变；
- 新插入行、删除后行和移动行的 state 正确；
- 对外 get 接口的非法行号和未解析行空状态行为不变；
- 多线程后台布局不会读到未加锁保护的中间状态。

## 6. 验证与收益评估

先运行 `TestDocument` 和 QSynEdit 测试，再依次运行 Release、Debug 全量 CTest。

以 `feb6632b` 为父提交，对比普通修改 10000 行和 fold 前/中/尾修改的 Document sync、重解析行数与最终状态。若锁开销不占显著比例，应明确记录无收益或收益不足，而不是继续修改 Syntaxer 或收敛算法。

## 7. KISS 与范围复审

候选实现只增加一层语义明确的内部接口并委托现有对外接口，不引入缓存、状态机或线程协议变化。任何需要修改 Syntaxer、getState 或锁生命周期的后续工作都不属于本项。
