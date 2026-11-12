# 跨边界 Replay 场景与正确性验收

> 本文是跨边界 replay 的测试依据。测试名为计划名称，实现时保持场景与测试一一对应。
> 每个场景除列出的状态断言外，都必须与关闭 replay 的全量扫描结果对比，并检查 scan/replay/skip/invalidate 计数。

## 1. 边界判定

### B1 用户根文件包含用户头

- 操作：根文件和直接头均属于项目。
- 预期：不建立系统边界记录，不追加跨边界前缀；用户→用户包含按实际文本扫描。
- 测试：`test_user_to_user_include_has_no_boundary_record`。

### B2 用户文件包含系统头

- 操作：项目 `.cpp` 包含编译器 include 路径中的头文件。
- 预期：建立跨边界记录，记录进入时用户宏和空前缀，生效后追加该头路径。
- 测试：`test_user_to_system_creates_boundary_record`。

### B3 系统子树内部包含

- 操作：跨边界头 A 内部包含系统头 B。
- 预期：B 不建立独立 replay 记录，不追加跨边界前缀；B 的 statement 以 A 为跨边界祖先，实际文件仍为 B。
- 测试：`test_system_subtree_belongs_to_boundary_ancestor`。

### B4 非项目 include 路径文件

- 操作：包含一个位于项目 include 路径、但不在项目文件列表中的头。
- 预期：按边界内文件处理。
- 测试：`test_nonproject_include_path_file_is_inside_boundary`。

### B5 直接打开系统头

- 操作：系统头作为 preprocess 根文件。
- 预期：视为从边界外进入；建立记录并追加前缀。
- 测试：`test_system_header_root_is_boundary_entry`。

### B6 不应出现的反向包含

- 操作：构造边界内文件包含项目文件的资源。
- 预期：产生明确诊断；测试记录处理规则，不允许 statement 被静默归入错误上下文。
- 测试：`test_system_to_user_include_is_diagnosed`。

## 2. 用户宏校验

### M1 用户宏与内容均不变

- 操作：同一用户头在两个根文件中以相同宏环境生效。
- 预期：第二次 replay，不扫描；define 和 statement 与 oracle 一致。
- 测试：`test_replay_same_user_macros`。

### M2 包含前定义不同

- 操作：根 A 在 include 前 `#define MODE 1`，根 B 定义为 2。
- 预期：第二次校验失败，旧产物失效并重扫；B 得到 MODE=2 对应内容。
- 测试：`test_user_macro_change_forces_rescan`。

### M3 定义与未定义不同

- 操作：根 A 定义 FEATURE，根 B 不定义。
- 预期：未定义与定义是不同状态，必须重扫。
- 测试：`test_user_macro_undefined_state_forces_rescan`。

### M4 函数式宏签名变化

- 操作：两次包含前分别定义同名但参数或替换列表不同的函数式宏。
- 预期：签名不同，重扫。
- 测试：`test_user_function_macro_signature_forces_rescan`。

### M5 hard define 变化

- 操作：修改编译器或编译参数。
- 预期：IDE reset parser，整批 Registry 按 reset 规则重建；不靠逐文件 replay 校验处理。
- 测试：`test_hard_define_change_resets_parser`。

### M6 头内部自定义宏

- 操作：头文件在条件前后自行 define/undef 同名特性宏。
- 预期：进入时用户宏相同即可 replay；内部自定义过程由扫描结果恢复，不要求外部再次提供。
- 测试：`test_header_internal_define_undef_replays`。

### M7 间接用户头改变宏

- 操作：根文件先包含用户头 U，再进入系统头 S；修改 U 改变 S 进入时可见的宏。
- 预期：U 的内容变化使相关记录失效；S 按新宏环境重扫。
- 测试：`test_indirect_user_header_macro_change_forces_rescan`。

## 3. 有序跨边界前缀

### P1 相同顺序且 replay 生效

