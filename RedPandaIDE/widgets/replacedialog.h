#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>
#include <qsynedit/SearchBase.h>

namespace Ui {
class ReplaceDialog;
}

class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(QWidget *parent = nullptr);
    ~ReplaceDialog();
    void replace(const QString& text);
private:
    void doSearch(bool backward);
    void doReplace(bool replaceAll);
    void prepareOptions(bool backward);
private slots:
    void on_cbFind_currentTextChanged(const QString &arg1);

    void on_btnClose_clicked();

    void on_btnNext_clicked();

    void on_btnPrevious_clicked();
    void on_btnReplace_clicked();

    void on_btnReplaceAll_clicked();

private:
    Ui::ReplaceDialog *ui;
    QSynedit::SearchOptions mSearchOptions;
    QSynedit::PSynSearchBase mBasicSearchEngine;
    QSynedit::PSynSearchBase mRegexSearchEngine;
};

#endif // REPLACEDIALOG_H
