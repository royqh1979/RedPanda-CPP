#include "editorfontdialog.h"
#include "ui_editorfontdialog.h"

EditorFontDialog::EditorFontDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditorFontDialog)
{
    ui->setupUi(this);
}

EditorFontDialog::~EditorFontDialog()
{
    delete ui;
}

QString EditorFontDialog::fontFamily() const
{
    return mFontFamily;
}

void EditorFontDialog::setFontFamily(const QString &fontFamily)
{
    mFontFamily = fontFamily;
    ui->fontComboBox->setCurrentFont(QFont(mFontFamily));
}

void EditorFontDialog::on_chkMonoOnly_toggled(bool checked)
{
    QString savedFont = ui->fontComboBox->currentFont().family();
    ui->fontComboBox->setFontFilters(checked ? QFontComboBox::MonospacedFonts : QFontComboBox::AllFonts);
    ui->fontComboBox->setCurrentFont(QFont(savedFont));
    ui->fontComboBox->adjustSize();
}


void EditorFontDialog::on_buttonBox_accepted()
{
    mFontFamily = ui->fontComboBox->currentFont().family();
}