- 操作：首轮扫描 A、B；第二轮以相同顺序包含 A、B。
- 预期：A replay 后前缀包含 A，B 的前缀校验因此仍能匹配；两者均不扫描。
- 测试：`test_replayed_entries_extend_prefix`。

### P2 include 顺序反转

- 操作：根 A 按 sys1、sys2 包含，根 B 按 sys2、sys1 包含。
- 预期：B 中前缀不匹配的记录重扫；最终公开符号与全量扫描一致。
- 测试：`test_include_order_change_forces_rescan`。

### P3 前缀相同但中间系统子树不同

- 操作：保持边界外宏和跨边界顺序不变，修改某跨边界头的系统子树文件。
- 预期：mtime 闭包失效相关记录；不能只比较前缀而 replay。
- 测试：`test_subtree_mtime_invalidates_matching_prefix`。

### P4 系统内部包含不追加前缀

- 操作：A 内部包含 B，边界外随后直接包含 B。
- 预期：A 内部的 B 不追加；边界外进入 B 时按其首次生效位置计算前缀。
- 测试：`test_system_internal_include_does_not_extend_prefix`。

### P5 中间无保护模板重复生效

- 操作：同一系统头中多次包含无保护 `.tpl`，每次位于不同命名空间或宏环境。
- 预期：每次实际包含均按当前上下文处理；statement 实际文件为 `.tpl`，祖先为该系统头，不重复也不覆盖。
- 测试：`test_repeated_template_include_preserves_each_context`。

## 4. Include 保护与跳过

### G1 include guard 第二次包含

- 操作：同一作用域两次包含 guard 头。
- 预期：第二次为保护 no-op，不 replay 内容、不追加前缀。
- 测试：`test_guarded_reinclude_is_noop`。

### G2 `#pragma once` 第二次包含

- 操作：同一作用域两次包含 pragma once 头。
- 预期：与 G1 相同。
- 测试：`test_pragma_once_reinclude_is_noop`。

### G3 上轮跳过，本轮首次生效

- 操作：上轮 H 已在系统子树中扫描，跨界包含时因保护跳过；新一轮直接跨边界包含 H。
- 预期：上轮未因 no-op 追加前缀；本轮必须校验/扫描使 H 的记录在新边界生效，不能误用旧前缀。
- 测试：`test_guarded_noop_then_first_boundary_entry`。

### G4 无保护文件重复包含

- 操作：同一作用域重复包含无保护头。
- 预期：按编译器语义再次进入；内容、define/undef 和 statement 数量正确。
- 测试：`test_unguarded_reinclude_reenters`。

### G5 guard 的 `#endif` 不在文件尾

- 操作：`#ifndef` 对应的 `#endif` 后仍有声明。
- 预期：不把该文件误判为整体 include guard；尾部内容在重包含时仍需处理。
- 测试：`test_guard_must_cover_whole_file`。

## 5. 文件与编辑器变化

### F1 磁盘 mtime/size 变化

- 操作：首次扫描后外部修改头文件。
- 预期：该文件及依赖它的记录级联失效；新内容被扫描。
- 测试：`test_disk_stamp_change_invalidates_dependents`。

### F2 touch 但内容不变

- 操作：只更新 mtime。
- 预期：允许保守重扫，结果与 oracle 一致；不得使用旧文件内容。
- 测试：`test_touch_conservatively_invalidates`。

### F3 编辑器未保存修改

- 操作：通过 Editor 内容回调改变头文件，磁盘不变。
- 预期：Editor 失效路径清理相关记录；使用编辑器内容，不读旧磁盘缓存。
- 测试：`test_editor_change_invalidates_without_disk_change`。

### F4 失效目标在当前 include 栈

- 操作：嵌套包含期间出现对当前栈文件的级联失效请求。
- 预期：当前扫描不删除自身记录；失效请求保留到扫描结束后执行，不产生空 fileInfo、过期内容或重复 statement。
- 测试：`test_active_include_stack_is_not_invalidated_mid_scan`。

