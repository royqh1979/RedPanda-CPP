#ifndef GITFETCHDIALOG_H
#define GITFETCHDIALOG_H

#include <QDialog>

namespace Ui {
class GitFetchDialog;
}

class GitFetchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GitFetchDialog(QWidget *parent = nullptr);
    ~GitFetchDialog();

private:
    Ui::GitFetchDialog *ui;
};

#endif // GITFETCHDIALOG_H
