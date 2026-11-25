# C/C++ 标准库公开接口基准

## 目标与边界

本基准用于 Parser 改动和 profile，验证 C++17 及其所包含的 C11 库接口在 StatementModel 中完整可见，并对真实表达式求值路径进行性能测量。它不属于日常构建或默认 CTest。

基准检查公开声明的完整名称，不检查函数签名、返回类型、调用参数或访问控制。相同 `fullName` 的重载在名称级基准中合并；不得把通过本基准表述为所有重载或所有表达式场景均已验证。

## 独立来源与生成

数据来源为 cppreference 官方离线原始文档包 `cppreference-doc-20250209.tar.xz`：

- https://github.com/PeterFeicht/cppreference-doc/releases/tag/v20250209
- `index-functions-cpp.xml`：C++ 公开类、成员、函数、常量、typedef、继承和别名索引。
- `index-functions-c.xml`：C 公开接口索引。
- C++17 标准草案用于核对版本、删除接口和 C 头边界：https://timsong-cpp.github.io/cppwp/n4659/headers

`RedPandaIDE/test/resources/stdlib/generate_public_api_baseline.py` 读取离线文档包并生成静态 TSV、头文件清单和元数据。生成器仅在更新基准时手工运行，构建和测试不访问网络，也不需要安装 Python 第三方包。

生成规则：

- 保留 C++98/03/11/14/17 接口，排除 C++20 及更新版本和 experimental 接口。
- 排除 C++14 的 `gets`、C++17 已删除的 `auto_ptr`、旧函数对象适配器、`random_shuffle` 和 unexpected-handler 系列。
- 按 XML 中明确的 `protected` 注释排除后续 protected 成员。
- 只检查实际公开声明，不把继承或 typedef 别名机械展开成新的声明路径；通用模板不复制特化专属成员。
- 修正索引中明确的拼写和所属类型错误，过滤 POSIX errno、平台相关的 imaginary 宏以及文档聚合产生的无效通用接口。
- C 基准排除 C++ 标准未包含的 `stdatomic.h`、`threads.h` 接口及 C11 已删除的 `gets`。
- 补齐上游 C 索引明确缺少的 C11 `aligned_alloc`、`timespec_get`、`TIME_UTC` 及数学函数 `f`/`l` 变体。

生成的 TSV 每行是 `kind<TAB>fullName`。元数据记录来源版本、过滤后数量及头文件清单，供审查生成结果。

## 编译开关与测试职责

`REDPANDA_PARSER_PUBLIC_API_BASELINE_TEST` 默认 `OFF`。开启时需要同时提供 `REDPANDA_PARSER_STDLIB_MINGW_ROOT`，仅给 `test-cppparser` 添加 `TestCppParserPublicApiBaseline` Suite，不启用其它标准库测试。

每个基准套件执行：

1. 创建显式包含全部对应标准头的 TU，使用真实编译器宏和 include 路径解析。当前基准包含 4107 个 C++17 名称和 833 个 C11 名称。
2. 逐项检查声明可见性，并分别汇总真实查找缺失、标准库实现缺口和可能由编译器隐式生成的特殊成员。只有真实查找缺失使测试失败；实现缺口必须能由当前头文件中的条件宏或声明缺失证实，不能作为隐藏 Parser 缺陷的白名单。
3. 名称比较穿透 `__cxx11`、`__1`、`__fs` 等 inline namespace，并核对实现用于转换运算符目标类型的实际 typedef/alias。继承成员只用于确认该名称在类中可查找，不把每个基类成员机械复制成派生类的新声明。
4. 对同一批完整名称调用 `evalExpression()`，经过真实 eval 路径，汇总求值失败项。析构器、operator 和未提供模板实参的 alias 等声明不一定适合作为独立表达式，因此 eval 失败用于后续语义分析，不与声明存在性混为同一断言。
5. 输出 eval 数量、成功率、总耗时、平均耗时及最慢项。性能输出不作为默认 CTest 的耗时阈值。

资源由 `test-resources` 依赖复制。默认配置不编译该 Suite，因此日常 Release/Debug 回归不增加标准库解析成本。

## 运行环境

直接运行任何标准库 Parser 测试或 profile 时必须设置 `LC_ALL=C`。测试通过编译器 `-E -v` 输出中的英文标记 `#include <...> search starts here:` 和 `End of search list.` 提取 include 路径；本地化输出会导致测试在 parse 前失败。CTest 已由 CMake 统一设置该环境变量，手工运行时需要显式设置：

```sh
LC_ALL=C QT_FORCE_STDERR_LOGGING=1 ./test-cppparser \
  --suite=TestCppParserStandardLibrary test_standard_library_parse_baseline
```

该要求同时适用于 `REDPANDA_PARSER_STDLIB_TEST` 和 `REDPANDA_PARSER_PUBLIC_API_BASELINE_TEST` 的 Suite。
