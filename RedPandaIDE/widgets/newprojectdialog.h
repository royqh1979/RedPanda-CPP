/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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
    void onUpdateIcons();

    void on_btnOk_clicked();

    void on_btnCancel_clicked();

private:
    void addTemplate(const QString& filename);
    void readTemplateDirs();
    void readTemplateDir(const QString& folderPath);
    void rebuildTabs();

private:
    Ui::NewProjectDialog *ui;
    QList<PProjectTemplate> mTemplates;
    QTabBar* mTemplatesTabBar;
    QMap<QString,int> mCategories;
};

#endif // NEWPROJECTDIALOG_H
