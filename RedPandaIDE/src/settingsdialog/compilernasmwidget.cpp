#include "compilernasmwidget.h"
#include "ui_compilernasmwidget.h"
#include "../iconsmanager.h"
#include "../settings.h"
#include "../systemconsts.h"
#include "../utils.h"
#include "../mainwindow.h"

#include <QFileDialog>

CompilerNASMWidget::CompilerNASMWidget(const QString& name, const QString& group,IconsManager *iconsManager, QWidget *parent) :
    SettingsWidget(name,group,iconsManager,parent),
    ui(new Ui::CompilerNASMWidget)
{
    ui->setupUi(this);
    ui->lblNASMInfo->setVisible(false);
}

CompilerNASMWidget::~CompilerNASMWidget()
{
    delete ui;
}

void CompilerNASMWidget::doLoad()
{
    ui->txtNASMPath->setText(pSettings->compile().NASMPath());
    ui->chkLinkStdlib->setChecked(pSettings->compile().NASMLinkCStandardLib());
    on_btnTestNASM_clicked();
}

void CompilerNASMWidget::doSave()
{
    pSettings->compile().setNASMPath(ui->txtNASMPath->text());
    pSettings->compile().setNASMLinkCStandardLib(ui->chkLinkStdlib->isChecked());
    pSettings->compile().save();
    pMainWindow->updateCompileActions();
}

void CompilerNASMWidget::updateIcons(const QSize &/*size*/)
{
    iconsManager()->setIcon(ui->btnBrowserNASM, IconsManager::ACTION_FILE_OPEN_FOLDER);
}


void CompilerNASMWidget::on_btnBrowserNASM_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("NASM"),
                QString(),
                tr("All files (%1)").arg(ALL_FILE_WILDCARD));
    if (!filename.isEmpty() && fileExists(filename)) {
        ui->txtNASMPath->setText(filename);
        on_btnTestNASM_clicked();
    }
}


void CompilerNASMWidget::on_btnTestNASM_clicked()
{
    QFileInfo fileInfo(ui->txtNASMPath->text());
    if (!fileInfo.exists()) {
        ui->lblNASMInfo->setVisible(false);
        return;
    }
    ui->lblNASMInfo->setVisible(true);
    ui->lblNASMInfo->setText("");
    QStringList args;
    args.append("--version");
    auto [nasmOutput, nasmError, processError]  = runAndGetOutput(
                fileInfo.absoluteFilePath(),
                fileInfo.absolutePath(),
                args);
    ui->lblNASMInfo->setText(nasmOutput);
}

