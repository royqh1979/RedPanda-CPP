#include "environmentfolderswidget.h"
#include "ui_environmentfolderswidget.h"
#include "../settings.h"
#include "../mainwindow.h"
#include "../iconsmanager.h"

#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QUrl>

EnvironmentFoldersWidget::EnvironmentFoldersWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentFoldersWidget)
{
    ui->setupUi(this);
}

EnvironmentFoldersWidget::~EnvironmentFoldersWidget()
{
    delete ui;
}

void EnvironmentFoldersWidget::doLoad()
{
    ui->txtConfigFolder->setText(pSettings->dirs().config());
}

void EnvironmentFoldersWidget::doSave()
{

}

void EnvironmentFoldersWidget::on_btnOpenConfigFolderInBrowser_clicked()
{
    QDesktopServices::openUrl(
                QUrl("file:///"+
                     includeTrailingPathDelimiter(pSettings->dirs().config()),QUrl::TolerantMode));

}


void EnvironmentFoldersWidget::on_btnResetDefault_clicked()
{
    if (QMessageBox::question(this,tr("Confirm"),
                          tr("Do you really want to delete all custom settings?"),
                          QMessageBox::Yes|QMessageBox::No,
                          QMessageBox::No)!=QMessageBox::Yes)
        return;
    QDir dir(pSettings->dirs().config());
    if (!dir.removeRecursively()) {
        QMessageBox::critical(this,tr("Error"),
                              tr("Failed to delete custom settings."));
        return;
    }
    emit shouldQuitApp();
}

void EnvironmentFoldersWidget::updateIcons(const QSize &size)
{
    pIconsManager->setIcon(ui->btnOpenConfigFolderInBrowser,IconsManager::ACTION_FILE_OPEN_FOLDER);
}

