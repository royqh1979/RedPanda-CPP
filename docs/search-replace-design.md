# Search / Replace 与 Find in Files 设计

## 目标

本文覆盖三个相关但不同的层次：

1. QSynEdit 单文档搜索与替换；
2. 编辑器中的交互式搜索/替换；
3. 当前文件、打开文件、项目和目录范围的 Find in Files。

首要目标是验证现有行为的正确性并建立性能基线，再决定重构或优化。接口调整不应改变现有用户可见行为，除非另行明确决定。

## 现状与主要问题

### QSynEdit 搜索/替换

当前核心入口是 `QSynEdit::searchReplace()`，其职责同时包括：

- 根据 caret、selection、scope 和选项决定搜索范围；
- 逐行调用 `Searcher`；
- 处理方向、wrap、整词和大小写；
- 修改 caret 和 selection；
- 通过回调决定是否跳过、替换、全部替换或退出；
- 执行替换并修正偏移量；
- 维护 `newScopeEnd`、Undo 分组和编辑状态。

这使搜索、状态展示和编辑操作紧密耦合。即使只做搜索，也会经过选择/caret 相关流程；批量搜索和批量替换也共享大量分支。

现有 `Searcher` 接口本身较简单，提供逐行匹配及替换文本生成，但它只覆盖模式匹配层，不能解决上层职责混杂的问题。

### Find in Files

`SearchInFileDialog::doSearch()` 当前同时负责：

- 收集对话框输入；
- 枚举目录或项目文件；
- 打开并读取文件；
- 创建临时 QSynEdit 并搜索；
- 显示进度和处理取消；
- 生成搜索结果模型；
- 在 UI 线程中逐目录、逐文件调用 `processEvents()`。

这部分难以独立测试，也无法准确区分文件枚举、IO、匹配和结果模型生成的成本。

## 设计原则

- 纯文本匹配不依赖 QSynEdit 或 UI 状态；
- 文档搜索只返回匹配，不修改 caret、selection 或文档；
- 编辑器交互层负责展示匹配、更新 caret/selection 和处理用户确认；
- 替换只处理已确认且仍有效的匹配；
- Find in Files 复用查询和匹配组件，但不依赖编辑器控件；
- 文件枚举、文件读取、匹配和结果模型生成分别测试和计时；
- 后台线程只作为有 profile 依据的可选优化。

## 具体接口与执行流程

### 基础类型

不再通过多个松散参数和隐式编辑器状态传递搜索条件。查询、范围和匹配分别使用独立类型：

```cpp
struct SearchQuery {
    QString pattern;
    bool useRegex = false;
    bool matchCase = false;
    bool wholeWord = false;
};

struct TextMatch {
    int ch;
    int length;
};

struct SearchRange {
    QSynedit::CharPos begin;
    QSynedit::CharPos end;

    bool contains(const QSynedit::CharPos &pos, int length) const;
};

struct DocumentMatch {
    QSynedit::CharPos start;
    int length;
    int documentVersion;
};

enum class SearchDirection {
    Forward,
    Backward,
};

enum class WrapMode {
    Never,
    Once,
};
```

替换文本不放入 `SearchQuery`，因为它只影响替换，不影响匹配。

`TextMatch` 是当前行内的 0 起始位置；`DocumentMatch` 在其基础上补充行号，并用 `documentVersion` 标记生成匹配时的文档版本。替换前必须检查该版本，禁止用过期匹配修改新内容。

`Searcher` 的查询结果直接返回 `QVector<TextMatch>`，不再通过 `result()`、`length()` 和结果下标重复访问内部状态。regex 的 `replace()` 仍按实际匹配文本生成替换内容。

### Whole word

whole word 使用统一的 Unicode 文本单词边界，不使用 syntaxer token 边界。旧实现会使字符串、注释和正文中的搜索行为不一致，也会迫使普通文本和 Find in Files 依赖具体语言。

单词字符包括：

- Unicode 字母和数字；
- Unicode 组合标记，避免在分解形式的字符内部形成边界；
- Unicode 连接符类别，包括 `_`。

其它字符构成单词边界。非零长度匹配只有在左端之前和右端之后都不是单词字符时，才满足 whole word；多词匹配也只检查整个匹配的两端。零长度 regex 匹配不做 whole word 过滤。

