#ifndef GITMERGEDIALOG_H
#define GITMERGEDIALOG_H

#include <QDialog>

namespace Ui {
class GitMergeDialog;
}

class GitManager;
class GitMergeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GitMergeDialog(const QString& folder, QWidget *parent = nullptr);
    ~GitMergeDialog();

private slots:
    void on_btnCancel_clicked();

    void on_btnOk_clicked();

    void on_cbBranch_currentIndexChanged(int index);

private:
    Ui::GitMergeDialog *ui;
    GitManager *mManager;
    QString mFolder;
    int mCurrentBranchIndex;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // GITMERGEDIALOG_H
