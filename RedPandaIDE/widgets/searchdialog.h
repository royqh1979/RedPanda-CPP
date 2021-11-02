#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include "../qsynedit/SynEdit.h"
#include "../utils.h"

namespace Ui {
class SearchDialog;
}

struct SearchResultTreeItem;
class QTabBar;
class Editor;
class SearchDialog : public QDialog
{
    Q_OBJECT

    enum class SearchAction {
        Find,
        FindFiles,
        Replace,
        ReplaceFiles
    };

public:
    explicit SearchDialog(QWidget *parent = nullptr);
    ~SearchDialog();
    void find(const QString& text);
    void findNext();
    void findPrevious();
    void findInFiles(const QString& text);
    void findInFiles(const QString& keyword, SearchFileScope scope, SynSearchOptions options);
    void replace(const QString& sFind, const QString& sReplace);
    PSynSearchBase searchEngine() const;

    QTabBar *tabBar() const;

private slots:
    void onTabChanged();
   void on_cbFind_currentTextChanged(const QString &arg1);

   void on_btnCancel_clicked();

   void on_btnExecute_clicked();
private:
   int execute(SynEdit* editor, const QString& sSearch,
               const QString& sReplace, SynSearchMathedProc matchCallback = nullptr);
   std::shared_ptr<SearchResultTreeItem> batchFindInEditor(SynEdit * editor,const QString& filename, const QString& keyword);
private:
    Ui::SearchDialog *ui;
    QTabBar *mTabBar;
    SynSearchOptions mSearchOptions;
    PSynSearchBase mSearchEngine;
    PSynSearchBase mBasicSearchEngine;
    PSynSearchBase mRegexSearchEngine;
};

#endif // SEARCHDIALOG_H
