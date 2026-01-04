#ifndef GITPUSHDIALOG_H
#define GITPUSHDIALOG_H

#include <QDialog>

namespace Ui {
class GitPushDialog;
}

class GitPushDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GitPushDialog(QWidget *parent = nullptr);
    ~GitPushDialog();

private:
    Ui::GitPushDialog *ui;
};

#endif // GITPUSHDIALOG_H
