# 跨边界 Replay 场景与正确性验收

> 本文是跨边界 replay 的测试依据。测试名为计划名称，实现时保持场景与测试一一对应。
> 每个场景除列出的状态断言外，都必须与关闭 replay 的全量扫描结果对比，并检查 scan/replay/skip/invalidate 计数。

## 1. 边界判定

### B1 用户根文件包含用户头

- 操作：根文件和直接头均属于项目。
- 预期：不建立系统边界记录，不追加跨边界前缀；用户→用户包含按实际文本扫描。
- 判定依据：两侧均在边界外，不包含系统边界的生效条件，因此不产生跨边界记录或前缀。
- 测试：`test_user_to_user_include_has_no_boundary_record`。

### B2 用户文件包含系统头

- 操作：项目 `.cpp` 包含编译器 include 路径中的头文件。
- 预期：建立跨边界记录，记录进入时用户宏和空前缀，生效后追加该头路径。
- 判定依据：包含者在外、被包含者在内，该文件的内容由此次跨界进入产生，必须记录其环境和前缀。
- 测试：`test_user_to_system_creates_boundary_record`。

### B3 系统子树内部包含

- 操作：跨边界头 A 内部包含系统头 B。
- 预期：B 不建立独立 replay 记录，不追加跨边界前缀；B 的 statement 以 A 为跨边界祖先，实际文件仍为 B。
- 判定依据：内部包含不是独立跨界进入，内部 Statement 的生命周期随当前跨边界祖先。
- 测试：`test_system_subtree_belongs_to_boundary_ancestor`。

### B4 非项目 include 路径文件

- 操作：包含一个位于项目 include 路径、但不在项目文件列表中的头。
- 预期：按边界内文件处理。
- 判定依据：项目 include 路径只影响查找，不表示该文件属于项目；非项目文件仍按边界内处理。
- 测试：`test_nonproject_include_path_file_is_inside_boundary`。

### B5 直接打开系统头

- 操作：系统头作为 preprocess 根文件。
- 预期：构成一次跨边界进入，建立跨边界记录并追加前缀。
- 判定依据：直接打开系统根没有边界外包含者，根本身就是本次跨界进入产生的记录。
- 测试：`test_system_header_root_is_boundary_entry`。

### B6 不应出现的反向包含

- 操作：构造边界内文件包含项目文件的资源。
- 预期：产生明确诊断；测试记录处理规则，不允许 statement 被静默归入错误上下文。
- 判定依据：该方向违反边界归属前提，显式诊断可避免把用户 Statement 错归到系统祖先。
- 测试：`test_system_to_user_include_is_diagnosed`。

## 2. 用户宏校验

### M1 用户宏与内容均不变

- 操作：同一用户头在两个根文件中以相同宏环境生效。
- 预期：第二次 replay，不扫描；define 和 statement 与 oracle 一致。
- 判定依据：相同用户宏和相同前缀保证相同分支及内容，因此可以 replay，无需重扫。
- 测试：`test_replay_same_user_macros`。

### M2 包含前定义不同

- 操作：根 A 在 include 前 `#define MODE 1`，根 B 定义为 2。
- 预期：第二次校验失败，旧产物失效并重扫；B 得到 MODE=2 对应内容。
- 判定依据：进入宏不同会改变分支或宏展开，旧产物不能复用。
- 测试：`test_user_macro_change_forces_rescan`。

### M3 定义与未定义不同

- 操作：根 A 定义 FEATURE，根 B 不定义。
- 预期：未定义与定义是不同状态，必须重扫。
- 判定依据：#ifdef/#ifndef 会区分未定义与已定义，未定义不能省略为相同状态。
- 测试：`test_user_macro_undefined_state_forces_rescan`。

### M4 函数式宏签名变化

- 操作：两次包含前分别定义同名但参数或替换列表不同的函数式宏。
- 预期：签名不同，重扫。
- 判定依据：参数或替换列表变化会改变函数式宏展开，必须比较完整签名。
- 测试：`test_user_function_macro_signature_forces_rescan`。

### M5 hard define 变化

- 操作：修改编译器或编译参数。
- 预期：IDE reset parser，整批 Registry 按 reset 规则重建；不靠逐文件 replay 校验处理。
- 判定依据：hard define 是全局编译配置，修改后所有记录都可能受影响，逐文件校验不能代替 reset。
- 测试：`test_hard_define_change_resets_parser`。

