#include "editorgeneralwidget.h"
#include "ui_editorgeneralwidget.h"
#include "../settings.h"
#include "../mainwindow.h"

#include <QStandardItemModel>

EditorGeneralWidget::EditorGeneralWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::editorgeneralwidget)
{
    ui->setupUi(this);
    QStringList caretTypes;
    caretTypes.append(tr("Vertical Line"));
    caretTypes.append(tr("Horizontal Line"));
    caretTypes.append(tr("Half Block"));
    caretTypes.append(tr("Block"));
    ui->cbCaretForInsert->addItems(caretTypes);
    ui->cbCaretForOverwrite->addItems(caretTypes);
}

EditorGeneralWidget::~EditorGeneralWidget()
{
    delete ui;
}

static void setCaretTypeIndex(QComboBox* combo, SynEditCaretType caretType) {
    int t = static_cast<int>(caretType);
    combo->setCurrentIndex(t);
}

static SynEditCaretType getCaretTypeIndex(QComboBox* combo) {
    if (combo->currentIndex()<0)
        return SynEditCaretType::ctVerticalLine;
    return static_cast<SynEditCaretType>(combo->currentIndex());
}
void EditorGeneralWidget::doLoad()
{
    pSettings->editor().load();
    //indents
    ui->chkAutoIndent->setChecked(pSettings->editor().autoIndent());
    ui->chkTabToSpaces->setChecked(pSettings->editor().tabToSpaces());
    ui->spTabWidth->setValue(pSettings->editor().tabWidth());
    ui->chkShowIndentLines->setChecked(pSettings->editor().showIndentLines());
    ui->colorIndentLine->setColor(pSettings->editor().indentLineColor());
    ui->chkFillIndents->setChecked(pSettings->editor().fillIndents());
    //carets
    ui->chkEnhanceHome->setChecked(pSettings->editor().enhanceHomeKey());
    ui->chkEnhanceEndKey->setChecked(pSettings->editor().enhanceEndKey());
    ui->chkKeepCaretX->setChecked(pSettings->editor().keepCaretX());
    setCaretTypeIndex(ui->cbCaretForInsert,pSettings->editor().caretForInsert());
    setCaretTypeIndex(ui->cbCaretForOverwrite,pSettings->editor().caretForOverwrite());
    ui->chkCaretUseTextColor->setChecked(pSettings->editor().caretUseTextColor());
    ui->colorCaret->setColor(pSettings->editor().caretColor());
    //scrolls;
    ui->chkAutoHideScrollBars->setChecked(pSettings->editor().autoHideScrollbar());
    ui->chkScrollPastEOF->setChecked(pSettings->editor().scrollPastEof());
    ui->chkScrollPastEOL->setChecked(pSettings->editor().scrollPastEol());
    ui->chkScrollHalfPage->setChecked(pSettings->editor().halfPageScroll());
    ui->chkScrollByOneLess->setChecked(pSettings->editor().scrollByOneLess());
    ui->spinMouseWheelScrollSpeed->setValue(pSettings->editor().mouseWheelScrollSpeed());

    //right margin line;
    ui->grpRightEdge->setChecked(pSettings->editor().showRightEdgeLine());
    ui->spRightEdge->setValue(pSettings->editor().rightEdgeWidth());
    ui->colorRightEdgeLine->setColor(pSettings->editor().rightEdgeLineColor());
}

void EditorGeneralWidget::doSave()
{
    //indents
    pSettings->editor().setAutoIndent(ui->chkAutoIndent->isChecked());
    pSettings->editor().setTabToSpaces(ui->chkTabToSpaces->isChecked());
    pSettings->editor().setTabWidth(ui->spTabWidth->value());
    pSettings->editor().setShowIndentLines(ui->chkShowIndentLines->isChecked());
    pSettings->editor().setIndentLineColor(ui->colorIndentLine->color());
    pSettings->editor().setFillIndents(ui->chkFillIndents->isChecked());

    //carets
    pSettings->editor().setEnhanceHomeKey(ui->chkEnhanceHome->isChecked());
    pSettings->editor().setEnhanceEndKey(ui->chkEnhanceEndKey->isChecked());
    pSettings->editor().setKeepCaretX(ui->chkKeepCaretX->isChecked());
    pSettings->editor().setCaretForInsert(getCaretTypeIndex(ui->cbCaretForInsert));
    pSettings->editor().setCaretForOverwrite(getCaretTypeIndex(ui->cbCaretForOverwrite));
    pSettings->editor().setCaretUseTextColor(ui->chkCaretUseTextColor->isChecked());
    pSettings->editor().setCaretColor(ui->colorCaret->color());
    //scrolls;
    pSettings->editor().setAutoHideScrollbar(ui->chkAutoHideScrollBars->isChecked());
    pSettings->editor().setScrollPastEof(ui->chkScrollPastEOF->isChecked());
    pSettings->editor().setScrollPastEol(ui->chkScrollPastEOL->isChecked());
    pSettings->editor().setScrollByOneLess(ui->chkScrollByOneLess->isChecked());
    pSettings->editor().setHalfPageScroll(ui->chkScrollHalfPage->isChecked());
    pSettings->editor().setMouseWheelScrollSpeed(ui->spinMouseWheelScrollSpeed->value());

    //right margin line;
    pSettings->editor().setShowRightEdgeLine(ui->grpRightEdge->isChecked());
    pSettings->editor().setRightEdgeWidth(ui->spRightEdge->value());
    pSettings->editor().setRightEdgeLineColor(ui->colorRightEdgeLine->color());
    pSettings->editor().save();
    pMainWindow->updateEditorSettings();
}

void EditorGeneralWidget::on_chkCaretUseTextColor_stateChanged(int )
{
    ui->lbCaretColor->setVisible(!ui->chkCaretUseTextColor->isChecked());
    ui->colorCaret->setVisible(!ui->chkCaretUseTextColor->isChecked());
}


void EditorGeneralWidget::on_chkShowIndentLines_stateChanged(int)
{
    ui->lbIndentLineColor->setVisible(ui->chkShowIndentLines->isChecked());
    ui->colorIndentLine->setVisible(ui->chkShowIndentLines->isChecked());
}

