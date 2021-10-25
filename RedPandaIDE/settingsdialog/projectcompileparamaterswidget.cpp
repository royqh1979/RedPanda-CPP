#include "projectcompileparamaterswidget.h"
#include "ui_projectcompileparamaterswidget.h"
#include "../mainwindow.h"
#include "../project.h"

ProjectCompileParamatersWidget::ProjectCompileParamatersWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectCompileParamatersWidget)
{
    ui->setupUi(this);
}

ProjectCompileParamatersWidget::~ProjectCompileParamatersWidget()
{
    delete ui;
}

void ProjectCompileParamatersWidget::doLoad()
{
    ui->txtCCompiler->setPlainText(pMainWindow->project()->options().compilerCmd);
    ui->txtCPPCompiler->setPlainText(pMainWindow->project()->options().cppCompilerCmd);
    ui->txtLinker->setPlainText(pMainWindow->project()->options().linkerCmd);
}

void ProjectCompileParamatersWidget::doSave()
{
    pMainWindow->project()->options().compilerCmd = ui->txtCCompiler->toPlainText();
    pMainWindow->project()->options().cppCompilerCmd = ui->txtCPPCompiler->toPlainText();
    pMainWindow->project()->options().linkerCmd = ui->txtLinker->toPlainText();
    pMainWindow->project()->saveOptions();
}
