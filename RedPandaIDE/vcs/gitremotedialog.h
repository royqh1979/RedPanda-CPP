#ifndef GITREMOTEDIALOG_H
#define GITREMOTEDIALOG_H

#include <QDialog>

namespace Ui {
class GitRemoteDialog;
}

class GitRemoteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GitRemoteDialog(const QString& folder, QWidget *parent = nullptr);
    ~GitRemoteDialog();
    QString chooseRemote();

private slots:
    void onUpdateIcons();
    void onRemotesSelectionChanged();
    void checkDetails();
    void refresh();
    void on_btnAdd_clicked();

    void on_btnRemove_clicked();
    void on_btnProcess_clicked();

    void on_txtName_textChanged(const QString &arg1);

    void on_txtURL_textChanged(const QString &arg1);


    void on_btnClose_clicked();

private:
    Ui::GitRemoteDialog *ui;
    QString mFolder;
    QStringList mRemotes;
    bool mChooseMode;
};

#endif // GITREMOTEDIALOG_H
