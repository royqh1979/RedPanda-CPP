/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <qsynedit/document.h>
#include "formattergeneralwidget.h"
#include "ui_formattergeneralwidget.h"
#include "../settings.h"

FormatterGeneralWidget::FormatterGeneralWidget(const QString& name, const QString& group, QWidget *parent):
    SettingsWidget(name,group,parent),
    ui(new Ui::FormatterGeneralWidget)
{
    ui->setupUi(this);
    ui->cbBraceStyle->setModel(&mStylesModel);
    connect(ui->cbBraceStyle, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FormatterGeneralWidget::onBraceStyleChanged);
    ui->editDemo->setReadOnly(true);
    connect(this, &SettingsWidget::settingsChanged,
               this, &FormatterGeneralWidget::updateDemo);

    connect(ui->chkSqueezeEmptyLines, &QCheckBox::toggled,
            ui->spinSqueezeEmptyLines, &QSpinBox::setEnabled);

    ui->cbMinConditionalIndent->addItem(tr("No minimal indent"),0);
    ui->cbMinConditionalIndent->addItem(tr("Indent at least one additional indent"),1);
    ui->cbMinConditionalIndent->addItem(tr("Indent at least two additional indents"),2);
    ui->cbMinConditionalIndent->addItem(tr("Indent at least one-half an additional indent."),3);
}

FormatterGeneralWidget::~FormatterGeneralWidget()
{
    delete ui;
}

void FormatterGeneralWidget::onBraceStyleChanged()
{
    PFormatterStyleItem item = mStylesModel.getStyle(ui->cbBraceStyle->currentIndex());
    if (item) {
        ui->lblBraceStyle->setText(item->description);
    }
}

