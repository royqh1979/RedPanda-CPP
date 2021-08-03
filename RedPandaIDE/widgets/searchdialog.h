#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include "../qsynedit/SearchBase.h"

namespace Ui {
class SearchDialog;
}

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
    void findInFiles(const QString& text);
    void replace(const QString& sFind, const QString& sReplace);
    void replaceInFiles(const QString& sFind, const QString& sReplace);
    PSynSearchBase searchEngine() const;

private slots:
    void onTabChanged();
   void on_cbFind_currentTextChanged(const QString &arg1);

   void on_btnCancel_clicked();

   void on_btnExecute_clicked();
private:
   int execute(Editor* editor, SearchAction actionType);
private:
    Ui::SearchDialog *ui;
    QTabBar *mTabBar;
    SynSearchOptions mSearchOptions;
    PSynSearchBase mSearchEngine;
    PSynSearchBase mBasicSearchEngine;
    PSynSearchBase mRegexSearchEngine;
};

#endif // SEARCHDIALOG_H
