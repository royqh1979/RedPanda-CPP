#ifndef EDITORFONTDIALOG_H
#define EDITORFONTDIALOG_H

#include <QDialog>

namespace Ui {
class EditorFontDialog;
}

class EditorFontDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditorFontDialog(bool onlyMonospaceFont, QWidget *parent = nullptr);
    ~EditorFontDialog();

    QString fontFamily() const;
    void setFontFamily(const QString &fontFamily);

private slots:
    void on_chkMonoOnly_toggled(bool checked);
    void on_buttonBox_accepted();

private:
    Ui::EditorFontDialog *ui;
    QString mFontFamily;
};

#endif // EDITORFONTDIALOG_H
