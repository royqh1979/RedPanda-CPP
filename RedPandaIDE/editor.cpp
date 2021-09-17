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
#include <QToolTip>
#include <QApplication>
#include <QInputDialog>
#include "iconsmanager.h"
#include "debugger.h"
#include "editorlist.h"
#include <QDebug>
#include "project.h"

using namespace std;

SaveException::SaveException(const QString& reason) {
    mReason = reason;
    mReasonBuffer = mReason.toLocal8Bit();
}
SaveException::SaveException(const QString&& reason) {
    mReason = reason;
    mReasonBuffer = mReason.toLocal8Bit();
}
const QString& SaveException::reason() const  noexcept{
    return mReason;
}
const char* SaveException::what() const noexcept {
    return mReasonBuffer;
}

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
  mSyntaxIssues(),
  mSyntaxErrorColor(QColorConstants::Red),
  mSyntaxWarningColor("orange"),
  mLineCount(0),
  mActiveBreakpointLine(-1),
  mLastIdCharPressed(0),
  mCurrentWord(),
  mCurrentTipType(TipType::None),
  mOldSelectionWord(),
  mSelectionWord()
{
    mUseCppSyntax = pSettings->editor().defaultFileCpp();
    if (mFilename.isEmpty()) {
        mFilename = tr("untitled")+QString("%1").arg(getNewFileNumber());
    }
    QFileInfo fileInfo(mFilename);
    if (mParentPageControl!=nullptr) {
        int index = mParentPageControl->addTab(this,QString());
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

    if (inProject) {
        mParser = pMainWindow->project()->cppParser();
    } else {
        initParser();
    }

    if (pSettings->editor().readOnlySytemHeader()
            && (mParser->isSystemHeaderFile(filename) || mParser->isProjectHeaderFile(filename))) {
        this->setModified(false);
        setReadOnly(true);
        updateCaption();
    }

    mCompletionPopup = pMainWindow->completionPopup();
    mHeaderCompletionPopup = pMainWindow->headerCompletionPopup();

    applySettings();
    applyColorScheme(pSettings->editor().colorScheme());

    connect(this,&SynEdit::statusChanged,this,&Editor::onStatusChanged);
    connect(this,&SynEdit::gutterClicked,this,&Editor::onGutterClicked);

    onStatusChanged(SynStatusChange::scOpenFile);

    setAttribute(Qt::WA_Hover,true);

    connect(this,&SynEdit::linesDeleted,
            this, &Editor::onLinesDeleted);
    connect(this,&SynEdit::linesInserted,
            this, &Editor::onLinesInserted);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            pMainWindow, &MainWindow::onEditorContextMenu);
}

Editor::~Editor() {
    pMainWindow->fileSystemWatcher()->removePath(mFilename);
    pMainWindow->caretList().removeEditor(this);
    pMainWindow->updateCaretActions();
    if (mParentPageControl!=nullptr) {
        int index = mParentPageControl->indexOf(this);
        mParentPageControl->removeTab(index);
    }
    this->setParent(nullptr);
}

void Editor::loadFile(const QString& filename) {
    if (filename.isEmpty()) {
        QFile file(mFilename);
        this->lines()->LoadFromFile(file,mEncodingOption,mFileEncoding);
    } else {
        QFile file(filename);
        this->lines()->LoadFromFile(file,mEncodingOption,mFileEncoding);
    }
    //this->setModified(false);
    updateCaption();
    pMainWindow->updateForEncodingInfo();
    switch(getFileType(mFilename)) {
    case FileType::CppSource:
        mUseCppSyntax = true;
        break;
    case FileType::CSource:
        mUseCppSyntax = false;
        break;
    default:
        mUseCppSyntax = pSettings->editor().defaultFileCpp();
    }
    if (highlighter() && mParser) {
        reparse();
        if (pSettings->editor().syntaxCheckWhenLineChanged()) {
            checkSyntaxInBack();
        }
    }
    mLastIdCharPressed = 0;
}

void Editor::saveFile(const QString &filename) {
    QFile file(filename);
    this->lines()->SaveToFile(file,mEncodingOption,mFileEncoding);
    pMainWindow->updateForEncodingInfo();
    if (pSettings->editor().syntaxCheckWhenSave())
        checkSyntaxInBack();
}

void Editor::convertToEncoding(const QByteArray &encoding)
{
    mEncodingOption = encoding;
    setModified(true);
    save();
}

bool Editor::save(bool force, bool doReparse) {
    if (this->mIsNew && !force) {
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
        pMainWindow->fileSystemWatcher()->removePath(mFilename);
        try {
            saveFile(mFilename);
            pMainWindow->fileSystemWatcher()->addPath(mFilename);
            setModified(false);
            mIsNew = false;
            this->updateCaption();
        }  catch (SaveException& exception) {
            if (!force) {
                QMessageBox::critical(pMainWindow,tr("Error"),
                                     exception.reason());
            }
            pMainWindow->fileSystemWatcher()->addPath(mFilename);
            return false;
        }
    }

    if (doReparse && mParser) {
        reparse();
    }
    return true;
}

bool Editor::saveAs(){
    QString selectedFileFilter;
    if (pSettings->editor().defaultFileCpp())
        selectedFileFilter = pSystemConsts->defaultCPPFileFilter();
    else
        selectedFileFilter = pSystemConsts->defaultCFileFilter();
    QFileDialog dialog(this,tr("Save As"),extractFilePath(mFilename),
                       pSystemConsts->defaultFileFilters().join(";;"));
    dialog.selectNameFilter(selectedFileFilter);
    dialog.selectFile(mFilename);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setOption(QFileDialog::DontConfirmOverwrite,false);

    if (!dialog.exec()) {
        return false;
    }
    QString newName = dialog.selectedFiles()[0];

    // Update project information
    if (mInProject && pMainWindow->project()) {
        int unitIndex = pMainWindow->project()->indexInUnits(mFilename);
        if (unitIndex>=0) {
            pMainWindow->project()->saveUnitAs(unitIndex,newName);
        }
    }

    pMainWindow->fileSystemWatcher()->removePath(mFilename);
    if (pSettings->codeCompletion().enabled() && mParser)
        mParser->invalidateFile(mFilename);
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
    pMainWindow->fileSystemWatcher()->addPath(mFilename);
    switch(getFileType(mFilename)) {
    case FileType::CppSource:
        mUseCppSyntax = true;
        break;
    case FileType::CSource:
        mUseCppSyntax = false;
        break;
    default:
        mUseCppSyntax = pSettings->editor().defaultFileCpp();
    }


    //update (reassign highlighter)
    PSynHighlighter newHighlighter = HighlighterManager().getHighlighter(mFilename);
    if (newHighlighter) {
        setUseCodeFolding(true);
    } else {
        setUseCodeFolding(false);
    }
    setHighlighter(newHighlighter);
    if (!newHighlighter || newHighlighter->getName() != SYN_HIGHLIGHTER_CPP) {
        mSyntaxIssues.clear();
    }
    applyColorScheme(pSettings->editor().colorScheme());

    reparse();

    if (pSettings->editor().syntaxCheckWhenSave())
        pMainWindow->checkSyntaxInBack(this);

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
    if (mInProject) {
        std::shared_ptr<Project> project = pMainWindow->project();
        if (project) {
            int index = project->indexInUnits(this);
            if (index>=0) {
                PProjectUnit unit = project->units()[index];
                unit->setEncoding(mEncodingOption);
            }
        }
    }
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
    QChar DeletedChar = lineText().at(pos);
    QChar NextChar = lineText().at(pos+1);
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
    if (mParser) {
        connect(mParser.get(),
                &CppParser::onEndParsing,
                this,
                &SynEdit::invalidate);
    }
    pMainWindow->updateEditorActions();
    pMainWindow->updateStatusbarForLineCol();
    pMainWindow->updateForStatusbarModeInfo();
    pMainWindow->updateClassBrowserForEditor(this);
}

