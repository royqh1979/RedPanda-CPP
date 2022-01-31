#ifndef NEWHEADERDIALOG_H
#define NEWHEADERDIALOG_H

#include <QDialog>

namespace Ui {
class NewHeaderDialog;
}

class NewHeaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewHeaderDialog(QWidget *parent = nullptr);
    ~NewHeaderDialog();

private:
    Ui::NewHeaderDialog *ui;
};

#endif // NEWHEADERDIALOG_H
