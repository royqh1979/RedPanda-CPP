#ifndef FILEPROPERTIESDIALOG_H
#define FILEPROPERTIESDIALOG_H

#include <QDialog>

namespace Ui {
class FilePropertiesDialog;
}

class FilePropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilePropertiesDialog(QWidget *parent = nullptr);
    ~FilePropertiesDialog();

private:
    Ui::FilePropertiesDialog *ui;
};

#endif // FILEPROPERTIESDIALOG_H