void Editor::focusOutEvent(QFocusEvent *event)
{
    SynEdit::focusOutEvent(event);
    if (mParser) {
        disconnect(mParser.get(),
                &CppParser::onEndParsing,
                this,
                &SynEdit::invalidate);
    }
    pMainWindow->updateClassBrowserForEditor(nullptr);
    pMainWindow->updateEditorActions();
    pMainWindow->updateStatusbarForLineCol();
    pMainWindow->updateForStatusbarModeInfo();
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    bool handled = false;
    auto action = finally([&,this]{
        if (!handled) {
            SynEdit::keyPressEvent(event);
        } else {
            event->accept();
        }
    });
    if (readOnly())
        return;

    switch (event->key()) {
    case Qt::Key_Delete:
        // remove completed character
        mLastIdCharPressed = 0;
        undoSymbolCompletion(caretX());
        return;
    case Qt::Key_Backspace:
        // remove completed character
        mLastIdCharPressed = 0;
        undoSymbolCompletion(caretX()-1);
        return;
    }

    QString t = event->text();
    if (t.isEmpty())
        return;

    QChar ch = t[0];
    if (isIdentChar(ch)) {
        mLastIdCharPressed++;
        if (pSettings->codeCompletion().enabled()
                && pSettings->codeCompletion().showCompletionWhileInput() ) {
            if (mLastIdCharPressed==1) {
                if (mParser->isIncludeLine(lineText())) {
                    // is a #include line
                    setSelText(ch);
                    showHeaderCompletion(false);
                    handled=true;
                    return;
                } else {
                    QString lastWord = getPreviousWordAtPositionForSuggestion(caretXY());
                    if (!lastWord.isEmpty()) {
                        if (CppTypeKeywords.contains(lastWord)) {
                            //last word is a type keyword, this is a var or param define, and dont show suggestion
      //                  if devEditor.UseTabnine then
      //                    ShowTabnineCompletion;
                            return;
                        }
                        PStatement statement = mParser->findStatementOf(
                                    mFilename,
                                    lastWord,
                                    caretY());
                        StatementKind kind = mParser->getKindOfStatement(statement);
                        if (kind == StatementKind::skClass
                                || kind == StatementKind::skEnumClassType
                                || kind == StatementKind::skEnumType
                                || kind == StatementKind::skTypedef) {
                            //last word is a typedef/class/struct, this is a var or param define, and dont show suggestion
      //                      if devEditor.UseTabnine then
      //                        ShowTabnineCompletion;
                            return;
                        }
                    }
                    setSelText(ch);
                    showCompletion(false);
                    handled=true;
                    return;
                }
            }

        }
    } else {
        //preprocessor ?
        if ((mLastIdCharPressed=0) && (ch=='#') && lineText().isEmpty()) {
            if (pSettings->codeCompletion().enabled()
                    && pSettings->codeCompletion().showCompletionWhileInput() ) {
                mLastIdCharPressed++;
                setSelText(ch);
                showCompletion(false);
                handled=true;
                return;
            }
        }
        //javadoc directive?
        if  ((mLastIdCharPressed=0) && (ch=='#') &&
              lineText().trimmed().startsWith('*')) {
            if (pSettings->codeCompletion().enabled()
                    && pSettings->codeCompletion().showCompletionWhileInput() ) {
                mLastIdCharPressed++;
                setSelText(ch);
                showCompletion(false);
                handled=true;
                return;
            }
        }
        mLastIdCharPressed = 0;
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
            return;
        }
    }

    // Spawn code completion window if we are allowed to
    if (pSettings->codeCompletion().enabled())
        handled = handleCodeCompletion(ch);
}

