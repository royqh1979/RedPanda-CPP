# 函数体表达式中的 Lambda 建模设计

## 问题与边界

当前 Parser 只在 declarator 的初始化表达式中收集 `LambdaInfo`。`win.run([&](Ui& ui) { ... });` 是普通表达式语句，声明解析失败后由 `CppSyntaxParser::skipDeclaration()` 直接跳过，因此 lambda 的形参和函数体局部声明没有 Statement，也无法按作用域导航。

本次只处理函数体普通表达式语句中的 lambda。保持“表达式不求完整 AST、除有限类型推导外只跳过 token”的既有方向，不扩展到 if/for 条件、控制语句头部或通用表达式建模。

## 数据结构

- `DeclarationInfo` 增加 `expressionLambdas`，只保存表达式扫描发现的 `LambdaInfo`。
- `DeclarationKind` 增加 `Expression`，用于区分表达式语句与普通声明。它不是新的 StatementKind，不进入名称索引。
- `CppDeclarationParser` 提供一个表达式扫描入口：复用现有 `skipExpression()` 和 `parseLambda()`，在临时 declarator 中收集 lambda 后转交 `expressionLambdas`。
- `CppSyntaxParser::parseBlock()` 在普通声明解析失败时调用该入口，以 `;` 和当前 Block 的 `}` 为边界，不再无信息地跳过整个表达式。

## Statement 构建

`CppStatementBuilder::build()` 遇到 `Expression` 时，只调用现有 lambda 构建路径并返回，不创建表达式 Statement。

现有 `buildLambdas()` 改为接收 lambda 列表；原 declarator 入口仅转发其 `declarator.lambdas`。因此 Lambda Statement、`operator()`、参数 Statement、body 内局部声明、bodyRange 和 `findScopeAtLine()` 全部沿用同一实现，不增加第二套导航逻辑。

## KISS 与范围审核

- 不改变 `LambdaInfo`、StatementKind 或 Eval 的表达式规则。
- 不构造表达式 AST，不保存普通表达式文本，不引入新缓存。
- 不把函数调用接收者或其它表达式标识符提前求值；只建立导航所需的 lambda 作用域。
- 不覆盖 if/for 条件、嵌套控制头部等额外场景，后续按实际缺陷单独评审。

## 测试

在 `TestCppParser` 中解析 `win.run([&](Ui& ui) { ui.panel(); Stack col; col.next(); });`，验证：

- lambda 形参 `ui` 可通过 `findStatementOf()` 找到，类型为 `Ui &`，并具有定义位置。
- lambda 内局部变量 `col` 可找到，类型为 `Stack`，并具有定义位置。
- `ui.panel`、`col.next` 的成员解析和 lambda 内作用域查询使用现有接口，不通过测试侧重建 Statement 来模拟实现。
- `test_nested_lambda_function_argument_scope` 额外验证外层/内层形参、内层局部变量和外层名称查找，确保递归括号扫描不混淆两层作用域。
- 标准库专项 `test_aino_replay_and_lambda_navigation` 使用 `resources/aino`，验证真实函数实参 lambda、连续 Parse 的 Statement 数量稳定及 Windows 头 replay；该测试只随 `REDPANDA_PARSER_STDLIB_TEST` 运行。
- `test_user_header_reparse_does_not_duplicate_statements` 验证 file/context include 闭包中的非系统文件同步失效，避免用户头重复 Statement。
- 原有 declarator 初始化 lambda 的返回类型推导测试保持通过。