`ch/length` 仍是 QString 的 UTF-16 code unit 下标。边界检查必须分别向左、向右解码一个完整 Unicode code point，不能把 surrogate pair 当作两个独立字符。该规则由纯搜索层实现，因此不再需要 `TokenBoundaryProvider` 回调。

### DocumentSearchSession

使用一个具体类，不引入没有必要的抽象接口：

```cpp
class DocumentSearchSession {
public:
    DocumentSearchSession(
            std::shared_ptr<QSynedit::Document> document,
            SearchQuery query,
            SearchRange scope);

    std::optional<DocumentMatch> findNext(
            QSynedit::CharPos from,
            SearchDirection direction,
            WrapMode wrap);
};
```

`from` 和 scope 始终使用 line/ch，不转换成全局字符偏移，也不把 Document 合并成一个大字符串。当前行调用 `Searcher::findAll()` 得到行内 `ch/length`；首行和末行直接按 ch 过滤；找到后补上行号返回，否则按方向移动到下一行。

职责仅限：

- 根据 `from`、方向和 scope 计算候选行；
- 调用现有 `Searcher` 得到行内匹配；
- 过滤不完整或越界的匹配；
- 按方向选择第一个有效匹配；
- 根据 `WrapMode::Once` 最多绕回一次；
- 零长度匹配必须有明确的下一次起点：向前搜索移到下一字符或下一行，向后搜索移到前一字符或前一行，不能在原位置循环。

它不读写 caret、selection，不执行替换，也不显示提示。相同输入必须得到相同匹配。

编辑器调用方负责把结果展示出来：

```cpp
auto match = session.findNext(from, direction, wrap);
if (match) {
    QSynedit::CharPos end{
            match->start.ch + match->length,
            match->start.line};
    editor->setCaretAndSelection(end, match->start, end);
}
```

### 单次搜索/替换流程

一次交互操作按以下顺序执行：

1. 根据 caret、selection 和选项计算初始搜索位置；
2. 用 `DocumentSearchSession` 查找匹配；
3. 没有匹配则直接结束，不改变文档；
4. 编辑器选中该匹配并定位 caret；
5. 用户选择 Skip、Replace、Replace All 或 Cancel；
6. 只有选择替换时才调用编辑操作；
7. 根据替换后的 caret 计算下一次搜索起点。

这样 Skip、Cancel 和纯搜索都不会进入编辑事务，也不会意外改变 Undo 列表。

### 替换接口

在 QSynEdit 上提供两个明确操作：

```cpp
struct ReplacementResult {
    QSynedit::CharPos caret;
    QSynedit::CharPos selectionBegin;
    QSynedit::CharPos selectionEnd;
    QSynedit::CharPos scopeEnd;
};

std::optional<ReplacementResult> replaceMatch(
        const QSynedit::DocumentMatch &match,
        const QString &replacement);

int replaceAll(
        DocumentSearchSession &session,
        const QString &replacement);
```

`replaceMatch()` 要求传入本次会话刚找到的匹配。只有 `match.documentVersion` 等于当前文档版本时才执行；否则返回空值并重新搜索。它负责：

- 校验匹配版本和范围；
- 删除匹配文本并插入替换文本；
- 更新 caret、selection 和 scope end；
- 返回下一次搜索所需的定位信息。

`replaceAll()` 复用会话中的文档、查询和 scope，并接收替换文本。它负责：

- 创建一次编辑事务和一个 Undo 组；
- 找出 scope 内全部匹配；
- 从文档末尾向开头执行替换，避免前一次替换改变后一次匹配的位置；
- 普通替换直接使用给定文本；regex 使用 `Searcher::replace(实际匹配文本, replacement)` 分别生成每项替换；
- 处理变长、空、多行和 Unicode 替换；
- 返回实际替换数量；
- 按现有用户行为恢复最终 caret 和 selection。

“先收集、逆序替换”用明确的 O(匹配数) 额外空间换取位置稳定性，避免继续依赖当前行偏移量补丁。如果以后证明内存占用有问题，再改为基于 lineSeq 和偏移量的增量方案。

### 旧接口迁移

现有 `searchReplace()` 在迁移期只保留为薄封装：

1. 构造查询、范围和会话；
2. 调用 `findNext()`；
3. 负责交互回调和当前匹配显示；
4. 调用 `replaceMatch()` 或 `replaceAll()`；
5. 返回匹配/替换结果。

