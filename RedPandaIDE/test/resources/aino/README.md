# Aino Parser 回归资源

`untitled1.cpp` 定义 `AINO_IMPLEMENTATION` 并包含同目录的 `aino.hpp`，覆盖单头 UI 库实现单元及其实际使用的标准库、Windows 和编译器 intrinsic 头。

用途：

- 验证连续 Parse 的 ReplayRecord 宏恢复、Statement 数量稳定和系统头 replay。
- 验证函数实参 lambda 的形参 `ui`、局部变量 `col` 及成员导航。
- 需要真实工具链的专项 profile 可复用同一场景；详细性能测量默认使用独立 CMake 开关，不把该大资源放入默认单元测试的强制耗时路径。

文件按 UTF-8 保存。`aino.hpp` 是相对独立的外部库头文件，测试只使用其声明和预处理结果，不编译或运行该库。
