#ifndef GITRESETDIALOG_H
#define GITRESETDIALOG_H

#include <QDialog>

namespace Ui {
class GitResetDialog;
}

class GitResetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GitResetDialog(const QString& folder, QWidget *parent = nullptr);
    ~GitResetDialog();
    int resetToCommit(const QString& commit);

private slots:
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

private:
    Ui::GitResetDialog *ui;
    QString mFolder;
};

#endif // GITRESETDIALOG_H