void Editor::onGutterPaint(QPainter &painter, int aLine, int X, int Y)
{
    // Get point where to draw marks
    //X := (fText.Gutter.RealGutterWidth(fText.CharWidth) - fText.Gutter.RightOffset) div 2 - 3;
    X = 5;
    Y += (this->textHeight() - 16) / 2;

    if (mActiveBreakpointLine == aLine) {
        painter.drawPixmap(X,Y,*(pIconsManager->activeBreakpoint()));
    } else if (hasBreakpoint(aLine)) {
        painter.drawPixmap(X,Y,*(pIconsManager->breakpoint()));
    } else {
        PSyntaxIssueList lst = getSyntaxIssuesAtLine(aLine);
        if (lst) {
            bool hasError=false;
            for (const PSyntaxIssue& issue : *lst) {
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
    }
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
        for (const PSyntaxIssue& issue: *lst) {
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
    if (Line == mActiveBreakpointLine &&
            mActiveBreakpointForegroundColor.isValid()
            && mActiveBreakpointBackgroundColor.isValid()) {
        foreground = mActiveBreakpointForegroundColor;
        backgroundColor = mActiveBreakpointBackgroundColor;
        return true;
    } else if (hasBreakpoint(Line)  &&
               mBreakpointForegroundColor.isValid()
               && mBreakpointBackgroundColor.isValid()) {
        foreground = mBreakpointForegroundColor;
        backgroundColor = mBreakpointBackgroundColor;
        return true;
    }
//    end else if Line = fErrorLine then begin
//      StrToThemeColor(tc,  devEditor.Syntax.Values[cErr]);
//      BG := tc.Background;
//      FG := tc.Foreground;
//      if (BG <> clNone) or (FG<>clNone) then
//        Special := TRUE;
//    end;
    return false;
}

void Editor::onPreparePaintHighlightToken(int line, int aChar, const QString &token, PSynHighlighterAttribute attr, SynFontStyles &style, QColor &foreground, QColor &background)
{
    if (token.isEmpty())
        return;
    //selection
    if (selAvail() && highlighter()) {
        if ((
          (attr->name() == SYNS_AttrIdentifier)
          || (attr->name() == SYNS_AttrReservedWord)
          || (attr->name() == SYNS_AttrPreprocessor)
          )
          && (token == mSelectionWord)) {
            foreground = selectedForeground();
            background = selectedBackground();
            return;
        }
    }


//    qDebug()<<token<<"-"<<attr->name()<<" - "<<line<<" : "<<aChar;
    if (mParser && mCompletionPopup && (attr->name() == SYNS_AttrIdentifier)) {
        BufferCoord p{aChar,line};
        BufferCoord pBeginPos,pEndPos;
        QString s= getWordAtPosition(this,p, pBeginPos,pEndPos, WordPurpose::wpInformation);
//        qDebug()<<s;
        PStatement statement = mParser->findStatementOf(mFilename,
          s , p.Line);
        StatementKind kind = mParser->getKindOfStatement(statement);
        if (kind == StatementKind::skUnknown) {
            if ((pEndPos.Line>=1)
              && (pEndPos.Char>=0)
              && (pEndPos.Char < lines()->getString(pEndPos.Line-1).length())
              && (lines()->getString(pEndPos.Line-1)[pEndPos.Char] == '(')) {
                kind = StatementKind::skFunction;
            } else {
                kind = StatementKind::skVariable;
            }
        }
        foreground = mCompletionPopup->colors().value(kind,
                                                      highlighter()->identifierAttribute()->foreground());
        return;
    }
}

bool Editor::event(QEvent *event)
{
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverMove) {
        QHoverEvent *helpEvent = static_cast<QHoverEvent *>(event);
        BufferCoord p;
        TipType reason = getTipType(helpEvent->pos(),p);
        PSyntaxIssue pError;
        int line ;
        if (reason == TipType::Error) {
            pError = getSyntaxIssueAtPosition(p);
        } else if (PointToLine(helpEvent->pos(),line)) {
            //it's on gutter
            //see if its error;
            PSyntaxIssueList issues = getSyntaxIssuesAtLine(line);
            if (issues && !issues->isEmpty()) {
                reason = TipType::Error;
                pError = issues->front();
            }
        }

        // Get subject
        bool isIncludeLine = false;
        BufferCoord pBeginPos,pEndPos;
        QString s;
        switch (reason) {
        case TipType::Preprocessor:
            // When hovering above a preprocessor line, determine if we want to show an include or a identifier hint
            s = lines()->getString(p.Line - 1);
            isIncludeLine = mParser->isIncludeLine(s);
            if (!isIncludeLine)
                s = WordAtRowCol(p);
            break;
        case TipType::Identifier:
            if (pMainWindow->debugger()->executing())
                s = getWordAtPosition(this,p, pBeginPos,pEndPos, WordPurpose::wpEvaluation); // debugging
            else if (//devEditor.ParserHints and
                     !mCompletionPopup->isVisible()
                     && !mHeaderCompletionPopup->isVisible())
                s = getWordAtPosition(this,p, pBeginPos,pEndPos, WordPurpose::wpInformation); // information during coding
            break;
        case TipType::Selection:
            s = selText(); // when a selection is available, always only use that
            break;
        case TipType::Error:
            s = pError->token;
            break;
        case TipType::None:
            cancelHint();
            event->ignore();
            return true;
        }

        //qDebug()<<s<<" -- "<<(int)reason;
        // Don't rescan the same stuff over and over again (that's slow)
        //  if (s = fCurrentWord) and (fText.Hint<>'') then
        s = s.trimmed();
        if ((s == mCurrentWord) && (mCurrentTipType == reason)) {
//            QApplication* app = dynamic_cast<QApplication *>(QApplication::instance());
//            if (app->keyboardModifiers().testFlag(Qt::ControlModifier)) {
            if (helpEvent->modifiers() == Qt::ControlModifier) {
                setCursor(Qt::PointingHandCursor);
            } else {
                setCursor(Qt::ArrowCursor);
            }
            event->ignore();
            return true; // do NOT remove hint when subject stays the same
        }

        // Remove hint
        cancelHint();
        mCurrentWord = s;
        mCurrentTipType = reason;


        // Determine what to do with subject
        QString hint = "";
        switch (reason) {
        case TipType::Preprocessor:
            if (isIncludeLine) {
                hint = getFileHint(s);
            } else if (//devEditor.ParserHints and
                     !mCompletionPopup->isVisible()
                     && !mHeaderCompletionPopup->isVisible()) {
                hint = getParserHint(s,p.Line);
            }
            break;
        case TipType::Identifier:
        case TipType::Selection:
            if (!mCompletionPopup->isVisible()
                    && !mHeaderCompletionPopup->isVisible()) {
                if (pMainWindow->debugger()->executing()) {
                    showDebugHint(s,p.Line);
                } else { //if devEditor.ParserHints {
                    hint = getParserHint(s, p.Line);
                }
            }
            break;
        case TipType::Error:
            hint = getErrorHint(pError);
        }
//        qDebug()<<"hint:"<<hint;
        if (!hint.isEmpty()) {
            //            QApplication* app = dynamic_cast<QApplication *>(QApplication::instance());
            //            if (app->keyboardModifiers().testFlag(Qt::ControlModifier)) {
            if (helpEvent->modifiers() == Qt::ControlModifier) {
                setCursor(Qt::PointingHandCursor);
            } else {
                setCursor(Qt::ArrowCursor);
            }
            QToolTip::showText(mapToGlobal(helpEvent->pos()),hint);
            event->ignore();
        } else {
            event->ignore();
        }
        return true;
    } else if (event->type() == QEvent::HoverLeave) {
        cancelHint();
        return true;
    } else if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease ) {
        if (!mCurrentWord.isEmpty()) {
            QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Control) {
                QApplication* app = dynamic_cast<QApplication *>(QApplication::instance());
                QHoverEvent* hoverEvent=new QHoverEvent(QEvent::HoverMove,
                                                        mapFromGlobal(QCursor::pos()),
                                                        mapFromGlobal(QCursor::pos()),
                                                        Qt::ControlModifier
                                                        );
                app->postEvent(this,hoverEvent);
            }
        }
    }
    return SynEdit::event(event);
}

void Editor::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() | Qt::LeftButton) {
        mLastIdCharPressed = 0;
    }

    // if ctrl+clicked
    if ((event->modifiers() == Qt::ControlModifier)
            && (event->button() == Qt::LeftButton)) {

        BufferCoord p;
        if (PointToCharLine(event->pos(),p)) {
            QString s = lines()->getString(p.Line - 1);
            if (mParser->isIncludeLine(s)) {
                QString filename = mParser->getHeaderFileName(mFilename,s);
                Editor * e = pMainWindow->editorList()->getEditorByFilename(filename);
                if (e) {
                    e->setCaretPositionAndActivate(1,1);
                    return;
                }
            } else {
                gotoDeclaration(p);
                return;
            }
        }
    }
    SynEdit::mouseReleaseEvent(event);
}

