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

`REDPANDA_PARSER_PUBLIC_API_BASELINE_TEST` 默认 `OFF`，仅在 Parser 改动或 profile 时开启。设计、过滤规则和覆盖边界见 `docs/design/parser/cpp-public-api-baseline.md`。
