#ifndef TEST_CPPREFACTOR_H
#define TEST_CPPREFACTOR_H

#include <QObject>
#include "src/refactor/cpprefactor.h"
#include "src/colorscheme.h"
#include "src/settings.h"

class TestCppRefactor : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void test_find_occurrences_current_file();
    void test_find_occurrences_same_line_distinct_declarations();
    void test_find_occurrences_project_unsaved_content();
    void test_same_name_isolation_across_scopes();
    void test_rename_rejects_project_external_declaration();
    void test_rename_same_class_defined_in_multiple_tus();
    void test_rename_class_constructor_destructor();
    void test_rename_overload_subset();
    void test_rename_overrides_both_directions();
    void test_rename_template_specializations();
    void test_rename_partial_template_specialization();
    void test_rename_visibility_and_using_declaration();
    void test_rename_namespace_alias_and_enum();
    void test_rename_local_lambda_and_structured_binding();
    void test_rename_undefined_local_conflict_and_scope();
    void test_rename_invalid_and_cross_file();
    void test_rename_prepare_apply_and_stale_content();
    void test_rename_undo_redo();

private:
    std::shared_ptr<Editor> makeEditor(const QString &fileName);

private:
    std::unique_ptr<Settings> mSettings;
    std::unique_ptr<ColorManager> mColorManager;
    std::shared_ptr<IconsManager> mIconsManager;
    PCppParser mParser;
    QHash<QString, QStringList> mFiles;
    QString mMainFile;
    std::shared_ptr<Project> mProject;
};

#endif // TEST_CPPREFACTOR_H
