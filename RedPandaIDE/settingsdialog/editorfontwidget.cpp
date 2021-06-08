#include "editorfontwidget.h"
#include "ui_editorfontwidget.h"

EditorFontWidget::EditorFontWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorFontWidget)
{
    ui->setupUi(this);
}

EditorFontWidget::~EditorFontWidget()
{
    delete ui;
}


void EditorFontWidget::on_chkOnlyMonospacedFonts_stateChanged(int)
{
    if (ui->chkOnlyMonospacedFonts->isChecked()) {
        ui->cbFont->setFontFilters(QFontComboBox::FontFilter::MonospacedFonts);
    } else {
        ui->cbFont->setFontFilters(QFontComboBox::FontFilter::AllFonts);
    }
}

void EditorFontWidget::on_chkGutterOnlyMonospacedFonts_stateChanged(int)
{
    if (ui->chkGutterOnlyMonospacedFonts->isChecked()) {
        ui->cbGutterFont->setFontFilters(QFontComboBox::FontFilter::MonospacedFonts);
    } else {
        ui->cbGutterFont->setFontFilters(QFontComboBox::FontFilter::AllFonts);
    }
}

void EditorFontWidget::doLoad()
{

}

void EditorFontWidget::doSave()
{

}