所有调用方迁移完成后删除旧接口，不保留两套搜索算法。

### IDE 搜索组件的归属

QSynEdit 只保留通用编辑能力：`Searcher`、Unicode whole word、`DocumentSearchSession` 和匹配后的文档编辑。文件枚举、批量文件读取、结果树以及选择性替换属于 IDE 工作流，放在 `RedPandaIDE/src/search/`：

- `FileSearchEngine`：搜索文本、文件和目录，不依赖编辑器控件；
- `SearchResultReplacer`：替换单个文件内已勾选的结果，不打开或保存文件，也不显示错误对话框；
- `SearchInFileDialog`：提供搜索范围和未保存的打开缓冲区，生成结果树；
- `MainWindow`：管理打开文件、磁盘保存、替换面板和错误提示。

对应测试使用独立的 `test-file-search` 目标，不混入 `test-widgets` 或 QSynEdit 测试。

### Find in Files 调用关系

文件搜索直接复用查询和匹配组件，不创建 QSynEdit：

```cpp
using ProgressCallback = std::function<void (int completed, int total)>;
using CancelCallback = std::function<bool ()>;
using MatchBatchCallback = std::function<void (QVector<FileSearchMatch>)>;

struct FileSearchCallbacks {
    MatchBatchCallback onBatch;
    ProgressCallback onProgress;
    CancelCallback shouldCancel;
};

class FileSearchEngine {
public:
    FileSearchEngine() = default;

    int searchText(
            const QString &fileName,
            const QStringList &lines,
            const SearchQuery &query,
            const FileSearchCallbacks &callbacks);

    int searchFile(
            const QString &fileName,
            const SearchQuery &query,
            const FileSearchCallbacks &callbacks);

    QList<QFileInfo> collectFiles(
            const FileSearchRequest &request,
            const ProgressCallback &progress,
            const CancelCallback &cancel);
};
```

搜索按有界批次通过 `onBatch` 返回结果，不在内存中累积整个任务的全部匹配；搜索文本、文件读取和文件枚举都响应取消。测试用简单回调收集结果，UI 则按批更新模型。

- `searchText()` 负责已知文本，可直接单元测试；
- `searchFile()` 只增加文件读取、编码判断和异常处理；
- `collectFiles()` 只负责目录遍历和过滤；
- 打开文件的未保存内容由调用方传入 `searchText()`；
- 最终结果由 MainWindow 在 UI 侧按文件聚合成树：将每批 `FileSearchMatch` 转换为 `SearchResultTreeItem`，加入 `SearchResultModel` 并展示；
- 引擎不创建树节点，也不直接操作搜索结果视图；取消后是否展示已有部分结果由 UI 决定；
- 若以后需要后台执行，以整个请求和结果为线程边界，不共享编辑器对象。

### SearchResultReplacer

`SearchResultReplacer` 只服务 Find in Files；查找引用和重命名不经过该组件。它接收一个已打开的 `QSynEdit`、该文件的搜索结果和替换文本，只处理 `selected=true` 的叶子项。`SearchResultTreeItem::line/start` 与 `CharPos` 一样从 0 开始，不得再做 `start-1` 转换。

执行顺序：

1. 收集已选项，并使用生成结果时保存的 `item->matchedText` 作为原始匹配文本；`item->text` 仅用于显示，其中的 Tab 已被替换为空格，不能用于校验；
2. 按 line/ch 逆序排列，避免替换改变后续匹配的位置；
3. 修改前先校验所有已选位置仍存在相同的原始匹配文本，任一不符则本文件完全不修改并返回内容变化错误；
4. 使用 `SearchResults` 中的 matchCase、wholeWord、useRegex 构造 `DocumentSearchSession`；
5. 通过会话生成每项替换文本，普通替换直接使用给定文本，regex 使用原始匹配展开捕获组；
6. 通过 `QSynEdit::replaceMatch()` 逆序执行，每个文件只使用一个编辑事务和一个 Undo 组。

打开编辑器的未保存内容、文件打开与编码保存仍由 MainWindow 负责。校验失败不得产生部分替换；未选择任何项时不创建编辑事务。

### CppRefactor 引用与重命名模型

