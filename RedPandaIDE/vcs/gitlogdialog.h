#ifndef GITLOGDIALOG_H
#define GITLOGDIALOG_H

#include <QDialog>
#include <QAbstractTableModel>
#include <QMap>
#include "gitutils.h"

namespace Ui {
class GitLogDialog;
}

class GitLogModel: public QAbstractTableModel {
public:
    explicit GitLogModel(const QString& folder,QObject *parent = nullptr);

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    PGitCommitInfo commitInfo(const QModelIndex &index);
    const QString &folder() const;

private:
    QString mFolder;
    int mCount;
};

class GitLogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GitLogDialog(const QString& folder, QWidget *parent = nullptr);
    ~GitLogDialog();

private slots:
    void on_btnClose_clicked();
    void onLogsContextMenu(const QPoint &pos);

    void on_actionReset_triggered();

private:
    Ui::GitLogDialog *ui;
    GitLogModel mModel;
};

#endif // GITLOGDIALOG_H