void Editor::copyToClipboard()
{
    if (pSettings->editor().copySizeLimit()) {
        int startLine = blockBegin().Line;
        int endLine = blockEnd().Line;
        if ((endLine-startLine+1) > pSettings->editor().copyLineLimits()) {
            QMessageBox::critical(pMainWindow,tr("Error"),
                                     tr("The text to be copied exceeds count limit!"));
            return;
        }
        if ((selText().length()) > pSettings->editor().copyCharLimits() * 1000) {
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

void Editor::setCaretPosition(int line, int aChar)
{
    this->uncollapseAroundLine(line);
    this->setCaretXYCentered(true,BufferCoord{aChar,line});
}

void Editor::setCaretPositionAndActivate(int line, int aChar)
{
    this->uncollapseAroundLine(line);
    if (!this->hasFocus())
        this->activate();
    this->setCaretXYCentered(true,BufferCoord{aChar,line});
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
    if (!lst)
        return PSyntaxIssue();
    foreach (const PSyntaxIssue& issue, *lst) {
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
    if (!changes.testFlag(SynStatusChange::scReadOnly)
            && !changes.testFlag(SynStatusChange::scInsertMode)
            && (lines()->count()!=mLineCount)
            && (lines()->count()!=0) && ((mLineCount>0) || (lines()->count()>1))) {
        reparse();
        if (pSettings->editor().syntaxCheckWhenLineChanged())
            checkSyntaxInBack();
    }
    mLineCount = lines()->count();
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
        mSelectionWord="";
        if (selAvail()) {
            BufferCoord wordBegin,wordEnd,bb,be;
            bb = blockBegin();
            be = blockEnd();
            wordBegin = WordStartEx(bb);
            wordEnd = WordEndEx(be);
            if (wordBegin.Line == bb.Line
                    && wordBegin.Char == bb.Char
                    && wordEnd.Line == be.Line
                    && wordEnd.Char == be.Char) {
                if (wordBegin.Line>=1 && wordBegin.Line<=lines()->count()) {
                    QString line = lines()->getString(wordBegin.Line-1);
                    mSelectionWord = line.mid(wordBegin.Char-1,wordEnd.Char-wordBegin.Char);
                }
            }
//            qDebug()<<QString("(%1,%2)").arg(bb.Line).arg(bb.Char)
//                   <<" - "<<QString("(%1,%2)").arg(be.Line).arg(be.Char)
//                  <<" - "<<QString("(%1,%2)").arg(wordBegin.Line).arg(wordBegin.Char)
//                 <<" - "<<QString("(%1,%2)").arg(wordEnd.Line).arg(wordEnd.Char)
//                <<" : "<<mSelectionWord;
        }
        if (mOldSelectionWord != mSelectionWord) {
            invalidate();
            mOldSelectionWord = mSelectionWord;
        }
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

    if (changes.testFlag(SynStatusChange::scCaretY)) {
        pMainWindow->caretList().addCaret(this,caretY(),caretX());
        pMainWindow->updateCaretActions();
    }
}

void Editor::onGutterClicked(Qt::MouseButton button, int , int , int line)
{
    if (button == Qt::LeftButton) {
        toggleBreakpoint(line);
    }
    mGutterClickedLine = line;
}

void Editor::onTipEvalValueReady(const QString &value)
{
    if (mCurrentWord == mCurrentDebugTipWord) {
        QToolTip::showText(QCursor::pos(), mCurrentDebugTipWord + " = " + value );
    }
    disconnect(pMainWindow->debugger(), &Debugger::evalValueReady,
               this, &Editor::onTipEvalValueReady);
}

void Editor::onLinesDeleted(int first, int count)
{
    pMainWindow->caretList().linesDeleted(this,first,count);
    pMainWindow->debugger()->breakpointModel()->onFileDeleteLines(mFilename,first,count);
    resetBreakpoints();
    if (!pSettings->editor().syntaxCheckWhenLineChanged()) {
        //todo: update syntax issues
    }
}

void Editor::onLinesInserted(int first, int count)
{
    pMainWindow->caretList().linesInserted(this,first,count);
    pMainWindow->debugger()->breakpointModel()->onFileInsertLines(mFilename,first,count);
    resetBreakpoints();
    if (!pSettings->editor().syntaxCheckWhenLineChanged()) {
        //todo: update syntax issues
    }
}

void Editor::resetBreakpoints()
{
    mBreakpointLines.clear();
    foreach (const PBreakpoint& breakpoint,
             pMainWindow->debugger()->breakpointModel()->breakpoints()) {
        if (breakpoint->filename == mFilename) {
            mBreakpointLines.insert(breakpoint->line);
        }
    }
    invalidate();
}

QChar Editor::getCurrentChar()
{
    if (lineText().length()<caretX())
        return QChar();
    else
        return lineText().at(caretX()-1);
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
        bool oldInsertMode = insertMode();
        setInsertMode(false); //set mode to overwrite
        CommandProcessor(SynEditorCommand::ecChar,'}');
        setInsertMode(oldInsertMode);
//        setCaretXY( BufferCoord{caretX() + 1, caretY()}); // skip over
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

bool Editor::handleCodeCompletion(QChar key)
{
    if (!mCompletionPopup->isEnabled())
        return false;
    switch(key.unicode()) {
    case '.':
        setSelText(key);
        showCompletion(false);
        return true;
    case '>':
        setSelText(key);
        if ((caretX() > 1) && (lineText().length() >= 1) &&
                (lineText()[caretX() - 2] == '-'))
            showCompletion(false);
        return true;
    case ':':
        setSelText(key);
        if ((caretX() > 1) && (lineText().length() >= 1) &&
                (lineText()[caretX() - 2] == ':'))
            showCompletion(false);
        return true;
    case '/':
    case '\\':
        setSelText(key);
        if (mParser->isIncludeLine(lineText())) {
            showHeaderCompletion(false);
        }
        return true;
    default:
        return false;
    }
}

void Editor::initParser()
{
    mParser = std::make_shared<CppParser>();
    mParser->setOnGetFileStream(
                std::bind(
                    &EditorList::getContentFromOpenedEditor,pMainWindow->editorList(),
                    std::placeholders::_1, std::placeholders::_2));
    resetCppParser(mParser);
    mParser->setEnabled((highlighter() && highlighter()->getClass() == SynHighlighterClass::CppHighlighter));
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

void Editor::reparse()
{
    parseFile(mParser,mFilename,mInProject);
}

void Editor::insertString(const QString &value, bool moveCursor)
{
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });

    BufferCoord oldCursorPos = caretXY();
    setSelText(value);
    if (!moveCursor) {
        setCaretXY(oldCursorPos);
    }
}

void Editor::showCompletion(bool autoComplete)
{
    if (!pSettings->codeCompletion().enabled())
        return;
    if (!mParser->enabled())
        return;

    if (mCompletionPopup->isVisible()) // already in search, don't do it again
        return;

    QString word="";

    QString s;
    PSynHighlighterAttribute attr;
    bool tokenFinished;
    SynHighlighterTokenType tokenType;
    BufferCoord pBeginPos, pEndPos;
    if (GetHighlighterAttriAtRowCol(
                BufferCoord{caretX() - 1,
                caretY()}, s, tokenFinished,tokenType, attr)) {
        if (tokenType == SynHighlighterTokenType::PreprocessDirective) {//Preprocessor
            word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpDirective);
            if (!word.startsWith('#')) {
                //showTabnineCompletion();
                return;
            }
        } else if (tokenType == SynHighlighterTokenType::Comment) { //Comment, javadoc tag
            word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpJavadoc);
            if (!word.startsWith('@')) {
                    return;
            }
        } else if (
                   (tokenType != SynHighlighterTokenType::Symbol) &&
                   (tokenType != SynHighlighterTokenType::Space) &&
                   (tokenType != SynHighlighterTokenType::Identifier)
                   ) {
            return;
        }
    }

    // Position it at the top of the next line
    QPoint p = RowColumnToPixels(displayXY());
    p+=QPoint(0,textHeight()+2);
    mCompletionPopup->move(mapToGlobal(p));

    mCompletionPopup->setRecordUsage(pSettings->codeCompletion().recordUsage());
    mCompletionPopup->setSortByScope(pSettings->codeCompletion().sortByScope());
    mCompletionPopup->setShowKeywords(pSettings->codeCompletion().showKeywords());
    mCompletionPopup->setShowCodeIns(pSettings->codeCompletion().showCodeIns());
    mCompletionPopup->setIgnoreCase(pSettings->codeCompletion().ignoreCase());
    mCompletionPopup->resize(pSettings->codeCompletion().width(),
                             pSettings->codeCompletion().height());
//    fCompletionBox.CodeInsList := dmMain.CodeInserts.ItemList;
//    fCompletionBox.SymbolUsage := dmMain.SymbolUsage;
//    fCompletionBox.ShowCount := devCodeCompletion.MaxCount;
    //Set Font size;
    mCompletionPopup->setFont(font());

    // Redirect key presses to completion box if applicable
    //todo:
    mCompletionPopup->setKeypressedCallback([this](QKeyEvent *event)->bool{
        return onCompletionKeyPressed(event);
    });
    mCompletionPopup->setParser(mParser);
    mCompletionPopup->setUseCppKeyword(mUseCppSyntax);
    mCompletionPopup->show();

    // Scan the current function body
    mCompletionPopup->setCurrentStatement(
                mParser->findAndScanBlockAt(mFilename, caretY())
                );

    if (word.isEmpty())
        word=getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpCompletion);
    //if not fCompletionBox.Visible then
    mCompletionPopup->prepareSearch(word, mFilename, pBeginPos.Line);

    // Filter the whole statement list
    if (mCompletionPopup->search(word, autoComplete)) { //only one suggestion and it's not input while typing
        completionInsert(pSettings->codeCompletion().appendFunc());
    }
}

void Editor::showHeaderCompletion(bool autoComplete)
{
    if (!pSettings->codeCompletion().enabled())
        return;
//    if not devCodeCompletion.Enabled then
//      Exit;

    if (mHeaderCompletionPopup->isVisible()) // already in search, don't do it again
        return;

    // Position it at the top of the next line
    QPoint p = RowColumnToPixels(displayXY());
    p.setY(p.y() + textHeight() + 2);
    mHeaderCompletionPopup->move(mapToGlobal(p));


    mHeaderCompletionPopup->setIgnoreCase(pSettings->codeCompletion().ignoreCase());
    mHeaderCompletionPopup->resize(pSettings->codeCompletion().width(),
                             pSettings->codeCompletion().height());
    //Set Font size;
    mHeaderCompletionPopup->setFont(font());

    // Redirect key presses to completion box if applicable
    mHeaderCompletionPopup->setKeypressedCallback([this](QKeyEvent* event)->bool{
        return onHeaderCompletionKeyPressed(event);
    });
    mHeaderCompletionPopup->setParser(mParser);

    BufferCoord pBeginPos,pEndPos;
    QString word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos,
                                     WordPurpose::wpHeaderCompletionStart);
    if (word.isEmpty())
        return;

    if (!word.startsWith('"') && !word.startsWith('<'))
        return;

    if (word.lastIndexOf('"')>0 || word.lastIndexOf('>')>0)
        return;

    mHeaderCompletionPopup->show();
    mHeaderCompletionPopup->setSearchLocal(word.startsWith('"'));
    word.remove(0,1);

    mHeaderCompletionPopup->prepareSearch(word, mFilename);

    // Filter the whole statement list
    if (mHeaderCompletionPopup->search(word, autoComplete)) //only one suggestion and it's not input while typing
        headerCompletionInsert(); // if only have one suggestion, just use it
}

