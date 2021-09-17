#include "projectdirectorieswidget.h"
#include "ui_projectdirectorieswidget.h"
#include "compilersetdirectorieswidget.h"
#include "../project.h"
#include "../mainwindow.h"

ProjectDirectoriesWidget::ProjectDirectoriesWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectDirectoriesWidget)
{
    ui->setupUi(this);

    mLibDirWidget = new CompilerSetDirectoriesWidget();
    ui->tabDirs->addTab(mLibDirWidget,tr("Libraries"));
    mIncludeDirWidget = new CompilerSetDirectoriesWidget();
    ui->tabDirs->addTab(mIncludeDirWidget,tr("Includes"));
    mResourceDirWidget = new CompilerSetDirectoriesWidget();
    ui->tabDirs->addTab(mResourceDirWidget,tr("Resources"));
}


ProjectDirectoriesWidget::~ProjectDirectoriesWidget()
{
    delete ui;
}

void ProjectDirectoriesWidget::doLoad()
{
    mLibDirWidget->setDirList(pMainWindow->project()->options().libs);
    mIncludeDirWidget->setDirList(pMainWindow->project()->options().includes);
    mResourceDirWidget->setDirList(pMainWindow->project()->options().resourceIncludes);

}

void ProjectDirectoriesWidget::doSave()
{
    pMainWindow->project()->options().libs = mLibDirWidget->dirList();
    pMainWindow->project()->options().includes = mIncludeDirWidget->dirList();
    pMainWindow->project()->options().resourceIncludes = mResourceDirWidget->dirList();
    pMainWindow->project()->saveOptions();
}
