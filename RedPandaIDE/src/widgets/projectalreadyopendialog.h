#ifndef PROJECTALREADOPENDIALOG_H
#define PROJECTALREADOPENDIALOG_H

#include <QDialog>

namespace Ui {
class ProjectAlreadyOpenDialog;
}



class ProjectAlreadyOpenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectAlreadyOpenDialog(QWidget *parent = nullptr);
    ~ProjectAlreadyOpenDialog();
    enum class OpenType {
        ThisWindow,
        NewWindow
    };

    OpenType openType() const;
    void setOpenType(OpenType newOpenType);

private slots:
    void on_btnCancel_clicked();

    void on_btnThisWindow_clicked();

    void on_btnNewWindow_clicked();

private:
    Ui::ProjectAlreadyOpenDialog *ui;
    OpenType mOpenType;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // PROJECTALREADOPENDIALOG_H
