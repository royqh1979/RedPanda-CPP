#ifndef TEST_EDITOR_MANAGER_REPARSE_H
#define TEST_EDITOR_MANAGER_REPARSE_H

#include "test_editor_base.h"
#include "src/parser/cppparser.h"
#include <QSplitter>
#include <QTabWidget>

class CodeCompletionSettings;
class EditorManager;
class QSignalSpy;

class TestEditorManagerReparse : public TestEditorBase
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void test_shared_parser_switches_editors();
    void test_split_panels_use_their_own_parsers();
    void test_batch_update_reparses_only_final_editor();

private:
    Editor *addEditor(const QString &fileName, const QStringList &content,
                      QTabWidget *page, const PCppParser &parser);
    void showAndWait(Editor *editor, const PCppParser &parser,
                     QSignalSpy &finished, int expectedCount, const QString &symbol);

    QTabWidget mLeftTabs;
    QTabWidget mRightTabs;
    QSplitter mSplitter;
    QWidget mPanel;
    EditorManager *mManager = nullptr;
    CodeCompletionSettings *mCompletionSettings = nullptr;
};

#endif // TEST_EDITOR_MANAGER_REPARSE_H
