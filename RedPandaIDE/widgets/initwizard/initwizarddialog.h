#ifndef INITWIZARDDIALOG_H
#define INITWIZARDDIALOG_H

#include <QDialog>

namespace Ui {
class InitWizardDialog;
}

class InitWizardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InitWizardDialog(QWidget *parent = nullptr);
    ~InitWizardDialog();

private:
    Ui::InitWizardDialog *ui;
};

#endif // INITWIZARDDIALOG_H
