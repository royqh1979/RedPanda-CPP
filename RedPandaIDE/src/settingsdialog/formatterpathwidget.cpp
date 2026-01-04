#include "formatterpathwidget.h"
#include "ui_formatterpathwidget.h"
#include "../iconsmanager.h"
#include "../settings.h"
#include "../systemconsts.h"

#include <QFileDialog>

FormatterPathWidget::FormatterPathWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::FormatterPathWidget)
{
    ui->setupUi(this);
}

FormatterPathWidget::~FormatterPathWidget()
{
    delete ui;
}

void FormatterPathWidget::doLoad()
{
    ui->txtAstyle->setText(pSettings->environment().AStylePath());
}

void FormatterPathWidget::doSave()
{
    pSettings->environment().setAStylePath(ui->txtAstyle->text());
}

void FormatterPathWidget::updateIcons(const QSize &/*size*/)
{
    pIconsManager->setIcon(ui->btnChooseAstyle, IconsManager::ACTION_FILE_OPEN_FOLDER);
}

void FormatterPathWidget::on_btnChooseAstyle_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Path to astyle"),
                QString(),
                tr("All files (%1)").arg(ALL_FILE_WILDCARD));
    if (!fileName.isEmpty() ) {
        ui->txtAstyle->setText(fileName);
    }
}

