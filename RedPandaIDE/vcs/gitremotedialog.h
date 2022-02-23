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

private slots:
    void updateIcons();
    void onRemotesSelectionChanged();
    void on_btnAdd_clicked();

    void on_btnRemove_clicked();
    void refresh();
private:
    Ui::GitRemoteDialog *ui;
    QString mFolder;
};

#endif // GITREMOTEDIALOG_H
