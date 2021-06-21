#include "editor.h"

#include <QtCore/QFileInfo>
#include <QFont>
#include <QTextCodec>
#include <QVariant>
#include <QWheelEvent>
#include <memory>
#include "settings.h"
#include "mainwindow.h"
#include "systemconsts.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QMimeData>
#include "qsynedit/highlighter/cpp.h"
#include "HighlighterManager.h"
#include "qsynedit/exporter/synrtfexporter.h"
#include "qsynedit/exporter/synhtmlexporter.h"
#include "qsynedit/Constants.h"
#include <QGuiApplication>
#include <QClipboard>


using namespace std;

SaveException::SaveException(const QString& reason) {
    mReason = reason;
}
SaveException::SaveException(const QString&& reason) {
    mReason = reason;
}
const QString& SaveException::reason() const  noexcept{
    return mReason;
}
const char *SaveException::what() const noexcept {
    return mReason.toLocal8Bit();
}

int Editor::newfileCount=0;

Editor::Editor(QWidget *parent):
    Editor(parent,QObject::tr("untitled"),ENCODING_SYSTEM_DEFAULT,false,true,nullptr)
{
}

Editor::Editor(QWidget *parent, const QString& filename,
                  const QByteArray& encoding,
                  bool inProject, bool isNew,
                  QTabWidget* parentPageControl):
  SynEdit(parent),
  mEncodingOption(encoding),
  mFilename(filename),
  mParentPageControl(parentPageControl),
  mInProject(inProject),
  mIsNew(isNew)
{
    if (mFilename.isEmpty()) {
        newfileCount++;
        mFilename = tr("untitled%1").arg(newfileCount);
    }
    QFileInfo fileInfo(mFilename);
    if (mParentPageControl!=nullptr) {
        mParentPageControl->addTab(this,QString());
        updateCaption();
    }
    PSynHighlighter highlighter;
    if (!isNew) {
        loadFile();
        highlighter = highlighterManager.getHighlighter(mFilename);
    } else {
        if (mEncodingOption == ENCODING_AUTO_DETECT)
            mFileEncoding = ENCODING_ASCII;
        else
            mFileEncoding = mEncodingOption;
        highlighter=highlighterManager.getCppHighlighter();
    }

    if (highlighter) {
        setHighlighter(highlighter);
        setUseCodeFolding(true);
    } else {
        setUseCodeFolding(false);
    }

    applySettings();
    applyColorScheme(pSettings->editor().colorScheme());

    connect(this,&SynEdit::statusChanged,this,&Editor::onStatusChanged);
}

Editor::~Editor() {
    if (mParentPageControl!=nullptr) {
        int index = mParentPageControl->indexOf(this);
        mParentPageControl->removeTab(index);
    }
    this->setParent(0);

}

void Editor::loadFile() {
    QFile file(mFilename);
    this->lines()->LoadFromFile(file,mEncodingOption,mFileEncoding);
    this->setModified(false);
    updateCaption();
    pMainWindow->updateForEncodingInfo();
}

void Editor::saveFile(const QString &filename) {
    QFile file(filename);
    this->lines()->SaveToFile(file,mEncodingOption,mFileEncoding);
    pMainWindow->updateForEncodingInfo();
}

void Editor::convertToEncoding(const QByteArray &encoding)
{
    mEncodingOption = encoding;
    setModified(true);
    save();
}

bool Editor::save(bool force, bool reparse) {
    if (this->mIsNew) {
        return saveAs();
    }
    QFileInfo info(mFilename);
    //is this file writable;
    if (!force && !info.isWritable()) {
        QMessageBox::critical(pMainWindow,tr("Error"),
                                 tr("File %1 is not writable!").arg(mFilename));
        return false;
    }
    if (this->modified()|| force) {
        try {
            saveFile(mFilename);
            setModified(false);
            mIsNew = false;
            this->updateCaption();
        }  catch (SaveException& exception) {
            QMessageBox::critical(pMainWindow,tr("Error"),
                                     exception.reason());
            return false;
        }
    }

    if (reparse) {
        //todo: reparse the file
    }
    return true;
}

