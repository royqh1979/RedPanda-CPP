#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <QTabBar>
#include <qsynedit/searcher/baseseacher.h>

namespace Ui {
class SearchDialog;
}

class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchDialog(QWidget *parent = nullptr);
    ~SearchDialog();
    void find(const QString& text);
    void replace(const QString& text);
    void findNext();
    void findPrevious();
private:
    void doSearch(bool backward);
    void doReplace(bool replaceAll);
    QSynedit::SearchOptions prepareOptions(bool backward, bool &regex, bool &searchInSelection);
private slots:
    void onTabBarCurrentChanged(int currentIndex);

    void on_cbFind_currentTextChanged(const QString &arg1);

    void on_btnClose_clicked();

    void on_btnNext_clicked();

    void on_btnPrevious_clicked();

    void on_btnReplace_clicked();

    void on_btnReplaceAll_clicked();

    void setOriginVisibility();

private:
    Ui::SearchDialog *ui;
    QTabBar * mTabBar;
    int mSearchTabIdx;
    int mReplaceTabIdx;
    QSynedit::PSearcher mBasicSearchEngine;
    QSynedit::PSearcher mRegexSearchEngine;
    QStringList mSearchKeys;
    QStringList mReplaceKeys;
};

#endif // SEARCHDIALOG_H
