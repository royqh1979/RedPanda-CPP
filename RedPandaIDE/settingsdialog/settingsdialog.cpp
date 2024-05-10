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
#include "../mainwindow.h"
#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settingswidget.h"
#include "compilersetoptionwidget.h"
#include "compilerautolinkwidget.h"
#include "editorgeneralwidget.h"
#include "editorfontwidget.h"
#include "editorclipboardwidget.h"
#include "editorcolorschemewidget.h"
#include "editorcodecompletionwidget.h"
#include "editorsyntaxcheckwidget.h"
#include "editorsymbolcompletionwidget.h"
#include "editortooltipswidget.h"
#include "editorautosavewidget.h"
#include "editorsnippetwidget.h"
#include "editorcustomctypekeywords.h"
#include "editormiscwidget.h"
#include "environmentappearancewidget.h"
#include "environmentshortcutwidget.h"
#include "environmentfolderswidget.h"
#include "environmentperformancewidget.h"
#include "environmentprogramswidget.h"
#include "executorgeneralwidget.h"
#include "executorproblemsetwidget.h"
#include "debuggeneralwidget.h"
#include "formattergeneralwidget.h"
#include "formatterpathwidget.h"
#include "languageasmgenerationwidget.h"
#include "projectgeneralwidget.h"
#include "projectfileswidget.h"
#include "projectcompilerwidget.h"
#include "projectcompileparamaterswidget.h"
#include "projectdirectorieswidget.h"
#include "projectprecompilewidget.h"
#include "projectoutputwidget.h"
#include "projectmakefilewidget.h"
#include "projectdllhostwidget.h"
#include "toolsgeneralwidget.h"
#ifdef ENABLE_VCS
#include "toolsgitwidget.h"
#endif
#ifdef Q_OS_WIN
#include "environmentfileassociationwidget.h"
#include "projectversioninfowidget.h"
#endif
#include <QDebug>
#include <QMessageBox>
#include <QModelIndex>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    setWindowFlag(Qt::WindowContextHelpButtonHint,false);
    ui->setupUi(this);

    QItemSelectionModel *m=ui->widgetsView->selectionModel();
    ui->widgetsView->setModel(&model);
    delete m;

    connect(ui->widgetsView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &SettingsDialog::onWidgetsViewCurrentChanged);

    model.setHorizontalHeaderLabels(QStringList());

    ui->btnApply->setEnabled(false);

    mAppShouldQuit = false;
    resize(pSettings->ui().settingsDialogWidth(),pSettings->ui().settingsDialogHeight());

    QList<int> sizes = ui->splitter->sizes();
    int tabWidth = pSettings->ui().settingsDialogSplitterPos();
    int totalSize = sizes[0] + sizes[1];
    sizes[0] = tabWidth;
    sizes[1] = std::max(1,totalSize - sizes[0]);
    ui->splitter->setSizes(sizes);
}

SettingsDialog::~SettingsDialog()
{
    for (SettingsWidget* p:mSettingWidgets) {
        p->setParent(nullptr);
        delete p;
    }
    delete ui;
}

void SettingsDialog::addWidget(SettingsWidget *pWidget)
{
    pWidget->init();
    QList<QStandardItem*> items = model.findItems(pWidget->group());
    QStandardItem* pGroupItem;
    if (items.count() == 0 ) {
        pGroupItem = new QStandardItem(pWidget->group());
        pGroupItem->setData(-1, GetWidgetIndexRole);
        model.appendRow(pGroupItem);
    } else {
        pGroupItem = items[0];
    }
    mSettingWidgets.append(pWidget);
    QStandardItem* pWidgetItem = new QStandardItem(pWidget->name());
    pWidgetItem->setData(mSettingWidgets.count()-1, GetWidgetIndexRole);
    pGroupItem->appendRow(pWidgetItem);
    connect(pWidget, &SettingsWidget::settingsChanged,
            this , &SettingsDialog::widget_settings_changed);
}

void SettingsDialog::selectFirstWidget()
{
    ui->widgetsView->expandAll();
    //select the first widget of the first group
    auto groupIndex = ui->widgetsView->model()->index(0,0);
    auto widgetIndex = ui->widgetsView->model()->index(0,0, groupIndex);
    ui->widgetsView->selectionModel()->setCurrentIndex(
                widgetIndex,
                QItemSelectionModel::Select
                );
    showWidget(widgetIndex);
}

