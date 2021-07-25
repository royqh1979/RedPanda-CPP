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
#include <QPainter>
#include "iconsmanager.h"


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
  mIsNew(isNew),
  mSyntaxErrorColor(QColorConstants::Red),
  mSyntaxWarningColor("orange"),
  mLineCount(0),
  mActiveBreakpointLine(-1)
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
    connect(this,&SynEdit::gutterClicked,this,&Editor::onGutterClicked);
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
    if (pSettings->editor().syntaxCheck() && pSettings->editor().syntaxCheckWhenSave())
        pMainWindow->checkSyntaxInBack(this);
}

void Editor::saveFile(const QString &filename) {
    QFile file(filename);
    this->lines()->SaveToFile(file,mEncodingOption,mFileEncoding);
    pMainWindow->updateForEncodingInfo();
    if (pSettings->editor().syntaxCheck() && pSettings->editor().syntaxCheckWhenSave())
        pMainWindow->checkSyntaxInBack(this);
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

void Editor::undoSymbolCompletion(int pos)
{
    PSynHighlighterAttribute Attr;
    QString Token;
    bool tokenFinished;
    SynHighlighterTokenType tokenType;

    if (!highlighter())
        return;
    if (!pSettings->editor().removeSymbolPairs())
        return;
    if (!GetHighlighterAttriAtRowCol(caretXY(), Token, tokenFinished, tokenType, Attr))
        return;
    if ((tokenType == SynHighlighterTokenType::Comment) && (!tokenFinished))
        return ;
    //convert caret x to string index;
    pos--;

    if (pos<0 || pos+1>=lineText().length())
        return;
    QChar DeletedChar = lineText()[pos];
    QChar NextChar = lineText()[pos+1];
    if ((tokenType == SynHighlighterTokenType::Character) && (DeletedChar != '\''))
        return;
    if (tokenType == SynHighlighterTokenType::StringEscapeSequence)
        return;
    if (tokenType == SynHighlighterTokenType::String) {
        if ((DeletedChar!='"') && (DeletedChar!='('))
            return;
        if ((DeletedChar=='"') && (Token!="\"\""))
            return;
        if ((DeletedChar=='(') && (!Token.startsWith("R\"")))
            return;
    }
    if ((DeletedChar == '\'') && (tokenType == SynHighlighterTokenType::Number))
        return;
    if ((DeletedChar == '<') &&
            ((tokenType != SynHighlighterTokenType::PreprocessDirective)
             || !lineText().startsWith("#include")))
        return;
    if ( (pSettings->editor().completeBracket() && (DeletedChar == '[') && (NextChar == ']')) ||
         (pSettings->editor().completeParenthese() && (DeletedChar == '(') && (NextChar == ')')) ||
         (pSettings->editor().completeGlobalInclude() && (DeletedChar == '<') && (NextChar == '>')) ||
         (pSettings->editor().completeBrace() && (DeletedChar == '{') && (NextChar == '}')) ||
         (pSettings->editor().completeSingleQuote() && (DeletedChar == '\'') && (NextChar == '\'')) ||
         (pSettings->editor().completeDoubleQuote() && (DeletedChar == '\"') && (NextChar == '\"'))) {
         CommandProcessor(SynEditorCommand::ecDeleteChar);
    }
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
        } else if  (event->angleDelta().y()<0) {
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

void Editor::keyPressEvent(QKeyEvent *event)
{
    bool handled = false;
    switch (event->key()) {
    case Qt::Key_Delete:
        // remove completed character
        //fLastIdCharPressed:=0;
        undoSymbolCompletion(caretX());
        break;;
    case Qt::Key_Backspace:
        // remove completed character
        //fLastIdCharPressed:=0;
        undoSymbolCompletion(caretX()-1);
        break;;
    default: {
        QString t = event->text();
        if (!t.isEmpty()) {
            QChar ch = t[0];
            switch (ch.unicode()) {
            case '"':
            case '\'':
            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case '<':
            case '>':
            case '*':
                handled = handleSymbolCompletion(ch);
            }
        }
    }
    }
    if (!handled) {
        SynEdit::keyPressEvent(event);
    } else {
        event->accept();
    }
}

void Editor::onGutterPaint(QPainter &painter, int aLine, int X, int Y)
{
    // Get point where to draw marks
    //X := (fText.Gutter.RealGutterWidth(fText.CharWidth) - fText.Gutter.RightOffset) div 2 - 3;
    X = 5;
    Y += (this->textHeight() - 16) / 2;

    PSyntaxIssueList lst = getSyntaxIssuesAtLine(aLine);
    if (lst) {
        bool hasError=false;
        for (PSyntaxIssue issue : *lst) {
            if (issue->issueType == CompileIssueType::Error) {
                hasError = true;
                break;;
            }
        }
        if (hasError) {
            painter.drawPixmap(X,Y,*(pIconsManager->syntaxError()));
        } else {
            painter.drawPixmap(X,Y,*(pIconsManager->syntaxWarning()));
        }
        return;
    }

    if (mActiveBreakpointLine == aLine) {
        painter.drawPixmap(X,Y,*(pIconsManager->activeBreakpoint()));
    } else if (hasBreakpoint(aLine)) {
        painter.drawPixmap(X,Y,*(pIconsManager->breakpoint()));
    }
//   if fActiveLine = Line then begin // prefer active line over breakpoints
//        dmMain.GutterImages.Draw(ACanvas, X, Y, 1);
//        drawn:=True;
//      end else if HasBreakpoint(Line) <> -1 then begin
//        dmMain.GutterImages.Draw(ACanvas, X, Y, 0);
//        drawn:=True;
//      end else if fErrorLine = Line then begin
//        dmMain.GutterImages.Draw(ACanvas, X, Y, 2);
//        drawn:=True;
//      end;
//      idx := CBUtils.FastIndexOf(fErrorList, Line);
//      if idx>=0 then begin
//        isError := False;
//        lst:=TList(fErrorList.Objects[idx]);
//        for j:=0 to lst.Count-1 do begin
//          if PSyntaxError(lst[j])^.errorType = setError then begin
//            isError := True;
//            break;
//          end;
//        end;
//        if isError then
//          dmMain.GutterImages.Draw(ACanvas, X, Y, 2)
//        else if not drawn then
//          dmMain.GutterImages.Draw(ACanvas, X, Y, 3);
//      end;

//      Inc(Y, fText.LineHeight);
//    end;

}

void Editor::onGetEditingAreas(int Line, SynEditingAreaList &areaList)
{
    areaList.clear();
//    if (fTabStopBegin >=0) and (fTabStopY=Line) then begin
//      areaType:=eatEditing;
//      System.new(p);
//      spaceCount := fText.LeftSpacesEx(fLineBeforeTabStop,True);
//      spaceBefore := Length(fLineBeforeTabStop) - Length(TrimLeft(fLineBeforeTabStop));
//      p.beginX := fTabStopBegin + spaceCount - spaceBefore ;
//      p.endX := fTabStopEnd + spaceCount - spaceBefore ;
//      p.color := dmMain.Cpp.StringAttri.Foreground;
//      areaList.Add(p);
//      ColBorder := dmMain.Cpp.StringAttri.Foreground;
//      Exit;
//    end;
//    StrToThemeColor(tc,devEditor.Syntax.Values[cWN]);
    PSyntaxIssueList lst = getSyntaxIssuesAtLine(Line);
    if (lst) {
        for (PSyntaxIssue issue: *lst) {
            PSynEditingArea p=std::make_shared<SynEditingArea>();
            p->beginX = issue->col;
            p->endX = issue->endCol;
            if (issue->issueType == CompileIssueType::Error) {
                p->color = mSyntaxErrorColor;
            } else {
                p->color = mSyntaxWarningColor;
            }
            p->type = SynEditingAreaType::eatWaveUnderLine;
            areaList.append(p);
        }
    }
}

bool Editor::onGetSpecialLineColors(int Line, QColor &foreground, QColor &backgroundColor)
{
    if (Line == mActiveBreakpointLine) {
        foreground = mActiveBreakpointForegroundColor;
        backgroundColor = mActiveBreakpointBackgroundColor;
    } else if (hasBreakpoint(Line)) {
        foreground = mBreakpointForegroundColor;
        backgroundColor = mBreakpointBackgroundColor;
    }
//    end else if Line = fErrorLine then begin
//      StrToThemeColor(tc,  devEditor.Syntax.Values[cErr]);
//      BG := tc.Background;
//      FG := tc.Foreground;
//      if (BG <> clNone) or (FG<>clNone) then
//        Special := TRUE;
//    end;
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

void Editor::setCaretPosition(int line, int col)
{
    this->uncollapseAroundLine(line);
    this->setCaretXYCentered(true,BufferCoord{col,line});
}

void Editor::setCaretPositionAndActivate(int line, int col)
{
    this->uncollapseAroundLine(line);
    if (!this->hasFocus())
        this->activate();
    this->setCaretXYCentered(true,BufferCoord{col,line});
}

void Editor::addSyntaxIssues(int line, int startChar, int endChar, CompileIssueType errorType, const QString &hint)
{
    PSyntaxIssue pError;
    BufferCoord p;
    QString token;
    SynHighlighterTokenType tokenType;
    int tokenKind,start;
    PSynHighlighterAttribute attr;
    PSyntaxIssueList lst;
    if ((line<1) || (line>lines()->count()))
        return;
    pError = std::make_shared<SyntaxIssue>();
    p.Char = startChar;
    p.Line = line;
    if (startChar >= lines()->getString(line-1).length()) {
        start = 1;
        token = lines()->getString(line-1);
    } else if (endChar < 1) {
        if (!GetHighlighterAttriAtRowColEx(p,token,tokenType,tokenKind,start,attr))
            return;
    } else {
        start = startChar;
        token = lines()->getString(line-1).mid(start-1,endChar-startChar);
    }
    pError->startChar = start;
    pError->endChar = start + token.length();
    pError->col = charToColumn(line,pError->startChar);
    pError->endCol = charToColumn(line,pError->endChar);
    pError->hint = hint;
    pError->token = token;
    pError->issueType = errorType;
    if (mSyntaxIssues.contains(line)) {
        lst = mSyntaxIssues[line];
    } else {
        lst = std::make_shared<SyntaxIssueList>();
        mSyntaxIssues[line] = lst;
    }
    lst->append(pError);
}

void Editor::clearSyntaxIssues()
{
    mSyntaxIssues.clear();
}

void Editor::gotoNextSyntaxIssue()
{
    auto iter = mSyntaxIssues.find(caretY());
    if (iter==mSyntaxIssues.end())
        return;
    iter++;
    if (iter==mSyntaxIssues.end())
        return;
    BufferCoord p;
    p.Char = (*iter)->at(0)->startChar;
    p.Line = iter.key();
    setCaretXY(p);
}

void Editor::gotoPrevSyntaxIssue()
{
    auto iter = mSyntaxIssues.find(caretY());
    if (iter==mSyntaxIssues.end())
        return;
    if (iter==mSyntaxIssues.begin())
        return;
    iter--;
    BufferCoord p;
    p.Char = (*iter)->at(0)->startChar;
    p.Line = iter.key();
    setCaretXY(p);

}

bool Editor::hasNextSyntaxIssue() const
{
    auto iter = mSyntaxIssues.find(caretY());
    if (iter==mSyntaxIssues.end())
        return false;
    iter++;
    if (iter==mSyntaxIssues.end())
        return false;
    return true;
}

bool Editor::hasPrevSyntaxIssue() const
{
    auto iter = mSyntaxIssues.find(caretY());
    if (iter==mSyntaxIssues.end())
        return true;
    if (iter==mSyntaxIssues.begin())
        return true;
    return false;
}

Editor::PSyntaxIssueList Editor::getSyntaxIssuesAtLine(int line)
{
    if (mSyntaxIssues.contains(line))
        return mSyntaxIssues[line];
    return PSyntaxIssueList();
}

Editor::PSyntaxIssue Editor::getSyntaxIssueAtPosition(const BufferCoord &pos)
{
    PSyntaxIssueList lst = getSyntaxIssuesAtLine(pos.Line);
    for (PSyntaxIssue issue: *lst) {
        if (issue->startChar<=pos.Char && pos.Char<=issue->endChar)
            return issue;
    }
    return PSyntaxIssue();
}

void Editor::onModificationChanged(bool) {
    updateCaption();
}

void Editor::onStatusChanged(SynStatusChanges changes)
{
    if (!changes.testFlag(SynStatusChange::scOpenFile)
            && !changes.testFlag(SynStatusChange::scReadOnly)
            && !changes.testFlag(SynStatusChange::scInsertMode)
            && (lines()->count()!=mLineCount)
            && (lines()->count()!=0) && ((mLineCount>0) || (lines()->count()>1))) {
        if (!readOnly() && pSettings->editor().syntaxCheck() && pSettings->editor().syntaxCheckWhenLineChanged())
            pMainWindow->checkSyntaxInBack(this);
    }
    mLineCount = lines()->count();
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

void Editor::onGutterClicked(Qt::MouseButton button, int x, int y, int line)
{
    if (button == Qt::LeftButton) {
        toggleBreakpoint(line);
    }
    mGutterClickedLine = line;
}

QChar Editor::getCurrentChar()
{
    if (lineText().length()<caretX())
        return QChar();
    else
        return lineText()[caretX()-1];
}

bool Editor::handleSymbolCompletion(QChar key)
{
    if (!pSettings->editor().completeSymbols() || selAvail())
        return false;
    if (!insertMode())
        return false;

    //todo: better methods to detect current caret type
    if (caretX() <= 1) {
        if (caretY()>1) {
            if (highlighter()->isLastLineCommentNotFinished(lines()->ranges(caretY() - 2).state))
                return false;
            if (highlighter()->isLastLineStringNotFinished(lines()->ranges(caretY() - 2).state)
                    && (key!='\"') && (key!='\''))
                return false;
        }
    } else {
        BufferCoord  HighlightPos = BufferCoord{caretX()-1, caretY()};
        // Check if that line is highlighted as  comment
        PSynHighlighterAttribute Attr;
        QString Token;
        bool tokenFinished;
        SynHighlighterTokenType tokenType;
        if (GetHighlighterAttriAtRowCol(HighlightPos, Token, tokenFinished, tokenType,Attr)) {
            if ((tokenType == SynHighlighterTokenType::Comment) && (!tokenFinished))
                return false;
            if ((tokenType == SynHighlighterTokenType::String) && (!tokenFinished)
                    && (key!='\'') && (key!='\"') && (key!='(') && (key!=')'))
                return false;
            if (( key=='<' || key =='>') && (tokenType != SynHighlighterTokenType::PreprocessDirective))
                return false;
            if ((key == '\'') && (Attr->name() == "SYNS_AttrNumber"))
                return false;
        }
    }

    // Check if that line is highlighted as string or character or comment
    //    if (Attr = fText.Highlighter.StringAttribute) or (Attr = fText.Highlighter.CommentAttribute) or SameStr(Attr.Name,
    //      'Character') then
    //      Exit;

    QuoteStatus status;
    switch(key.unicode()) {
    case '(':
        if (pSettings->editor().completeParenthese()) {
            return handleParentheseCompletion();
        }
        return false;
    case ')':
        if (pSettings->editor().completeParenthese() && pSettings->editor().overwriteSymbols()) {
            return handleParentheseSkip();
        }
        return false;
    case '[':
          if (pSettings->editor().completeBracket()) {
              return handleBracketCompletion();
          }
          return false;
    case ']':
          if (pSettings->editor().completeBracket() && pSettings->editor().overwriteSymbols()) {
              return handleBracketSkip();
          }
          return false;
    case '*':
          status = getQuoteStatus();
          if (pSettings->editor().completeComment() && (status == QuoteStatus::NotQuote)) {
              return handleMultilineCommentCompletion();
          }
          return false;
    case '{':
          if (pSettings->editor().completeBrace()) {
              return handleBraceCompletion();
          }
          return false;
    case '}':
        if (pSettings->editor().completeBrace() && pSettings->editor().overwriteSymbols()) {
            return handleBraceSkip();
        }
        return false;
    case '\'':
        if (pSettings->editor().completeSingleQuote()) {
            return handleSingleQuoteCompletion();
        }
        return false;
    case '\"':
        if (pSettings->editor().completeDoubleQuote()) {
            return handleDoubleQuoteCompletion();
        }
        return false;
    case '<':
        if (pSettings->editor().completeGlobalInclude()) { // #include <>
            return handleGlobalIncludeCompletion();
        }
        return false;
    case '>':
        if (pSettings->editor().completeGlobalInclude() && pSettings->editor().overwriteSymbols()) { // #include <>
            return handleGlobalIncludeSkip();
        }
        return false;
    }
    return false;
}

bool Editor::handleParentheseCompletion()
{
    QuoteStatus status = getQuoteStatus();
    if (status == QuoteStatus::RawString || status == QuoteStatus::NotQuote) {
        beginUpdate();
        CommandProcessor(SynEditorCommand::ecChar,'(');
        BufferCoord oldCaret = caretXY();
        CommandProcessor(SynEditorCommand::ecChar,')');
        setCaretXY(oldCaret);
        endUpdate();
        return true;
    }
//    if (status == QuoteStatus::NotQuote) && FunctionTipAllowed then
    //        fFunctionTip.Activated := true;
    return false;
}

bool Editor::handleParentheseSkip()
{
      if (getCurrentChar() != ')')
          return false;
      QuoteStatus status = getQuoteStatus();
      if (status == QuoteStatus::RawStringNoEscape) {
          setCaretXY( BufferCoord{caretX() + 1, caretY()}); // skip over
          return true;
      }
      if (status != QuoteStatus::NotQuote)
          return false;
      BufferCoord pos = getMatchingBracket();
      if (pos.Line != 0) {
          setCaretXY( BufferCoord{caretX() + 1, caretY()}); // skip over
          return true;
      }
//      if FunctionTipAllowed then
      //        fFunctionTip.Activated := false;
      return false;
}

bool Editor::handleBracketCompletion()
{
//    QuoteStatus status = getQuoteStatus();
//    if (status == QuoteStatus::RawString || status == QuoteStatus::NotQuote) {
    beginUpdate();
    CommandProcessor(SynEditorCommand::ecChar,'[');
    BufferCoord oldCaret = caretXY();
    CommandProcessor(SynEditorCommand::ecChar,']');
    setCaretXY(oldCaret);
    endUpdate();
    return true;
        //    }
}

bool Editor::handleBracketSkip()
{
    if (getCurrentChar() != ']')
        return false;
    BufferCoord pos = getMatchingBracket();
    if (pos.Line != 0) {
        setCaretXY( BufferCoord{caretX() + 1, caretY()}); // skip over
        return true;
    }
    return false;
}

bool Editor::handleMultilineCommentCompletion()
{
    if (((caretX() > 1) && (caretX()-1 < lineText().length())) && (lineText()[caretX() - 1] == '/')) {
        beginUpdate();
        CommandProcessor(SynEditorCommand::ecChar,'*');
        BufferCoord oldCaret = caretXY();
        CommandProcessor(SynEditorCommand::ecChar,'*');
        CommandProcessor(SynEditorCommand::ecChar,'/');
        setCaretXY(oldCaret);
        endUpdate();
        return true;
    }
    return false;
}

bool Editor::handleBraceCompletion()
{
    QString s = lineText().trimmed();
    int i= caretY()-2;
    while ((s.isEmpty()) && (i>=0)) {
        s=lines()->getString(i);
        i--;
    }
    beginUpdate();
    CommandProcessor(SynEditorCommand::ecChar,'{');
    BufferCoord oldCaret = caretXY();
    CommandProcessor(SynEditorCommand::ecChar,'}');
    if (
        ( (s.startsWith("struct")
          || s.startsWith("class")
          || s.startsWith("union")
          || s.startsWith("typedef")
          || s.startsWith("public")
          || s.startsWith("private")
          || s.startsWith("enum") )
          && !s.contains(';')
        ) || s.endsWith('=')) {
        CommandProcessor(SynEditorCommand::ecChar,';');
    }
    setCaretXY(oldCaret);
    endUpdate();
    return true;
}

bool Editor::handleBraceSkip()
{
    if (getCurrentChar() != '}')
        return false;
    BufferCoord pos = getMatchingBracket();
    if (pos.Line != 0) {
        setCaretXY( BufferCoord{caretX() + 1, caretY()}); // skip over
        return true;
    }
    return false;
}

bool Editor::handleSingleQuoteCompletion()
{
    QuoteStatus status = getQuoteStatus();
    QChar ch = getCurrentChar();
    if (ch == '\'') {
        if (status == QuoteStatus::SingleQuote) {
            setCaretXY( BufferCoord{caretX() + 1, caretY()}); // skip over
            return true;
        }
    } else {
        if (status == QuoteStatus::NotQuote) {
            if (ch == 0 || highlighter()->isWordBreakChar(ch) || highlighter()->isSpaceChar(ch)) {
                // insert ''
                beginUpdate();
                CommandProcessor(SynEditorCommand::ecChar,'\'');
                BufferCoord oldCaret = caretXY();
                CommandProcessor(SynEditorCommand::ecChar,'\'');
                setCaretXY(oldCaret);
                endUpdate();
                return true;
            }
        }
    }
    return false;
}

bool Editor::handleDoubleQuoteCompletion()
{
    QuoteStatus status = getQuoteStatus();
    QChar ch = getCurrentChar();
    if (ch == '"') {
        if (status == QuoteStatus::DoubleQuote || status == QuoteStatus::RawString) {
            setCaretXY( BufferCoord{caretX() + 1, caretY()}); // skip over
            return true;
        }
    } else {
        if (status == QuoteStatus::NotQuote) {
            if ((ch == 0) || highlighter()->isWordBreakChar(ch) || highlighter()->isSpaceChar(ch)) {
                // insert ""
                beginUpdate();
                CommandProcessor(SynEditorCommand::ecChar,'"');
                BufferCoord oldCaret = caretXY();
                CommandProcessor(SynEditorCommand::ecChar,'"');
                setCaretXY(oldCaret);
                endUpdate();
                return true;
            }
        }
    }
    return false;
}

bool Editor::handleGlobalIncludeCompletion()
{
    if (!lineText().startsWith('#'))
        return false;
    QString s= lineText().mid(1).trimmed();
    if (!s.startsWith("include"))  //it's not #include
        return false;
    beginUpdate();
    CommandProcessor(SynEditorCommand::ecChar,'<');
    BufferCoord oldCaret = caretXY();
    CommandProcessor(SynEditorCommand::ecChar,'>');
    setCaretXY(oldCaret);
    endUpdate();
    return true;
}

bool Editor::handleGlobalIncludeSkip()
{
    if (getCurrentChar()!='>')
        return false;
    QString s= lineText().mid(1).trimmed();
    if (!s.startsWith("include"))  //it's not #include
        return false;
    BufferCoord pos = getMatchingBracket();
    if (pos.Line != 0) {
        setCaretXY( BufferCoord{caretX() + 1, caretY()}); // skip over
        return true;
    }
    return false;
}

Editor::QuoteStatus Editor::getQuoteStatus()
{
    QuoteStatus Result = QuoteStatus::NotQuote;
    if ((caretY()>1) && highlighter()->isLastLineStringNotFinished(lines()->ranges(caretY() - 2).state))
        Result = QuoteStatus::DoubleQuote;

    QString Line = lines()->getString(caretY()-1);
    int posX = caretX()-1;
    if (posX >= Line.length()) {
        posX = Line.length()-1;
    }
    for (int i=0; i<posX;i++) {
        if (i+1<Line.length() && (Line[i] == 'R') && (Line[i+1] == '"') && (Result == QuoteStatus::NotQuote)) {
            Result = QuoteStatus::RawString;
            i++; // skip R
        } else if (Line[i] == '(') {
            switch(Result) {
            case QuoteStatus::RawString:
                Result=QuoteStatus::RawStringNoEscape;
                break;
            //case RawStringNoEscape: do nothing
            }
        } else if (Line[i] == ')') {
            switch(Result) {
            case QuoteStatus::RawStringNoEscape:
                Result=QuoteStatus::RawString;
                break;
            }
        } else if (Line[i] == '"') {
            switch(Result) {
            case QuoteStatus::NotQuote:
                Result = QuoteStatus::DoubleQuote;
                break;
            case QuoteStatus::SingleQuote:
                Result = QuoteStatus::SingleQuote;
                break;
            case QuoteStatus::SingleQuoteEscape:
                Result = QuoteStatus::SingleQuote;
                break;
            case QuoteStatus::DoubleQuote:
                Result = QuoteStatus::NotQuote;
                break;
            case QuoteStatus::DoubleQuoteEscape:
                Result = QuoteStatus::DoubleQuote;
                break;
            case QuoteStatus::RawString:
                Result=QuoteStatus::NotQuote;
                break;
            //RawStringNoEscape: do nothing
            }
        } else if (Line[i] == '\'') {
            switch(Result) {
            case QuoteStatus::NotQuote:
                Result = QuoteStatus::SingleQuote;
                break;
            case QuoteStatus::SingleQuote:
                Result = QuoteStatus::NotQuote;
                break;
            case QuoteStatus::SingleQuoteEscape:
                Result = QuoteStatus::SingleQuote;
                break;
            case QuoteStatus::DoubleQuote:
                Result = QuoteStatus::DoubleQuote;
                break;
            case QuoteStatus::DoubleQuoteEscape:
                Result = QuoteStatus::DoubleQuote;
                break;
            }
        } else if (Line[i] == '\\') {
            switch(Result) {
            case QuoteStatus::NotQuote:
                Result = QuoteStatus::NotQuote;
                break;
            case QuoteStatus::SingleQuote:
                Result = QuoteStatus::SingleQuoteEscape;
                break;
            case QuoteStatus::SingleQuoteEscape:
                Result = QuoteStatus::SingleQuote;
                break;
            case QuoteStatus::DoubleQuote:
                Result = QuoteStatus::DoubleQuoteEscape;
                break;
            case QuoteStatus::DoubleQuoteEscape:
                Result = QuoteStatus::DoubleQuote;
                break;
            }
        } else {
            switch(Result) {
            case QuoteStatus::NotQuote:
                Result = QuoteStatus::NotQuote;
                break;
            case QuoteStatus::SingleQuote:
                Result = QuoteStatus::SingleQuote;
                break;
            case QuoteStatus::SingleQuoteEscape:
                Result = QuoteStatus::SingleQuote;
                break;
            case QuoteStatus::DoubleQuote:
                Result = QuoteStatus::DoubleQuote;
                break;
            case QuoteStatus::DoubleQuoteEscape:
                Result = QuoteStatus::DoubleQuote;
                break;
            }
        }
    }
    return Result;
}

int Editor::gutterClickedLine() const
{
    return mGutterClickedLine;
}

void Editor::toggleBreakpoint(int line)
{
    if (hasBreakpoint(line)) {
        mBreakpointLines.remove(line);
        //todo
       // MainForm.Debugger.RemoveBreakPoint(Line, self)
    } else {
        mBreakpointLines.insert(line);
        //todo
       // MainForm.Debugger.AddBreakPoint(Line, self);
    }

    invalidateGutterLine(line);
    invalidateLine(line);
}

bool Editor::hasBreakpoint(int line)
{
    return mBreakpointLines.contains(line);
}

void Editor::removeBreakpointFocus()
{
    if (mActiveBreakpointLine!=-1) {
        int oldLine = mActiveBreakpointLine;
        mActiveBreakpointLine = -1;
        invalidateGutterLine(oldLine);
        invalidateLine(oldLine);
    }
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
        codeFolding().folderBarLinesColor = item->foreground();
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_INDENT_GUIDE_LINE);
    if (item) {
        codeFolding().indentGuidesColor = item->foreground();
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_ERROR);
    if (item) {
        this->mSyntaxErrorColor = item->foreground();
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_WARNING);
    if (item) {
        this->mSyntaxWarningColor = item->foreground();
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_ACTIVE_BREAKPOINT);
    if (item) {
        this->mActiveBreakpointForegroundColor = item->foreground();
        this->mActiveBreakpointBackgroundColor = item->background();
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_BREAKPOINT);
    if (item) {
        this->mBreakpointForegroundColor = item->foreground();
        this->mBreakpointBackgroundColor = item->foreground();
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
