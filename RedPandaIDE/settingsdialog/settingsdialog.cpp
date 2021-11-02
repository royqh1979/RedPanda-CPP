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
#include "editormiscwidget.h"
#include "environmentappearencewidget.h"
#include "environmentshortcutwidget.h"
#include "environmentfileassociationwidget.h"
#include "environmentfolderswidget.h"
#include "executorgeneralwidget.h"
#include "executorproblemsetwidget.h"
#include "debuggeneralwidget.h"
#include "formattergeneralwidget.h"
#include "projectgeneralwidget.h"
#include "projectfileswidget.h"
#include "projectcompilerwidget.h"
#include "projectcompileparamaterswidget.h"
#include "projectdirectorieswidget.h"
#include "projectprecompilewidget.h"
#include "projectoutputwidget.h"
#include "projectmakefilewidget.h"
#include "projectversioninfowidget.h"
#include "projectdllhostwidget.h"
#include "toolsgeneralwidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QModelIndex>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    ui->widgetsView->setModel(&model);

    model.setHorizontalHeaderLabels(QStringList());

    ui->btnApply->setEnabled(false);

    mAppShouldQuit = false;

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
    on_widgetsView_clicked(widgetIndex);
}

PSettingsDialog SettingsDialog::optionDialog()
{
    PSettingsDialog dialog = std::make_shared<SettingsDialog>();

    SettingsWidget* widget = new EnvironmentAppearenceWidget(tr("Appearence"),tr("Environment"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EnvironmentFileAssociationWidget(tr("File Association"),tr("Environment"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EnvironmentShortcutWidget(tr("Shortcuts"),tr("Environment"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EnvironmentFoldersWidget(tr("Folders"),tr("Environment"));
    widget->init();
    dialog->addWidget(widget);

    connect((EnvironmentFoldersWidget*)widget,
            &EnvironmentFoldersWidget::shouldQuitApp,
            dialog.get(),
            &SettingsDialog::closeAndQuit,
            Qt::QueuedConnection);

    widget = new CompilerSetOptionWidget(tr("Compiler Set"),tr("Compiler"));
    widget->init();
    dialog->addWidget(widget);

    widget = new CompilerAutolinkWidget(tr("Auto Link"),tr("Compiler"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EditorGeneralWidget(tr("General"),tr("Editor"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EditorFontWidget(tr("Font"),tr("Editor"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EditorClipboardWidget(tr("Copy & Export"),tr("Editor"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EditorColorSchemeWidget(tr("Color"),tr("Editor"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EditorCodeCompletionWidget(tr("Code Completion"),tr("Editor"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EditorSymbolCompletionWidget(tr("Symbol Completion"),tr("Editor"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EditorSnippetWidget(tr("Snippet"),tr("Editor"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EditorSyntaxCheckWidget(tr("Auto Syntax Checking"),tr("Editor"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EditorTooltipsWidget(tr("Tooltips"),tr("Editor"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EditorAutoSaveWidget(tr("Auto save"),tr("Editor"));
    widget->init();
    dialog->addWidget(widget);

    widget = new EditorMiscWidget(tr("Misc"),tr("Editor"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ExecutorGeneralWidget(tr("General"),tr("Program Runner"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ExecutorProblemSetWidget(tr("Problem Set"),tr("Program Runner"));
    widget->init();
    dialog->addWidget(widget);

    widget = new DebugGeneralWidget(tr("General"),tr("Debugger"));
    widget->init();
    dialog->addWidget(widget);

    widget = new FormatterGeneralWidget(tr("General"),tr("Code Formatter"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ToolsGeneralWidget(tr("General"),tr("Tools"));
    widget->init();
    dialog->addWidget(widget);

    dialog->selectFirstWidget();

    return dialog;
}

PSettingsDialog SettingsDialog::projectOptionDialog()
{
    PSettingsDialog dialog = std::make_shared<SettingsDialog>();

    SettingsWidget* widget = new ProjectGeneralWidget(tr("General"),tr("Project"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ProjectFilesWidget(tr("Files"),tr("Project"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ProjectCompilerWidget(tr("Compiler Set"),tr("Project"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ProjectCompileParamatersWidget(tr("Custom Compile options"),tr("Project"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ProjectDirectoriesWidget(tr("Directories"),tr("Project"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ProjectPreCompileWidget(tr("Precompiled Header"),tr("Project"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ProjectMakefileWidget(tr("Makefile"),tr("Project"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ProjectOutputWidget(tr("Output"),tr("Project"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ProjectDLLHostWidget(tr("DLL host"),tr("Project"));
    widget->init();
    dialog->addWidget(widget);

    widget = new ProjectVersionInfoWidget(tr("Version info"),tr("Project"));
    widget->init();
    dialog->addWidget(widget);

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
            on_widgetsView_clicked(pWidgetItem->index());
            return true;
        }
    }
    return false;
}


void SettingsDialog::on_widgetsView_clicked(const QModelIndex &index)
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

void SettingsDialog::widget_settings_changed(bool value)
{
    ui->btnApply->setEnabled(value);
}

void SettingsDialog::on_btnCancle_pressed()
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

bool SettingsDialog::appShouldQuit() const
{
    return mAppShouldQuit;
}

void SettingsDialog::closeAndQuit()
{
    mAppShouldQuit = true;
    close();
}