PSettingsDialog SettingsDialog::optionDialog()
{
    PSettingsDialog dialog = std::make_shared<SettingsDialog>();

    dialog->setWindowTitle(tr("Options"));

    SettingsWidget* widget;
    widget = new EnvironmentAppearanceWidget(tr("Appearance"),tr("Environment"));
    dialog->addWidget(widget);

#ifdef Q_OS_WIN
    widget = new EnvironmentFileAssociationWidget(tr("File Association"),tr("Environment"));
    dialog->addWidget(widget);
#endif

    widget = new EnvironmentShortcutWidget(tr("Shortcuts"),tr("Environment"));
    dialog->addWidget(widget);

    widget = new EnvironmentProgramsWidget(tr("Terminal"),tr("Environment"));
    dialog->addWidget(widget);

    widget = new EnvironmentPerformanceWidget(tr("Performance"),tr("Environment"));
    dialog->addWidget(widget);

    widget = new EnvironmentFoldersWidget(tr("Folders / Restore Default Settings"),tr("Environment"));
    connect((EnvironmentFoldersWidget*)widget,
            &EnvironmentFoldersWidget::shouldQuitApp,
            dialog.get(),
            &SettingsDialog::closeAndQuit);
    dialog->addWidget(widget);

    widget = new CompilerSetOptionWidget(tr("Compiler Set"),tr("Compiler"));
    dialog->addWidget(widget);

    widget = new CompilerAutolinkWidget(tr("Auto Link"),tr("Compiler"));
    dialog->addWidget(widget);

    widget = new EditorGeneralWidget(tr("General"),tr("Editor"));
    dialog->addWidget(widget);

    widget = new EditorFontWidget(tr("Font"),tr("Editor"));
    dialog->addWidget(widget);

    widget = new EditorClipboardWidget(tr("Copy & Export"),tr("Editor"));
    dialog->addWidget(widget);

    widget = new EditorColorSchemeWidget(tr("Color"),tr("Editor"));
    dialog->addWidget(widget);

    widget = new EditorCodeCompletionWidget(tr("Code Completion"),tr("Editor"));
    dialog->addWidget(widget);

    widget = new EditorSymbolCompletionWidget(tr("Symbol Completion"),tr("Editor"));
    dialog->addWidget(widget);

    widget = new EditorSnippetWidget(tr("Snippet"),tr("Editor"));
    dialog->addWidget(widget);

    widget = new EditorSyntaxCheckWidget(tr("Auto Syntax Checking"),tr("Editor"));
    dialog->addWidget(widget);

    widget = new EditorTooltipsWidget(tr("Tooltips"),tr("Editor"));
    dialog->addWidget(widget);

    widget = new EditorAutoSaveWidget(tr("Auto save"),tr("Editor"));
    dialog->addWidget(widget);

    widget = new EditorMiscWidget(tr("Misc"),tr("Editor"));
    dialog->addWidget(widget);

    widget = new EditorCustomCTypeKeywordsWidget(tr("Custom C/C++ Keywords"),tr("Languages"));
    dialog->addWidget(widget);

//    widget = new LanguageCFormatWidget(tr("C/C++ Format"),tr("Languages"));
//    dialog->addWidget(widget);
    widget = new LanguageAsmGenerationWidget(tr("ASM Generation"),tr("Languages"));
    dialog->addWidget(widget);

    widget = new ExecutorGeneralWidget(tr("General"),tr("Program Runner"));
    dialog->addWidget(widget);

    widget = new ExecutorProblemSetWidget(tr("Problem Set"),tr("Program Runner"));
    dialog->addWidget(widget);

    widget = new DebugGeneralWidget(tr("General"),tr("Debugger"));
    dialog->addWidget(widget);

    widget = new FormatterGeneralWidget(tr("General"),tr("Code Formatter"));
    dialog->addWidget(widget);

    widget = new FormatterPathWidget(tr("Program"),tr("Code Formatter"));
    dialog->addWidget(widget);

    widget = new ToolsGeneralWidget(tr("General"),tr("Tools"));
    dialog->addWidget(widget);

#ifdef ENABLE_VCS
    widget = new ToolsGitWidget(tr("Git"),tr("Tools"));
    dialog->addWidget(widget);
#endif

    dialog->selectFirstWidget();

    return dialog;
}