实施顺序上先完成单文件语义查找和重命名：当前文件内完整命中、同名隔离、修改前校验、失败不修改及 Undo/Redo；发现文件外声明、定义或引用时明确拒绝。项目级查找和修改作为后续独立阶段，通过逐个重新解析 TU 主文件完成，不要求 Parser 共享跨 TU statement 或提供项目符号表。项目级测试场景保留，在该阶段统一启用。

`CppRefactor` 核心处理语义 statement 的引用与重命名，不直接依赖 Find in Files 的 `SearchResults`、`SearchResultTreeItem`，避免语义核心携带 regex、勾选状态、`matchedText` 等文本搜索或 UI 概念。查找面板本身继续复用现有 `SearchResults`、树模型和视图，不另做一套引用展示界面；MainWindow 在 UI 边界把语义结果转换为面板结果，核心不知道该适配层存在。

查找引用与重命名共用同一套目标解析、关联符号扩展和引用查找逻辑，并使用相同核心结果类型：

- `SymbolReference`：文件、0 起始 line/ch、匹配长度、原始显示文本和引用角色；角色至少区分声明、定义与普通引用，构造函数、析构函数和模板特化仍通过其对应 statement 判断；
- `SymbolReferenceFile`：一个文件及其引用列表；
- `SymbolReferenceResults`：目标符号、关联符号、所属 scope、当前文件/项目范围和按文件分组的结果；
- `SymbolReferenceScope`：语义查找自己的范围枚举，不复用 `SearchFileScope`。

`findOccurrences()` 只查找并返回核心结果，不修改文档，也不创建查找面板节点。独立的 Find References 命令把这些结果显示在现有查找面板中，但不打开文本替换面板。打开文件的未保存内容通过显式 content provider 获取，项目通过构造参数提供，不再从 MainWindow 隐式读取。

重命名复用相同的语义查找结果，不另维护一套可能产生不同命中集合的搜索算法，按以下顺序执行：

1. 在发起文件中确定目标及调用方选定的重载子集；若项目查找从头文件发起，先把该头文件作为主文件单独解析一次；
2. 单文件操作重新解析当前文件；项目操作收集各 TU 主文件和打开编辑器的最新内容，并逐个将 TU 主文件交给 Parser 重新解析；
3. 在每次解析得到的当前主文件上下文中重新识别目标，不复用其他 TU 或重解析前的 statement 指针；
4. 扩展本次必须联动的关联符号：类与构造/析构函数、模板主定义与特化、继承链中同签名覆盖函数；
5. 查找目标及所有关联符号的声明、定义和普通引用；项目查找覆盖当前各 TU 的 include 闭包，不搜索未被任何 TU 包含的其他头文件，同一头文件在多个 TU 中的相同命中按文件及位置去重；
6. 在任何文档修改前完成名称冲突、声明/定义归属和范围外引用校验；
7. UI 将全部待修改位置作为只读预览显示在现有查找面板中，用户确认或取消整个操作；预览不提供逐引用勾选，避免漏改声明、定义、覆盖函数或其它必须联动的位置；
8. 用户确认后，先重新校验文档版本和各位置的原始文本，再按文件和位置逆序替换全部引用，并为每个文件建立一个 Undo 组；内容已变化或任一校验失败都不得产生部分修改。

重命名允许用户选择的是重载等语义单位，不是任意文本命中项。底层可以复用位置校验、逆序替换和 Undo 分组等通用编辑能力，但不直接复用 `SearchResultReplacer` 当前逐文件、按勾选项替换的工作流；该工作流不能保证项目范围的全部语义校验先于任何文件修改。

单文件重命名要求本次目标及所有关联符号的声明、定义和引用都位于当前文件。项目级重命名要求它们都位于当前各 TU 的 include 闭包及可修改的项目范围内，包括打开编辑器的未保存内容；不要求是同一个物理文件。发现范围外声明、定义或引用时必须明确返回错误并拒绝整个操作。未勾选的重载不属于本次目标，不参与联动和完整性校验。

重命名返回明确结果码，不在核心弹 QMessageBox。入口必须安全处理空 editor、无 parser、空表达式、无效位置和非法新名称。局部符号在准备和校验成功后直接整体重命名，不经过查找/预览面板；`renameUndefinedLocalVariable()` 同样直接处理，只在包含的函数/lambda 内替换未解析标识符，并拒绝与已有符号冲突。

