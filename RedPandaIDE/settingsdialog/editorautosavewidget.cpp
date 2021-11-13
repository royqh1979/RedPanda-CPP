#include "editorautosavewidget.h"
#include "ui_editorautosavewidget.h"
#include "../settings.h"
#include "../mainwindow.h"

EditorAutoSaveWidget::EditorAutoSaveWidget(const QString& name, const QString& group,
                                                             QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorAutoSaveWidget)
{
    ui->setupUi(this);
}

EditorAutoSaveWidget::~EditorAutoSaveWidget()
{
    delete ui;
}

void EditorAutoSaveWidget::onAutoSaveStrategyChanged()
{
    if (ui->rbOverwrite->isChecked()) {
        ui->lblFilename->setText(tr("Demo file name: ") + "main.cpp");
    } else if (ui->rbAppendUNIXTimestamp->isChecked()) {
        ui->lblFilename->setText(tr("Demo file name: ") +
                                 QString("main.%1.cpp").arg(QDateTime::currentSecsSinceEpoch()));
    } else if (ui->rbAppendFormattedTimestamp->isChecked()) {
        QDateTime time = QDateTime::currentDateTime();
        ui->lblFilename->setText(tr("Demo file name: ") +
                                 QString("main.%1.cpp").arg(time.toString("yyyy.MM.dd.hh.mm.ss")));
    }
}

void EditorAutoSaveWidget::doLoad()
{
    //pSettings->editor().load();
    //font
    ui->grpEnableAutoSave->setChecked(pSettings->editor().enableAutoSave());
    ui->spinInterval->setValue(pSettings->editor().autoSaveInterval());
    switch(pSettings->editor().autoSaveTarget()) {
    case astCurrentFile:
        ui->rbCurrentFile->setChecked(true);
        break;
    case astAllOpennedFiles:
        ui->rbAllOpennedFiles->setChecked(true);
        break;
    default:
        ui->rbProjectFiles->setChecked(true);
    }
    switch(pSettings->editor().autoSaveStrategy()) {
    case assOverwrite:
        ui->rbOverwrite->setChecked(true);
        break;
    case assAppendUnixTimestamp:
        ui->rbAppendUNIXTimestamp->setChecked(true);
        break;
    default:
        ui->rbAppendFormattedTimestamp->setChecked(true);
    }
}

void EditorAutoSaveWidget::doSave()
{
    pSettings->editor().setEnableAutoSave(ui->grpEnableAutoSave->isChecked());
    pSettings->editor().setAutoSaveInterval(ui->spinInterval->value());
    if (ui->rbCurrentFile->isChecked())
        pSettings->editor().setAutoSaveTarget(astCurrentFile);
    else if (ui->rbAllOpennedFiles->isChecked())
        pSettings->editor().setAutoSaveTarget(astAllOpennedFiles);
    else
        pSettings->editor().setAutoSaveTarget(astAllProjectFiles);
    if (ui->rbOverwrite->isChecked())
        pSettings->editor().setAutoSaveStrategy(assOverwrite);
    else if (ui->rbAppendUNIXTimestamp->isChecked())
        pSettings->editor().setAutoSaveStrategy(assAppendUnixTimestamp);
    else
        pSettings->editor().setAutoSaveStrategy(assAppendFormatedTimeStamp);
    pSettings->editor().save();
    pMainWindow->resetAutoSaveTimer();
}

void EditorAutoSaveWidget::on_rbOverwrite_toggled(bool)
{
    onAutoSaveStrategyChanged();
}


void EditorAutoSaveWidget::on_rbAppendUNIXTimestamp_toggled(bool)
{
    onAutoSaveStrategyChanged();
}


void EditorAutoSaveWidget::on_rbAppendFormattedTimestamp_toggled(bool)
{
    onAutoSaveStrategyChanged();
}