bool Editor::saveAs(){
    QString selectedFileFilter = pSystemConsts->defaultFileFilter();
    QString newName = QFileDialog::getSaveFileName(pMainWindow,
        tr("Save As"), QString(), pSystemConsts->defaultFileFilters().join(";;"),
        &selectedFileFilter);
    if (newName.isEmpty()) {
        return false;
    }
    try {
        mFilename = newName;
        saveFile(mFilename);
        mIsNew = false;
        setModified(false);
        this->updateCaption();
    }  catch (SaveException& exception) {
        QMessageBox::critical(pMainWindow,tr("Error"),
                                 exception.reason());
        return false;
    }

    //todo: update (reassign highlighter)
    //todo: remove old file from parser and reparse file
    //todo: unmoniter/ monitor file
    //todo: update windows caption
    //todo: update class browser;
    return true;
}

void Editor::activate()
{
    if (mParentPageControl!=nullptr)
        mParentPageControl->setCurrentWidget(this);
    setFocus();
}

const QByteArray& Editor::encodingOption() const noexcept{
    return mEncodingOption;
}
void Editor::setEncodingOption(const QByteArray& encoding) noexcept{
    mEncodingOption = encoding;
    if (!isNew())
        loadFile();
    else
        pMainWindow->updateForEncodingInfo();
}
const QByteArray& Editor::fileEncoding() const noexcept{
    return mFileEncoding;
}
const QString& Editor::filename() const noexcept{
    return mFilename;
}
bool Editor::inProject() const noexcept{
    return mInProject;
}
bool Editor::isNew() const noexcept {
    return mIsNew;
}

QTabWidget* Editor::pageControl() noexcept{
    return mParentPageControl;
}

void Editor::wheelEvent(QWheelEvent *event) {
    if ( (event->modifiers() & Qt::ControlModifier)!=0) {
        int size = pSettings->editor().fontSize();
        if (event->angleDelta().y()>0) {
            size = std::min(99,size+1);
            pSettings->editor().setFontSize(size);
            pMainWindow->updateEditorSettings();
            event->accept();
            return;
        } else {
            size = std::max(2,size-1);
            pSettings->editor().setFontSize(size);
            pMainWindow->updateEditorSettings();
            event->accept();
            return;
        }
    }
    SynEdit::wheelEvent(event);
}

void Editor::focusInEvent(QFocusEvent *event)
{
    SynEdit::focusInEvent(event);
    pMainWindow->updateEditorActions();
    pMainWindow->updateStatusbarForLineCol();
    pMainWindow->updateForStatusbarModeInfo();
}

void Editor::focusOutEvent(QFocusEvent *event)
{
    SynEdit::focusOutEvent(event);
    pMainWindow->updateEditorActions();
    pMainWindow->updateStatusbarForLineCol();
    pMainWindow->updateForStatusbarModeInfo();
}

void Editor::copyToClipboard()
{
    if (pSettings->editor().copySizeLimit()) {
        if (lines()->count() > pSettings->editor().copyLineLimits()) {
            QMessageBox::critical(pMainWindow,tr("Error"),
                                     tr("The text to be copied exceeds count limit!"));
            return;
        }
        if (lines()->getTextLength() > pSettings->editor().copyCharLimits() * 1000) {
            QMessageBox::critical(pMainWindow,tr("Error"),
                                     tr("The text to be copied exceeds character limit!"));
            return;
        }
    }
    switch(pSettings->editor().copyWithFormatAs()) {
    case 1: //HTML
        copyAsHTML();
        break;;
    default:
        SynEdit::copyToClipboard();
    }
}

void Editor::cutToClipboard()
{
    if (pSettings->editor().copySizeLimit()) {
        if (lines()->count() > pSettings->editor().copyLineLimits()) {
            QMessageBox::critical(pMainWindow,tr("Error"),
                                     tr("The text to be cut exceeds count limit!"));
            return;
        }
        if (lines()->getTextLength() > pSettings->editor().copyCharLimits() * 1000) {
            QMessageBox::critical(pMainWindow,tr("Error"),
                                     tr("The text to be cut exceeds character limit!"));
            return;
        }
    }
    SynEdit::cutToClipboard();
}