bool Editor::testInFunc(int x, int y)
{
    bool result = false;
    QString s = lines()->getString(y);
    int posY = y;
    int posX = std::min(x,s.length()-1); // x is started from 1
    int bracketLevel=0;
    while (true) {
        while (posX < 0) {
            posY--;
            if (posY < 0)
                return false;
            s = lines()->getString(posY);
            posX = s.length()-1;
        }
        if (s[posX] == '>'
                || s[posX] == ']') {
            bracketLevel++;
        } else if (s[posX] == '<'
                   || s[posX] == '[') {
            bracketLevel--;
        } else if (bracketLevel==0) {
            switch (s[posX].unicode()) {
            case '(':
                return true;
            case ';':
            case '{':
                return false;
            }
            if (!(isIdentChar(s[posX])
                  || s[posX] == ' '
                  || s[posX] == '\t'
                  || s[posX] == '*'
                  || s[posX] == '&'))
                break;;
        }
        posX--;
    }
    return result;
}

void Editor::completionInsert(bool appendFunc)
{
    PStatement statement = mCompletionPopup->selectedStatement();
    if (!statement)
        return;
//    if devCodeCompletion.RecordUsage and (Statement^._Kind <> skUserCodeIn) then begin
//        idx:=Utils.FastIndexOf(dmMain.SymbolUsage,Statement^._FullName);
//        if idx = -1 then begin
//            usageCount:=1;
//            dmMain.SymbolUsage.AddObject(Statement^._FullName, pointer(1))
//        end else begin
//            usageCount := 1 + integer(dmMain.SymbolUsage.Objects[idx]);
//            dmMain.SymbolUsage.Objects[idx] := pointer( usageCount );
//        end;
//        Statement^._UsageCount := usageCount;
//    end;

    QString funcAddOn = "";

// delete the part of the word that's already been typed ...
    BufferCoord p = WordEnd();
    setBlockBegin(WordStart());
    setBlockEnd(p);

    // if we are inserting a function,
    if (appendFunc) {
        if (statement->kind == StatementKind::skFunction
                || statement->kind == StatementKind::skConstructor
                || statement->kind == StatementKind::skDestructor) {
            if ((p.Char >= lineText().length()) // it's the last char on line
                    || (lineText().at(p.Char) != '(')) {  // it don't have '(' after it
                if (statement->fullName!="std::endl")
                    funcAddOn = "()";
            }
        }
    }

    // ... by replacing the selection
    if (statement->kind == StatementKind::skUserCodeIn) { // it's a user code template
        //insertUserCodeIn(Statement->value);
    } else {
        if (
                (statement->kind == StatementKind::skKeyword
                 || statement->kind == StatementKind::skPreprocessor)
                && (statement->command.startsWith('#')
                    || statement->command.startsWith('@'))
                ) {
            setSelText(statement->command.mid(1));
        } else
            setSelText(statement->command + funcAddOn);

        if (!funcAddOn.isEmpty())
            mLastIdCharPressed = 0;

        // Move caret inside the ()'s, only when the user has something to do there...
        if (!funcAddOn.isEmpty()
                && (statement->args != "()")
                && (statement->args != "(void)")) {
            setCaretX(caretX() - funcAddOn.length()+1);

            //todo: function hint
            // immediately activate function hint
//            if devEditor.ShowFunctionTip and Assigned(fText.Highlighter) then begin
//            fText.SetFocus;
//            fFunctionTip.Parser := fParser;
//            fFunctionTip.FileName := fFileName;
//            fFunctionTip.Show;
//              end;
        }
    }
    mCompletionPopup->hide();
}

void Editor::headerCompletionInsert()
{
    QString headerName = mHeaderCompletionPopup->selectedFilename();
    if (headerName.isEmpty())
        return;

    // delete the part of the word that's already been typed ...
    BufferCoord p = caretXY();
    int posBegin = p.Char-1;
    int posEnd = p.Char-1;
    QString sLine = lineText();
    while ((posBegin>0) &&
           (isIdentChar(sLine[posBegin-1]) || (sLine[posBegin-1]=='.')))
        posBegin--;

    while ((posEnd < sLine.length())
           && (isIdentChar(sLine[posEnd]) || (sLine[posEnd]=='.')))
        posEnd++;
    p.Char = posBegin+1;
    setBlockBegin(p);
    p.Char = posEnd+1;
    setBlockEnd(p);

    setSelText(headerName);

    mCompletionPopup->hide();
}

bool Editor::onCompletionKeyPressed(QKeyEvent *event)
{
    bool processed = false;
    if (!mCompletionPopup->isEnabled())
        return false;
    QString oldPhrase = mCompletionPopup->phrase();
    WordPurpose purpose = WordPurpose::wpCompletion;
    if (oldPhrase.startsWith('#')) {
        purpose = WordPurpose::wpDirective;
    } else if (oldPhrase.startsWith('@')) {
        purpose = WordPurpose::wpJavadoc;
    }
    QString phrase;
    BufferCoord pBeginPos,pEndPos;
    switch (event->key()) {
    case Qt::Key_Backspace:
        ExecuteCommand(
                    SynEditorCommand::ecDeleteLastChar,
                    QChar(), nullptr); // Simulate backspace in editor
        phrase = getWordAtPosition(this,caretXY(),
                                   pBeginPos,pEndPos,
                                   purpose);
        mLastIdCharPressed = phrase.length();
        mCompletionPopup->search(phrase, false);
        return true;
    case Qt::Key_Escape:
        mCompletionPopup->hide();
        return true;
    case Qt::Key_Return:
    case Qt::Key_Tab:
        completionInsert(pSettings->codeCompletion().appendFunc());
        return true;
    default:
        if (event->text().isEmpty()) {
            //stop completion
            mCompletionPopup->hide();
            keyPressEvent(event);
            return true;
        }
    }
    QChar ch = event->text().front();
    if (isIdentChar(ch)) {
        setSelText(ch);
        phrase = getWordAtPosition(this,caretXY(),
                                            pBeginPos,pEndPos,
                                            purpose);
        mLastIdCharPressed = phrase.length();
        mCompletionPopup->search(phrase, false);
        return true;
    } else {
        //stop completion
        mCompletionPopup->hide();
        keyPressEvent(event);
        return true;
    }
    return processed;
}