### M6 头内部自定义宏

- 操作：头文件在条件前后自行 define/undef 同名特性宏。
- 预期：进入时用户宏相同即可 replay；内部自定义过程由扫描结果恢复，不要求外部再次提供。
- 判定依据：内部 define/undef 由该文件自身确定，相同进入环境下 replay 会恢复同一结果。
- 测试：`test_header_internal_define_undef_replays`。

### M7 间接用户头改变宏

- 操作：根文件先包含用户头 U，再进入系统头 S；修改 U 改变 S 进入时可见的宏。
- 预期：U 的内容变化使相关记录失效；S 按新宏环境重扫。
- 判定依据：用户头的内容变化会改变后续系统头的进入环境，现有失效链必须覆盖该间接依赖。
- 测试：`test_indirect_user_header_macro_change_forces_rescan`。

### M8 多 root 切换时旧分支 Statement 失效

- 操作：根 A 定义 MODE 后包含系统头 H，根 B 不定义 MODE 并包含同一 H；H 的两个分支声明不同符号。按 A→B→A 往返解析。
- 预期：每次切换后当前 root 只看见新分支符号；上一环境的旧分支 Statement 必须从 StatementMap 删除，而不能因预处理文本正确就继续残留。
- 判定依据：宏校验失败使 H 的旧扫描记录整体失效，Statement 与 define/undef 都是该记录产物，必须在重扫前同步清理。
- 测试：`test_root_switch_with_macro_change_removes_stale_statements`。

## 3. 有序跨边界前缀

### P1 相同顺序且 replay 生效

- 操作：首轮扫描 A、B；第二轮以相同顺序包含 A、B。
- 预期：A replay 后前缀包含 A，B 的前缀校验因此仍能匹配；两者均不扫描。
- 判定依据：replay 与扫描产生相同的本轮语义效果；若不追加前缀，后续记录的既定前缀就无法匹配。
- 测试：`test_replayed_entries_extend_prefix`。

### P2 include 顺序反转

- 操作：根 A 按 sys1、sys2 包含，根 B 按 sys2、sys1 包含。
- 预期：B 中前缀不匹配的记录重扫；最终公开符号与全量扫描一致。
- 判定依据：有序前缀不同意味着系统内容的生效顺序不同，即使宏表碰巧相同也不能复用旧记录。
- 测试：`test_include_order_change_forces_rescan`。

### P2a 顺序反转后的 Statement 清理

- 操作：sys1 定义供 sys2 判断的宏；根 A 按 sys1、sys2 包含，根 B 反向包含，sys2 在两种顺序下声明不同符号。按 A→B→A 往返解析。
- 预期：每次切换后 sys2 的旧分支 Statement 被删除，新分支可见；当前 root 不能通过 include 闭包继续找到旧符号。
- 判定依据：前缀校验失败不仅要求生成新预处理文本，也使旧记录的 Statement 失效。文本和 StatementMap 必须同时反映新顺序。
- 测试：`test_root_switch_with_include_order_change_removes_stale_statements`。

### P3 前缀相同但中间系统子树不同

- 操作：保持边界外宏和跨边界顺序不变，修改某跨边界头的系统子树文件。
- 预期：mtime 闭包失效相关记录；不能只比较前缀而 replay。
- 判定依据：内部文件变化会改变祖先内容，前缀匹配不能抵消文件闭包失效。
- 测试：`test_subtree_mtime_invalidates_matching_prefix`。

### P4 系统内部包含不追加前缀

- 操作：A 内部包含 B，边界外随后直接包含 B。
- 预期：A 内部的 B 不追加；边界外进入 B 时按其首次生效位置计算前缀。
- 判定依据：系统内部包含已由祖先记录覆盖，追加它会错误改变后续跨界记录的环境标识。
- 测试：`test_system_internal_include_does_not_extend_prefix`。

### P5 中间无保护模板重复生效

- 操作：同一系统头中多次包含无保护 `.tpl`，每次位于不同命名空间或宏环境。
- 预期：每次实际包含均按当前上下文处理；statement 实际文件为 `.tpl`，祖先为该系统头，不重复也不覆盖。
- 判定依据：无保护模板每次包含都受当前命名空间和宏影响，必须保留各次结果而非按文件名去重。
- 测试：`test_repeated_template_include_preserves_each_context`。

