# bits/stdc++.h 完整解析性能基线

## 1. 场景与方法

- 对照提交：`48119372 Fix std::function evaluation`。
- 工具链：GCC 15.3 + libstdc++，C++17，Qt 5 Release；直接运行 `test-cppparser`，不使用 gdb。
- 测试入口：`TestCppParserStandardLibrary::test_standard_library_parse_baseline`。
- 输入主文件包含 `<bits/stdc++.h>`，并声明、使用 `std::vector<std::string>`。
- 正式基线为连续三次完整解析；`completeParseMs` 不包含测试初始化和清理。

```sh
./test-cppparser --suite=TestCppParserStandardLibrary \
  test_standard_library_parse_baseline
```

另设 `REDPANDA_PARSER_PROFILE=1` 采集一次详细调用结构。详细插桩会明显放大 tokenize 等阶段耗时，不能与正式基线直接比较。

## 2. 无插桩基线

| 次数 | Preprocess | Tokenize | Parse declarations | Build statements | Declaration 合计 | 完整解析 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 308 | 99 | 268 | 88 | 557 | 967 |
| 2 | 301 | 101 | 276 | 92 | 568 | 973 |
| 3 | 299 | 100 | 273 | 93 | 568 | 969 |
| **中位数** | **301** | **100** | **273** | **92** | **568** | **969** |

单位均为 ms。声明合计不包含预处理和 tokenize；完整解析还包含其它工作，因此不等于表中各阶段之和。

文档中 `20925702` 的旧基线为完整解析 930 ms、preprocess 301 ms、tokenize 100 ms、声明解析 245 ms、statement 构建 88 ms。当前中位数相对旧值增加 39 ms（约 4.2%），其中声明解析增加 28 ms、statement 构建增加 4 ms。两个提交之间包含累计功能变化，该差异不能直接归因于 `48119372`；精确回归分析需要另行做相邻提交 A/B 构建。

## 3. 头文件耗时

三次 `PARSE_FILE_TIMING` 的中位数如下。该值是单文件 inclusive 时间，包含其子阶段，不能跨文件求和。

| 文件 | 中位数（ms） | 三次原始值（ms） |
| --- | ---: | --- |
| `bits/hashtable.h` | 55 | 55、58、54 |
| `bits/stl_algobase.h` | 35 | 38、30、35 |
| `bits/hashtable_policy.h` | 22 | 22、19、22 |
| `bits/shared_ptr_base.h` | 17 | 17、19、17 |
| `bits/random.h` | 13 | 13、14、13 |
| `sstream` | 13 | 12、13、13 |
| `bits/random.tcc` | 12 | 11、12、12 |
| `bits/stl_tempbuf.h` | 11.5 | 11、12 |
| `bits/stl_vector.h` | 10 | 9、12、10 |
| `bits/stl_bvector.h` | 10 | 9、11 |

`bits/std_function.h` 不在最高耗时文件中；本轮整体基线不能用来推断 `std::function` 专项修复的收益或代价。

## 4. 详细调用结构

一次 `REDPANDA_PARSER_PROFILE=1` 运行得到：

- 完整解析 1073 ms；preprocess 308 ms、tokenize 186 ms、声明解析 271 ms、statement 构建 103 ms。
- 预处理：`openInclude` 73.2 ms、`handleInclude` 84.7 ms、`expandLine` 80.1 ms、`evaluateIf` 52.9 ms、`readFile` 37.6 ms。以上均包含嵌套工作，不能相加。
- Statement builder：`createStatementMs` 90.9 ms，其中 `autoTypeMs` 38.2 ms；`MERGE_DECLARATION_PROFILE` 31.9 ms。
- Resolver：`resolveType` 103.9 ms / 3897 次，`resolveSegments` 93.0 ms / 2849 次，`declarationLookup` 12.9 ms / 6297 次，`declarationBindings` 6.1 ms / 1870 次。
- Resolver self-time 较分散：`parseTypeNameSegments` 6.9 ms、`resolveSegments` 5.1 ms、`resolveType` 4.2 ms、`findDeclaration` 3.7 ms、`declarationBindings` 2.3 ms。
- Statement model：60753 次 add，共 10.6 ms。

当前没有单一占总耗时多数的 resolver self-time 尖峰。后续优化应先做相邻提交 A/B，区分累计功能成本与具体改动收益；不得用跳过必要类型解析来降低数字。
