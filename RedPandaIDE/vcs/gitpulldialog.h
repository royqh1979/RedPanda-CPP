#ifndef GITPULLDIALOG_H
#define GITPULLDIALOG_H

#include <QDialog>

namespace Ui {
class GitPullDialog;
}

class GitPullDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GitPullDialog(QWidget *parent = nullptr);
    ~GitPullDialog();

private:
    Ui::GitPullDialog *ui;
};

#endif // GITPULLDIALOG_H