bool Editor::onHeaderCompletionKeyPressed(QKeyEvent *event)
{
    bool processed = false;
    if (!mCompletionPopup->isEnabled())
        return false;
    QString phrase;
    BufferCoord pBeginPos,pEndPos;
    switch (event->key()) {
    case Qt::Key_Backspace:
        ExecuteCommand(
                    SynEditorCommand::ecDeleteLastChar,
                    QChar(), nullptr); // Simulate backspace in editor
        phrase = getWordAtPosition(this,caretXY(),
                                   pBeginPos,pEndPos,
                                   WordPurpose::wpHeaderCompletion);
        mLastIdCharPressed = phrase.length();
        mHeaderCompletionPopup->search(phrase, false);
        return true;
    case Qt::Key_Escape:
        mHeaderCompletionPopup->hide();
        return true;
    case Qt::Key_Return:
    case Qt::Key_Tab:
        headerCompletionInsert();
        mHeaderCompletionPopup->hide();
        return true;
    default:
        if (event->text().isEmpty()) {
            //stop completion
            mHeaderCompletionPopup->hide();
            keyPressEvent(event);
            return true;
        }
    }
    QChar ch = event->text().front();
    if (isIdentChar(ch)) {
        setSelText(ch);
        phrase = getWordAtPosition(this,caretXY(),
                                            pBeginPos,pEndPos,
                                            WordPurpose::wpHeaderCompletion);
        mLastIdCharPressed = phrase.length();
        mHeaderCompletionPopup->search(phrase, false);
        return true;
    } else {
        //stop completion
        mHeaderCompletionPopup->hide();
        keyPressEvent(event);
        return true;
    }
    return processed;
}

Editor::TipType Editor::getTipType(QPoint point, BufferCoord& pos)
{
    // Only allow in the text area...
    if (PointToCharLine(point, pos)) {
        if (!pMainWindow->debugger()->executing()
                && getSyntaxIssueAtPosition(pos)) {
            return TipType::Error;
        }

        PSynHighlighterAttribute attr;
        QString s;

        // Only allow hand tips in highlighted areas
        if (GetHighlighterAttriAtRowCol(pos,s,attr)) {
            // Only allow Identifiers, Preprocessor directives, and selection
            if (attr) {
                if (selAvail()) {
                    // do not allow when dragging selection
                    if (IsPointInSelection(pos))
                        return TipType::Selection;
                } else if (attr->name() == SYNS_AttrIdentifier)
                    return TipType::Identifier;
                else if (attr->name() == SYNS_AttrPreprocessor)
                    return TipType::Preprocessor;
            }
        }
    }
    return TipType::None;
}

void Editor::cancelHint()
{
    //MainForm.Debugger.OnEvalReady := nil;

    // disable editor hint
    QToolTip::hideText();
    mCurrentWord = "";
    mCurrentTipType = TipType::None;
    setCursor(Qt::IBeamCursor);
}

QString Editor::getFileHint(const QString &s)
{
    QString fileName = mParser->getHeaderFileName(mFilename, s);
    if (fileExists(fileName)) {
        return fileName + " - " + tr("Ctrl+click for more info");
    }
    return "";
}

QString Editor::getParserHint(const QString &s, int line)
{
    // This piece of code changes the parser database, possibly making hints and code completion invalid...
    QString result;
    PStatement statement = mParser->findStatementOf(mFilename, s, line);
    if (!statement)
        return result;
    if (statement->kind == StatementKind::skFunction
            || statement->kind == StatementKind::skConstructor
            || statement->kind == StatementKind::skDestructor) {
        PStatement parentScope = statement->parentScope.lock();
        if (parentScope && parentScope->kind == StatementKind::skNamespace) {
            PStatementList namespaceStatementsList =
                    mParser->findNamespace(parentScope->command);
            if (namespaceStatementsList) {
                foreach (const PStatement& namespaceStatement, *namespaceStatementsList) {
                    QString hint = getHintForFunction(statement,namespaceStatement,
                                                      mFilename,line);
                    if (!hint.isEmpty()) {
                        if (!result.isEmpty())
                            result += "<BR />";
                        result += hint;
                    }
                }
            }
        } else
          result = getHintForFunction(statement, parentScope,
                                      mFilename,line);
    } else if (statement->line>0) {
        QFileInfo fileInfo(statement->fileName);
        result = mParser->prettyPrintStatement(statement,mFilename, line) + " - "
                + QString("%1(%2) ").arg(fileInfo.fileName()).arg(line)
                + tr("Ctrl+click for more info");
    } else {  // hard defines
        result = mParser->prettyPrintStatement(statement, mFilename);
    }
//    Result := StringReplace(Result, '|', #5, [rfReplaceAll]);
    return result;
}

void Editor::showDebugHint(const QString &s, int line)
{
    PStatement statement = mParser->findStatementOf(mFilename,s,line);
    if (statement) {
        if (statement->kind != StatementKind::skVariable
                && statement->kind != StatementKind::skGlobalVariable
                && statement->kind != StatementKind::skLocalVariable
                && statement->kind != StatementKind::skParameter) {
            return;
        }
    }
    if (pMainWindow->debugger()->commandRunning())
        return;
    connect(pMainWindow->debugger(), &Debugger::evalValueReady,
               this, &Editor::onTipEvalValueReady);
    mCurrentDebugTipWord = s;
    pMainWindow->debugger()->sendCommand("print",s,false);
}

QString Editor::getErrorHint(const PSyntaxIssue& issue)
{
    if (issue) {
        return issue->hint;
    } else {
        return "";
    }
}

QString Editor::getHintForFunction(const PStatement &statement, const PStatement &scopeStatement, const QString& filename, int line)
{
    QString result;
    const StatementMap& children = mParser->statementList().childrenStatements(scopeStatement);
    foreach (const PStatement& childStatement, children){
        if (statement->command == childStatement->command
                && statement->kind == childStatement->kind) {
            if ((line < childStatement->line) &&
                    childStatement->fileName == filename)
                continue;
            if (!result.isEmpty())
                result += "<BR />";
            QFileInfo fileInfo(childStatement->fileName);
            result = mParser->prettyPrintStatement(childStatement,filename,line) + " - "
                    + QString("%1(%2) ").arg(fileInfo.fileName()).arg(line)
                    + tr("Ctrl+click for more info");
        }
    }
    return result;
}

void Editor::setInProject(bool newInProject)
{
    if (mInProject == newInProject)
        return;
    if (mInProject) {
        initParser();
    } else {
        mParser = pMainWindow->project()->cppParser();
    }
}

void Editor::gotoDeclaration(const BufferCoord &pos)
{
    // Exit early, don't bother creating a stream (which is slow)
    BufferCoord pBeginPos, pEndPos;
    QString phrase = getWordAtPosition(this,pos,pBeginPos,pEndPos, WordPurpose::wpInformation);
    if (phrase.isEmpty())
        return;

    PStatement statement = mParser->findStatementOf(
                mFilename,phrase,pos.Line);

    if (!statement) {
        pMainWindow->updateStatusbarMessage(tr("Symbol '%1' not found!").arg(phrase));
        return;
    }
    QString filename;
    int line;
    if (statement->fileName == mFilename && statement->line == pos.Line) {
            filename = statement->definitionFileName;
            line = statement->definitionLine;
    } else {
        filename = statement->fileName;
        line = statement->line;
    }
    Editor* e = pMainWindow->editorList()->getEditorByFilename(filename);
    if (e) {
        e->setCaretPositionAndActivate(line,1);
    }
}