多个重载由 UI 提供可勾选列表，默认全选，允许选择任意子集。核心通过 `overloadsOf()` 提供同 scope 重载，并接收调用方选定的 statement 集合；空选择不修改文档。父类函数与子类同签名覆盖函数双向联动，未勾选重载不受影响。

`PStatement` 指针只在同一冻结快照内作为身份快速路径。不同 TU 或重解析后可能重建 statement，每个 TU 中应重新取得目标，再使用 kind、完整 scope/name、函数签名、模板信息及关联符号关系判断语义等价；definition position 可以佐证同一 occurrence，但不能单独作为跨 TU 身份，也不能只比较首选声明或依赖指针。namespace 等允许合法重复打开的实体还需按其语义规则处理。文档内容变化后应使旧结果失效，而不是继续沿用旧位置。

凡需要持有并使用 parser statement 的只读查找、修改前校验和实际批量修改，都必须在对应操作期间 freeze parser，并由作用域守卫保证所有正常返回和错误返回路径都与 unfreeze 配对。只读操作同样要防止后台重解析使 statement 失效；操作结束、校验失败或用户取消预览时均不应遗留冻结状态。每次主文件 reparse 必须在取得该次目标及 freeze 稳定快照之前完成。

不得为了加速重命名而修改 Parser 接口。优先使用现有 statement、scope、签名、继承和模板信息；若确实无法表达所需语义，必须先记录具体场景、缺失信息和候选方案并征得同意后，才能调整接口。

独立 `test-refactor` 覆盖：

- 局部/全局符号的当前文件与项目引用、未保存内容和 scope；多文件测试只建立项目与内容并调用公开接口，逐 TU reparse、目标重识别、引用查找和重复命中去重由被测实现完成，不得在测试中代做；
- 核心引用结果到现有查找面板的适配，保证文件、位置、长度和显示文本不丢失；
- 重命名预览取消时不修改文档，确认后整体修改，预览后内容变化时拒绝过期替换；
- 在同一测试文件中放置多个命名空间、类、函数和局部作用域的同名符号，同时断言应修改项全部修改、不应修改项保持原文；
- 同声明行的不同列、注释与字符串排除；
- 类/构造函数/析构函数/成员/局部变量重命名及 Undo/Redo；
- namespace、别名、枚举、using declaration 和可见性；
- 重载的全部选择、单个选择和任意子集选择，以及继承链中同签名覆盖函数的双向联动；
- 主模板、偏特化、显式特化、模板成员和模板参数；函数模板不得使用不存在的偏特化场景；
- 单文件操作遇到文件外声明、定义或引用时拒绝修改，项目级操作遇到项目范围外声明、定义或引用时拒绝修改，并验证失败后文本和 Undo 列表均不变；
- 已有名称冲突、空表达式和非法输入；
- 未定义局部变量的作用域、lambda、嵌套作用域和已有名称冲突；
- parser freeze/unfreeze 在所有成功和失败路径上的配对，以及编辑后索引一致性。

Go to Definition/Declaration 与引用查找使用相同的精确目标匹配规则。测试必须分别检查重载函数按实参定位到正确声明/定义，类模板在主模板、偏特化和显式特化之间定位到正确 statement；断言目标身份及准确的 0 起始 line/ch，不能只比较名称。

## 正确性测试方案

### Replace 测试矩阵

先审计现有 `TestQSyneditCppSearchReplace`，保留已有覆盖，再补齐以下类别：

- 等长、变短、变长和空替换；
- 同行多处替换及后续位置修正；
- 替换文本包含原搜索文本，如 `a → aa`，不得重复匹配；
- 文件、行和 scope 的起止边界；
- 向前、向后、wrap、from caret；
- 正向/反向 selection，以及包含或排除当前 selection；
- 普通文本、整词、大小写、regex 捕获组和零长度匹配；
- 空串、多行文本、Unicode/中文；
- Replace once、Replace all、Skip、Exit；
- 无匹配、空搜索词、read-only；
- Undo/Redo 后内容、caret、selection、scope end 和 Undo 分组。

每个测试应同时检查必要的结果，而不只是最终文本：匹配位置、替换数量、caret/selection 和 `newScopeEnd` 中与实际行为相关的部分。

### Find in Files 正确性测试

使用临时目录和已知内容覆盖：