PSettingsDialog SettingsDialog::projectOptionDialog()
{
    PSettingsDialog dialog = std::make_shared<SettingsDialog>();


    bool isMicroControllerProject=false;
    std::shared_ptr<Project> project = pMainWindow->project();
#ifdef ENABLE_SDCC
    if (project)
        isMicroControllerProject=(project->options().type==ProjectType::MicroController);
#endif

    dialog->setWindowTitle(tr("Project Options"));

    SettingsWidget* widget = new ProjectGeneralWidget(tr("General"),tr("Project"));
    dialog->addWidget(widget);

    widget = new ProjectFilesWidget(tr("Files"),tr("Project"));
    dialog->addWidget(widget);

    widget = new ProjectCompilerWidget(tr("Compiler Set"),tr("Project"));
    dialog->addWidget(widget);

    widget = new ProjectCompileParamatersWidget(tr("Custom Compile options"),tr("Project"));
    dialog->addWidget(widget);

    widget = new ProjectDirectoriesWidget(tr("Directories"),tr("Project"));
    dialog->addWidget(widget);

    if (!isMicroControllerProject) {
        widget = new ProjectPreCompileWidget(tr("Precompiled Header"),tr("Project"));
        dialog->addWidget(widget);
    }

    widget = new ProjectMakefileWidget(tr("Makefile"),tr("Project"));
    dialog->addWidget(widget);

    widget = new ProjectOutputWidget(tr("Output"),tr("Project"));
    dialog->addWidget(widget);

    if (!isMicroControllerProject) {
        widget = new ProjectDLLHostWidget(tr("DLL host"),tr("Project"));
        dialog->addWidget(widget);
    }

#ifdef Q_OS_WIN
    if (!isMicroControllerProject) {
        widget = new ProjectVersionInfoWidget(tr("Version info"),tr("Project"));
        dialog->addWidget(widget);
    }
#endif

    dialog->selectFirstWidget();

    return dialog;
}

bool SettingsDialog::setCurrentWidget(const QString &widgetName, const QString &groupName)
{
    QList<QStandardItem*> items = model.findItems(groupName);
    if (items.isEmpty())
        return false;
    QStandardItem* pGroupItem = items[0];
    for (int i=0;i<pGroupItem->rowCount();i++) {
        QStandardItem* pWidgetItem = pGroupItem->child(i);
        if (pWidgetItem->text() == widgetName) {
            ui->widgetsView->setCurrentIndex(pWidgetItem->index());
            showWidget(pWidgetItem->index());
            return true;
        }
    }
    return false;
}

void SettingsDialog::widget_settings_changed(bool value)
{
    ui->btnApply->setEnabled(value);
}

void SettingsDialog::onWidgetsViewCurrentChanged(const QModelIndex &index, const QModelIndex &/*previous*/)
{
    showWidget(index);
}

void SettingsDialog::on_btnCancel_pressed()
{
    this->close();
}

void SettingsDialog::on_btnApply_pressed()
{
    saveCurrentPageSettings(false);
}

void SettingsDialog::on_btnOk_pressed()
{
    saveCurrentPageSettings(false);
    this->close();
}

void SettingsDialog::saveCurrentPageSettings(bool confirm)
{
    if (ui->scrollArea->widget()==ui->scrollAreaWidgetContents)
        return;
    SettingsWidget* pWidget = (SettingsWidget*) ui->scrollArea->widget();
    if (!pWidget->isSettingsChanged())
        return;
    if (confirm) {
        if (QMessageBox::warning(this,tr("Save Changes"),
               tr("There are changes in the settings, do you want to save them before swtich to other page?"),
               QMessageBox::Yes, QMessageBox::No)!=QMessageBox::Yes) {
            return;
        }
    }
    pWidget->save();
}

void SettingsDialog::closeEvent(QCloseEvent *event)
{
    pSettings->ui().setSettingsDialogWidth(width());
    pSettings->ui().setSettingsDialogHeight(height());

    QList<int> sizes = ui->splitter->sizes();
    pSettings->ui().setSettingsDialogSplitterPos(sizes[0]);

    QDialog::closeEvent(event);
}

bool SettingsDialog::appShouldQuit() const
{
    return mAppShouldQuit;
}

void SettingsDialog::closeAndQuit()
{
    mAppShouldQuit = true;
    close();
}

void SettingsDialog::showWidget(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    int i = index.data(GetWidgetIndexRole).toInt();
    if (i>=0) {
        saveCurrentPageSettings(true);
        SettingsWidget* pWidget = mSettingWidgets[i];
        if (ui->scrollArea->widget()!=nullptr) {
            QWidget* w = ui->scrollArea->takeWidget();
            w->setParent(nullptr);
        }
        ui->scrollArea->setWidget(pWidget);
        ui->lblWidgetCaption->setText(QString("%1 > %2").arg(pWidget->group()).arg(pWidget->name()));

        ui->btnApply->setEnabled(false);
    } else if (model.hasChildren(index)) {
        ui->widgetsView->expand(index);
        QModelIndex childIndex = this->model.index(0,0,index);
        emit ui->widgetsView->clicked(childIndex);
    }
}
