#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>

namespace Ui {
class SearchDialog;
}

class QTabBar;
class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchDialog(QWidget *parent = nullptr);
    ~SearchDialog();
    void find(const QString& text);
    void findInFiles(const QString& text);
    void replace(const QString& sFind, const QString& sReplace);
    void replaceInFiles(const QString& sFind, const QString& sReplace);
private slots:
   void onTabChanged();
   void on_cbFind_currentTextChanged(const QString &arg1);

   void on_btnCancel_clicked();

   void on_btnExecute_clicked();

private:
    Ui::SearchDialog *ui;
    QTabBar *mTabBar;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // SEARCHDIALOG_H
