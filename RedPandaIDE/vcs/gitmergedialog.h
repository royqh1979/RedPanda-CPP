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

private:
    Ui::GitMergeDialog *ui;
    GitManager *mManager;
    QString mFolder;
};

#endif // GITMERGEDIALOG_H