void Editor::copyAsHTML()
{
    if (!selAvail())
        return;
    SynHTMLExporter SynExporterHTML;

    SynExporterHTML.setTitle(QFileInfo(mFilename).fileName());
    SynExporterHTML.setExportAsText(false);
    SynExporterHTML.setUseBackground(pSettings->editor().copyHTMLUseBackground());
    SynExporterHTML.setFont(font());
    PSynHighlighter hl = highlighter();
    if (!pSettings->editor().copyHTMLUseEditorColor()) {
        hl = highlighterManager.copyHighlighter(highlighter());
        highlighterManager.applyColorScheme(hl,pSettings->editor().copyHTMLColorScheme());
    }
    SynExporterHTML.setHighlighter(hl);
    SynExporterHTML.setCreateHTMLFragment(true);

    SynExporterHTML.ExportRange(lines(),blockBegin(),blockEnd());

    QMimeData * mimeData = new QMimeData;

    //sethtml will convert buffer to QString , which will cause encoding trouble
    mimeData->setData(SynExporterHTML.clipboardFormat(),SynExporterHTML.buffer());
    mimeData->setText(selText());

    QGuiApplication::clipboard()->clear();
    QGuiApplication::clipboard()->setMimeData(mimeData);
}

void Editor::onModificationChanged(bool) {
    updateCaption();
}

void Editor::onStatusChanged(SynStatusChanges changes)
{
//    if (not (scOpenFile in Changes)) and  (fText.Lines.Count <> fLineCount)
//      and (fText.Lines.Count <> 0) and ((fLineCount>0) or (fText.Lines.Count>1)) then begin
//      if devCodeCompletion.Enabled
//        and SameStr(mainForm.ClassBrowser.CurrentFile,FileName) // Don't reparse twice
//        then begin
//        Reparse;
//      end;
//      if fText.Focused and devEditor.AutoCheckSyntax and devEditor.CheckSyntaxWhenReturn
//        and (fText.Highlighter = dmMain.Cpp) then begin
//        mainForm.CheckSyntaxInBack(self);
//      end;
//    end;
//    fLineCount := fText.Lines.Count;
//    // scModified is only fired when the modified state changes
    if (changes.testFlag(scModified)) {
        updateCaption();
    }

//    if (fTabStopBegin >=0) and (fTabStopY=fText.CaretY) then begin
//      if StartsStr(fLineBeforeTabStop,fText.LineText) and EndsStr(fLineAfterTabStop, fText.LineText) then
//        fTabStopBegin := Length(fLineBeforeTabStop);
//        if fLineAfterTabStop = '' then
//          fTabStopEnd := Length(fText.LineText)+1
//        else
//          fTabStopEnd := Length(fText.LineText) - Length(fLineAfterTabStop);
//        fXOffsetSince := fTabStopEnd - fText.CaretX;
//        if (fText.CaretX < fTabStopBegin) or (fText.CaretX >  (fTabStopEnd+1)) then begin
//          fTabStopBegin :=-1;
//        end;
//    end;

    // scSelection includes anything caret related
    if (changes.testFlag(SynStatusChange::scSelection)) {
        pMainWindow->updateStatusbarForLineCol();

//      // Update the function tip
//      fFunctionTip.ForceHide := false;
//      if Assigned(fFunctionTipTimer) then begin
//        if fFunctionTip.Activated and FunctionTipAllowed then begin
//          fFunctionTip.Parser := fParser;
//          fFunctionTip.FileName := fFileName;
//          fFunctionTip.Show;
//        end else begin // Reset the timer
//          fFunctionTipTimer.Enabled := false;
//          fFunctionTipTimer.Enabled := true;
//        end;
    }

//      // Remove error line colors
//      if not fIgnoreCaretChange then begin
//        if (fErrorLine <> -1) and not fText.SelAvail then begin
//          fText.InvalidateLine(fErrorLine);
//          fText.InvalidateGutterLine(fErrorLine);
//          fErrorLine := -1;
//        end;
//      end else
//        fIgnoreCaretChange := false;

//      if fText.SelAvail then begin
//        if fText.GetWordAtRowCol(fText.CaretXY) = fText.SelText then begin
//          fSelChanged:=True;
//          BeginUpdate;
//          EndUpdate;
//        end else if fSelChanged then begin
//          fSelChanged:=False; //invalidate to unhighlight others
//          BeginUpdate;
//          EndUpdate;
//        end;
//      end else if fSelChanged then begin
//        fSelChanged:=False; //invalidate to unhighlight others
//        BeginUpdate;
//        EndUpdate;
//      end;
//  end;

    if (changes.testFlag(scInsertMode) | changes.testFlag(scReadOnly))
        pMainWindow->updateForStatusbarModeInfo();

    pMainWindow->updateEditorActions();

//    mainForm.CaretList.AddCaret(self,fText.CaretY,fText.CaretX);
}

