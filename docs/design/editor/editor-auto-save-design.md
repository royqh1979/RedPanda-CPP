# Editor 自动备份与自动保存设计

## 1. 职责边界

- 自动备份属于单个 `Editor`：备份当前文档内容、按编辑活动更新、在生命周期结束时清理。
- 自动保存属于 `EditorManager`：它已经有当前编辑器、编辑器枚举、项目文件判断和批量保存职责。
- `MainWindow` 只负责显示状态栏和错误提示，不负责选择保存目标或拼接文件名。

不新增独立 AutoSaveController 类，也不把多文件协调移入单个 `Editor`。

## 2. EditorManager 自动保存接口

EditorManager 持有自动保存 QTimer，并提供：

- `resetAutoSaveTimer()`：按 Settings 启停、设置分钟间隔，并保持现有的启用时立即处理行为；
- 私有的立即处理入口：依次处理当前文件、所有打开文件或项目文件；
- 私有的单编辑器保存入口：检查 `canAutoSave()`，执行覆盖保存或时间戳副本保存；
- `autoSaveFinished(int)` 和 `autoSaveError(...)` 信号：由 MainWindow 显示，不在 Manager 中直接操作状态栏或 QMessageBox。

外部环境只注入自动保存实际需要的能力：

- 当前是否允许运行（应用退出时禁止）；
- 当前是否存在打开的项目；
- 当前时间（测试可使用固定时间）。

这些回调是环境 stub 的接入点，不需要 StubMainWindow，也不需要模拟 Parser、Debugger 或 Qt 控件。

## 3. 自动备份测试

在 `test-editor` 中使用真实 Editor、Settings 和 QTemporaryDir 覆盖：

- 功能开关、只读状态和备份文件创建；
- 备份内容随编辑更新，定时器节流规则保持不变；
- 正常保存、重命名、关闭和析构时的备份清理；
- 已存在备份不被无条件覆盖，失败路径不留下无效 QFile 或活动定时器。

测试可以直接调用私有备份入口并检查定时器状态，不依赖真实等待。

## 4. 自动保存测试

使用真实 EditorManager、QTabWidget、QSplitter、Editor 和 QTemporaryDir。测试夹具直接创建带测试 Settings 的 Editor 并加入 Tab，不调用包含全部 IDE 服务的完整 newEditor()。

覆盖：

- 功能禁用、退出状态、定时器启停和分钟间隔；
- 当前文件、所有打开文件、项目文件的目标选择；
- `canAutoSave()` 为 false 时不写文件；
- 覆盖保存更新原文件并清除 modified；
- Unix 时间和格式化时间副本使用固定时钟产生可预期文件名，原文件不变；
- 保存失败通过信号报告，不在测试中弹出对话框；
- 状态通知和保存范围一致。

直接调用立即处理入口验证行为；定时器本身只检查状态、间隔和信号连接，不等待分钟级超时。

## 5. KISS 与范围审核

不创建新的生产模块，不 stub 被测 EditorManager，不把 MainWindow 整体复制进测试，也不为测试拆分 newEditor()。优先复用真实 Editor 和真实文件；只对项目、退出、时间和 UI 通知这些外部环境做隔离。

现有自动保存规则先保持不变，测试用于暴露问题。若发现缺陷，单独记录根因和最小回归测试，不通过修改场景掩盖。
