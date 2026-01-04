#ifndef GITUSERCONFIGDIALOG_H
#define GITUSERCONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class GitUserConfigDialog;
}

class GitUserConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GitUserConfigDialog(const QString& folder, QWidget *parent = nullptr);
    ~GitUserConfigDialog();

private:
    Ui::GitUserConfigDialog *ui;
    QString mFolder;

private:
    void checkInfo();
private slots:
    void on_btnOk_clicked();
    void on_btnCancel_clicked();
    void on_txtUserName_textChanged(const QString &arg1);
    void on_txtUserEmail_textChanged(const QString &arg1);

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // GITUSERCONFIGDIALOG_H
