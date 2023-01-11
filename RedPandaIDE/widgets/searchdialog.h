#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
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
    void findNext();
    void findPrevious();
private:
    void doSearch(bool backward);
private slots:
    void on_cbFind_currentTextChanged(const QString &arg1);

    void on_btnClose_clicked();

    void on_btnNext_clicked();


    void on_btnPrevious_clicked();

private:
    Ui::SearchDialog *ui;
    QSynedit::SearchOptions mSearchOptions;
    QSynedit::PSynSearchBase mBasicSearchEngine;
    QSynedit::PSynSearchBase mRegexSearchEngine;
};

#endif // SEARCHDIALOG_H
