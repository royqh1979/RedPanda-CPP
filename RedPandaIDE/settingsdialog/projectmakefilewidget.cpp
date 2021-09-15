#include "projectmakefilewidget.h"
#include "ui_projectmakefilewidget.h"
#include "compilersetdirectorieswidget.h"
#include "../mainwindow.h"
#include "../project.h"
#include "../widgets/custommakefileinfodialog.h"

#include <QFileDialog>

ProjectMakefileWidget::ProjectMakefileWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectMakefileWidget)
{
    ui->setupUi(this);

    mIncludesDirWidget = new CompilerSetDirectoriesWidget(this);
    ui->verticalLayout->addWidget(mIncludesDirWidget);
}

ProjectMakefileWidget::~ProjectMakefileWidget()
{
    delete ui;
}

void ProjectMakefileWidget::doLoad()
{
    ui->grpCustomMakefile->setChecked(pMainWindow->project()->options().useCustomMakefile);
    ui->txtCustomMakefile->setText(pMainWindow->project()->options().customMakefile);
    mIncludesDirWidget->setDirList(pMainWindow->project()->options().makeIncludes);
}

void ProjectMakefileWidget::doSave()
{
    pMainWindow->project()->options().useCustomMakefile = ui->grpCustomMakefile->isChecked();
    pMainWindow->project()->options().customMakefile = ui->txtCustomMakefile->text();
    pMainWindow->project()->options().makeIncludes = mIncludesDirWidget->dirList();
    pMainWindow->project()->saveOptions();

}

void ProjectMakefileWidget::on_btnBrowse_triggered(QAction *arg1)
{
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Custom makefile"),
                pMainWindow->project()->directory(),
                tr("All files (*.*)"));
    if (!fileName.isEmpty() && QFileInfo(fileName).exists()) {
        ui->txtCustomMakefile->setText(fileName);
    }
}


void ProjectMakefileWidget::on_pushButton_clicked()
{
    CustomMakefileInfoDialog dialog(this);
    dialog.exec();
}

