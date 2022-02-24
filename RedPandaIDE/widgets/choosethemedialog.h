#ifndef CHOOSETHEMEDIALOG_H
#define CHOOSETHEMEDIALOG_H

#include <QDialog>

namespace Ui {
class ChooseThemeDialog;
}

class ChooseThemeDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Theme {
        Dark,
        Light
    };
    explicit ChooseThemeDialog(QWidget *parent = nullptr);
    ~ChooseThemeDialog();
    Theme theme();

private slots:
    void on_btnOk_clicked();

private:
    Ui::ChooseThemeDialog *ui;
};

#endif // CHOOSETHEMEDIALOG_H