void FormatterGeneralWidget::doLoad()
{
    Settings::CodeFormatter& format = pSettings->codeFormatter();
    for (int i=0;i<mStylesModel.rowCount(QModelIndex());i++) {
        PFormatterStyleItem item = mStylesModel.getStyle(i);
        if (item->style == format.braceStyle()) {
            ui->cbBraceStyle->setCurrentIndex(i);
            break;
        }
    }
    if (format.indentStyle() == FormatterIndentType::fitSpace) {
        ui->rbIndentSpaces->setChecked(true);
    } else {
        ui->rbIndentTabs->setChecked(true);
    }
    ui->spinTabSize->setValue(format.tabWidth());
    ui->chkAttachNamespaces->setChecked(format.attachNamespaces());
    ui->chkAttachClasses->setChecked(format.attachClasses());
    ui->chkAttachInline->setChecked(format.attachInlines());
    ui->chkAttachExternC->setChecked(format.attachExternC());
    ui->chkAttachClosingWhile->setChecked(format.attachClosingWhile());
    ui->chkIndentClasses->setChecked(format.indentClasses());
    ui->chkIndentModifiers->setChecked(format.indentModifiers());
    ui->chkIndentSwiches->setChecked(format.indentSwitches());
    ui->chkIndentCases->setChecked(format.indentCases());
    ui->chkIndentNamespaces->setChecked(format.indentNamespaces());
    ui->chkIndentAfterParens->setChecked(format.indentAfterParens());
    ui->spinIndentContinuation->setValue(format.indentContinuation());
    ui->chkIndentLabels->setChecked(format.indentLabels());
    ui->chkIndentPreprocBlock->setChecked(format.indentPreprocBlock());
    ui->chkIndentPreprocCond->setChecked(format.indentPreprocCond());
    ui->chkIndentPreprocDefine->setChecked(format.indentPreprocDefine());
    ui->chkIndentCol1Comments->setChecked(format.indentCol1Comments());
    int index=ui->cbMinConditionalIndent->findData(format.minConditionalIndent());
    ui->cbMinConditionalIndent->setCurrentIndex(index);
    ui->spinMaxContinuationIndent->setValue(format.maxContinuationIndent());
    ui->chkBreakBlocks->setChecked(format.breakBlocks());
    ui->chkBreakBlocksAll->setChecked(format.breakBlocksAll());
    ui->chkPadOper->setChecked(format.padOper());
    ui->chkPadComma->setChecked(format.padComma());
    ui->chkPadParen->setChecked(format.padParen());
    ui->chkPadParenOut->setChecked(format.padParenOut());
    ui->chkPadFirstParenOut->setChecked(format.padFirstParenOut());
    ui->chkPadParenIn->setChecked(format.padParenIn());
    ui->chkPadHeader->setChecked(format.padHeader());
    ui->chkUnpadParen->setChecked(format.unpadParen());
    ui->chkDeleteEmptyLines->setChecked(format.deleteEmptyLines());
    ui->chkFillEmptyLines->setChecked(format.fillEmptyLines());
    ui->chkSqueezeEmptyLines->setChecked(format.squeezeLines());
    ui->spinSqueezeEmptyLines->setEnabled(format.squeezeLines());
    ui->spinSqueezeEmptyLines->setValue(format.squeezeLinesNumber());
    ui->chkSqueezeWhitespace->setChecked(format.squeezeWhitespace());
    switch(format.alignPointerStyle()) {
    case FormatterOperatorAlign::foaNone:
        ui->rbAlignPointNone->setChecked(true);
        break;
    case FormatterOperatorAlign::foaType:
        ui->rbAlignPointType->setChecked(true);
        break;
    case FormatterOperatorAlign::foaMiddle:
        ui->rbAlignPointerMiddle->setChecked(true);
        break;
    case FormatterOperatorAlign::foaName:
        ui->rbAlignPointerName->setChecked(true);
        break;
    }
    switch(format.alignReferenceStyle()) {
    case FormatterOperatorAlign::foaNone:
        ui->rbAlignReferenceNone->setChecked(true);
        break;
    case FormatterOperatorAlign::foaType:
        ui->rbAlignReferenceType->setChecked(true);
        break;
    case FormatterOperatorAlign::foaMiddle:
        ui->rbAlignReferenceMiddle->setChecked(true);
        break;
    case FormatterOperatorAlign::foaName:
        ui->rbAlignReferenceName->setChecked(true);
        break;
    }
    ui->chkBreakClosingBraces->setChecked(format.breakClosingBraces());
    ui->chkBreakElseIf->setChecked(format.breakElseIf());
    ui->chkBreakOneLineHeaders->setChecked(format.breakOneLineHeaders());
    ui->chkAddBraces->setChecked(format.addBraces());
    ui->chkAddOneLineBraces->setChecked(format.addOneLineBraces());
    ui->chkRemoveBraces->setChecked(format.removeBraces());
    ui->chkBreakReturnType->setChecked(format.breakReturnType());
    ui->chkBreakReturnTypeDecl->setChecked(format.breakReturnTypeDecl());
    ui->chkAttachReturnType->setChecked(format.attachReturnType());
    ui->chkAttachReturnTypeDecl->setChecked(format.attachReturnTypeDecl());
    ui->chkKeepOneLineBlocks->setChecked(format.keepOneLineBlocks());
    ui->chkKeepOneLineStatements->setChecked(format.keepOneLineStatements());
    ui->chkConvertTabs->setChecked(format.convertTabs());
    ui->chkCloseTemplates->setChecked(format.closeTemplates());
    ui->chkRemoveCommentPrefix->setChecked(format.removeCommentPrefix());
    ui->chkBreakMaxCodeLength->setChecked(format.breakMaxCodeLength());
    ui->spinMaxCodeLength->setValue(format.maxCodeLength());
    ui->chkBreakAfterLogical->setChecked(format.breakAfterLogical());
    updateDemo();
}

void FormatterGeneralWidget::doSave()
{
    Settings::CodeFormatter& format = pSettings->codeFormatter();
    updateCodeFormatter(format);
    format.save();
}

FormatterStyleModel::FormatterStyleModel(QObject *parent):QAbstractListModel(parent)
{
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Default"),
                    tr("The opening braces will not be changed and closing braces will be broken from the preceding line."),
                    FormatterBraceStyle::fbsDefault)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Allman"),
                    tr("Broken braces."),
                    FormatterBraceStyle::fbsAllman)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Java"),
                    tr("Attached braces."),
                    FormatterBraceStyle::fbsJava)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("K&R"),
                    tr("Linux braces."),
                    FormatterBraceStyle::fbsKR)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Stroustrup"),
                    tr("Linux braces, with broken closing headers."),
                    FormatterBraceStyle::fbsStroustrup)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Whitesmith"),
                    tr("Broken, indented braces.")
                    + " "
                    +tr("Indented class blocks and switch blocks."),
                    FormatterBraceStyle::fbsWitesmith)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("VTK"),
                    tr("Broken, indented braces except for the opening braces."),
                    FormatterBraceStyle::fbsVtk)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Ratliff"),
                    tr("Attached, indented braces."),
                    FormatterBraceStyle::fbsRatliff)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("GNU"),
                    tr("Broken braces, indented blocks."),
                    FormatterBraceStyle::fbsGNU)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Linux"),
                    tr("Linux braces, minimum conditional indent is one-half indent."),
                    FormatterBraceStyle::fbsLinux)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Horstmann"),
                    tr("Run-in braces, indented switches."),
                    FormatterBraceStyle::fbsHorstmann)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("One True Brace"),
                    tr("Linux braces, add braces to all conditionals."),
                    FormatterBraceStyle::fbs1TBS)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Google"),
                    tr("Attached braces, indented class modifiers."),
                    FormatterBraceStyle::fbsGoogle)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Mozilla"),
                    tr("Linux braces, with broken braces for structs and enums, and attached braces for namespaces."),
                    FormatterBraceStyle::fbsMozilla)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Webkit"),
                    tr("Linux braces, with attached closing headers."),
                    FormatterBraceStyle::fbsWebkit)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Pico"),
                    tr("Run-in opening braces and attached closing braces.")
                    +" "+
                    tr("Uses keep one line blocks and keep one line statements."),
                    FormatterBraceStyle::fbsPico)
                );
    mStyles.append(
                std::make_shared<FormatterStyleItem>(
                    tr("Lisp"),
                    tr("Attached opening braces and attached closing braces.")
                    +" "+
                    tr("Uses keep one line statements."),
                    FormatterBraceStyle::fbsLisp)
                );
}

