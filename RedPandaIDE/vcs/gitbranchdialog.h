#ifndef GITBRANCHDIALOG_H
#define GITBRANCHDIALOG_H

#include <QDialog>

namespace Ui {
class GitBranchDialog;
}

class GitManager;
class GitBranchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GitBranchDialog(const QString& folder, QWidget *parent = nullptr);
    ~GitBranchDialog();

private slots:
    void on_btnCancel_clicked();

    void on_btnOk_clicked();

    void on_lstBranches_currentIndexChanged(int index);

    void on_chkCreate_stateChanged(int arg1);

private:
    Ui::GitBranchDialog *ui;
    GitManager *mManager;
    QString mFolder;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // GITBRANCHDIALOG_H
