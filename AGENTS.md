# Red Panda C++ — Agent 规则与文档索引

## 1. 项目约定

- Red Panda C++ 是跨平台 C/C++/GNU 汇编 IDE，使用 C++17 和 Qt（Qt 6.8+ 或 Qt 5.15），许可证为 GPL v3+。
- 版本号以 `version.json` 为准，构建号由 `git rev-list HEAD --count` 生成。
- 主构建系统为 CMake，Xmake 也在 CI 中使用；qmake 工程仅作遗留兼容。权威构建说明见 `BUILD.md`、`BUILD_cn.md`、`BUILD_ru.md`，不要凭空创造构建步骤。
- 新增受特性开关控制的源文件时，必须同步登记到 `RedPandaIDE/CMakeLists.txt` 对应 `if(...)` 块及 `RedPandaIDE/xmake.lua` / 根目录 `xmake.lua`。
- `libs/lua/lua/` 必须与上游 Lua v5.4.6 保持一致，不得本地打补丁。

## 2. 文档目录规则

- `AGENTS.md` 只保存长期规则和顶层索引，不堆叠具体设计、profile 数据或本机命令。
- 根目录保留 `README*`、`BUILD*`、`AGENTS.md`、`NEWS.md`、`Contributors.md` 和 `PRIVACYPOLICY.md`，便于用户直接查找；内部设计、性能和环境文档放入 `docs/` 子目录。
- `docs/design/`：现状、约定、算法和实施方案，按模块组织。
- `docs/profile/`：性能基线、报告、结果与配套工具，按模块组织。
- `docs/env/template/`：不含机器专有路径的环境模板；`docs/env/local/`：本机实际配置，默认不提交。
- 设计文档描述现状和约定，具体修改记录写入 git 提交信息，不放进设计文档。
- `AGENTS.md` 使用中文；代码注释和 git 提交信息使用英文；其它文档语言随已有文档。
- 完成用户可见功能或修复时，更新 `NEWS.md` 当前版本节，不记录开发过程中临时引入又修复的问题。
- 修改目录职责、构建选项、测试命令或上述规则时，同步更新本索引及相关文档。

## 3. 代码与资源约定

- 使用 Qt 惯用法（signals/slots、用户可见字符串 `tr()`、QObject 父子所有权）。
- 新代码与所在文件现有命名、结构和行尾风格保持一致；仓库没有统一的 `.clang-format` / `.editorconfig`。
- `RedPandaIDE/src` 新类通过 `target_qt_plain_cpp`、`target_moc_classes`、`target_ui_classes` 注册。
- 资源通过 `target_embed_resources` / `target_embed_translations` 管理，不手写 `.qrc`。
- 特性代码及其 `#include` 使用 `ENABLE_LUA_ADDON`、`ENABLE_SDCC`、`ENABLE_VCS` 条件编译。
- Lua 插件 API 使用 Teal 标注，修改前核对 `addon/defs/*.d.tl` 和调用方。
- 不提交密钥、凭据或本机专有配置；执行外部工具和不可信代码时优先复用 `src/utils/escape.*`。

### Lambda / 槽寿命

- `connect()`、`QTimer::singleShot()` 等以 lambda 作槽时必须传入合适的 QObject context，且连接寿命不得超过其捕获对象寿命。
- 不得用 `&` 引用捕获局部变量供长命连接使用；需要写回局部变量时，使用与局部变量同生命周期的局部 QObject context，或在返回前显式 `disconnect`。
- 按值捕获的裸指针必须明显比 context 长寿，优先捕获值类型或 `shared_ptr`。

## 4. 构建与测试规则

- 测试使用 Qt Test，CTest 已接入根目录构建。
- 完整回归必须先 Release 后 Debug，分别执行 `all-test-targets` 和 `ctest --output-on-failure`；两种构建均通过才算完成。
- 测试按被测组件职责放入对应 Suite 和文件，不得因链接方便混放：
  - Parser：`RedPandaIDE/test/` 的 `test-cppparser`；
  - Editor：`test-editor`；
  - Widgets：`test-widgets`；
  - 文件搜索与替换：`test-file-search`；
  - 语义引用与重命名：`test-refactor`；
  - 试题集与输出验证：`test-problemset`；
  - QSynEdit/Syntaxer：`libs/qsynedit/test/` 的 `test-qsynedit`。
- Parser 组件和集成测试统一由 `test-cppparser` 执行，可用 `--suite=类名` 筛选。
- 调试形成的有效验证场景必须保留为正式断言，不得修完后删除。
- 不得因测试不符合当前实现或预设而丢弃场景；先判断是否存在新合法场景或实现 bug。夹具错误只能修正错误部分，并保留原场景或增加等价回归测试。
- 测试/profile 配套 Python 脚本、汇总和对比工具必须随对应测试 resource 存入仓库，并在专门文档中记录用途、依赖、输入输出和运行命令，不得只放临时目录或用完即弃。
- 性能测量直接运行 Release 可执行文件，不在 gdb 下计时；优先使用已扣除子阶段的 self-time，不能把 inclusive 时间相加。
- 所有 Parser 相关修改在收尾时都必须提供标准库 Parse profile 和验证结果；至少报告编译器/标准库、完整 Parse 总耗时、各阶段耗时、Statement 数量和测试结果。已有同场景基线时同时报告对比变化。
- 性能优化逐项实施：独立分支 → 设计文档 → KISS/范围审核 → 测试保护 → 严格范围编码 → 专项和全量回归 → 同场景 profile → 审核迭代 → 结果总结与提交。存在多个值得验证的方案时分别建分支比较。完成审核后可合并回 `document-edit-profile`；合并到 `master` 和推送仍需明确授权。

## 5. 环境与工具索引

- 构建指南：`BUILD.md`、`BUILD_cn.md`、`BUILD_ru.md`。
- 通用本机环境模板：`docs/env/template/local-development.md`。
- 本机实际环境：`docs/env/local/AGENTS.local.md`（gitignore，不存在时先复制模板）。
- `AGENTS.local.md` 及其它本机文件不得提交；R: 内存盘内容重启后失效，构建目录需重新配置。

## 6. 总体设计与 Profile 索引

- `docs/design/architecture.md` — 总体设计及全部模块设计文档索引。
- `docs/profile/README.md` — 性能基线、报告、优化结果与工具索引。

## 7. 参考资料索引

- `NEWS.md` — 更新日志。
- `Contributors.md` — 贡献者。
- `PRIVACYPOLICY.md` — 隐私政策。

## 8. Git 与安全规则

- 大的计划性改动必须在独立分支进行，不直接在 `master` 上工作。
- 修复 bug 后先报告根因和方案，得到确认再提交；提交信息使用英文。
- 合并到 `master` 或推送前必须获得明确授权，不得擅自执行。
- 不得提交密钥、`.env`、SSH 私钥或本机专有配置；构建产物不进入仓库。
