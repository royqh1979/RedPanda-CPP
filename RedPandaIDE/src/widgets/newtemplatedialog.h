#ifndef NEWTEMPLATEDIALOG_H
#define NEWTEMPLATEDIALOG_H

#include <QDialog>
#include <QSet>

namespace Ui {
class NewTemplateDialog;
}

class NewTemplateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewTemplateDialog(QWidget *parent = nullptr);
    ~NewTemplateDialog();
    QString getName() const;
    QString getDescription() const;
    QString getCategory() const;
private slots:
    void on_btnCreate_clicked();

    void on_btnCancel_clicked();

    void on_txtName_textChanged(const QString &arg1);

    void on_cbCategory_currentTextChanged(const QString &arg1);

private:
    QStringList findCategories();
    void readTemplateCategory(const QString& filename, QSet<QString>& categories);
    void readTemplateCategoriesInDir(const QString& folderPath, QSet<QString>& categories);
    void updateCreateState();
private:
    Ui::NewTemplateDialog *ui;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // NEWTEMPLATEDIALOG_H
