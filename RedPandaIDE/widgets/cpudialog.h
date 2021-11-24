#ifndef CPUDIALOG_H
#define CPUDIALOG_H

#include <QDialog>

namespace Ui {
class CPUDialog;
}

class CPUDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CPUDialog(QWidget *parent = nullptr);
    ~CPUDialog();
    void updateInfo();
public slots:
    void setDisassembly(const QString& file, const QString& funcName,const QStringList& lines);
signals:
    void closed();
private:
    void sendSyntaxCommand();
private:
    Ui::CPUDialog *ui;
    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void on_rdIntel_toggled(bool checked);
    void on_rdATT_toggled(bool checked);
    void on_chkBlendMode_stateChanged(int arg1);
};

#endif // CPUDIALOG_H