### F5 有效 replay 不重建 Statement

- 操作：同一跨边界记录连续 replay 两次。
- 预期：StatementMap 复用已有对象，statement 数量不增加；define/undef 按本轮需要恢复。
- 测试：`test_replay_reuses_existing_statements`。

## 6. 循环与深度

### C1 保护文件形成环

- 操作：A↔B，二者有保护。
- 预期：环上重进为保护 no-op，不无限递归。
- 测试：`test_protected_include_cycle_stops`。

### C2 无保护文件形成环

- 操作：A↔B，无保护。
- 预期：按编译器语义重进，达到统一深度上限后停止；不死循环。
- 测试：`test_unguarded_include_cycle_depth_cap`。

### C3 无重复的深链

- 操作：include 栈中无重复文件，但总深度超过上限。
- 预期：也必须执行深度限制。
- 测试：`test_depth_cap_without_repeated_file`。

### C4 同栈重复包含但环境不同

- 操作：同一文件在栈中第二次出现，进入时宏环境或传递闭包不同。
- 预期：不按文件名复用第一次的临时状态；结果与编译器语义一致，并受深度上限约束。
- 测试：`test_stack_reentry_uses_current_environment`。

## 7. Statement 生命周期

### S1 两个边界头共享系统子树

- 操作：A.h、B.h 均包含内部头 S.h，根文件同时使用 A、B。
- 预期：statement 能区分祖先；A 失效时 B 所需符号仍保留，A 重扫后不重复。
- 测试：`test_invalidate_one_boundary_preserves_shared_symbols`。

### S2 两个边界头共享无保护模板

- 操作：A.h、B.h 分别包含 `C.tpl`。
- 预期：两份 statement 的实际文件均为 C.tpl，祖先分别为 A/B；失效和重扫均不产生重复或丢失。
- 测试：`test_shared_template_statement_ownership`。

### S3 declaration/definition 合并后失效

- 操作：声明位于跨边界头，定义位于用户文件；声明和定义分别记录自身的跨边界祖先，然后分别失效头和用户文件。
- 预期：失效定义时保留有效声明并清除定义位置、定义侧祖先和 definition 标记；失效声明时，若定义仍有效，则将定义提升为声明位置并使用定义侧归属；两侧都失效才删除 Statement。namespace 缓存同步。
- 测试：`test_statement_ownership_declaration_definition_invalidation`。

### S4 define 与 statement 同步失效

- 操作：边界校验失败后重扫。
- 预期：旧 define/undef 与 statement 同时删除；不能只清理其中一类。
- 测试：`test_boundary_invalidation_removes_defines_and_statements`。

### S5 标准库接口完整性

- 操作：扫描/重扫包含 C/C++ 标准库的 TU。
- 预期：所有公开类、方法、函数都能在 StatementMap 找到；重复扫描后数量不异常增长。
- 测试：`test_standard_library_public_api_complete`（独立 CMake 开关，默认关闭）。

## 8. 性能与缓存命中验收

### R1 `bits/stdc++.h` 稳定 reparse

- 操作：同一 TU 在内容和用户宏不变时连续 reparse。
- 预期：首轮扫描；后续 replay/保护跳过；`bits/stdc++.h` 扫描计数不增加。
- 测试：`test_bits_stdcpp_not_rescanned_on_stable_reparse`。

### R2 缓存未命中必须可见

- 操作：分别改变用户宏、include 前缀和 mtime。
- 预期：对应记录 scan/invalidate 计数增加；不能用结果碰巧相同冒充 replay 正确。
- 测试：`test_replay_miss_reasons_are_observable`。

### R3 有界缓存

- 操作：连续扫描超过容量的大量文件。
- 预期：LRU 不超过配置的字节/条目上限；淘汰后结果仍与 oracle 一致。
- 测试：`test_content_cache_is_bounded`。
