#include "projectdllhostwidget.h"
#include "ui_projectdllhostwidget.h"
#include "../project.h"
#include "../mainwindow.h"

#include <QFileDialog>

ProjectDLLHostWidget::ProjectDLLHostWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectDLLHostWidget)
{
    ui->setupUi(this);
}

ProjectDLLHostWidget::~ProjectDLLHostWidget()
{
    delete ui;
}

void ProjectDLLHostWidget::doLoad()
{
    ui->txtHost->setText(pMainWindow->project()->options().hostApplication);
}

void ProjectDLLHostWidget::doSave()
{
    pMainWindow->project()->options().hostApplication = ui->txtHost->text();
}

void ProjectDLLHostWidget::on_btnBrowse_triggered(QAction *arg1)
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Choose host application"),
                pMainWindow->project()->directory(),
                tr("All files (*.*)"));
    if (!filename.isEmpty() && fileExists(filename)) {
        ui->txtHost->setText(filename);
    }
}