void Editor::applySettings()
{
    SynEditorOptions options = eoAltSetsColumnMode |
            eoDragDropEditing | eoDropFiles |  eoKeepCaretX | eoTabsToSpaces |
            eoRightMouseMovesCursor | eoScrollByOneLess | eoTabIndent | eoHideShowScrollbars;

    //options
    options.setFlag(eoAddIndent,pSettings->editor().addIndent());
    options.setFlag(eoAutoIndent,pSettings->editor().autoIndent());
    options.setFlag(eoTabsToSpaces,pSettings->editor().tabToSpaces());

    options.setFlag(eoKeepCaretX,pSettings->editor().keepCaretX());
    options.setFlag(eoEnhanceHomeKey,pSettings->editor().enhanceHomeKey());
    options.setFlag(eoEnhanceEndKey,pSettings->editor().enhanceEndKey());

    options.setFlag(eoHideShowScrollbars,pSettings->editor().autoHideScrollbar());
    options.setFlag(eoScrollPastEol,pSettings->editor().scrollPastEol());
    options.setFlag(eoScrollPastEof,pSettings->editor().scrollPastEof());
    options.setFlag(eoScrollByOneLess,pSettings->editor().scrollByOneLess());
    options.setFlag(eoHalfPageScroll,pSettings->editor().halfPageScroll());
    setOptions(options);

    setTabWidth(pSettings->editor().tabWidth());
    setInsertCaret(pSettings->editor().caretForInsert());
    setOverwriteCaret(pSettings->editor().caretForOverwrite());
    setCaretColor(pSettings->editor().caretColor());

    QFont f=QFont(pSettings->editor().fontName(),pSettings->editor().fontSize());
    f.setStyleStrategy(QFont::PreferAntialias);
    setFont(f);

    // Set gutter properties
    gutter().setLeftOffset(pSettings->editor().gutterLeftOffset());
    gutter().setRightOffset(pSettings->editor().gutterRightOffset());
    gutter().setBorderStyle(SynGutterBorderStyle::None);
    gutter().setUseFontStyle(pSettings->editor().gutterUseCustomFont());
    if (pSettings->editor().gutterUseCustomFont()) {
        f=QFont(pSettings->editor().gutterFontName(),pSettings->editor().gutterFontSize());
    } else {
        f=QFont(pSettings->editor().fontName(),pSettings->editor().fontSize());
    }
    f.setStyleStrategy(QFont::PreferAntialias);
    gutter().setFont(f);
    gutter().setDigitCount(pSettings->editor().gutterDigitsCount());
    gutter().setVisible(pSettings->editor().gutterVisible());
    gutter().setAutoSize(pSettings->editor().gutterAutoSize());
    gutter().setShowLineNumbers(pSettings->editor().gutterShowLineNumbers());
    gutter().setLeadingZeros(pSettings->editor().gutterAddLeadingZero());
    if (pSettings->editor().gutterLineNumbersStartZero())
        gutter().setLineNumberStart(0);
    else
        gutter().setLineNumberStart(1);
    //font color

}

void Editor::applyColorScheme(const QString& schemeName)
{
    highlighterManager.applyColorScheme(highlighter(),schemeName);
    PColorSchemeItem item = pColorManager->getItem(schemeName,COLOR_SCHEME_ACTIVE_LINE);
    if (item) {
        setActiveLineColor(item->background());
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_GUTTER);
    if (item) {
        gutter().setTextColor(item->foreground());
        gutter().setColor(item->background());
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_FOLD_LINE);
    if (item) {
        //todo
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_INDENT_GUIDE_LINE);
    if (item) {
        //todo
    }
    this->invalidate();
}

void Editor::updateCaption(const QString& newCaption) {
    if (mParentPageControl==nullptr) {
        return;
    }
    int index = mParentPageControl->indexOf(this);
    if (index==-1)
        return;
    if (newCaption.isEmpty()) {
        QString caption = QFileInfo(mFilename).fileName();
        if (this->modified()) {
            caption.append("[*]");
        }
        mParentPageControl->setTabText(index,caption);
    } else {
        mParentPageControl->setTabText(index,newCaption);
    }

}
