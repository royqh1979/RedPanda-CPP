# 本机开发环境模板

## 1. 使用方式

本模板只记录通用配置方法，不保存任何机器专有路径。复制到 `docs/env/local/AGENTS.local.md` 后填写本机实际值；该文件默认被 gitignore，不得提交。

必须另行阅读 `BUILD.md`；本模板只补充本机环境约定。

## 2. 需要填写的路径

- C/C++ 编译器目录：`<COMPILER_BIN>`
- Qt 前缀：`<QT_PREFIX>`
- 构建目录：`<BUILD_DIR>`（建议内存盘或 SSD 损耗较小的位置）
- clangd：`<CLANGD_EXE>`（可选）
- xmake：`<XMAKE_EXE>`（可选）
- GCC 标准库测试工具链：`<GCC_STDLIB_ROOT>`（可选）
- Clang/libc++ 测试工具链：`<CLANG_STDLIB_ROOT>`（可选）

## 3. 通用构建约定

- Release 和 Debug 使用独立构建目录。
- 构建产物、测试日志、profile CSV 和 `compile_commands.json` 不进入源码仓库。
- 使用内存盘时，重启后必须重新 configure，并重新建立 `compile_commands.json` 链接。
- 完整回归必须先 Release 后 Debug，均构建 `all-test-targets` 并运行 `ctest --output-on-failure`。
- Profile 使用独立 Release 构建，不能复用调试器中的计时。

## 4. 示例（替换占位符）

```sh
export PATH="<COMPILER_BIN>:<QT_PREFIX>/bin:$PATH"

cmake -S . -B <BUILD_DIR> \
  -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=<QT_PREFIX> \
  -DFORCE_QT5=ON

cmake --build <BUILD_DIR> --target all-test-targets --parallel
cd <BUILD_DIR> && ctest --output-on-failure
```

Debug 配置将 `CMAKE_BUILD_TYPE` 改为 `Debug`，并使用对应的 Debug Qt 和构建目录。

## 5. QSynEdit / Parser 可选项

- `QSYNEDIT_BUILD_PROFILE_TESTS=ON`：编译 QSynEdit profiling 用例，默认 OFF。
- `REDPANDA_PARSER_STDLIB_TEST=ON`：启用标准库 Parser 验收，需设置 `REDPANDA_PARSER_STDLIB_MINGW_ROOT`。
- `REDPANDA_PARSER_DECLARATION_PROFILE=ON`：启用声明级细分计时，默认 OFF。
- 其它选项以根目录 `CMakeLists.txt` 和 `BUILD.md` 为准。

## 6. clangd

构建时启用 `CMAKE_EXPORT_COMPILE_COMMANDS=ON`，再将构建目录中的 `compile_commands.json` 链接到源码根目录。构建目录被清理后需重新生成并建链。