void Editor::gotoDefinition(const BufferCoord &pos)
{
    // Exit early, don't bother creating a stream (which is slow)
    BufferCoord pBeginPos, pEndPos;
    QString phrase = getWordAtPosition(this,pos,pBeginPos,pEndPos, WordPurpose::wpInformation);
    if (phrase.isEmpty())
        return;

    PStatement statement = mParser->findStatementOf(
                mFilename,phrase,pos.Line);

    if (!statement) {
        pMainWindow->updateStatusbarMessage(tr("Symbol '%1' not found!").arg(phrase));
        return;
    }
    QString filename;
    int line;
    filename = statement->definitionFileName;
    line = statement->definitionLine;
    Editor* e = pMainWindow->editorList()->getEditorByFilename(filename);
    if (e) {
        e->setCaretPositionAndActivate(line,1);
    }
}

QString getWordAtPosition(SynEdit *editor, const BufferCoord &p, BufferCoord &pWordBegin, BufferCoord &pWordEnd, Editor::WordPurpose purpose)
{
    QString result = "";
    QString s;
    if ((p.Line<1) || (p.Line>editor->lines()->count())) {
        pWordBegin = p;
        pWordEnd = p;
        return "";
    }

    s = editor->lines()->getString(p.Line - 1);
    int len = s.length();

    int wordBegin = p.Char - 1 - 1; //BufferCoord::Char starts with 1
    int wordEnd = p.Char - 1 - 1;

    // Copy forward until end of word
    if (purpose == Editor::WordPurpose::wpEvaluation
            || purpose == Editor::WordPurpose::wpInformation) {
        while (wordEnd + 1 < len) {
            if ((purpose == Editor::WordPurpose::wpEvaluation)
                    && (s[wordEnd + 1] == '[')) {
                if (!findComplement(s, '[', ']', wordEnd, 1))
                    break;
            } else if (editor->isIdentChar(s[wordEnd + 1])) {
                wordEnd++;
            } else
                break;
        }
    }

    // Copy backward until #
    if (purpose == Editor::WordPurpose::wpDirective) {
        while ((wordBegin >= 0) && (wordBegin < len)) {
           if (editor->isIdentChar(s[wordBegin]))
               wordBegin--;
           else if (s[wordBegin] == '#') {
               wordBegin--;
               break;
           } else
               break;
        }
    }

    // Copy backward until @
    if (purpose == Editor::WordPurpose::wpJavadoc) {
        while ((wordBegin >= 0) && (wordBegin < len)) {
           if (editor->isIdentChar(s[wordBegin]))
               wordBegin--;
           else if (s[wordBegin] == '@') {
               wordBegin--;
               break;
           } else
               break;
        }
    }

    // Copy backward until begin of path
    if (purpose == Editor::WordPurpose::wpHeaderCompletion) {
        while ((wordBegin >= 0) && (wordBegin < len)) {
            if (editor->isIdentChar(s[wordBegin]))
                wordBegin--;
            else if (s[wordBegin] == '/'
                     || s[wordBegin] == '\\'
                     || s[wordBegin] == '.') {
                wordBegin--;
                break;
            } else
                break;
        }
    }

    if (purpose == Editor::WordPurpose::wpHeaderCompletionStart) {
        while ((wordBegin >= 0) && (wordBegin < len)) {
            if (s[wordBegin] == '"'
                    || s[wordBegin] == '<') {
                wordBegin--;
                break;
            } else if (s[wordBegin] == '/'
                         || s[wordBegin] == '\\'
                         || s[wordBegin] == '.') {
                    wordBegin--;
            } else  if (editor->isIdentChar(s[wordBegin]))
                wordBegin--;
            else
                break;
        }
    }

//        && ( wordBegin < len)
    // Copy backward until begin of word
    if (purpose == Editor::WordPurpose::wpCompletion
            || purpose == Editor::WordPurpose::wpEvaluation
            || purpose == Editor::WordPurpose::wpInformation) {
        while ((wordBegin >= 0) && (wordBegin<len)) {
            if (s[wordBegin] == ']') {
                if (!findComplement(s, ']', '[', wordBegin, -1))
                    break;
                else
                    wordBegin++; // step over mathing [
            } else if (editor->isIdentChar(s[wordBegin])) {
                wordBegin--;
            } else if (s[wordBegin] == '.'
                       || s[wordBegin] == ':'
                       || s[wordBegin] == '~') { // allow destructor signs
                wordBegin--;
            } else if (
                       (s[wordBegin] == '>')
                       && (wordBegin+2<len)
                       && (s[wordBegin+1]==':')
                       && (s[wordBegin+2]==':')
                       ) { // allow template
                if (!findComplement(s, '>', '<', wordBegin, -1))
                    break;
                else
                    wordBegin--; // step over >
            } else if ((wordBegin-1 >= 0)
                       && (s[wordBegin - 1] == '-')
                       && (s[wordBegin] == '>')) {
                wordBegin-=2;
            } else if ((wordBegin-1 >= 0)
                   && (s[wordBegin - 1] == ':')
                   && (s[wordBegin] == ':')) {
                wordBegin-=2;
            } else if ((wordBegin > 0)
                       && (s[wordBegin] == ')')) {
                if (!findComplement(s, ')', '(', wordBegin, -1))
                    break;
                else
                    wordBegin--; // step over mathing (
            } else
                break;
        }
    }

    // Get end result
    result = s.mid(wordBegin+1, wordEnd - wordBegin);
    pWordBegin.Line = p.Line;
    pWordBegin.Char = wordBegin+1;
    pWordEnd.Line = p.Line;
    pWordEnd.Char = wordEnd;

    // last line still have part of word
    if (!result.isEmpty()
            && (
                result[0] == '.'
                || result[0] == '-')
            && (purpose == Editor::WordPurpose::wpCompletion
                || purpose == Editor::WordPurpose::wpEvaluation
                || purpose == Editor::WordPurpose::wpInformation)) {
        int i = wordBegin;
        int line=p.Line;
        while (line>=1) {
            while (i>=0) {
                if (s[i] == ' '
                        || s[i] == '\t')
                    i--;
                else
                    break;
            }
            if (i<0) {
                line--;
                if (line>=1) {
                    s=editor->lines()->getString(line-1);
                    i=s.length();
                    continue;
                } else
                    break;
            } else {
                BufferCoord highlightPos;
                BufferCoord pDummy;
                highlightPos.Line = line;
                highlightPos.Char = i+1;
                result = getWordAtPosition(editor, highlightPos,pWordBegin,pDummy,purpose)+result;
                break;
            }
        }
    }

    // Strip function parameters
    int paramBegin,paramEnd;
    while (true) {
        paramBegin = result.indexOf('(');
        if (paramBegin > 0) {
            paramEnd = paramBegin;
            if ((paramBegin==0)
                    && findComplement(result, '(', ')', paramEnd, 1)
                    && (paramEnd = result.length()-1) ) {
                //remove the enclosing parenthese pair
                result = result.mid(1,result.length()-2);
                continue;
            } else {
                paramEnd = paramBegin;
              if (findComplement(result, '(', ')', paramEnd, 1)) {
                  result.remove(paramBegin, paramEnd - paramBegin + 1);
              } else
                  break;
            }
        } else
            break;
    }

    paramBegin = 0;
    while ((paramBegin < result.length()) && (result[paramBegin] == '*')) {
        paramBegin++;
    }
    result.remove(0,paramBegin);
    return result;
}

