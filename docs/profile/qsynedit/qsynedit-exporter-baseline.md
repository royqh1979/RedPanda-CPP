# QSynEdit HTML / RTF 导出基线

## 方法与样本

- 日期：2026-11-22；Windows，MinGW GCC 8.1.0，Qt 5.15.18 static Release。
- 使用 `test-qsynedit` 中由 `QSYNEDIT_BUILD_PROFILE_TESTS` 控制的 `TestHTMLExporter`、`TestRTFExporter`。
- 样本为 10,000/100,000 行 `int valueN = N; // note`，使用真实 Document 和 CppSyntaxer。
- 直接运行 Release，不使用调试器。时间为 Qt Test `QBENCHMARK` 的 WalltimeMilliseconds；字节数为 UTF-8 输出。

| 格式 | 行数 | 单次输出字节数 | 耗时 ms |
|---|---:|---:|---:|
| HTML | 10,000 | 618,361 | 22.25 |
| HTML | 100,000 | 6,378,361 | 222 |
| RTF | 10,000 | 358,016 | 21.5 |
| RTF | 100,000 | 3,778,016 | 224 |

RTF 在同一 exporter 上重复导出时，10,000 行样本在 358,016 和 358,041 字节之间变化；颜色表残留已另设正确性测试。HTML 的逐空格 `&nbsp;` 是主要尺寸来源之一。

运行：

```sh
cmake -S . -B R:/build-redpanda-qt5-release -DQSYNEDIT_BUILD_PROFILE_TESTS=ON
cmake --build R:/build-redpanda-qt5-release --target test-qsynedit -j 8
R:/build-redpanda-qt5-release/libs/qsynedit/test-qsynedit.exe --suite=TestHTMLExporter test_large_document_profile
R:/build-redpanda-qt5-release/libs/qsynedit/test-qsynedit.exe --suite=TestRTFExporter test_large_document_profile
```

本基线只覆盖内存生成，不包含磁盘写入、剪贴板和 QTextDocument 后续渲染。

## Editor 语义导出基线

- 日期：2026-11-23；同一 Release 环境。
- 样本：`libs/qsynedit/test/resources/miniaudio.h`，95,844 行；使用真实 Editor、CppParser 和 Editor 配色。
- HTML 通过 `createCopyMimeData()` 测量，RTF 通过 `exportAsRTF()` 写入 R: 临时目录。每种格式测量 3 次取平均；冷缓存每轮清空 `mIdCache`，热缓存保留前一轮结果。
- 原实现逐 token 调用 `onExportedFormatToken()`，不读取或写入 `mIdCache`，所以冷热结果没有直接收益，波动主要来自系统调度。

| 格式 | mIdCache | 输出字节数 | 平均耗时 ms |
|---|---|---:|---:|
| HTML | 冷 | 4,309,778 | 7,058.055 |
| RTF | 冷 | 4,712,466 | 9,061.742 |
| HTML | 热 | 4,309,778 | 9,269.845 |
| RTF | 热 | 4,712,466 | 9,123.051 |

运行：

```sh
cd R:/build-redpanda-qt5-release/RedPandaIDE
QT_QPA_PLATFORM=offscreen QT_QPA_FONTDIR=C:/Windows/Fonts \
  QT_FORCE_STDERR_LOGGING=1 REDPANDA_EXPORT_PROFILE=1 \
  REDPANDA_EXPORT_PROFILE_REPEAT=3 \
  ./test-editor.exe --suite=TestEditorGeneral test_semantic_export_profile
```

该测试默认跳过；环境变量仅用于显式 profile，不影响普通测试。
