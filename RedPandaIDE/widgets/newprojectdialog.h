#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QTabBar>
#include "projecttemplate.h"

namespace Ui {
class NewProjectDialog;
}

class NewProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget *parent = nullptr);
    ~NewProjectDialog();
private:
    void addTemplate(const QString& filename);
    void readTemplateDir();
    void updateView();
private:
    Ui::NewProjectDialog *ui;
    QList<PProjectTemplate> mTemplates;
    QTabBar* mTemplatesTabBar;
};

#endif // NEWPROJECTDIALOG_H