QString Editor::getPreviousWordAtPositionForSuggestion(const BufferCoord &p)
{
    QString result;
    if ((p.Line<1) || (p.Line>lines()->count())) {
        return "";
    }
    bool inFunc = testInFunc(p.Char-1,p.Line-1);

    QString s = lines()->getString(p.Line - 1);
    int wordBegin;
    int wordEnd = p.Char-1;
    if (wordEnd >= s.length())
        wordEnd = s.length()-1;
    while (true) {
        int bracketLevel=0;
        bool skipNextWord=false;
        while (wordEnd > 0) {
          if (s[wordEnd] == '>'
                 || s[wordEnd] == ']') {
              bracketLevel++;
          } else if (s[wordEnd] == '<'
                     || s[wordEnd] == '[') {
              bracketLevel--;
          } else if (bracketLevel==0) {
              //we can't differentiate multiple definition and function parameter define here , so we don't handle ','
              if (s[wordEnd] == ',') {
                  if (inFunc) // in func, dont skip ','
                      break;
                  else
                      skipNextWord=true;
              } else if (s[wordEnd] != ' '
                         && s[wordEnd] != '\t') {
                  break;
              }
          }
          wordEnd--;
        }
        if (wordEnd<0)
            return "";
        if (bracketLevel > 0)
            return "";
        if (!isIdentChar(s[wordEnd]))
            return "";

        wordBegin = wordEnd;
        while ((wordBegin >= 0) && isIdentChar(s[wordBegin])) {
            wordBegin--;
        }
        wordBegin++;

        if (s[wordBegin]>='0' && s[wordBegin]<='9') // not valid word
            return "";

        result = s.mid(wordBegin, wordEnd - wordBegin+1);
        if ((result != "const") && !skipNextWord)
            break;
        wordEnd = wordBegin-1;
    }
    return result;
}

void Editor::reformat()
{
    if (readOnly())
        return;
    //we must remove all breakpoints and syntax issues
    onLinesDeleted(1,lines()->count());
    QByteArray content = lines()->text().toUtf8();
    QStringList args = pSettings->codeFormatter().getArguments();
    QByteArray newContent = runAndGetOutput("astyle.exe",
                                            pSettings->dirs().app(),
                                            args,
                                            content);

    selectAll();
    setSelText(QString::fromUtf8(newContent));
    reparse();
    checkSyntaxInBack();
    pMainWindow->updateEditorActions();
}

void Editor::checkSyntaxInBack()
{
    if (readOnly())
        return;
    if(pSettings->editor().syntaxCheck())
        pMainWindow->checkSyntaxInBack(this);
}

const PCppParser &Editor::parser()
{
    return mParser;
}

int Editor::gutterClickedLine() const
{
    return mGutterClickedLine;
}

void Editor::toggleBreakpoint(int line)
{
    if (hasBreakpoint(line)) {
        mBreakpointLines.remove(line);
        pMainWindow->debugger()->removeBreakpoint(line,this);
    } else {
        mBreakpointLines.insert(line);
        pMainWindow->debugger()->addBreakpoint(line,this);
    }

    invalidateGutterLine(line);
    invalidateLine(line);
}

void Editor::clearBreakpoints()
{
    pMainWindow->debugger()->deleteBreakpoints(this);
    mBreakpointLines.clear();
    invalidate();
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

void Editor::modifyBreakpointProperty(int line)
{
    int index;
    PBreakpoint breakpoint = pMainWindow->debugger()->breakpointAt(line,this,index);
    if (!breakpoint)
        return;
    bool isOk;
    QString s=QInputDialog::getText(this,
                              tr("Break point condition"),
                              tr("Enter the condition of the breakpoint:"),
                            QLineEdit::Normal,
                            breakpoint->condition,&isOk);
    if (isOk) {
        pMainWindow->debugger()->setBreakPointCondition(index,s);
    }
}

void Editor::setActiveBreakpointFocus(int Line, bool setFocus)
{
    if (Line != mActiveBreakpointLine) {
        removeBreakpointFocus();

        // Put the caret at the active breakpoint
        mActiveBreakpointLine = Line;

        if (setFocus)
            setCaretPositionAndActivate(Line,1);
        else
            setCaretPosition(Line,1);

        // Invalidate new active line
        invalidateGutterLine(Line);
        invalidateLine(Line);
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
    setCaretUseTextColor(pSettings->editor().caretUseTextColor());
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

    if (pSettings->editor().showRightEdgeLine()) {
        setRightEdge(pSettings->editor().rightEdgeWidth());
        setRightEdgeColor(pSettings->editor().rightEdgeLineColor());
    } else {
        setRightEdge(0);
    }
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
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_SELECTION);
    if (item) {
        setSelectedForeground(item->foreground());
        setSelectedBackground(item->background());
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_ACTIVE_BREAKPOINT);
    if (item) {
        this->mActiveBreakpointForegroundColor = item->foreground();
        this->mActiveBreakpointBackgroundColor = item->background();
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_BREAKPOINT);
    if (item) {
        this->mBreakpointForegroundColor = item->foreground();
        this->mBreakpointBackgroundColor = item->background();
    }
    //color for code completion popup
    if (mCompletionPopup) {
        item = pColorManager->getItem(schemeName, SYNS_AttrFunction);
        if (item) {
            mCompletionPopup->colors().insert(StatementKind::skFunction,item->foreground());
            mCompletionPopup->colors().insert(StatementKind::skConstructor,item->foreground());
            mCompletionPopup->colors().insert(StatementKind::skDestructor,item->foreground());
        }
        item = pColorManager->getItem(schemeName, SYNS_AttrClass);
        if (item) {
            mCompletionPopup->colors().insert(StatementKind::skClass,item->foreground());
            mCompletionPopup->colors().insert(StatementKind::skTypedef,item->foreground());
            mCompletionPopup->colors().insert(StatementKind::skAlias,item->foreground());
        }
        item = pColorManager->getItem(schemeName, SYNS_AttrIdentifier);
        if (item) {
            mCompletionPopup->colors().insert(StatementKind::skEnumType,item->foreground());
            mCompletionPopup->colors().insert(StatementKind::skEnumClassType,item->foreground());
        }
        item = pColorManager->getItem(schemeName, SYNS_AttrVariable);
        if (item) {
            mCompletionPopup->colors().insert(StatementKind::skVariable,item->foreground());
        }
        item = pColorManager->getItem(schemeName, SYNS_AttrLocalVariable);
        if (item) {
            mCompletionPopup->colors().insert(StatementKind::skLocalVariable,item->foreground());
            mCompletionPopup->colors().insert(StatementKind::skParameter,item->foreground());
        }
        item = pColorManager->getItem(schemeName, SYNS_AttrGlobalVariable);
        if (item) {
            mCompletionPopup->colors().insert(StatementKind::skGlobalVariable,item->foreground());
        }
        item = pColorManager->getItem(schemeName, SYNS_AttrGlobalVariable);
        if (item) {
            mCompletionPopup->colors().insert(StatementKind::skGlobalVariable,item->foreground());
        }
        item = pColorManager->getItem(schemeName, SYNS_AttrPreprocessor);
        if (item) {
            mCompletionPopup->colors().insert(StatementKind::skPreprocessor,item->foreground());
            mCompletionPopup->colors().insert(StatementKind::skEnum,item->foreground());
            mHeaderCompletionPopup->setSuggestionColor(item->foreground());
        }
        item = pColorManager->getItem(schemeName, SYNS_AttrReservedWord);
        if (item) {
            mCompletionPopup->colors().insert(StatementKind::skKeyword,item->foreground());
            mCompletionPopup->colors().insert(StatementKind::skUserCodeIn,item->foreground());
        }
        item = pColorManager->getItem(schemeName, SYNS_AttrString);
        if (item) {
            mCompletionPopup->colors().insert(StatementKind::skNamespace,item->foreground());
            mCompletionPopup->colors().insert(StatementKind::skNamespaceAlias,item->foreground());
        }
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
        if (this->readOnly()) {
            caption.append("["+tr("Readonly")+"]");
        }
        mParentPageControl->setTabText(index,caption);
    } else {
        mParentPageControl->setTabText(index,newCaption);
    }

}
