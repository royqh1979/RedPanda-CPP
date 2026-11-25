#ifndef TEST_CPPPREPROCESSOR_INCLUDE_H
#define TEST_CPPPREPROCESSOR_INCLUDE_H
#include <QObject>
#include <QStringList>
#include <memory>
#include "src/parser/cpppreprocessor.h"

class TestCppPreprocessorInclude: public QObject
{
    Q_OBJECT
public:
    TestCppPreprocessorInclude(QObject *parent=nullptr);
private slots:
    void test_basic_include();
    // group E: directives & include
    void test_nested_include();
    void test_include_guard();
    void test_nested_include_guard_visibility();
    void test_nested_false_guard_does_not_block_outer_guard();
    void test_pragma_once();
    void test_no_guard();
    void test_guard_after_comment();
    void test_guard_after_define();
    void test_guard_after_include();
    void test_include_via_macro();
    void test_recursive_include();
    void test_unguarded_recursive_include_visibility();
    void test_include_nonexistent();
    void test_include_next();
    void test_undef_external();
    void test_ignored_directives();
    void test_include_order_quote();
    void test_include_order_angle();
    void test_include_in_false_branch();
    void test_has_include();
    void test_recursive_include_env_sensitive();
    void test_env_sensitive_included_twice();
    void test_depth_bounded_iteration();
    void test_cycle_depth_cap();
    // group F: incremental / registry behavior
    void test_reprocess_same_instance();
    void test_clear_registry();
    void test_reprocess_after_remove_replay_record();
    void test_hard_define_invalidation_rescans();
    void test_undef_invalidation_rescans();
    void test_on_get_file_stream();
    void test_unfiltered_file_stream_callback_exhausts_depth_cap();
    void test_session_file_cache_reuse_and_clear();
    // group G: replay correctness
    void test_replay_env_dependent();
    void test_replay_env_independent();
    void test_replay_config_macro_stable();
    void test_replay_unguarded_cross_reparse();
    void test_replay_external_modification();
    void test_subtree_mtime_invalidates_matching_prefix();
    void test_replay_event_counts();
    void test_replay_miss_reasons_are_observable();
    void test_metadata_only_open_is_not_counted_as_scan();
    void test_replay_same_user_macros();
    void test_user_macro_change_forces_rescan();
    void test_replayed_entries_extend_prefix();
    void test_include_order_change_forces_rescan();
    void test_invalidated_prefix_entry_invalidates_suffix_record();
    void test_guarded_noop_then_first_boundary_entry();
    void test_user_to_user_include_has_no_boundary_record();
    void test_system_subtree_belongs_to_boundary_ancestor();
    void test_file_info_and_replay_record_partition();
    void test_nonproject_include_path_file_is_inside_boundary();
    void test_system_header_root_is_boundary_entry();
    void test_guard_must_cover_whole_file();
    void test_pragma_once_different_environments();
    void test_user_macro_undefined_state_forces_rescan();
    void test_user_function_macro_signature_forces_rescan();
    void test_hard_define_change_resets_parser();
    void test_header_internal_define_undef_replays();
    void test_indirect_user_header_macro_change_forces_rescan();
    void test_system_internal_include_does_not_extend_prefix();
    void test_system_to_user_include_is_diagnosed();
    void test_touch_conservatively_invalidates();
    void test_editor_change_invalidates_without_disk_change();
    void test_depth_cap_without_repeated_file();
    void test_active_include_stack_is_not_invalidated_mid_scan();
    void test_stack_reentry_uses_current_environment();
    void test_user_to_system_creates_boundary_record();
    void test_protected_include_cycle_stops();
    void test_replayed_subtree_marks_pragma_once_file_processed();
private:
    // Absolute path of resources/preprocessor/<name>
    static QString scenarioPath(const QString& name);
    // Result lines with the scenario dir replaced by "<DIR>"
    static QStringList normalizedResult(const CppPreprocessor::Session &session, const QString& scenarioDir);
    static QStringList readGolden(const QString& fileName);
    // Preprocess <dir>/<mainFile> and return the normalized result lines.
    static QStringList runScenario(CppPreprocessor& p, CppPreprocessor::Session &session,
                                   const QString& dir, const QString& mainFile);
};

#endif // TEST_CPPPREPROCESSOR_INCLUDE_H
