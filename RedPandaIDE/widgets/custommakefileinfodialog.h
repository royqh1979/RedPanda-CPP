#ifndef CUSTOMMAKEFILEINFODIALOG_H
#define CUSTOMMAKEFILEINFODIALOG_H

#include <QDialog>

namespace Ui {
class CustomMakefileInfoDialog;
}

class CustomMakefileInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomMakefileInfoDialog(QWidget *parent = nullptr);
    ~CustomMakefileInfoDialog();

private slots:
    void on_pushButton_clicked();

private:
    Ui::CustomMakefileInfoDialog *ui;
};

#endif // CUSTOMMAKEFILEINFODIALOG_H
