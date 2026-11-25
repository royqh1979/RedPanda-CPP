# 标准库公开接口基准资源

`public-api-cpp17.tsv` 和 `public-api-c11.tsv` 是独立 cppreference 索引生成的公开声明名称基准，不是 Parser 当前输出的快照。对应 `-headers.txt` 指定测试 TU 显式包含的标准头，`.json` 记录来源和数量。

更新基准时：

1. 从 https://github.com/PeterFeicht/cppreference-doc/releases/tag/v20250209 下载并解压 `cppreference-doc-20250209.tar.xz`。
2. 在本仓库根目录运行（路径按实际位置调整）：

```sh
python RedPandaIDE/test/resources/stdlib/generate_public_api_baseline.py \
  --cppreference-doc R:/cppreference-doc \
  --output RedPandaIDE/test/resources/stdlib
```

生成器只依赖 Python 标准库。测试运行时不访问网络，也不需要该离线文档包。

编译和运行 GCC 15.3 基准：

```sh
cmake -S . -B R:/build-redpanda-public-api \
  -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=D:/qt/qt5-static-64 \
  -DFORCE_QT5=ON \
  -DREDPANDA_PARSER_PUBLIC_API_BASELINE_TEST=ON \
  -DREDPANDA_PARSER_STDLIB_MINGW_ROOT=D:/mingw
cmake --build R:/build-redpanda-public-api --target test-cppparser --parallel 8
R:/build-redpanda-public-api/RedPandaIDE/test-cppparser.exe \
  --suite=TestCppParserPublicApiBaseline
```

`REDPANDA_PARSER_PUBLIC_API_BASELINE_TEST` 默认 `OFF`，仅在 Parser 改动或 profile 时开启。设计、过滤规则和覆盖边界见 `docs/design/parser/testing/public-api-baseline.md`。

`compare_parse_baseline.py` 用于比较多次 `test_standard_library_parse_baseline` 日志，计算完整 Parse 的中位耗时和 Statement 变化，只依赖 Python 标准库：

```sh
python RedPandaIDE/test/resources/stdlib/compare_parse_baseline.py \
  R:/parse-baseline-timing-gcc.log R:/parse-merge-timing-gcc.log
```

第一个参数是基线日志，第二个是候选日志；每次运行测试时追加写入同一文件即可。测量场景和结果见 `docs/profile/parser/parse-time-declaration-merge.md`。

`analyze_resolver_trace.py` 用于分析 `REDPANDA_PARSER_TRACE=1` 生成的 Resolver 日志，统计各操作的 cache hit/miss、重复的 `resolveType` 文本和上下文、绑定帧及候选声明，只依赖 Python 标准库：

```sh
python RedPandaIDE/test/resources/stdlib/analyze_resolver_trace.py \
  R:/hotspot-trace-gcc-unordered.log R:/hotspot-trace-gcc-map.log --top 15
```

`--top` 控制每类输出的条目数。它用于区分相同语义键的重复解析与不同绑定环境下的必要解析，不把短类型名的重复直接当成缓存问题。
