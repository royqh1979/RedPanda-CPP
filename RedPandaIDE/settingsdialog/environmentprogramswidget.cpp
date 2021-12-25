#include "environmentprogramswidget.h"
#include "ui_environmentprogramswidget.h"
#include "../settings.h"
#include "../iconsmanager.h"

#include <QFileDialog>

EnvironmentProgramsWidget::EnvironmentProgramsWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentProgramsWidget)
{
    ui->setupUi(this);
}

EnvironmentProgramsWidget::~EnvironmentProgramsWidget()
{
    delete ui;
}

void EnvironmentProgramsWidget::doLoad()
{
    ui->txtTerminal->setText(pSettings->environment().terminalPath());
}

void EnvironmentProgramsWidget::doSave()
{
    pSettings->environment().setTerminalPath(ui->txtTerminal->text());
    pSettings->environment().save();
}

void EnvironmentProgramsWidget::updateIcons(const QSize &)
{
    pIconsManager->setIcon(ui->btnChooseTerminal,IconsManager::ACTION_FILE_OPEN_FOLDER);
}

void EnvironmentProgramsWidget::on_btnChooseTerminal_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Choose Terminal Program"),
                QString(),
                tr("All files (*.*)"));
    if (!filename.isEmpty() && fileExists(filename) ) {
        ui->txtTerminal->setText(filename);
    }
}
