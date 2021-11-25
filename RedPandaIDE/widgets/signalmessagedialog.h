#ifndef SIGNALMESSAGEDIALOG_H
#define SIGNALMESSAGEDIALOG_H

#include <QDialog>

namespace Ui {
class SignalMessageDialog;
}

class SignalMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SignalMessageDialog(QWidget *parent = nullptr);
    ~SignalMessageDialog();
    void setMessage(const QString& message);
    bool openCPUInfo();
    void setOpenCPUInfo(bool value);

private:
    Ui::SignalMessageDialog *ui;
};

#endif // SIGNALMESSAGEDIALOG_H