int FormatterStyleModel::rowCount(const QModelIndex &) const
{
    return mStyles.count();
}

QVariant FormatterStyleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    int row = index.row();
    if (row<0 || row>=mStyles.count())
        return QVariant();
    PFormatterStyleItem item = mStyles[row];
    switch (role) {
    case Qt::DisplayRole:
        return item->name;
    case Qt::ToolTipRole:
        return item->description;
    }
    return QVariant();
}

PFormatterStyleItem FormatterStyleModel::getStyle(const QModelIndex &index)
{
    if (index.isValid()) {
        return getStyle(index.row());
    } else {
        return PFormatterStyleItem();
    }
}

PFormatterStyleItem FormatterStyleModel::getStyle(int index)
{
    if (index<0 || index>=mStyles.count())
        return PFormatterStyleItem();
    return mStyles[index];
}

FormatterStyleItem::FormatterStyleItem(const QString &name, const QString &description, FormatterBraceStyle style)
{
    this->name = name;
    this->description = description;
    this->style = style;
}

void FormatterGeneralWidget::on_chkBreakMaxCodeLength_stateChanged(int)
{
    ui->spinMaxCodeLength->setEnabled(ui->chkBreakMaxCodeLength->isChecked());
    ui->chkBreakAfterLogical->setEnabled(ui->chkBreakMaxCodeLength->isChecked());
}

void FormatterGeneralWidget::updateDemo()
{
    if (!fileExists(pSettings->environment().AStylePath())) {
        ui->editDemo->document()->setText(Editor::tr("Can't find astyle in \"%1\".").arg(pSettings->environment().AStylePath()));
        return;
    }
    QFile file(":/codes/formatdemo.cpp");
    if (!file.open(QFile::ReadOnly))
        return;
    QByteArray content = file.readAll();

    Settings::CodeFormatter formatter(nullptr);
    updateCodeFormatter(formatter);
    QByteArray newContent = reformatContentUsingAstyle(content, formatter.getArguments());
    ui->editDemo->document()->setText(newContent);
}

