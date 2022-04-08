#ifndef NEWCLASSDIALOG_H
#define NEWCLASSDIALOG_H

#include <QDialog>

namespace Ui {
class NewClassDialog;
}

class NewClassDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewClassDialog(QWidget *parent = nullptr);
    ~NewClassDialog();

    QString className() const;
    QString baseClass() const;
    QString headerName() const;
    QString sourceName() const;
    QString path() const;
    void setPath(const QString& location);

private slots:
    void on_btnCancel_clicked();

    void on_btnCreate_clicked();

    void on_btnBrowsePath_clicked();

    void on_txtClassName_textChanged(const QString &arg1);

private:
    Ui::NewClassDialog *ui;

private:
    void onUpdateIcons();

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // NEWCLASSDIALOG_H