### P6 失效不影响前序跨界记录

- 操作：main.cpp 先包含 B、再包含 A；随后失效 A 并重扫 main。
- 判定依据：B 在受影响位置之前已经生效，A 的变化不会改变 B 的内容；B 可以 replay，其记录和 Statement 对象保持不变。
- 预期：A 失效并重扫，B replay。
- 测试：`test_invalidation_preserves_earlier_boundary_record`。

### P7 前序重扫使后续记录惰性失效

- 操作：main.cpp 先包含 A、再包含 B；失效 A 后、重扫前检查 B，然后重扫 main。
- 判定依据：失效 A 只需同时失效其包含者，不必提前删除 B。重扫 A 后，本轮前缀虽然仍与 B 记录的路径序列相同，但其中的 A 不是 replay，不能证明本轮系统状态与 B 原记录相同，因此 B 必须在自身入口处失效并重扫。
- 预期：失效 A 后 B 的记录仍有效且 invalidate 计数不增加；重扫时 A、B 均扫描，B 的 invalidate 计数此时才增加，结果与全量扫描一致。
- 测试：`test_scanned_prefix_entry_forces_lazy_suffix_rescan`。

### P8 后续记录未再次包含

- 操作：首轮 main.cpp 包含 A、B；随后 A 和 main 改变，main 不再包含 B。重扫 main 后，另一 TU 再按 A、B 的相同顺序包含。
- 判定依据：B 未在 main 的新一轮中生效，但其记录可能是其它 TU 的有效缓存，不能做全局 orphan GC。main 的新 include 闭包必须使旧 B 符号在该 TU 中不可见；另一 TU 中 A、B 均 replay 成功时，B 的宏环境和有序前缀重新得到完整校验，可以复用旧记录。
- 预期：main 中不能解析到 B 的符号；另一 TU 中 A、B 都 replay，B 不提前失效，符号可见且与全量扫描一致。
- 测试：`test_removed_boundary_include_remains_inactive_cached_record`。

## 4. Include 保护与跳过

### G1 include guard 第二次包含

- 操作：同一作用域两次包含 guard 头。
- 预期：第二次为保护 no-op，不 replay 内容、不追加前缀。
- 判定依据：完整 guard 的第二次包含由预处理器语义保证不产生新内容，因此无需 replay 或追加前缀。
- 测试：`test_include_guard`。

### G2 `#pragma once` 第二次包含

- 操作：同一作用域两次包含 pragma once 头。
- 预期：与 G1 相同。
- 判定依据：#pragma once 同样保证重复包含无新内容，规则与完整 guard 一致。
- 测试：`test_pragma_once`。

### G3 上轮跳过，本轮首次生效

- 操作：上轮 H 已在系统子树中扫描，跨界包含时因保护跳过；新一轮直接跨边界包含 H。
- 预期：上轮未因 no-op 追加前缀；本轮必须校验/扫描使 H 的记录在新边界生效，不能误用旧前缀。
- 判定依据：no-op 未产生跨界语义效果，不能留下前缀；后续首次真正生效必须建立对应记录。
- 测试：`test_guarded_noop_then_first_boundary_entry`。

### G4 无保护文件重复包含

- 操作：同一作用域重复包含无保护头。
- 预期：按编译器语义再次进入；内容、define/undef 和 statement 数量正确。
- 判定依据：无保护文件没有重复包含抑制语义，必须重新进入以产生每次包含的内容。
- 测试：`test_no_guard`。

### G5 guard 的 `#endif` 不在文件尾

- 操作：`#ifndef` 对应的 `#endif` 后仍有声明。
- 预期：不把该文件误判为整体 include guard；尾部内容在重包含时仍需处理。
- 判定依据：#endif 后的内容不受 guard 保护，重复包含时仍应产生，因此不能把整个文件判为 protected。
- 测试：`test_guard_must_cover_whole_file`。

### G6 `#pragma once` 在不同宏环境下首次分析

- 操作：根 A 定义 MODE 后包含 pragma 头；根 B 不定义 MODE 并包含同一头。
- 判定依据：pragma 只抑制同一轮中已生效文件的重复包含，不能保证不同宏环境的首次结果相同；B 的宏校验不同，必须重扫。
- 预期：A 得到 MODE 分支，B 得到另一分支；B 不复用 A 的 Statement。
- 测试：`test_pragma_once_different_environments`。