void FormatterGeneralWidget::updateCodeFormatter(Settings::CodeFormatter &format)
{
    PFormatterStyleItem item = mStylesModel.getStyle(ui->cbBraceStyle->currentIndex());
    if (item)
        format.setBraceStyle(item->style);
    if (ui->rbIndentSpaces->isChecked()) {
        format.setIndentStyle(FormatterIndentType::fitSpace);
    } else {
        format.setIndentStyle(FormatterIndentType::fitTab);
    }
    format.setTabWidth(ui->spinTabSize->value());
    format.setAttachNamespaces(ui->chkAttachNamespaces->isChecked());
    format.setAttachClasses(ui->chkAttachClasses->isChecked());
    format.setAttachInlines(ui->chkAttachInline->isChecked());
    format.setAttachExternC(ui->chkAttachExternC->isChecked());
    format.setAttachClosingWhile(ui->chkAttachClosingWhile->isChecked());
    format.setIndentClasses(ui->chkIndentClasses->isChecked());
    format.setIndentModifiers(ui->chkIndentModifiers->isChecked());
    format.setIndentSwitches(ui->chkIndentSwiches->isChecked());
    format.setIndentCases(ui->chkIndentCases->isChecked());
    format.setIndentNamespaces(ui->chkIndentNamespaces->isChecked());
    format.setIndentAfterParens(ui->chkIndentAfterParens->isChecked());
    format.setIndentContinuation(ui->spinIndentContinuation->value());
    format.setIndentLabels(ui->chkIndentLabels->isChecked());
    format.setIndentPreprocBlock(ui->chkIndentPreprocBlock->isChecked());
    format.setIndentPreprocCond(ui->chkIndentPreprocCond->isChecked());
    format.setIndentPreprocDefine(ui->chkIndentPreprocDefine->isChecked());
    format.setIndentCol1Comments(ui->chkIndentCol1Comments->isChecked());
    format.setMinConditionalIndent(ui->cbMinConditionalIndent->currentData().toInt());
    format.setMaxContinuationIndent(ui->spinMaxContinuationIndent->value());
    format.setBreakBlocks(ui->chkBreakBlocks->isChecked());
    format.setBreakBlocksAll(ui->chkBreakBlocksAll->isChecked());
    format.setPadOper(ui->chkPadOper->isChecked());
    format.setPadComma(ui->chkPadComma->isChecked());
    format.setPadParen(ui->chkPadParen->isChecked());
    format.setPadParenOut(ui->chkPadParenOut->isChecked());
    format.setPadFirstParenOut(ui->chkPadFirstParenOut->isChecked());
    format.setPadParenIn(ui->chkPadParenIn->isChecked());
    format.setPadHeader(ui->chkPadHeader->isChecked());
    format.setUnpadParen(ui->chkUnpadParen->isChecked());
    format.setDeleteEmptyLines(ui->chkDeleteEmptyLines->isChecked());
    format.setFillEmptyLines(ui->chkFillEmptyLines->isChecked());
    format.setSqueezeLines(ui->chkSqueezeEmptyLines->isChecked());
    format.setSqueezeLinesNumber(ui->spinSqueezeEmptyLines->value());
    format.setSqueezeWhitespace(ui->chkSqueezeWhitespace->isChecked());

    if (ui->rbAlignPointNone->isChecked()) {
        format.setAlignPointerStyle(FormatterOperatorAlign::foaNone);
    } else if (ui->rbAlignPointType->isChecked()) {
        format.setAlignPointerStyle(FormatterOperatorAlign::foaType);
    } else if (ui->rbAlignPointerMiddle->isChecked()) {
        format.setAlignPointerStyle(FormatterOperatorAlign::foaMiddle);
    } else if (ui->rbAlignPointerName->isChecked()) {
        format.setAlignPointerStyle(FormatterOperatorAlign::foaName);
    }
    if (ui->rbAlignReferenceNone->isChecked()) {
        format.setAlignReferenceStyle(FormatterOperatorAlign::foaNone);
    } else if (ui->rbAlignReferenceType->isChecked()) {
        format.setAlignReferenceStyle(FormatterOperatorAlign::foaType);
    } else if (ui->rbAlignReferenceMiddle->isChecked()) {
        format.setAlignReferenceStyle(FormatterOperatorAlign::foaMiddle);
    } else if (ui->rbAlignReferenceName->isChecked()) {
        format.setAlignReferenceStyle(FormatterOperatorAlign::foaName);
    }
    format.setBreakClosingBraces(ui->chkBreakClosingBraces->isChecked());
    format.setBreakElseIf(ui->chkBreakElseIf->isChecked());
    format.setBreakOneLineHeaders(ui->chkBreakOneLineHeaders->isChecked());
    format.setAddBraces(ui->chkAddBraces->isChecked());
    format.setAddOneLineBraces(ui->chkAddOneLineBraces->isChecked());
    format.setRemoveBraces(ui->chkRemoveBraces->isChecked());
    format.setBreakReturnType(ui->chkBreakReturnType->isChecked());
    format.setBreakReturnTypeDecl(ui->chkBreakReturnTypeDecl->isChecked());
    format.setAttachReturnType(ui->chkAttachReturnType->isChecked());
    format.setAttachReturnTypeDecl(ui->chkAttachReturnTypeDecl->isChecked());
    format.setKeepOneLineBlocks(ui->chkKeepOneLineBlocks->isChecked());
    format.setKeepOneLineStatements(ui->chkKeepOneLineStatements->isChecked());
    format.setConvertTabs(ui->chkConvertTabs->isChecked());
    format.setCloseTemplates(ui->chkCloseTemplates->isChecked());
    format.setRemoveCommentPrefix(ui->chkRemoveCommentPrefix->isChecked());
    format.setBreakMaxCodeLength(ui->chkBreakMaxCodeLength->isChecked());
    format.setMaxCodeLength(ui->spinMaxCodeLength->value());
    format.setBreakAfterLogical(ui->chkBreakAfterLogical->isChecked());
}