- 当前文件、打开文件、项目、目录四种范围；
- 过滤器、递归、空目录、多层目录；
- 文件不存在、二进制文件、无权限或读取失败；
- 常见编码及打开编辑器中尚未保存的内容；
- 普通、整词、大小写和 regex；
- 多个匹配的文件、行、列、长度和行文本；
- 取消后的部分结果语义；
- 搜索不得改变打开编辑器的 caret、selection 或滚动位置。

### SearchResultReplacer 正确性测试

在 `test-file-search` 的 `TestSearchResultReplacer` 中覆盖：

- 只替换已选项，未选项保持不变，单文件一次 Undo/Redo；
- 首列、同行多个匹配和忽略大小写；
- regex 捕获组、变长匹配和零长度匹配；
- 内容变化时拒绝本文件的全部替换且不产生 Undo 项；
- Unicode、空替换、多行替换和原始 Tab 匹配；
- 无已选项时不修改文档、不创建编辑事务。

## 性能基线与优化顺序

修改前后使用同一组临时数据，分别记录：

- 文件枚举耗时；
- 文件读取耗时；
- 匹配耗时；
- 结果模型构建耗时；
- 文件数、匹配数、总字节数和总耗时。

场景至少包括：无匹配、少量匹配、密集匹配、长行、多层目录，以及普通/整词/regex 三种模式。需要 UI 时还应记录单次事件循环最长阻塞时间，而不只看后台总吞吐量。

优化顺序建议：

1. 消除重复扫描、无效对象创建和不必要的 UI 状态修改；
2. 减少逐文件/逐目录 `processEvents()` 和模型逐行通知；
3. 缓存或复用纯文本匹配器；
4. 根据实测结果决定是否使用后台线程；
5. 只有在匹配确为热点时，才考虑更底层的字符串算法优化。

### 当前引擎基线

通过 `QSYNEDIT_SEARCH_PROFILE=1 test-file-search --suite=TestFileSearchEngine test_profile` 可重复测量。Windows、GCC 8.1、Qt 5 static Release 的第二轮结果如下：

- 2000 行无匹配：0.063 ms；
- 2000 行普通匹配（2000 个结果）：0.246 ms；
- 2000 行密集匹配（6000 个结果）：0.475 ms；
- 2000 行 whole word（6000 个结果）：0.489 ms；
- 2000 行 regex（8000 个结果）：2.073 ms；
- 单个长行（10000 个结果）：0.697 ms；
- 64 个文件的目录枚举：1.701 ms；
- 64 个文件读取并匹配（12288 个结果）：9.254 ms。

这些数据只描述新引擎，不包含旧实现对照、结果模型构建或 UI 阻塞时间，不能据此计算界面加速比例。目前没有必要仅因匹配耗时引入后台线程；目录和项目搜索仍应通过减少 UI 工作及事件循环开销继续优化。

## 建议实施顺序

1. 审计并补齐现有 Replace 测试；
2. 为当前 `searchReplace()` 建立行为基线；
3. 提取纯文本匹配与文档搜索会话；
4. 将 Replace 改成显式匹配后的编辑操作；
5. 提取 Find in Files 引擎并补正确性测试；
6. 建立性能基线，按数据选择进一步优化；
7. 最后清理旧接口，不在过渡期同时维护两套重复实现。

## 独立复审结论

复审后修正了以下实质问题：

- 删除重复且不一致的早期接口定义；
- `Searcher` 直接返回行内匹配列表，不再通过下标读取内部状态；
- `DocumentMatch` 增加文档版本，替换前拒绝过期匹配；
- Replace All 显式接收替换文本，并复用同一会话的查询、scope 和整词边界；
- regex 按每项实际匹配生成替换文本；
- 定义零长度匹配的前进/后退规则；
- Find in Files 使用有界结果批次，搜索和读取阶段也可取消。

仍需保留的设计取舍：

- Replace All 的“先收集、逆序替换”需要 O(匹配数) 额外内存，只有实测证明有问题才改成更复杂的增量偏移方案；
- 文件枚举首版仍返回列表；大目录若存在内存或首结果延迟问题，再根据 profile 改成流式枚举；
- 后台执行不是默认目标，先验证同步引擎的正确性和实际热点；
- 在现有行为测试补齐前，不应开始替换旧实现。
