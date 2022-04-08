#ifndef NEWPROJECTUNITDIALOG_H
#define NEWPROJECTUNITDIALOG_H

#include <QDialog>

namespace Ui {
class NewProjectUnitDialog;
}

class NewProjectUnitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewProjectUnitDialog(QWidget *parent = nullptr);
    ~NewProjectUnitDialog();

    QString folder() const;
    void setFolder(const QString& folderName);

    QString filename() const;
    void setFilename(const QString& filename);

    bool suffix() const;
    void setSuffix(bool newSuffix);

private slots:
    void onUpdateIcons();
    void on_btnBrowse_clicked();

    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_txtFilename_textChanged(const QString &arg1);
    void on_txtFolder_textChanged(const QString &arg1);

private:
    void updateBtnOkStatus();
private:
    Ui::NewProjectUnitDialog *ui;
private:
    bool mSuffix;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // NEWPROJECTUNITDIALOG_H