### G7 replay 子树中的 `#pragma once` 文件再次包含

- 操作：边界头 A 内部包含 pragma 头 H，根文件随后直接包含 H；稳定重扫使 A replay，然后再次直接包含 H。
- 预期：首轮 H 实际扫描，第二次包含为保护 no-op；重扫时 A replay 将 H 标记为本轮已逻辑生效，直接包含 H 仍为保护 no-op，不扫描也不追加跨边界前缀。
- 判定依据：replay 恢复了 A 的完整 include 闭包，其语义效果等同于扫描 A；`processed` 因而必须包含闭包内文件，而非仅包含本轮实际读取的文件。
- 测试：`test_replayed_subtree_marks_pragma_once_file_processed`。

## 5. 文件与编辑器变化

### F1 磁盘 mtime/size 变化

- 操作：首次扫描后外部修改头文件。
- 预期：该文件及依赖它的记录级联失效；新内容被扫描。
- 判定依据：磁盘变化使文件及其依赖记录的扫描结果不再可信，必须级联失效。
- 测试：`test_replay_external_modification`。

### F2 touch 但内容不变

- 操作：只更新 mtime。
- 预期：允许保守重扫，结果与 oracle 一致；不得使用旧文件内容。
- 判定依据：mtime 变化虽不一定改变内容，但无法据此证明未变，保守重扫不影响正确性。
- 测试：`test_touch_conservatively_invalidates`。

### F3 编辑器未保存修改

- 操作：通过 Editor 内容回调改变头文件，磁盘不变。
- 预期：Editor 失效路径清理相关记录；使用编辑器内容，不读旧磁盘缓存。
- 判定依据：编辑器内容优先于磁盘内容，磁盘 stamp 不变不能阻止编辑器失效。
- 测试：`test_editor_change_invalidates_without_disk_change`。

### F4 失效目标在当前 include 栈

- 操作：嵌套包含期间出现对当前栈文件的级联失效请求。
- 预期：当前扫描不删除自身记录；失效请求保留到扫描结束后执行，不产生空 fileInfo、过期内容或重复 statement。下一轮根文件重扫；若子头的用户宏和前缀未变，子头可以 replay，不要求产生扫描文本。
- 判定依据：当前栈仍被扫描器使用，立即删除会破坏状态；延后执行可兼顾当前完整性和后续正确性。根文件不是跨界前缀项，其重扫本身不阻止子头 replay。
- 测试：`test_active_include_stack_is_not_invalidated_mid_scan`。

### F5 有效 replay 不重建 Statement

- 操作：同一跨边界记录连续 replay 两次。
- 预期：StatementMap 复用已有对象，statement 数量不增加；define/undef 按本轮需要恢复。
- 判定依据：有效记录的 Statement 已在 StatementMap 中，重复创建只会产生重复定义。
- 测试：`test_replay_reuses_existing_statements`。

### F6 边界外间接头失效

- 操作：项目根 main 包含项目头 A，A 再包含项目头 B；修改并显式失效 B 后重扫 main。
- 预期：失效集合包含 B、A 和 main；B 的旧 Statement 删除，新 Statement 生效，A/main 的 Statement 正确恢复。
- 判定依据：边界外 include 不进入 replay 前缀，但传递 include 闭包仍决定内容依赖；叶子变化必须级联到全部包含者。
- 测试：`test_user_header_change_invalidates_dependent_roots`。

### F7 边界内共享头显式失效

- 操作：跨边界头 A、B 共享 pragma 内部头 H；修改并显式失效 H 后重扫 main。
- 预期：H 没有独立 Registry 记录，但失效通过 include 闭包到达 A、B 和 main；旧 Statement 删除，新 Statement 恢复且数量不增加。
- 判定依据：内部头没有独立记录不等于没有依赖关系；其生命周期由所有包含它的跨边界祖先记录共同决定。
- 测试：`test_shared_internal_header_change_invalidates_boundary_ancestors`。

### F8 边界内磁盘变化同步清理 Statement

