#ifndef NEWHEADERDIALOG_H
#define NEWHEADERDIALOG_H

#include <QDialog>

namespace Ui {
class NewHeaderDialog;
}

class NewHeaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewHeaderDialog(QWidget *parent = nullptr);
    ~NewHeaderDialog();
    QString headerName() const;
    QString path() const;
    void setPath(const QString& location);

private:
    Ui::NewHeaderDialog *ui;

private:
    void updateIcons();

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void on_btnCreate_clicked();
    void on_btnCancel_clicked();
    void on_btnBrowse_clicked();
};

#endif // NEWHEADERDIALOG_H
