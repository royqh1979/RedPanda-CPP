#include "editormiscwidget.h"
#include "ui_editormiscwidget.h"
#include "../settings.h"

EditorMiscWidget::EditorMiscWidget(const QString& name, const QString& group,
                                   QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorMiscWidget)
{
    ui->setupUi(this);
}

EditorMiscWidget::~EditorMiscWidget()
{
    delete ui;
}

void EditorMiscWidget::doLoad()
{
    ui->chkReadonlySystemHeaders->setChecked(pSettings->editor().readOnlySytemHeader());
    ui->chkLoadLastFiles->setChecked(pSettings->editor().autoLoadLastFiles());
    ui->rbCppFile->setChecked(pSettings->editor().defaultFileCpp());
}

void EditorMiscWidget::doSave()
{
    pSettings->editor().setReadOnlySytemHeader(ui->chkReadonlySystemHeaders->isChecked());
    pSettings->editor().setAutoLoadLastFiles(ui->chkLoadLastFiles->isChecked());
    pSettings->editor().setDefaultFileCpp(ui->rbCppFile->isChecked());
    pSettings->editor().save();
}