- 操作：跨边界头 A 包含内部头 H；直接修改 H 的磁盘内容，不显式调用 Parser 失效接口，随后重扫 main。
- 预期：stamp 校验失效 A 及其依赖记录；H 的旧 Statement 在重扫前删除，新 Statement 可见，A 的 Statement 正确恢复。
- 判定依据：自动 stamp 失效与编辑器显式失效必须具有相同的 Statement 生命周期语义，不能只清 Preprocessor 的宏和 Registry 记录。
- 测试：`test_internal_header_disk_change_removes_stale_statements`。

## 6. 循环与深度

### C1 保护文件形成环

- 操作：A↔B，二者有保护。
- 预期：环上重进为保护 no-op，不无限递归。
- 判定依据：保护使环上重进成为 no-op，因此无需达到深度上限即可终止。
- 测试：`test_protected_include_cycle_stops`。

### C2 无保护文件形成环

- 操作：A↔B，无保护。
- 预期：按编译器语义重进，达到统一深度上限后停止；不死循环。
- 判定依据：无保护环没有自然终止条件，统一深度上限提供有限终止保证。
- 测试：`test_cycle_depth_cap`。

### C3 无重复的深链

- 操作：include 栈中无重复文件，但总深度超过上限。
- 预期：也必须执行深度限制。
- 判定依据：深度风险与文件名是否重复无关，所有调用都必须受同一上限约束。
- 测试：`test_depth_cap_without_repeated_file`。

### C4 同栈重复包含但环境不同

- 操作：同一文件在栈中第二次出现，进入时宏环境或传递闭包不同。
- 预期：不按文件名复用第一次的临时状态；结果与编译器语义一致，并受深度上限约束。
- 判定依据：同栈两次进入可能处于不同宏和闭包环境，复用第一次状态会改变分支语义。
- 测试：`test_stack_reentry_uses_current_environment`。

## 7. Statement 生命周期

### S1a 失效后重扫共享关系

- 初始状态：main.cpp 包含 A.h、B.h；A、B 均包含受 `#pragma once` 保护的内部头 inner.h。
- 操作：失效 A，然后重扫 main.cpp。
- 正确性依据：失效阶段只移除 A 及其包含者 main。重扫 A 使本轮已生效前缀不再全部来自 replay，因此 B 在自身入口处失效并重扫；inner 随 B 在新 Session 中重新生效，不依赖 A 留下的 Statement。
- 预期：失效 A 时 B 的记录保留；重扫后 A、B 和 inner 的符号均恢复且不重复。
- 测试：`test_shared_boundary_statement_invalidation_only`。

### S1b 修改后移除共享 include 并重扫

- 初始状态：与 S1a 相同。
- 操作：修改 A，使其不再包含 inner.h，然后失效 A 并重扫 main.cpp；main 和 B 的内容不变。
- 正确性依据：失效计算使用修改前的闭包，因此失效阶段移除 A 和 main，但不必立即移除 B。A 重扫不再产生 inner；由于 A 已重扫，B 不能 replay，而是在入口处清除旧产物并重扫，从而在新 Session 中进入 inner 并生成正确 Statement。无需保留或转移 A 的旧 Statement。
- 预期：A 的新内容生效，B 重扫后恢复 inner，inner 的公开符号存在且 StatementMap 不重复。
- 测试：`test_shared_boundary_statement_invalidation`。

### S2 两个边界头共享无保护模板

- 操作：A.h、B.h 分别包含 `C.tpl`。
- 预期：两份 statement 的实际文件均为 C.tpl，祖先分别为 A/B；失效和重扫均不产生重复或丢失。
- 判定依据：模板的实际文件与边界祖先共同决定归属，失效和重扫必须保留其它上下文的合法实例。
- 测试：`test_shared_template_statement_ownership`。

### S3 declaration/definition 合并后失效

- 操作：声明位于跨边界头，定义位于用户文件；声明和定义分别记录自身的跨边界祖先，然后分别失效头和用户文件。
- 预期：失效定义时保留有效声明并清除定义位置、定义侧祖先和 definition 标记；失效声明时，若定义仍有效，则将定义提升为声明位置并使用定义侧归属；两侧都失效才删除 Statement。namespace 缓存同步。
- 判定依据：声明与定义可来自不同记录，分别失效才能保留仍有效的一侧并维护跳转位置。
- 测试：`test_statement_ownership_declaration_definition_invalidation`。

### S4 define 与 statement 同步失效

