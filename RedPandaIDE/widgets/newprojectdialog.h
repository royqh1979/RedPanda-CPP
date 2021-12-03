#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QListWidget>
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
    PProjectTemplate getTemplate();
    QString getLocation();
    QString getProjectName();
    bool useAsDefaultProjectDir();
    bool isCProject();
    bool isCppProject();
    bool makeDefaultLanguage();
private slots:
    void updateView();
    void updateProjectLocation();
    void on_lstTemplates_itemDoubleClicked(QListWidgetItem *item);

    void on_lstTemplates_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_btnBrowse_clicked();

private:
    void addTemplate(const QString& filename);
    void readTemplateDir();
    void rebuildTabs();
private:
    Ui::NewProjectDialog *ui;
    QList<PProjectTemplate> mTemplates;
    QTabBar* mTemplatesTabBar;
    QMap<QString,int> mCategories;
};

#endif // NEWPROJECTDIALOG_H
