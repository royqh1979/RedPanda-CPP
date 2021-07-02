#ifndef CPUDIALOG_H
#define CPUDIALOG_H

#include <QDialog>

namespace Ui {
class CPUDialog;
}

class CPUDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CPUDialog(QWidget *parent = nullptr);
    ~CPUDialog();

private:
    Ui::CPUDialog *ui;
};

#endif // CPUDIALOG_H
