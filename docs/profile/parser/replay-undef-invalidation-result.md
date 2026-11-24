# Replay undef 恢复与失效清理结果

## 场景与环境

- Windows，Qt 5.15.18 Static Release，GCC 15.3，C++17。
- 输入为单头 UI 库的实现 TU：主文件定义 `AINO_IMPLEMENTATION` 后包含约 1.78 万行用户头，间接覆盖 libstdc++、Windows 和编译器 intrinsic 头。
- 在实际 Release IDE 中打开该文件，设置 `REDPANDA_PARSER_PROFILE=1`，对比同一进程中的两次 Parse。详细插桩数据只用于定位阶段成本，不作为无插桩正式性能基线。

## 根因与范围

首次失败来自前一边界 replay 后宏表与全量扫描不一致：ReplayRecord 的 `undefines` 未应用到 Session。补上该恢复后，又确认同一宏可能依次经历 define、undef、redefine，因此记录维护必须保证同名 define/undef 只保留最终状态。

修复仅涉及 ReplayRecord 的宏记录与恢复，不调整 Statement 所有权、StatementModel 索引或 Parser 查询接口。replay 校验失败后直接失效并重扫，不存在“replay 到一半撤销”的路径，因此删除旧的 Session 宏恢复函数及其空转发接口。

## 实测结果

| 第二次 Parse 阶段 | 修复前 | 修复后 |
| --- | ---: | ---: |
| 预处理 | 9875 ms | 157 ms |
| Tokenize | 327 ms | 26 ms |
| 声明分析 | 345 ms | 23 ms |
| Statement 构建 | 133 ms | 84 ms |
| declarations 合计 | 1250 ms | 111 ms |
| 本轮实际扫描 include 数 | 731 | 2 |

第一次 Parse 的预处理约为 790 ms，修复前后基本一致。修复前第二次预处理中，`windows.h` 和 `wincodec.h` 的失效清理分别约 7.7 秒和 1.7 秒；修复后稳定 reparse 不再进入这两项清理，因此本场景无需新增 Statement 索引或批量删除框架。真实头文件发生变化时的清理成本不属于本次稳定 replay 优化范围。

## 验证

- `TestCppParserInvalidation`：27/27，通过。
- `TestCppPreprocessorInclude`：72/72，通过。
- 最小场景同时覆盖最终 undef、undef 后重定义、后续边界宏校验、Statement 身份与数量稳定，以及 scan/replay 计数。
- 重复 Parse 的触发来源作为独立问题处理，不把“仍需两次 Parse”与本次 replay 失效问题混为一谈。
