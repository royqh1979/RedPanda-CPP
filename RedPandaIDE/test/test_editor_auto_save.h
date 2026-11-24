#ifndef TEST_EDITOR_AUTO_SAVE_H
#define TEST_EDITOR_AUTO_SAVE_H

#include "test_editor_base.h"
#include <QSplitter>
#include <QTabWidget>
#include <QDateTime>

class EditorManager;

class TestEditorManagerAutoSave : public TestEditorBase
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void test_overwrite_current_file();
    void test_current_and_all_opened_targets();
    void test_project_target();
    void test_timestamp_strategies_data();
    void test_timestamp_strategies();
    void test_timer_and_disabled_states();
    void test_can_auto_save_false();
    void test_save_error_signal();
    void test_opened_editor_content_version();

private:
    Editor *addEditor(const QString &filename, const QString &initial,
                      QTabWidget *page, bool inProject = false);
    void replaceContent(Editor *editor, const QString &content);
    QString readFile(const QString &filename) const;

    QTabWidget mLeftTabs;
    QTabWidget mRightTabs;
    QSplitter mSplitter;
    QWidget mPanel;
    EditorManager *mManager = nullptr;
    bool mAllowed = true;
    bool mHasProject = false;
    QDateTime mNow;
};

#endif // TEST_EDITOR_AUTO_SAVE_H
