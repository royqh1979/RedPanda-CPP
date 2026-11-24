# Parser 强制与条件重解析设计

## 目标

打开、显示或切换 Editor 时，多个条件重解析请求可能与正在执行的 Parse 重叠。条件请求不得因为 UI 预检查时 ReplayRecord 尚未建立而退化为一轮额外的完整 Parse；显式强制重解析行为保持不变。

## 两种模式

- force：调用方明确要求重新分析。根文件按现有流程失效并重扫，include 逐个执行 replay 校验。
- non-force：调用方只要求保证分析结果可用且未过期。请求文件就是当前主文件，且根文件/include 的 mtime、size 均未变化时直接结束，否则按 force 的同一流程解析。ReplayRecord 只负责 include replay，不用于判断整次 Parse 能否跳过。

non-force 沿用 `onlyIfNotParsed=true`，不增加新的 Parser 层次或第三套接口。`Editor::reparse()` 使用 force，`Editor::reparseIfNeeded()` 使用 non-force。

## 执行时机与并发

正确性判断由 `CppParser::parseFile()` 在请求真正执行前完成，不能只依赖 Editor 的 `needReparse()` 预检查。worker 正在处理前一个请求时，后续请求仍按原队列保存；轮到条件请求执行时再次检查当前 Parse Root 和文件状态。若前一个请求已经建立有效结果，条件请求直接结束，不再次失效根文件。

non-force 直接检查 `fileName` 与 `contextFilename` 的 FileInfo 和文件状态；空 context 忽略，两者都未变化时跳过。普通单文件不需要额外记录 Parse Root，也不附加 ReplayRecord 条件。project 分支先计算 `filesToScan`，仅当列表中全部文件及其 include 闭包都未变化时跳过。project/context 的原有分析顺序保持不变。

`isFileParsedInContext()` 使用 FileInfo 判断头文件是否已由 context 覆盖：context 有 FileInfo 时检查 include 闭包；没有 FileInfo 时先按包含处理，等待 context 主分析覆盖。该接口不读取 ReplayRecord。

变化检查必须包含文件的 include 闭包。Preprocessor 通过 `GetFileVersionFunc` 查询 Editor 当前 `Document::contentVersion()`，ParsedFileInfo 保存上次分析时的版本。查询值和保存值均为正数时只比较版本；任一方为 0（没有版本信息）时回退到磁盘 mtime/size。EditorManager 在 `mMutex` 保护下提供版本，取不到 Editor 或版本时返回 0。Document 版本从 1 开始，接口统一使用 `int`。

`QSynedit::StatusChange::FileLoaded` 在文件加载完成时随本批状态通知。Editor 在该批次中不执行 `onStatusChanged()` 的 force reparse，避免初始加载与 `openFiles()` 收尾或激活时的条件请求形成重复完整 Parse；普通编辑的通知路径不变。

## 失效与变化检查集合

force 和 non-force 必须使用同一组相关文件，不能只处理根文件：

- 以当前 root/context 的 FileInfo include 闭包为候选集合，包含 root 自身。
- 位于系统边界外的所有相关文件都必须失效并接受变化检查；用户头没有独立 ReplayRecord 时也必须包含在内。
- 系统头不做强制失效，继续在各自入口处校验 ReplayRecord。
- non-force 对同一集合逐个复用 `hasChangedFiles()`，只有全部未变化才跳过。project 分支的候选集合由 `filesToScan` 给出，非 project 分支由 file/context 的 include 闭包给出。
- `calculateFilesToBeReparsed()` 表示“文件及其依赖者”，用于级联失效；它不能替代当前 root/context 的 include 闭包，也不能据此漏掉没有独立 ReplayRecord 的用户头。

只调用 `invalidateSingleFile(root)` 会使用户头中的旧 Statement 在重扫时重复，不属于正确行为。

## 后续统一化记录

当前 project 分支仍保留“计算受影响文件、按 include 关系排序并主动扫描”的旧项目级流程。按当前只保证主文件及 include 闭包准确的设计，project 与普通文件最终应统一为当前 root/context 的处理方式；project 身份只影响项目路径、系统边界、Statement 属性和依赖失效。该统一化需要独立批次及多 TU 验证，不与表达式 lambda 修复混做。

## KISS 与范围审核

- 不修改 Statement、ReplayRecord 或 include 缓存结构。
- 不改变显式 Reparse Code、编辑、编译器配置变化等 force 场景。
- 不通过 suppress signal 或隐藏进度来伪造“只解析一次”；判定依据是被跳过的条件请求没有进入 preprocess/tokenize。
- 复用 Preprocessor 的文件变化检查和 Document 版本，不增加 ReplayRecord 的对外查询；新增的版本回调只返回 `int`，不暴露 Editor 或 Document 指针。

## 测试

- non-force 在首次解析后直接结束，Statement 对象和扫描次数不变。
- 根文件或 include 的 mtime/size 变化后，non-force 必须重新扫描并更新结果。
- force 即使记录未变化也重新扫描。
- 一个 force 请求执行期间排入 non-force 请求，前者完成后后者不得再次读取文件或增加扫描次数。