- 操作：边界校验失败后重扫。
- 预期：旧 define/undef 与 statement 同时删除；不能只清理其中一类。
- 判定依据：define 和 Statement 都是同一记录的产物，只清一类会使重扫后的状态不一致。
- 测试：`test_boundary_invalidation_removes_defines_and_statements`。

### S5 系统子树继承的所属记录

- 操作：跨边界头包含内部 base.h，base 类位于 base.h，派生类位于跨边界头。
- 预期：继承处理、别名解析和继承覆盖均通过 Statement 的跨边界祖先找到 Registry 记录；不得按实际内部文件路径索取独立 fileInfo。继承成员可见，失效时 handledInheritance 状态随所属记录清理。
- 判定依据：边界内子树文件没有独立 Registry 记录，但其实际文件和跨边界祖先已经共同记录在 Statement 中；所属记录应由该祖先确定。
- 测试：`test_system_subtree_inheritance_uses_boundary_record`。

### S6 标准库接口完整性

- 操作：扫描/重扫包含 C/C++ 标准库的 TU。
- 预期：所有公开类、方法、函数都能在 StatementMap 找到；重复扫描后数量不异常增长。
- 判定依据：标准库验收直接检查公开接口集合，可发现局部 replay 中符号丢失或重复，而不仅是文本相同。
- 测试：`test_standard_library_public_api_complete`（独立 CMake 开关，默认关闭）。

## 8. 性能与缓存命中验收

### R1 `bits/stdc++.h` 稳定 reparse

- 操作：同一 TU 在内容和用户宏不变时连续 reparse。
- 预期：首轮扫描；后续 replay/保护跳过；`bits/stdc++.h` 扫描计数不增加。
- 判定依据：稳定宏、前缀和文件闭包保证其内容不变，重扫没有语义收益且违背性能目标。
- 测试：`test_bits_stdcpp_not_rescanned_on_stable_reparse`。

### R1a 多 root 往返包含 `bits/stdc++.h`

- 操作：A、B 都只额外声明各自的 root 符号，并以相同宏环境包含 `bits/stdc++.h`；按 A→B→A→B→A 解析。
- 预期：仅首轮扫描 `bits/stdc++.h`，之后每次切换都 replay；Statement 总数稳定，两个 root 各自符号正确保留。
- 判定依据：root 自身的 Statement 失效不会失效其包含的系统记录；相同用户宏和空前缀满足 replay 条件，往返切换不构成重扫理由。本场景要求切换时保留并共享 Parser；IDE 默认启用“编辑器隐藏时自动清空 Parser”时，Registry 被清空，切回后重扫是该设置的正常结果。
- 测试：`test_bits_stdcpp_not_rescanned_when_switching_roots`。使用仓库内的 `boundary_replay/include/bits/stdc++.h` 测试头，不依赖外部工具链。

### R2 缓存未命中必须可见

- 操作：分别改变用户宏、include 前缀和 mtime。
- 预期：对应记录 scan/invalidate 计数增加；不能用结果碰巧相同冒充 replay 正确。
- 判定依据：只有计数能区分真正 replay 与意外全扫，从而同时验证正确性和缓存机制。
- 测试：`test_replay_miss_reasons_are_observable`。

### R3 元数据探测不计为内容扫描

- 操作：以 `parseSystem=false` 预处理包含系统头的项目文件，用于 include 关系排序。
- 预期：系统头可以被打开以建立临时关系，但不读取其内容，scan 计数保持为零；随后的正式扫描计数从实际内容扫描开始。
- 判定依据：scan 计数用于证明 replay 避免真实扫描，若把元数据探测计入，就无法区分首扫和正式扫描，也会掩盖性能回归。
- 测试：`test_metadata_only_open_is_not_counted_as_scan`。

### R4 Session 文件内容缓存

- 操作：同一轮中重复扫描无保护文件，随后开始新一轮 preprocess。
- 预期：同一轮第二次扫描复用已处理的行表，不重复读取或处理文件；新一轮 Session 清空缓存，必须重新取得内容，结果与 oracle 一致。
- 判定依据：文件内容缓存只优化同一轮中的重复包含，生命周期随 Session 结束，不引入跨 TU 内容污染或全局 LRU 容量管理。
- 测试：`test_session_file_cache_reuse_and_clear`。
