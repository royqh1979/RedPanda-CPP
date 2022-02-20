#ifndef INFOMESSAGEBOX_H
#define INFOMESSAGEBOX_H

#include <QDialog>

namespace Ui {
class InfoMessageBox;
}

class InfoMessageBox : public QDialog
{
    Q_OBJECT

public:
    explicit InfoMessageBox(QWidget *parent = nullptr);
    void setMessage(const QString message);
    ~InfoMessageBox();

private slots:
    void on_btnOk_clicked();

private:
    Ui::InfoMessageBox *ui;
};

#endif // INFOMESSAGEBOX_H
