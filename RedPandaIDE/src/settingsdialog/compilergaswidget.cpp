#include "compilergaswidget.h"
#include "ui_compilergaswidget.h"
#include "../iconsmanager.h"
#include "../settings.h"
#include "../systemconsts.h"
#include "../utils.h"
#include "../mainwindow.h"

#include <QFileDialog>

CompilerGASWidget::CompilerGASWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::CompilerGASWidget)
{
    ui->setupUi(this);
}

CompilerGASWidget::~CompilerGASWidget()
{
    delete ui;
}

void CompilerGASWidget::doLoad()
{
    ui->chkLinkStdlib->setChecked(pSettings->compile().GASLinkCStandardLib());
}

void CompilerGASWidget::doSave()
{
    pSettings->compile().setGASLinkCStandardLib(ui->chkLinkStdlib->isChecked());
    pSettings->compile().save();
    pMainWindow->updateCompileActions();
}

void CompilerGASWidget::updateIcons(const QSize &/*size*/)
{
    //pIconsManager->setIcon(ui->btnBrowserNASM, IconsManager::ACTION_FILE_OPEN_FOLDER);
}


