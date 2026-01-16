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
#include "editor.h"

#include <QtCore/QFileInfo>
#include <QFont>
#include <QVariant>
#include <QWheelEvent>
#include <QGuiApplication>
#include <QClipboard>
#include <QPainter>
#include <QToolTip>
#include <QApplication>
#include <QInputDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QScrollBar>
#include <QScreen>
#include <memory>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QMimeData>
#include <QTemporaryFile>
#include <QFileSystemWatcher>
#include <qsynedit/document.h>
#include <qsynedit/syntaxer/cpp.h>
#include <qsynedit/syntaxer/gas.h>
#include <qsynedit/syntaxer/nasm.h>
#include <qsynedit/exporter/rtfexporter.h>
#include <qsynedit/exporter/htmlexporter.h>
#include <qsynedit/exporter/qtsupportedhtmlexporter.h>
#include <qsynedit/constants.h>
#include "systemconsts.h"
#include "syntaxermanager.h"
#include "iconsmanager.h"
#include <QDebug>
#include <qt_utils/charsetinfo.h>
#include "utils/escape.h"
#include "utils/ui.h"
#include "widgets/functiontooltipwidget.h"
#include "widgets/bookmarkmodel.h"
#include "codesnippetsmanager.h"
#include "debugger/debuggermodels.h"
#include "settings/editorsettings.h"
#include "settings/codecompletionsettings.h"
#include "colorscheme.h"

using QSynedit::CharPos;

static QSet<QString> CppTypeQualifiers {
    "const",
    "consteval",
    "constexpr",
    "constinit",
    "extern",
    "static",
    "mutable",
    "volatile",
    "inline"
};

static bool findComplement(const QString &s, const QChar &fromToken, const QChar &toToken, int &curPos, int increment)
{
    int curPosBackup = curPos;
    int level = 0;
    //todo: skip comment, char and strings
    while ((curPos < s.length()) && (curPos >= 0)) {
        if (s[curPos] == fromToken) {
            level++;
        } else if (s[curPos] == toToken) {
            level--;
            if (level == 0)
                return true;
        }
        curPos += increment;
    }
    curPos = curPosBackup;
    return false;
}


Editor::Editor(QWidget *parent):
    QSynEdit{parent},
    mInited{false},
    mSyntaxErrorColor{Qt::red},
    mSyntaxWarningColor{"orange"},
    mLineCount{0},
    mActiveBreakpointLine{-1},
    mCurrentTipType{TipType::None},
    mSaving{false},
    mHoverModifiedLine{-1},
    mWheelAccumulatedDelta{0},
    mCtrlClicking{false},
    mFileType{FileType::None}
{
    mEditorEncoding = ENCODING_UTF8;
    mFileEncoding = ENCODING_ASCII;
    mIsNew = true;
    mInProject = false;
    mCodeCompletionEnabled = false;

    mCodeSnippetsManager = nullptr;

    mGetSharedParserFunc = nullptr;
    mGetOpennedEditorFunc  = nullptr;
    mGetFileStreamFunc = nullptr;
    mCanShowEvalTipFunc = nullptr;
    mRequestEvalTipFunc = nullptr;
    mEvalTipReadyCallback = nullptr;
    mGetReformatterFunc = nullptr;
    mGetMacroVarsFunc = nullptr;
    mGetCppParserFunc = nullptr;
#ifdef ENABLE_SDCC
    mGetCompilerTypeForEditorFunc = nullptr;
#endif
    mFileSystemWatcher = nullptr;

    mColorManager = nullptr;
    mCodeCompletionSettings = nullptr;
    mEditorSettings = nullptr;
    mIconsManager = nullptr;

    mStatementColors = std::make_shared<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > >();
    mAutoBackupEnabled = false;
    mLastFocusOutTime = 0;
    mInited=false;
    mBackupFile=nullptr;
    mHighlightCharPos1 = CharPos{-1,-1};
    mHighlightCharPos2 = CharPos{-1,-1};
    mCurrentLineModified = false;

    mFunctionTooltip = nullptr;
    mCompletionPopup = nullptr;
    mHeaderCompletionPopup = nullptr;
    //Initialize User Code Template stuff;
    mXOffsetSince =0;
    mTabStopY=-1;
    mTabStopBegin= -1;
    mTabStopEnd= -1;
    //mLineBeforeTabStop="";
    //mLineAfterTabStop = "";

    connect(this,&QSynEdit::statusChanged,this,&Editor::onStatusChanged);

    connect(this,&QSynEdit::gutterClicked,this,&Editor::onGutterClicked);

    setAttribute(Qt::WA_Hover,true);

    setContextMenuPolicy(Qt::CustomContextMenu);

    mCanAutoSave = false;

//    if (mParentPageControl) {
//        //first showEvent triggered here
//        mParentPageControl->addTab(this,"");
//    }

    connect(&mFunctionTipTimer, &QTimer::timeout,
        this, &Editor::onFunctionTipsTimer);
    mAutoBackupTimer.setInterval(1000);
    connect(&mAutoBackupTimer, &QTimer::timeout,
                this, &Editor::onAutoBackupTimer);

    connect(&mTooltipTimer, &QTimer::timeout,
            this, &Editor::onTooltipTimer);

    connect(horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &Editor::onScrollBarValueChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, &Editor::onScrollBarValueChanged);
    mInited=true;
}

Editor::~Editor() {
    //qDebug()<<"editor "<<mFilename<<" deleted";
    cleanAutoBackup();
}

void Editor::loadFile(QString filename, bool parse) {
    if (filename.isEmpty()) {
        filename=mFilename;
        // save backup
//        for (int i=0;i<100;i++) {
//            QString backfilename = filename+".savebak";
//            if (i>0)
//                backfilename += QString("%1").arg(i);
//            if (fileExists(backfilename)) {
//                if (QMessageBox::question(this,tr("Restore backup"),
//                                          tr("Backup file '%1' detected.").arg(backfilename)
//                                          +"<br />"
//                                          +tr("Error occurred at last save.")
//                                          +"<br />"
//                                          +tr("Do you want to load the backup file?"),
//                                          QMessageBox::Yes | QMessageBox::No)==QMessageBox::Yes)
//                    filename = backfilename;
//                break;
//            }
//        }
    } else {
        filename = QFileInfo(filename).absoluteFilePath();
    }

    //FileError should by catched by the caller of loadFile();
    QByteArray oldEditorEncoding = mEditorEncoding;
    QByteArray oldFileEncoding = mFileEncoding;
    loadFromFile(filename,mEditorEncoding,mFileEncoding);
    if (mEditorEncoding==ENCODING_AUTO_DETECT) {
        if (mFileEncoding==ENCODING_ASCII)
            mEditorEncoding=mEditorSettings->defaultEncoding();
        else
            mEditorEncoding=mFileEncoding;
    }
    if (oldEditorEncoding!=mEditorEncoding)
        emit editorEncodingChanged(this);
    if (oldFileEncoding!=mFileEncoding)
        emit fileEncodingChanged(this);

    applyColorScheme(mEditorSettings->colorScheme());
    mIsNew = false;
    saveAutoBackup();
    setStatusChanged(QSynedit::StatusChange::Custom0);

    if (shouldOpenInReadonly()) {
        this->setModified(false);
        setReadOnly(true);
    }
    if (parse) {
        setCppParser();
        reparse();
        reparseTodo();
        checkSyntaxInBack();
    }
}

void Editor::saveFile(QString filename) {
    Q_ASSERT(mEditorEncoding!=ENCODING_AUTO_DETECT);
    Q_ASSERT(mEditorEncoding!=ENCODING_PROJECT);
    QFile file(filename);
    QByteArray oldFileEncoding = mFileEncoding;
    this->document()->saveToFile(file, mEditorEncoding, mFileEncoding);
    if (oldFileEncoding != mFileEncoding)
        emit fileEncodingChanged(this);
}

void Editor::convertToEncoding(const QByteArray &encoding)
{
    if (mEditorEncoding == encoding)
        return;
    mEditorEncoding = encoding;
    setModified(true);
    save();
    emit editorEncodingChanged(this);
}

bool Editor::save(bool force, bool doReparse) {
    if (this->mIsNew && !force) {
        return saveAs();
    }    
    try {
        if (mEditorSettings->autoFormatWhenSaved()) {
            reformat(false);
        } else if (mEditorSettings->removeTrailingSpacesWhenSaved()) {
            trimTrailingSpaces();
        }
        // must emit fileSaving/fileSaved signal out of saveFile(),
        // to let fileSystemWatcher's addPath/removePath() invoked out of the saveFile().
        // If not, fileSystemWatcher would generate fileChanged signal when saving files.
        if (mFileSystemWatcher)
            mFileSystemWatcher->removePath(mFilename);
        emit fileSaving(this, mFilename);
        saveFile(mFilename);
        emit fileSaved(this, mFilename);
        if (mFileSystemWatcher)
            mFileSystemWatcher->addPath(mFilename);
        setModified(false);
        mIsNew = false;
        setStatusChanged(QSynedit::StatusChange::Custom0);
    } catch (FileError& exception) {
        if (mFileSystemWatcher)
            mFileSystemWatcher->addPath(mFilename);
        QMessageBox::critical(parentWidget(),tr("Save Error"), exception.reason());
        emit fileSaveError(this, mFilename, exception.reason());
        return false;
    }

    if (doReparse && isVisible()) {
        reparse();
        if (mEditorSettings->syntaxCheckWhenSave())
            checkSyntaxInBack();
        reparseTodo();
    }

    return true;
}

bool Editor::saveAs(const QString &name){
    QString newName = name;
    QString oldName = mFilename;
    if (name.isEmpty()) {
        QString selectedFileFilter;
        QString defaultExt;
        defaultExt=QFileInfo(oldName).suffix();
        if (defaultExt.isEmpty()) {
            if (mEditorSettings->defaultFileCpp()) {
                selectedFileFilter = pSystemConsts->defaultCPPFileFilter();
                defaultExt = "cpp";
            } else {
                selectedFileFilter = pSystemConsts->defaultCFileFilter();
                defaultExt = "c";
            }
        } else {
            selectedFileFilter = pSystemConsts->fileFilterFor(defaultExt);
        }
        QFileDialog dialog(this,tr("Save As"),extractFilePath(mFilename),
                           pSystemConsts->defaultFileFilters().join(";;"));
        dialog.selectNameFilter(selectedFileFilter);
        //dialog.setDefaultSuffix(defaultExt);
        dialog.selectFile(mFilename);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setAcceptMode(QFileDialog::AcceptSave);

        if (dialog.exec()!=QFileDialog::Accepted) {
            return false;
        }
        QStringList selectedFiles=dialog.selectedFiles();
        newName = selectedFiles.first();
        QFileInfo fileInfo(newName);
        if (fileInfo.suffix().isEmpty()) {
            QString filter = dialog.selectedNameFilter();
            if (!filter.contains("*.*")) {
                int pos = filter.indexOf("*.");
                if (pos>=0) {
                    QString suffix;
                    pos+=2;
                    while (pos<filter.length()) {
                        if (filter[pos] == ';' || filter[pos] ==' ' || filter[pos] == ')')
                                    break;
                        suffix+=filter[pos];
                        pos++;
                    }
                    newName += "." + suffix;
                }
            }
        }
        QDir::setCurrent(extractFileDir(newName));
    }

    if (mGetOpennedEditorFunc && mGetOpennedEditorFunc(newName)!=nullptr) {
        QMessageBox::critical(parentWidget(),tr("Error"),
                                      tr("File %1 already opened!").arg(newName));
        return false;
    }
    // Update project information

    clearSyntaxIssues();
    if (mCodeCompletionEnabled && mParser && !inProject()) {
        mParser->invalidateFile(mFilename);
    }

    if (mEditorSettings->autoFormatWhenSaved()) {
        reformat(false);
    } else if (mEditorSettings->removeTrailingSpacesWhenSaved()) {
        trimTrailingSpaces();
    }
    mFilename = newName;
    setFileType(getFileType(mFilename));
    if (!syntaxer() || syntaxer()->language() != QSynedit::ProgrammingLanguage::CPP) {
        mSyntaxIssues.clear();
    }
    reparse();
    if (mEditorSettings->syntaxCheckWhenSave())
        checkSyntaxInBack();
    if (!shouldOpenInReadonly()) {
        setReadOnly(false);
    }
    setStatusChanged(QSynedit::StatusChange::Custom0);

    if (mFileSystemWatcher)
        mFileSystemWatcher->removePath(oldName);

    emit fileSaveAsed(this, oldName, newName);

    try {
        // must emit fileSaving/fileSaved signal out of saveFile(),
        // to let fileSystemWatcher's addPath/removePath() invoked out of the saveFile().
        // If not, fileSystemWatcher would generate fileChanged signal when saving files.
        emit fileSaving(this, mFilename);
        saveFile(mFilename);
        emit fileSaved(this, mFilename);
        if (mFileSystemWatcher)
            mFileSystemWatcher->addPath(mFilename);
        mIsNew = false;
        setModified(false);
        setStatusChanged(QSynedit::StatusChange::Custom0);
    }  catch (FileError& exception) {
        if (mFileSystemWatcher)
            mFileSystemWatcher->addPath(mFilename);
        QMessageBox::critical(parentWidget(),tr("Save Error"), exception.reason());
        emit fileSaveError(this, mFilename, exception.reason());
        return false;
    }

    initAutoBackup();
    return true;
}

void Editor::rename(const QString &newName)
{
    if (mFilename == newName)
        return;
    if (mGetOpennedEditorFunc && mGetOpennedEditorFunc(newName)) {
        return;
    }
    QString oldName = mFilename;


    clearSyntaxIssues();
    if (mCodeCompletionEnabled && mParser && !inProject()) {
        mParser->invalidateFile(oldName);
    }

    mFilename = newName;
    setFileType(getFileType(mFilename));
    if (!syntaxer() || syntaxer()->language() != QSynedit::ProgrammingLanguage::CPP) {
        mSyntaxIssues.clear();
    }
    if (mEditorSettings->syntaxCheckWhenSave())
        checkSyntaxInBack();

    if (mFileSystemWatcher) {
        mFileSystemWatcher->removePath(oldName);
        mFileSystemWatcher->addPath(newName);
    }
    emit fileRenamed(this, oldName, newName);

    initAutoBackup();
    return;
}

void Editor::setFilename(const QString &newName)
{
    mFilename = newName;
}

const QByteArray& Editor::editorEncoding() const noexcept{
    return mEditorEncoding;
}

void Editor::setEditorEncoding(const QByteArray& encoding) noexcept{
    if (encoding.isEmpty())
        return;
    QByteArray newEncoding=encoding;
    Q_ASSERT(encoding!=ENCODING_PROJECT);
    if (mEditorEncoding == newEncoding)
        return;
    mEditorEncoding = newEncoding;
    if (!isNew()) {
        if (modified()) {
            if (QMessageBox::warning(this,tr("Confirm Reload File"),
                           tr("The editing file will be reloaded. <br />All unsaved modifications will be lost. <br />Are you sure to continue?"),
                           QMessageBox::Yes, QMessageBox::No)!=QMessageBox::Yes)
                return;
        }
        try {
            loadFile();
            setModified(false);
        } catch (FileError& e) {
            QMessageBox::critical(nullptr,
                                  tr("Error Load File"),
                                  e.reason());
        }
    }
    emit editorEncodingChanged(this);
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

void Editor::undoSymbolCompletion(const CharPos &pos)
{
    QSynedit::PTokenAttribute attr;
    QString token;
    QSynedit::PSyntaxState syntaxState;

    if (!mEditorSettings->removeSymbolPairs())
        return;
    if (!getTokenAttriAtRowCol(pos, token, attr, syntaxState))
        return;
    if (syntaxer()->isCommentNotFinished(syntaxState))
        return ;
    //convert caret x to string index;

    if (pos.ch<0 || pos.ch+1>=lineText().length())
        return;
    QChar deletedChar = lineText().at(pos.ch);
    QChar nextChar = lineText().at(pos.ch+1);
    if ((attr->tokenType() == QSynedit::TokenType::Character) && (deletedChar != '\''))
        return;
//    if (attr->tokenType() == QSynedit::TokenType::StringEscapeSequence)
//        return;
    if (attr->tokenType() == QSynedit::TokenType::String) {
        if (token=="\\\"")
            return;
        if ((deletedChar!='"') || (nextChar!='"'))
            return;
    }
    if (deletedChar == '\'') {
        if (attr->tokenType() != QSynedit::TokenType::Character)
            return;
        if (attr->tokenType() == QSynedit::TokenType::Number)
            return;
        if (token == "'\\''")
            return;
    }

    if ((deletedChar == '<') &&
            !(mParser && mParser->isIncludeLine(lineText())))
        return;
    if ( (mEditorSettings->completeBracket() && (deletedChar == '[') && (nextChar == ']')) ||
         (mEditorSettings->completeParenthese() && (deletedChar == '(') && (nextChar == ')')) ||
         (mEditorSettings->completeGlobalInclude() && (deletedChar == '<') && (nextChar == '>')) ||
         (mEditorSettings->completeBrace() && (deletedChar == '{') && (nextChar == '}')) ||
         (mEditorSettings->completeSingleQuote() && (deletedChar == '\'') && (nextChar == '\'')) ||
         (mEditorSettings->completeDoubleQuote() && (deletedChar == '\"') && (nextChar == '\"'))) {
         processCommand(QSynedit::EditCommand::DeleteChar);
    }
}

void Editor::wheelEvent(QWheelEvent *event) {
    if ( (event->modifiers() & Qt::ControlModifier)!=0) {
        int size = mEditorSettings->fontSize();
        int oldSize = size;
        if ( (mWheelAccumulatedDelta>0 &&event->angleDelta().y()<0)
             || (mWheelAccumulatedDelta<0 &&event->angleDelta().y()>0))
            mWheelAccumulatedDelta=0;
        mWheelAccumulatedDelta+=event->angleDelta().y();
        while (mWheelAccumulatedDelta>=120) {
            mWheelAccumulatedDelta-=120;
            size = std::min(99,size+1);
        }
        while (mWheelAccumulatedDelta<=-120) {
            mWheelAccumulatedDelta+=120;
            size = std::max(2,size-1);
        }
        if (size!=oldSize) {
            emit fontSizeChangedByWheel(size);
        }
        event->accept();
        return;
    }
    QSynEdit::wheelEvent(event);
}

void Editor::focusInEvent(QFocusEvent *event)
{
    QSynEdit::focusInEvent(event);
    emit focusInOccured(this);
}

void Editor::focusOutEvent(QFocusEvent *event)
{
    QSynEdit::focusOutEvent(event);
    //if (mMainWindow) mMainWindow->updateClassBrowserForEditor(nullptr);
    if (mFunctionTooltip) mFunctionTooltip->hide();
    mLastFocusOutTime = QDateTime::currentMSecsSinceEpoch();
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    bool handled = false;
    auto action = finally([&,this]{
        if (!handled) {
            QSynEdit::keyPressEvent(event);
        } else {
            event->accept();
        }
    });
    if (event->modifiers() == Qt::ControlModifier
            && event->key() == Qt::Key_Control
            && !completionPopupVisible()
            && !headerCompletionPopupVisible()
            ) {
        //setMouseTracking(true);
        handled=true;
        QMouseEvent mouseEvent{
            QEvent::MouseMove,
                    mapFromGlobal(QCursor::pos()),
                    Qt::NoButton,
                    Qt::NoButton,
                    Qt::ControlModifier};
        mouseMoveEvent(&mouseEvent);
        return;
    }
    if (readOnly())
        return;

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (mTabStopBegin>=0) { // editing user code template
            handled = true;
            mTabStopBegin = -1;
            invalidateLine(caretY());
            clearUserCodeInTabStops();
        } else {
            QString sLine;
            if (caretX()>=0)
                sLine = lineText().mid(0,caretX()).trimmed();
            QSynedit::PSyntaxState state = calcSyntaxStateAtLine(caretY(), sLine);
            if (syntaxer()->isCommentNotFinished(state)) {
                if (sLine=="/**") { //javadoc style docstring
                    sLine = lineText().mid(caretX()).trimmed();
                    if (sLine=="*/") {
                        CharPos p = caretXY();
                        p.ch = lineText().length();
                        setSelBeginEnd(p, CharPos{lineText().length(), p.line});
                        setSelText("");
                    }
                    handled = true;
                    QStringList insertString;
                    insertString.append("");
                    PStatement function;
                    if (mParser)
                        function = mParser->findFunctionAt(mFilename,caretY()+1);
                    if (function) {
                        bool isVoid = (function->type  == "void");
                        QStringList params = mParser->getFunctionParameterNames(function);
                        insertString.append(QString(" * @brief ")+USER_CODE_IN_INSERT_POS);
                        if (!params.isEmpty())
                            insertString.append(" * ");
                        foreach (const QString& param, params) {
                            insertString.append(QString(" * @param %1 %2")
                                                .arg(param, USER_CODE_IN_INSERT_POS));
                        }
                        if (!isVoid) {
                            insertString.append(" * ");
                            insertString.append(QString(" * @return ")+USER_CODE_IN_INSERT_POS);
                        }
                        insertString.append(" */");
                    } else {
                        insertString.append(QString(" * ")+USER_CODE_IN_INSERT_POS);
                        insertString.append(" */");
                    }
                    insertCodeSnippet(linesToText(insertString));
                } else {
                    sLine=trimLeft(lineText());
                    if (sLine.startsWith("* ")) {
                        handled = true;
                        int right = lineText(caretY()).length()-caretX();
                        sLine = lineBreak()+"* ";
                        insertString(sLine,false);
                        CharPos p = caretXY();
                        p.line++;
                        p.ch = lineText(p.line).length();
                        if (right>0) {
                            p.ch -= right;
                        }
                        setCaretXY(p);
                    }
                }
            }
        }
        return;
    case Qt::Key_Escape: // Update function tip
        if (mTabStopBegin>=0) {
            mTabStopBegin = -1;
            setSelBeginEnd(caretXY(),caretXY());
            invalidateLine(caretY());
            clearUserCodeInTabStops();
        }
        mFunctionTooltip->hide();
        return;
    case Qt::Key_Tab:
        handled = true;
        tab();
        return;
    case Qt::Key_Backtab:
        handled = true;
        shifttab();
        return;
    case Qt::Key_Up:
        if (functionTooltipVisible()) {
            handled = true;
            mFunctionTooltip->previousTip();
        } else {
            clearUserCodeInTabStops();
        }
        return;
    case Qt::Key_Down:
        if (functionTooltipVisible()) {
            handled = true;
            mFunctionTooltip->nextTip();
        } else {
            clearUserCodeInTabStops();
        }
        return;
    case Qt::Key_Delete:
        // remove completed character
        if (!selAvail()) {
            undoSymbolCompletion(caretXY());
        }
        return;
    case Qt::Key_Backspace:
        // remove completed character
        if (!selAvail()) {
            undoSymbolCompletion(CharPos{caretX()-1,caretY()});
        }
        return;
    }

    QString t = event->text();
    if (t.length() != 1) // handled by qsynedit
        return;

    if (activeSelectionMode()==QSynedit::SelectionMode::Column) // handled by qsynedit
        return;

    QChar ch = t[0];
    int idCharPressed=previousIdChars(caretXY());
    if (isIdentStartChar(ch)) {
        CharPos ws = CharPos{caretX()-idCharPressed,caretY()};
        idCharPressed++;
        if (mCodeCompletionEnabled
                && mCodeCompletionSettings->showCompletionWhileInput()
                && idCharPressed>=mCodeCompletionSettings->minCharRequired()) {
            if (mParser) {
                if (mParser->isIncludeLine(lineText())) {
                    // is a #include line
                    processCommand(QSynedit::EditCommand::Input,ch);
                    showHeaderCompletion(false);
                    handled=true;
                    return;
                } else {
                    QSynedit::TokenType tokenType = QSynedit::TokenType::Default;
                    QString lastWord;
                    lastWord = getPreviousWordAtPositionForSuggestion(ws, tokenType);
                    if (mParser && (tokenType == QSynedit::TokenType::Keyword
                                    || tokenType == QSynedit::TokenType::Identifier)) {
                        if (CppTypeQualifiers.contains(lastWord)) {
                            processCommand(QSynedit::EditCommand::Input,ch);
                            showCompletion(lastWord,false, CodeCompletionType::Types);
                            handled=true;
                            return;
                        } else if (lastWord == "typedef" ) {
                            processCommand(QSynedit::EditCommand::Input,ch);
                            showCompletion(lastWord,false, CodeCompletionType::Types);
                            handled=true;
                            return;
                        } else if (lastWord == "using") {
                            processCommand(QSynedit::EditCommand::Input,ch);
                            showCompletion(lastWord,false, CodeCompletionType::ComplexKeyword);
                            handled=true;
                            return;
                        } else if (lastWord == "goto") {
                            processCommand(QSynedit::EditCommand::Input,ch);
                            showCompletion(lastWord,false, CodeCompletionType::Labels);
                            handled=true;
                            return;
                        } else if (lastWord == "namespace") {
                            processCommand(QSynedit::EditCommand::Input,ch);
                            showCompletion(lastWord,false, CodeCompletionType::Namespaces);
                            handled=true;
                            return;
                        } else if (CppTypeKeywords.contains(lastWord)) {
                            PStatement currentScope = mParser->findScopeStatement(mFilename,caretY());
                            while(currentScope && currentScope->kind==StatementKind::Block) {
                                currentScope = currentScope->parentScope.lock();
                            }
                            if (!currentScope || currentScope->kind == StatementKind::Namespace
                                   || currentScope->kind == StatementKind::Class) {
                                //may define a function
                                processCommand(QSynedit::EditCommand::Input,ch);
                                showCompletion(lastWord,false,CodeCompletionType::FunctionWithoutDefinition);
                                handled=true;
                                return;
                            }
                            if (lastWord == "long" ||
                                    lastWord == "short" ||
                                    lastWord == "signed" ||
                                    lastWord == "unsigned") {
                                processCommand(QSynedit::EditCommand::Input,ch);
                                showCompletion(lastWord,false, CodeCompletionType::ComplexKeyword);
                                handled=true;
                                return;
                            }

                            //last word is a type keyword, this is a var or param define, and dont show suggestion
                            return;
                        } else if (lastWord == "#ifdef" || lastWord == "#ifndef" || lastWord == "#undef") {
                            processCommand(QSynedit::EditCommand::Input,ch);
                            showCompletion(lastWord,false, CodeCompletionType::Macros);
                            handled=true;
                            return;
                        }
                        PStatement statement = mParser->findStatementOf(
                                    mFilename,
                                    lastWord,
                                    caretY());
                        StatementKind kind = getKindOfStatement(statement);
                        if (kind == StatementKind::Class
                                || kind == StatementKind::EnumClassType
                                || kind == StatementKind::EnumType
                                || kind == StatementKind::Typedef) {

                            PStatement currentScope = mParser->findScopeStatement(mFilename,caretY());
                            while(currentScope && currentScope->kind==StatementKind::Block) {
                                currentScope = currentScope->parentScope.lock();
                            }
                            if (!currentScope || currentScope->kind == StatementKind::Namespace) {
                                //may define a function
                                processCommand(QSynedit::EditCommand::Input,ch);
                                showCompletion("",false,CodeCompletionType::FunctionWithoutDefinition);
                                handled=true;
                                return;
                            }

                            //last word is a typedef/class/struct, this is a var or param define, and dont show suggestion
                            return;
                        }
                    } else  if (mParser && (tokenType == QSynedit::TokenType::String
                                            || tokenType == QSynedit::TokenType::Character
                                            || tokenType == QSynedit::TokenType::Number
                                            )) {
                        processCommand(QSynedit::EditCommand::Input,ch);
                        showCompletion(lastWord,false, CodeCompletionType::LiteralOperators);
                        handled=true;
                        return;
                        return;
                    }
                    lastWord = getPreviousWordAtPositionForCompleteFunctionDefinition(ws);
                    if (mParser && !lastWord.isEmpty()) {
                        PStatement currentScope = mParser->findScopeStatement(mFilename,caretY());
                        while(currentScope && currentScope->kind==StatementKind::Block) {
                            currentScope = currentScope->parentScope.lock();
                        }
                        if (!currentScope || currentScope->kind == StatementKind::Namespace) {
                            //may define a function
                            processCommand(QSynedit::EditCommand::Input,ch);
                            showCompletion("",false,CodeCompletionType::FunctionWithoutDefinition);
                            handled=true;
                            return;
                        }
                    }
                    processCommand(QSynedit::EditCommand::Input,ch);
                    showCompletion("",false,CodeCompletionType::Normal);
                    handled=true;
                    return;
                }
            } else {
                //show keywords
                processCommand(QSynedit::EditCommand::Input,ch);
                showCompletion("",false,CodeCompletionType::KeywordsOnly);
                handled=true;
                return;
            }
        }
    } else {
        if (mCodeCompletionEnabled
                && mCodeCompletionSettings->showCompletionWhileInput() ) {
            if (mParser && mParser->isIncludeLine(lineText())
                    && ch.isDigit()) {
                // is a #include line
                processCommand(QSynedit::EditCommand::Input,ch);
                showHeaderCompletion(false);
                handled=true;
                return;
            } else if (syntaxer()->language()==QSynedit::ProgrammingLanguage::CPP) {
                //preprocessor ?
                if ((idCharPressed==0) && (ch=='#') && lineText().isEmpty()) {
                    processCommand(QSynedit::EditCommand::Input,ch);
                    showCompletion("",false,CodeCompletionType::Normal);
                    handled=true;
                    return;
                }
                //javadoc directive?
                if  ((idCharPressed==0) && (ch=='@') &&
                      lineText().trimmed().startsWith('*')) {
                    processCommand(QSynedit::EditCommand::Input,ch);
                    showCompletion("",false,CodeCompletionType::Normal);
                    handled=true;
                    return;
                }
            } else if (syntaxer()->language()==QSynedit::ProgrammingLanguage::LUA) {
                if (ch=='.') {
                    processCommand(QSynedit::EditCommand::Input,ch);
                    showCompletion("",false,CodeCompletionType::KeywordsOnly);
                    handled=true;
                    return;
                }
            } else if (syntaxer()->language()==QSynedit::ProgrammingLanguage::GNU_Assembly) {
                if ((idCharPressed==0) && (ch=='.')) {
                    processCommand(QSynedit::EditCommand::Input,ch);
                    showCompletion("",false,CodeCompletionType::KeywordsOnly);
                    handled=true;
                    return;
                }
                if ((idCharPressed==0) && (ch=='%')
                        && std::dynamic_pointer_cast<QSynedit::GASSyntaxer>(syntaxer())->prefixRegisterNames()) {
                    processCommand(QSynedit::EditCommand::Input,ch);
                    showCompletion("",false,CodeCompletionType::KeywordsOnly);
                    handled=true;
                    return;
                }
            }
        }
        switch (ch.unicode()) {
        case '"':
        case '\'':
        case ')':
        case '{':
        case '}':
        case '[':
        case ']':
        case '*':
        case ';':
        case ',':
            handled = handleSymbolCompletion(ch);
            return;
        case '(': {
            if (!selAvail()) {
                // QChar nextCh = nextNonSpaceChar(caretY()-1,caretX()-1);
                // if (!isIdentChar(nextCh) && nextCh!='('
                //         && nextCh!='"' && nextCh!='\'' ){
                //     handled = handleSymbolCompletion(ch);
                // }
                handled = handleSymbolCompletion(ch);
            } else {
                handled = handleSymbolCompletion(ch);
            }
            return;
        }
        case '<':
        case '>':
            if (mParser && mParser->isIncludeLine(lineText())) {
                handled = handleSymbolCompletion(ch);
                return;
            }
            break;
        }
    }

    // Spawn code completion window if we are allowed to
    if (mCodeCompletionEnabled)
        handled = handleCodeCompletion(ch);
}

void Editor::keyReleaseEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::NoModifier
            && event->key() == Qt::Key_Control) {
        //setMouseTracking(false);
        cancelHoverLink();
        updateMouseCursor();
        return;
    }
    QSynedit::QSynEdit::keyReleaseEvent(event);
}

void Editor::mouseMoveEvent(QMouseEvent *event)
{
    if(event->modifiers() == Qt::ControlModifier && event->buttons() == Qt::NoButton) {
        CharPos p;
        TipType reason = getTipType(event->pos(),p);
        if (reason == TipType::Include) {
            QString sLine = lineText(p.line);
            if (mParser->isIncludeNextLine(sLine) || mParser->isIncludeLine(sLine))
                updateHoverLink(p.line);
        } else if (reason == TipType::Identifier) {
            updateHoverLink(p.line);
        } else {
            cancelHoverLink();
        }
        event->accept();
        return;
    }
    QSynedit::QSynEdit::mouseMoveEvent(event);
}

void Editor::onGutterPaint(QPainter &painter, int aLine, int X, int Y)
{
    if(mIconsManager==nullptr)
        return;
    IconsManager::PPixmap icon;

    if (mActiveBreakpointLine == aLine) {
        icon = mIconsManager->getPixmap(IconsManager::GUTTER_ACTIVEBREAKPOINT);
    } else if (hasBreakpoint(aLine)) {
        icon = mIconsManager->getPixmap(IconsManager::GUTTER_BREAKPOINT);
    } else {
        PSyntaxIssueList lst = getSyntaxIssuesAtLine(aLine);
        if (lst) {
            bool hasError=false;
            foreach (const PSyntaxIssue& issue, *lst) {
                if (issue->issueType == CompileIssueType::Error) {
                    hasError = true;
                    break;;
                }
            }
            if (hasError) {
                icon = mIconsManager->getPixmap(IconsManager::GUTTER_SYNTAX_ERROR);
            } else {
                icon = mIconsManager->getPixmap(IconsManager::GUTTER_SYNTAX_WARNING);
            }
        } else if (hasBookmark(aLine)) {
            icon = mIconsManager->getPixmap(IconsManager::GUTTER_BOOKMARK);
        }
    }
    if (icon) {
        qreal dpr=icon->devicePixelRatioF();
        X = 5/dpr;
        Y += (this->textHeight() - icon->height()/dpr) / 2;
        painter.drawPixmap(X,Y,*icon);
    }
}

void setIncludeUnderline(const QString& lineText, int startPos,
                  const QChar& quoteEndChar,
                  QSynedit::PSyntaxer syntaxer,
                  QColor foreColor,
                  QSynedit::EditingAreaList &areaList
                  ) {
    int pos1=startPos;
    int pos2=lineText.indexOf(quoteEndChar,pos1+1);
    if (pos1>=0 && pos2>=0 && pos1 < pos2 ) {
        QSynedit::PEditingArea p=std::make_shared<QSynedit::EditingArea>();
        p->beginX = pos1+1;
        p->endX = pos2;
        p->type = QSynedit::EditingAreaType::eatUnderLine;
        if (syntaxer) {
            if (quoteEndChar=='\"')
                p->color = syntaxer->stringAttribute()->foreground();
            else
                p->color = syntaxer->identifierAttribute()->foreground();
        } else {
            p->color = foreColor;
        }
        areaList.append(p);
    }
}

void Editor::onGetEditingAreas(int line, QSynedit::EditingAreaList &areaList)
{
    areaList.clear();
    if (mTabStopBegin>=0 && mTabStopY == line) {
        QSynedit::PEditingArea p = std::make_shared<QSynedit::EditingArea>();
        p->type = QSynedit::EditingAreaType::eatRectangleBorder;
//        int spaceCount = leftSpaces(mLineBeforeTabStop);
//        int spaceBefore = mLineBeforeTabStop.length()-TrimLeft(mLineBeforeTabStop).length();
        p->beginX = mTabStopBegin;
        p->endX =  mTabStopEnd;
        p->color = syntaxer()->stringAttribute()->foreground();
        areaList.append(p);
    }
    PSyntaxIssueList lst = getSyntaxIssuesAtLine(line);
    if (lst) {
        for (const PSyntaxIssue& issue: *lst) {
            QSynedit::PEditingArea p=std::make_shared<QSynedit::EditingArea>();
            p->beginX = issue->startChar;
            p->endX = issue->endChar;
            if (issue->issueType == CompileIssueType::Error) {
                p->color = mSyntaxErrorColor;
            } else {
                p->color = mSyntaxWarningColor;
            }
            p->type = QSynedit::EditingAreaType::eatWaveUnderLine;
            areaList.append(p);
        }
    }
    QString sLine = lineText(line);
    if (mParser && mParser->isIncludeLine(sLine)) {
        if (line == mHoverModifiedLine) {
            int i=0;
            while (i<sLine.length() && sLine[i]!='<' && sLine[i]!='\"')
                i++;
            if (i<sLine.length()) {
                if (sLine[i]=='<') {
                    setIncludeUnderline(sLine,i,'>',syntaxer(), foregroundColor(), areaList);
                } else {
                    setIncludeUnderline(sLine,i,'"',syntaxer(), foregroundColor(), areaList);
                }
            }
        }
    }
}

bool Editor::onGetSpecialLineColors(int Line, QColor &foreground, QColor &backgroundColor)
{
    if (Line == mActiveBreakpointLine &&
        mActiveBreakpointBackgroundColor.isValid()) {
        if (mActiveBreakpointForegroundColor.isValid())
            foreground = mActiveBreakpointForegroundColor;
        backgroundColor = mActiveBreakpointBackgroundColor;
        return true;
    } else if (hasBreakpoint(Line) && mBreakpointBackgroundColor.isValid()) {
        if (mBreakpointForegroundColor.isValid())
            foreground = mBreakpointForegroundColor;
        backgroundColor = mBreakpointBackgroundColor;
        return true;
    }
    return false;
}

void Editor::onPreparePaintHighlightToken(int line, int aChar, const QString &token, QSynedit::PTokenAttribute attr, QSynedit::FontStyles &style, QColor &foreground, QColor &background)
{
    if (token.isEmpty())
        return;

    if (mParser) {
        // ifdef lines
        if (!mParser->isLineVisible(mFilename, line)) {
            background = syntaxer()->commentAttribute()->background();
            if (attr->tokenType() != QSynedit::TokenType::Space)
                foreground = syntaxer()->commentAttribute()->foreground();
            return;
        }
        QString sLine = lineText(line);
        if (mParser->isIncludeLine(sLine) && attr->tokenType() != QSynedit::TokenType::Comment) {
            // #include header names (<>)
            int pos1=sLine.indexOf("<");
            int pos2=sLine.indexOf(">",pos1);
            if (pos1>=0 && pos2>=0 && pos1<aChar && aChar<pos2) {
                style = syntaxer()->identifierAttribute()->styles();
                foreground = syntaxer()->identifierAttribute()->foreground();
                background = syntaxer()->identifierAttribute()->background();
            }
            return;
        } else if (mParser->enabled() && attr->tokenType() == QSynedit::TokenType::Identifier) {
            //Syntax color for different identifier types
            CharPos p{aChar,line};

            StatementKind kind;
            if (mParser->parsing()){
                kind=mIdCache.value(QString("%1 %2 %3").arg(line).arg(aChar).arg(token),StatementKind::Unknown);
            } else {
                QStringList expression = getExpressionAtPosition(p);
                PStatement statement = parser()->findStatementOf(
                            filename(),
                            expression,
                            p.line);
                while (statement && statement->kind == StatementKind::Alias)
                    statement = mParser->findAliasedStatement(statement);
                if (statement && statement->kind == StatementKind::Constructor) {
                    QString s=document()->getLine(line);
                    int pos  = aChar+token.length();
                    while(pos<s.length() && CppParser::isSpaceChar(s[pos])) {
                        pos++;
                    }
                    if (pos >= s.length() || (s[pos]!='(' && s[pos]!='{')) {
                        statement = statement->parentScope.lock();
                    }
                }
                kind = getKindOfStatement(statement);
                mIdCache.insert(QString("%1 %2 %3").arg(line).arg(aChar).arg(token),kind);
            }
            if (kind == StatementKind::Unknown) {
                CharPos pBeginPos,pEndPos;
                QString s= getWordAtPosition(this,p, pBeginPos,pEndPos, WordPurpose::wpInformation);
                if ((pEndPos.line>=0)
                  && (pEndPos.ch>=0)
                  && (pEndPos.ch < lineText(pEndPos.line).length())
                  && (lineText(pEndPos.line)[pEndPos.ch] == '(')) {
                    kind = StatementKind::Function;
                } else {
                    kind = StatementKind::Variable;
                }
            }
            PColorSchemeItem item = mStatementColors->value(kind,PColorSchemeItem());

            if (item) {
                foreground = item->foreground();
                background = item->background();
                style.setFlag(QSynedit::FontStyle::fsBold,item->bold());
                style.setFlag(QSynedit::FontStyle::fsItalic,item->italic());
                style.setFlag(QSynedit::FontStyle::fsUnderline,item->underlined());
                style.setFlag(QSynedit::FontStyle::fsStrikeOut,item->strikeout());
            } else {
                foreground = syntaxer()->identifierAttribute()->foreground();
                background = syntaxer()->identifierAttribute()->background();
            }
            if (line == mHoverModifiedLine) {
                CharPos p;
                if (pointToCharLine(mapFromGlobal(QCursor::pos()),p)) {
                    if (line==p.line && (aChar<=p.ch && p.ch<aChar+token.length())) {
                        style.setFlag(QSynedit::FontStyle::fsUnderline);
                    }
                }
            }
        }
    }

    if (attr) {
        if (attr->tokenType() == QSynedit::TokenType::Keyword) {
            //C++ type keywords
            if (CppTypeKeywords.contains(token)
                    ||
                    (
                        syntaxer()->language()==QSynedit::ProgrammingLanguage::CPP
                        &&
                        ((QSynedit::CppSyntaxer*)syntaxer().get())->customTypeKeywords().contains(token)
                        )
                )
            {
                PColorSchemeItem item = mStatementColors->value(StatementKind::KeywordType,PColorSchemeItem());

                if (item) {
                    foreground = item->foreground();
                    background = item->background();
                    style.setFlag(QSynedit::FontStyle::fsBold,item->bold());
                    style.setFlag(QSynedit::FontStyle::fsItalic,item->italic());
                    style.setFlag(QSynedit::FontStyle::fsUnderline,item->underlined());
                    style.setFlag(QSynedit::FontStyle::fsStrikeOut,item->strikeout());
                }
            }
        }
        if (((attr->tokenType() == QSynedit::TokenType::Identifier)
             || (attr->tokenType() == QSynedit::TokenType::Keyword)
             || (attr->tokenType() == QSynedit::TokenType::Preprocessor)
             || (attr->tokenType() == QSynedit::TokenType::String)
             || (attr->tokenType() == QSynedit::TokenType::Comment)
                )
            && (token == mCurrentHighlightedWord)) {
            // occurrencies of the selected identifier
            if (mCurrentHighlighWordForeground.isValid())
                foreground = mCurrentHighlighWordForeground;
            if (mCurrentHighlighWordBackground.isValid())
                background = mCurrentHighlighWordBackground;
        } else if (!selAvail() && attr->name() == SYNS_AttrSymbol
                   && mEditorSettings->highlightMathingBraces()) {
            // matching braces
            if ( (line == mHighlightCharPos1.line)
                    && (aChar == mHighlightCharPos1.ch)) {
                if (mCurrentHighlighWordForeground.isValid())
                    foreground = mCurrentHighlighWordForeground;
                if (mCurrentHighlighWordBackground.isValid())
                    background = mCurrentHighlighWordBackground;
            }
            if ((line == mHighlightCharPos2.line)
                    && (aChar == mHighlightCharPos2.ch)) {
                if (mCurrentHighlighWordForeground.isValid())
                    foreground = mCurrentHighlighWordForeground;
                if (mCurrentHighlighWordBackground.isValid())
                    background = mCurrentHighlighWordBackground;
            }
        }
    } else {
        // occurrencies of the selected word
        if (token == mCurrentHighlightedWord) {
            if (mCurrentHighlighWordForeground.isValid())
                foreground = mCurrentHighlighWordForeground;
            if (mCurrentHighlighWordBackground.isValid())
                background = mCurrentHighlighWordBackground;
        }
    }
}

bool Editor::event(QEvent *event)
{
    if ((event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverMove)
            && qApp->mouseButtons() == Qt::NoButton
            && mEditorSettings->enableTooltips()
            && !completionPopupVisible()
            && !functionTooltipVisible()
            && !functionTooltipVisible()) {
        cancelHint();
        mTooltipTimer.stop();
        if (mEditorSettings->tipsDelay()>0) {
            mTooltipTimer.setSingleShot(true);
            mTooltipTimer.start(mEditorSettings->tipsDelay());
        } else {
            onTooltipTimer();
        }
        event->ignore();
    } else if (event->type() == QEvent::HoverLeave) {
        cancelHint();
        return true;
    } else if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease ) {
        if (!mCurrentWord.isEmpty()) {
            onTooltipTimer();
        }
    }
    return QSynEdit::event(event);
}

static bool checkForCtrlClick(QMouseEvent *event, QCursor cursor) {
    return ((event->modifiers() == Qt::ControlModifier)
        && (event->button() == Qt::LeftButton)
            && cursor == Qt::PointingHandCursor);
}
void Editor::mouseReleaseEvent(QMouseEvent *event)
{
    // if ctrl+clicked
    if (mCtrlClicking
        && checkForCtrlClick(event, cursor() )) {
        CharPos p;
        if (mParser && pointToCharLine(event->pos(),p)) {
            cancelHoverLink();
            QString sLine = lineText(p.line);
            if (mParser->isIncludeNextLine(sLine)) {
                QString filename = mParser->getHeaderFileName(mFilename,sLine, true);
                openFileInContext(filename, CharPos{0,0});
                return;
            } if (mParser->isIncludeLine(sLine)) {
                QString filename = mParser->getHeaderFileName(mFilename,sLine);
                openFileInContext(filename, CharPos{0,0});
                return;
            } else if (mParser->enabled()) {
                gotoDefinition(p);
                return;
            }
        }
    }
    QSynedit::QSynEdit::mouseReleaseEvent(event);
}

void Editor::mousePressEvent(QMouseEvent *event)
{
    // if ctrl+clicked
    mCtrlClicking = checkForCtrlClick(event, cursor());
    QSynedit::QSynEdit::mousePressEvent(event);
}

void Editor::inputMethodEvent(QInputMethodEvent *event)
{
    QSynedit::QSynEdit::inputMethodEvent(event);  
    QString s = event->commitString();
    if (s.isEmpty())
        return;
    if (completionPopupVisible()) {
        onCompletionInputMethod(event);
        return;
    } else {
        if (mCodeCompletionEnabled
                && mCodeCompletionSettings->showCompletionWhileInput() ) {
            int idCharPressed= previousIdChars(caretXY());
            idCharPressed += s.length();
            if (idCharPressed>=mCodeCompletionSettings->minCharRequired()) {
                QSynedit::TokenType wordType;
                CharPos ws{caretX()-idCharPressed,caretY()};
                QString lastWord = getPreviousWordAtPositionForSuggestion(ws, wordType);
                if (mParser && (wordType == QSynedit::TokenType::Keyword
                                || wordType == QSynedit::TokenType::Identifier)) {
                    if (CppTypeKeywords.contains(lastWord)) {
                        return;
                    }
                    PStatement statement = mParser->findStatementOf(
                                mFilename,
                                lastWord,
                                caretY());
                    StatementKind kind = getKindOfStatement(statement);
                    if (kind == StatementKind::Class
                            || kind == StatementKind::EnumClassType
                            || kind == StatementKind::EnumType
                            || kind == StatementKind::Typedef) {
                        //last word is a typedef/class/struct, this is a var or param define, and dont show suggestion
  //                      if devEditor.UseTabnine then
  //                        ShowTabnineCompletion;
                        return;
                    }
                }
                showCompletion("",false,CodeCompletionType::Normal);
                return;
            }
        }
    }
}

void Editor::closeEvent(QCloseEvent *)
{
    if (mHeaderCompletionPopup)
        mHeaderCompletionPopup->hide();
    if (mCompletionPopup)
        mCompletionPopup->hide();
    if (mFunctionTooltip)
        mFunctionTooltip->hide();
    emit closeOccured(this);
}

void Editor::showEvent(QShowEvent */*event*/)
{
    if (mParser) {
        connect(mParser.get(),
                &CppParser::parseFinished,
                this,
                &Editor::onParseFinished);
    }
    emit showOccured(this);
    setHideTime(QDateTime::currentDateTime());
}

void Editor::hideEvent(QHideEvent */*event*/)
{
    if (mParser) {
        disconnect(mParser.get(),
                &CppParser::parseFinished,
                this,
                &Editor::onParseFinished);
    }
    setHideTime(QDateTime::currentDateTime());
}

void Editor::resizeEvent(QResizeEvent *event)
{
    QSynedit::QSynEdit::resizeEvent(event);
    if (mFunctionTooltip) {
        mFunctionTooltip->setMinWidth(width()*3/4);
        mFunctionTooltip->hide();
    }
}

void Editor::copyToClipboard()
{
    switch(mEditorSettings->copyWithFormatAs()) {
    case 1: //HTML
        copyAsHTML();
        break;;
    default:
        QSynedit::QSynEdit::copyToClipboard();
    }
}

void Editor::cutToClipboard()
{
    QSynedit::QSynEdit::cutToClipboard();
}

void Editor::copyAsHTML()
{
    if (!selAvail()) {
        doSelectLine();
    }
    QSynedit::HTMLExporter exporter(tabSize(), pCharsetInfoManager->getDefaultSystemEncoding());

    exporter.setTitle(QFileInfo(mFilename).fileName());
    exporter.setUseBackground(mEditorSettings->copyHTMLUseBackground());
    exporter.setFont(font());
    QSynedit::PSyntaxer hl = syntaxer();
    if (!mEditorSettings->copyHTMLUseEditorColor()) {
        hl = SyntaxerManager::copy(syntaxer());
        mColorManager->applySchemeToSyntaxer(hl,mEditorSettings->copyHTMLColorScheme());
    }
    exporter.setSyntaxer(hl);
    exporter.setOnFormatToken(std::bind(&Editor::onExportedFormatToken,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3,
                                        std::placeholders::_4,
                                        std::placeholders::_5
                                        ));
    exporter.setCreateHTMLFragment(true);

    if (mEditorSettings->copyHTMLWithLineNumber()) {
        exporter.setExportLineNumber(true);
        exporter.setRecalcLineNumber(mEditorSettings->copyHTMLRecalcLineNumber());
        exporter.setLineNumberStartFromZero(mEditorSettings->gutterLineNumbersStartZero());
        exporter.setLineNumberColor(gutter().textColor());
        exporter.setLineNumberBackgroundColor(gutter().color());
    }
    exporter.exportRange(document(),selBegin(),selEnd());

    //clipboard takes the owner ship
    QMimeData * mimeData = new QMimeData;

    //sethtml will convert buffer to QString , which will cause encoding trouble
    mimeData->setData(exporter.clipboardFormat(),exporter.buffer());
    mimeData->setText(selText());

    QGuiApplication::clipboard()->clear();
    QGuiApplication::clipboard()->setMimeData(mimeData);
}

void Editor::setCaretPosition(const CharPos & pos)
{
    this->uncollapseAroundLine(pos.line);
    this->setCaretXYCentered(pos);
}

void Editor::addSyntaxIssues(int line, int startChar, int endChar, CompileIssueType errorType, const QString &hint)
{
    PSyntaxIssue pError;
    CharPos p;
    QString token;
    int start;
    QSynedit::PTokenAttribute attr;
    PSyntaxIssueList lst;
    if ((line<0) || (line>=lineCount()))
        return;
    pError = std::make_shared<SyntaxIssue>();
    p.ch = startChar;
    p.line = line;
    if (startChar >= lineText(line).length()) {
        start = 0;
        token = lineText(line);
    } else if (endChar < 0) {
        if (!getTokenAttriAtRowCol(p,token,start,attr))
            return;
    } else {
        start = startChar;
        token = lineText(line).mid(start,endChar-startChar);
    }
    pError->startChar = start;
    pError->endChar = start + token.length();
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
    CharPos p;
    p.ch = (*iter)->at(0)->startChar;
    p.line = iter.key();
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
    CharPos p;
    p.ch = (*iter)->at(0)->startChar;
    p.line = iter.key();
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

Editor::PSyntaxIssue Editor::getSyntaxIssueAtPosition(const CharPos &pos)
{
    PSyntaxIssueList lst = getSyntaxIssuesAtLine(pos.line);
    if (!lst)
        return PSyntaxIssue();
    foreach (const PSyntaxIssue& issue, *lst) {
        if (issue->startChar<=pos.ch && pos.ch<=issue->endChar)
            return issue;
    }
    return PSyntaxIssue();
}

void Editor::onStatusChanged(QSynedit::StatusChanges changes)
{
    if ((!changes.testFlag(QSynedit::StatusChange::ReadOnlyChanged)
            && !changes.testFlag(QSynedit::StatusChange::InsertMode)
            && (lineCount()!=mLineCount)
            && (lineCount()!=0) && ((mLineCount>0) || (lineCount()>1)))
            ||
        (mCurrentLineModified
            && !changes.testFlag(QSynedit::StatusChange::ReadOnlyChanged)
            && changes.testFlag(QSynedit::StatusChange::CaretY))) {
        mCurrentLineModified = false;
        reparse();
        if (mEditorSettings->syntaxCheckWhenLineChanged())
            checkSyntaxInBack();
        reparseTodo();
//        if (mCodeCompletionSettings->clearWhenEditorHidden()
//                && changes.testFlag(SynStatusChange::scOpenFile)) {
//        } else{
//            reparse();
//        }
    }
    mLineCount = lineCount();
    if (changes.testFlag(QSynedit::StatusChange::Modified)) {
        mCurrentLineModified = true;
        if (!mFilename.isEmpty())
            mCanAutoSave = true;
    }

    if (changes.testFlag(QSynedit::StatusChange::CaretX)
            || changes.testFlag(QSynedit::StatusChange::CaretY)) {
        if (mEditorSettings->highlightMathingBraces()) {
            if (mHighlightCharPos1.isValid())
                invalidateLine(mHighlightCharPos1.line);
            if (mHighlightCharPos2.isValid())
                invalidateLine(mHighlightCharPos2.line);
        }
        mHighlightCharPos1 = CharPos{};
        mHighlightCharPos2 = CharPos{};
        if (mTabStopBegin >=0) {
            if (mTabStopY==caretY()) {
                if (mLineAfterTabStop.isEmpty()) {
                    if (lineText().startsWith(mLineBeforeTabStop))
                        mTabStopBegin = mLineBeforeTabStop.length();
                    mTabStopEnd = lineText().length();
                } else {
                    if (lineText().startsWith(mLineBeforeTabStop)
                        && lineText().endsWith(mLineAfterTabStop))
                        mTabStopBegin = mLineBeforeTabStop.length();
                    mTabStopEnd = lineText().length()
                            - mLineAfterTabStop.length();
                }
                mXOffsetSince = mTabStopEnd - caretX();
                if (caretX() < mTabStopBegin ||
                        caretX() >  (mTabStopEnd+1)) {
                    mTabStopBegin = -1;
                }
            } else {
                if (mTabStopBegin>=0) {
                    invalidateLine(mTabStopY);
                    mTabStopBegin = -1;
                    clearUserCodeInTabStops();
                }
            }
        } else if (!selAvail() && mEditorSettings->highlightMathingBraces()){
            CharPos coord;
            //are there a brace char under caret?
            if (isBraceChar(charAt(caretXY()))) {
                coord=caretXY();
            } else if (isBraceChar(charAt(CharPos{caretX()-1,caretY()}))) {
                coord=CharPos{caretX()-1,caretY()};
            }
            if (coord.isValid()) {
                QSynedit::PTokenAttribute attr;
                QString token;
                if (getTokenAttriAtRowCol(coord,token,attr)
                        && attr->tokenType() == QSynedit::TokenType::Symbol) {
                    CharPos complementCharPos = getMatchingBracket(coord);
                    if (!foldHidesLine(coord.line)
                            && !foldHidesLine(complementCharPos.line)) {
                        mHighlightCharPos1 = coord;
                        mHighlightCharPos2 = complementCharPos;
                        invalidateLine(mHighlightCharPos1.line);
                        invalidateLine(mHighlightCharPos2.line);
                    }
                }
            }
        }
    }

    if (changes.testFlag(QSynedit::StatusChange::CaretX)
            || changes.testFlag(QSynedit::StatusChange::CaretY)) {
        // Update the function tip
        if (mEditorSettings->showFunctionTips()) {
            updateFunctionTip(false);
            mFunctionTipTimer.stop();
            if (mEditorSettings->tipsDelay()>0)
                mFunctionTipTimer.start(500);
            else
                onFunctionTipsTimer();
        }
    }

    if (changes.testFlag(QSynedit::StatusChange::Selection)
            || changes.testFlag(QSynedit::StatusChange::CaretX)
            || changes.testFlag(QSynedit::StatusChange::CaretY)
            ) {
        if (!selAvail() && mEditorSettings->highlightCurrentWord()) {
            QString token;
            QSynedit::PTokenAttribute attri;
            if (getTokenAttriAtRowCol(caretXY(), token,attri)
                && (
                    (attri->tokenType()==QSynedit::TokenType::Identifier)
                        || (attri->tokenType() == QSynedit::TokenType::Keyword)
                        || (attri->tokenType() == QSynedit::TokenType::Preprocessor))
                    && !token.isEmpty()
                    && isIdentStartChar(token[0])) {
                mCurrentHighlightedWord = token;
            } else {
                mCurrentHighlightedWord = "";
            }
        } else if (selAvail() && selBegin() == getTokenBegin(caretXY())
                   && selEnd() == getTokenEnd(caretXY())){
            mCurrentHighlightedWord = selText();
        } else {
            mCurrentHighlightedWord = "";
        }

        if (mOldHighlightedWord != mCurrentHighlightedWord) {
            invalidate();
            mOldHighlightedWord = mCurrentHighlightedWord;
        }
    }

    if (changes.testFlag(QSynedit::StatusChange::ModifyChanged)
        || changes.testFlag(QSynedit::StatusChange::Modified)
        || changes.testFlag(QSynedit::StatusChange::Selection)
        || changes.testFlag(QSynedit::StatusChange::ReadOnlyChanged)) {
        if (!readOnly())
            initAutoBackup();
    }
}

void Editor::onGutterClicked(Qt::MouseButton button, int , int , int line)
{
    if (button == Qt::LeftButton) {
        if (isC_CPP_ASMSourceFile(mFileType)
                || isC_CPPHeaderFile(mFileType))
            toggleBreakpoint(line);
    }
}

void Editor::onTipEvalValueReady(const QString& value)
{
    if (mCurrentWord == mCurrentDebugTipWord) {
        QString newValue;
        if (value.length()>100) {
            newValue = value.left(100) + "...";
        } else {
            newValue = value;
        }
        QToolTip::showText(QCursor::pos(), mCurrentDebugTipWord + " = " + newValue, this);
    }
    Q_ASSERT(mEvalTipReadyCallback!=nullptr);
    mEvalTipReadyCallback(this);
}

void Editor::onFunctionTipsTimer()
{
    mFunctionTipTimer.stop();
    updateFunctionTip(true);
}

void Editor::onAutoBackupTimer()
{
    if (!mAutoBackupEnabled)
        return;
    if (mBackupTime>lastModifyTime())
        return;
    QDateTime current=QDateTime::currentDateTime();
    if (lastModifyTime().secsTo(current) <= 3)
        return;
    saveAutoBackup();
}

void Editor::onTooltipTimer()
{
    if (mHoverModifiedLine != -1)
        return;

    CharPos p;
    QPoint pos = mapFromGlobal(QCursor::pos());
    TipType reason = getTipType(pos,p);
    PSyntaxIssue pError;
    int line ;
    if (reason == TipType::Error) {
        pError = getSyntaxIssueAtPosition(p);
    } else if (pointToLine(pos,line)) {
        //issue tips is prefered
        PSyntaxIssueList issues = getSyntaxIssuesAtLine(line);
        if (issues && !issues->isEmpty()) {
            reason = TipType::Error;
            pError = issues->front();
        }
    }
    if (reason == TipType::Number) {
        if (!QSynedit::isAssemblyLanguage(syntaxer()->language())) {
            reason=TipType::None;
        }
    }


    // Get subject
    CharPos pBeginPos,pEndPos;
    QString s;
    QStringList expression;
    switch (reason) {
    case TipType::Include:
        if (mParser) {
            s = lineText(p.line);
        }
        break;
    case TipType::Identifier:
        if (mCanShowEvalTipFunc && mCanShowEvalTipFunc()) {
            s = getWordAtPosition(this,p, pBeginPos,pEndPos, WordPurpose::wpEvaluation); // debugging
        } else if (!completionPopupVisible()
                 && !headerCompletionPopupVisible()) {
            expression = getExpressionAtPosition(p);
            s = expression.join(""); // information during coding
        }
        break;
    case TipType::Keyword:
        if (QSynedit::isAssemblyLanguage(syntaxer()->language())) {
            if (!completionPopupVisible()
                 && !headerCompletionPopupVisible()) {
                s = tokenAt(p);
            }
        }
        break;
    case TipType::Selection:
        s = selText(); // when a selection is available, always only use that
        break;
    case TipType::Error:
        s = pError->token;
        break;
    case TipType::Number:
        if (!completionPopupVisible()
             && !headerCompletionPopupVisible()) {
            QSynedit::PTokenAttribute attr;
            int start;
            if (getTokenAttriAtRowCol(p,s,start,attr)) {
                QString line=lineText(p.line);
                int idx=start-2;
                if (idx>=0 && idx<line.length() && line[idx]=='-')
                    s='-'+s;
            }
        }
        break;
    case TipType::None:
        cancelHint();
        mCurrentWord = "";
        mCurrentTipType = TipType::None;
        return;
    }

    s = s.trimmed();
    // Remove hint
    cancelHint();
    mCurrentWord = s;
    mCurrentTipType = reason;

    // Determine what to do with subject
    QString hint = "";
    switch (reason) {
    case TipType::Include:
        if (mEditorSettings->enableHeaderToolTips()
                && mParser)
            hint = getHeaderFileHint(s, mParser->isIncludeNextLine(s));
        break;
        if (mEditorSettings->enableHeaderToolTips())
            hint = getHeaderFileHint(s, true);
        break;
    case TipType::Identifier:
    case TipType::Selection:
        if (!completionPopupVisible()
                && !headerCompletionPopupVisible()) {
            if (mCanShowEvalTipFunc && mCanShowEvalTipFunc()
                    && (mEditorSettings->enableDebugTooltips())) {
                if (QFileInfo::exists(mFilename)) {
                    showDebugHint(s,p.line);
                }
            } else if (mEditorSettings->enableIdentifierToolTips()) {
                hint = getParserHint(expression, p);
            }
        }
        break;
    case TipType::Number:
        if (QSynedit::isAssemblyLanguage(syntaxer()->language())) {
            bool neg=false;
            qlonglong val;
            bool ok;
            if (s.startsWith("-")) {
                s=s.mid(1);
                neg=true;
            }
            if (s.startsWith("0x")) {
                val=s.toLongLong(&ok,16);
            } else {
                val=s.toLongLong(&ok,10);
            }
            if (ok) {
                if (neg)
                    val = -val;
                hint=tr("hex: %1").arg((qulonglong)val,0,16)
                        +"<br />"
                     +tr("dec: %1").arg(val,0,10);
            }
        }
        break;
    case TipType::Keyword:
        if (mEditorSettings->enableIdentifierToolTips()) {
            if (QSynedit::isAssemblyLanguage(syntaxer()->language())) {
                hint = QSynedit::ASMSyntaxer::Instructions.value(s.toLower(),"");
            }
        }
        break;
    case TipType::Error:
        if (mEditorSettings->enableIssueToolTips())
            hint = getErrorHint(pError);
        break;
    default:
        break;
    }
//        qDebug()<<"hint:"<<hint;
    if (!hint.isEmpty()) {
        //            QApplication* app = dynamic_cast<QApplication *>(QApplication::instance());
        //            if (app->keyboardModifiers().testFlag(Qt::ControlModifier)) {
        if (mFunctionTooltip) mFunctionTooltip->hide();
        QToolTip::showText(mapToGlobal(pos),hint,this);
    }
}

void Editor::onParseFinished()
{
    mIdCache.clear();
    invalidateAllNonTempLineWidth();
    invalidate();
}

void Editor::setCppParser()
{
    if (mGetCppParserFunc)
        setCppParser(mGetCppParserFunc(this));
}

void Editor::setCppParser(PCppParser parser)
{
    if (parser == mParser)
        return;
    if (mParser) {
        disconnect(mParser.get(),
                &CppParser::parseFinished,
                this,
                &Editor::onParseFinished);
        mParser->invalidateFile(mFilename);
    }
    mParser = parser;
    if (mParser) {
        connect(mParser.get(),
                &CppParser::parseFinished,
                this,
                &Editor::onParseFinished);
    }
    //todo:
}

bool Editor::completionPopupVisible() const
{
    return mCompletionPopup && mCompletionPopup->isVisible();
}

bool Editor::headerCompletionPopupVisible() const
{
    return mHeaderCompletionPopup && mHeaderCompletionPopup->isVisible();
}

bool Editor::functionTooltipVisible() const
{
    return mFunctionTooltip && mFunctionTooltip->isVisible();
}

void Editor::loadContent(const QString& filename)
{

}

bool Editor::isBraceChar(QChar ch) const
{
    switch( ch.unicode()) {
    case '{':
    case '}':
    case '[':
    case ']':
    case '(':
    case ')':
        return true;
    default:
        return false;
    }
}

bool Editor::shouldOpenInReadonly()
{
    if (inProject())
        return false;
    return mEditorSettings->readOnlySytemHeader()
                && mParser && (mParser->isSystemHeaderFile(mFilename) || mParser->isProjectHeaderFile(mFilename));
}

void Editor::resetBookmarks(BookmarkModel *model)
{
    mBookmarkLines=model->bookmarksInFile(mFilename,inProject());
    invalidate();
}

void Editor::resetBreakpoints(BreakpointModel *model)
{
    mBreakpointLines.clear();
    foreach (const PBreakpoint& breakpoint, model->breakpoints(inProject())) {
        if (breakpoint->filename == mFilename) {
            mBreakpointLines.insert(breakpoint->line);
        }
    }
    invalidate();
}

bool Editor::notParsed()
{
    if (!mParser)
        return true;
    return mParser->findFileInfo(mFilename)==nullptr;
}

void Editor::breakLine()
{
    processCommand(QSynedit::EditCommand::LineBreak);
}

void Editor::deleteWord()
{
    processCommand(QSynedit::EditCommand::DeleteWord);
}

void Editor::deleteToWordStart()
{
    processCommand(QSynedit::EditCommand::DeleteWordStart);
}

void Editor::deleteToWordEnd()
{
    processCommand(QSynedit::EditCommand::DeleteWordEnd);
}

void Editor::deleteLine()
{
    processCommand(QSynedit::EditCommand::DeleteLine);
}

void Editor::duplicate()
{
    processCommand(QSynedit::EditCommand::Duplicate);
}

void Editor::deleteToEOL()
{
    processCommand(QSynedit::EditCommand::DeleteEOL);
}

void Editor::deleteToBOL()
{
    processCommand(QSynedit::EditCommand::DeleteBOL);
}

void Editor::gotoBlockStart()
{
    processCommand(QSynedit::EditCommand::BlockStart);
}

void Editor::gotoBlockEnd()
{
    processCommand(QSynedit::EditCommand::BlockEnd);
}

void Editor::showCodeCompletion()
{
    if (!mCodeCompletionEnabled)
        return;

    if (mParser) {
        if (mParser->isIncludeLine(lineText())) {
            // is a #include line
            showHeaderCompletion(false);
        } else {
            showCompletion("",true,CodeCompletionType::Normal);
        }
    } else {
        showCompletion("",true,CodeCompletionType::KeywordsOnly);
    }
}

QStringList Editor::getOwnerExpressionAndMemberAtPositionForCompletion(
        const CharPos &pos,
        QString &memberOperator,
        QStringList &memberExpression)
{
    QStringList expression = getExpressionAtPosition(pos);
    // *(Deference) and &(Address-of) has low precedence than '.'/'->',
    //  so don't includes them in the owner expression in comletion calculation
    while (!expression.isEmpty() && (expression.front()=='*' || expression.front()=='&'))
        expression.pop_front();
    return getOwnerExpressionAndMember(expression,memberOperator,memberExpression);
}

QStringList Editor::getExpressionAtPosition(
        const QSynedit::CharPos &pos)
{
    QStringList result;
    if (!parser())
        return result;
    int line = pos.line;
    int ch = pos.ch;
    int symbolMatchingLevel = 0;
    LastSymbolType lastSymbolType=LastSymbolType::None;
    QSynedit::CppSyntaxer syntaxer;
    while (true) {
        if (line>=lineCount() || line<0)
            break;
        QStringList tokens;
        startParseLine(&syntaxer, line);
        while (!syntaxer.eol()) {
            int start = syntaxer.getTokenPos();
            QString token = syntaxer.getToken();
            int endPos = start + token.length()-1;
            if (start>ch) {
                break;
            }
            QSynedit::PTokenAttribute attr = syntaxer.getTokenAttribute();
            if ( (line == pos.line)
                 && (start<=ch) && (ch<=endPos)) {
                if (attr->tokenType() == QSynedit::TokenType::Comment
                        || attr->tokenType() == QSynedit::TokenType::String) {
                    return result;
                }
            }
            if (attr->tokenType() != QSynedit::TokenType::Comment
                    && attr->tokenType() != QSynedit::TokenType::Space){
                tokens.append(token);
            }
            syntaxer.next();
        }
        for (int i=tokens.count()-1;i>=0;i--) {
            QString token = tokens[i];
            if (token=="using")
                return result;
            switch(lastSymbolType) {
            case LastSymbolType::ScopeResolutionOperator: //before '::'
                if (token==">") {
                    lastSymbolType=LastSymbolType::MatchingAngleQuotation;
                    symbolMatchingLevel=0;
                } else if (isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::ObjectMemberOperator: //before '.'
            case LastSymbolType::PointerMemberOperator: //before '->'
            case LastSymbolType::PointerToMemberOfObjectOperator: //before '.*'
            case LastSymbolType::PointerToMemberOfPointerOperator: //before '->*'
                if (token == ")" ) {
                    lastSymbolType=LastSymbolType::MatchingParenthesis;
                    symbolMatchingLevel = 0;
                } else if (token == "]") {
                    lastSymbolType=LastSymbolType::MatchingBracket;
                    symbolMatchingLevel = 0;
                } else if (isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::AsteriskSign: // before '*':
                if (token == '*') {                    
                } else {
                    QChar ch=token.front();
                    if (isIdentChar(ch)
                            || ch.isDigit()
                            || ch == '.'
                            || ch == ')' ) {
                        result.pop_front();
                    }
                    return result;
                }
                break;
            case LastSymbolType::AmpersandSign: // before '&':
            {
                QChar ch=token.front();
                if (isIdentChar(ch)
                        || ch.isDigit()
                        || ch == '.'
                        || ch == ')' ) {
                    result.pop_front();
                }
                return result;
            }
                break;
            case LastSymbolType::ParenthesisMatched: //before '()'
//                if (token == ".") {
//                    lastSymbolType=LastSymbolType::ObjectMemberOperator;
//                } else if (token=="->") {
//                    lastSymbolType = LastSymbolType::PointerMemberOperator;
//                } else if (token == ".*") {
//                    lastSymbolType = LastSymbolType::PointerToMemberOfObjectOperator;
//                } else if (token == "->*"){
//                    lastSymbolType = LastSymbolType::PointerToMemberOfPointerOperator;
//                } else if (token==">") {
//                    lastSymbolType=LastSymbolType::MatchingAngleQuotation;
//                    symbolMatchingLevel=0;
//                } else
                if (token == ")" ) {
                    lastSymbolType=LastSymbolType::MatchingParenthesis;
                    symbolMatchingLevel = 0;
                } else if (token == "]") {
                    lastSymbolType=LastSymbolType::MatchingBracket;
                    symbolMatchingLevel = 0;
                } else if (token == "*") {
                    lastSymbolType=LastSymbolType::AsteriskSign;
                } else if (token == "&") {
                    lastSymbolType=LastSymbolType::AmpersandSign;
                } else if (isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::BracketMatched: //before '[]'
                if (token == ")" ) {
                    lastSymbolType=LastSymbolType::MatchingParenthesis;
                    symbolMatchingLevel = 0;
                } else if (token == "]") {
                    lastSymbolType=LastSymbolType::MatchingBracket;
                    symbolMatchingLevel = 0;
                } else if (isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::AngleQuotationMatched: //before '<>'
                if (isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::None:
                if (token =="::") {
                    lastSymbolType=LastSymbolType::ScopeResolutionOperator;
                } else if (token == ".") {
                    lastSymbolType=LastSymbolType::ObjectMemberOperator;
                } else if (token=="->") {
                    lastSymbolType = LastSymbolType::PointerMemberOperator;
                } else if (token == ".*") {
                    lastSymbolType = LastSymbolType::PointerToMemberOfObjectOperator;
                } else if (token == "->*"){
                    lastSymbolType = LastSymbolType::PointerToMemberOfPointerOperator;
                } else if (token == ")" ) {
                    lastSymbolType=LastSymbolType::MatchingParenthesis;
                    symbolMatchingLevel = 0;
                } else if (token == "]") {
                    lastSymbolType=LastSymbolType::MatchingBracket;
                    symbolMatchingLevel = 0;
                } else if (isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::TildeSign:
                if (token =="::") {
                    lastSymbolType=LastSymbolType::ScopeResolutionOperator;
                } else {
                    // "~" must appear after "::"
                    result.pop_front();
                    return result;
                }
                break;;
            case LastSymbolType::Identifier:
                if (token =="::") {
                    lastSymbolType=LastSymbolType::ScopeResolutionOperator;
                } else if (token == ".") {
                    lastSymbolType=LastSymbolType::ObjectMemberOperator;
                } else if (token=="->") {
                    lastSymbolType = LastSymbolType::PointerMemberOperator;
                } else if (token == ".*") {
                    lastSymbolType = LastSymbolType::PointerToMemberOfObjectOperator;
                } else if (token == "->*"){
                    lastSymbolType = LastSymbolType::PointerToMemberOfPointerOperator;
                } else if (token == "~") {
                    lastSymbolType=LastSymbolType::TildeSign;
                } else if (token == "*") {
                    lastSymbolType=LastSymbolType::AsteriskSign;
                } else if (token == "&") {
                    lastSymbolType=LastSymbolType::AmpersandSign;
                } else
                    return result; // stop matching;
                break;
            case LastSymbolType::MatchingParenthesis:
                if (token=="(") {
                    if (symbolMatchingLevel==0) {
                        lastSymbolType=LastSymbolType::ParenthesisMatched;
                    } else {
                        symbolMatchingLevel--;
                    }
                } else if (token==")") {
                    symbolMatchingLevel++;
                }
                break;
            case LastSymbolType::MatchingBracket:
                if (token=="[") {
                    if (symbolMatchingLevel==0) {
                        lastSymbolType=LastSymbolType::BracketMatched;
                    } else {
                        symbolMatchingLevel--;
                    }
                } else if (token=="]") {
                    symbolMatchingLevel++;
                }
                break;
            case LastSymbolType::MatchingAngleQuotation:
                if (token=="<") {
                    if (symbolMatchingLevel==0) {
                        lastSymbolType=LastSymbolType::AngleQuotationMatched;
                    } else {
                        symbolMatchingLevel--;
                    }
                } else if (token==">") {
                    symbolMatchingLevel++;
                }
                break;
            }
            result.push_front(token);
        }

        line--;
        if (line>=0)
            ch = document()->getLine(line).length()+1;
    }
    return result;
}

QString Editor::getWordForCompletionSearch(const CharPos &pos,bool permitTilde)
{
    QString result = "";
    QString s;

    s = lineText(pos.line);
    int len = s.length();

    int wordBegin = pos.ch - 1 ; //BufferCoord::Char starts with 1
    int wordEnd = pos.ch - 1 ;

    while ((wordBegin >= 0) && (wordBegin<len)) {
        if (isIdentChar(s[wordBegin])) {
            wordBegin--;
        } else if (permitTilde && s[wordBegin] == '~') { // allow destructor signs
            wordBegin--;
        } else
            break;
    }
    // Get end result
    return s.mid(wordBegin+1, wordEnd - wordBegin);
}

QChar Editor::getCurrentChar()
{
    return charAt(caretXY());
}

bool Editor::handleSymbolCompletion(QChar key)
{
    if (!mEditorSettings->completeSymbols())
        return false;
    if (!insertMode())
        return false;

    //todo: better methods to detect current caret type
    if (caretX() <= 0) {
        if (caretY()>0) {
            if (syntaxer()->isCommentNotFinished(document()->getSyntaxState(caretY() - 1)))
                return false;
            if (syntaxer()->isStringNotFinished(document()->getSyntaxState(caretY() - 1))
                    && (key!='\"') && (key!='\''))
                return false;
        }
    } else {
        CharPos  highlightPos = CharPos{caretX()-1, caretY()};
        // Check if that line is highlighted as  comment
        QSynedit::PTokenAttribute attr;
        QString token;
        QSynedit::PSyntaxState syntaxState;
        if (getTokenAttriAtRowCol(highlightPos, token, attr, syntaxState)) {
            if (syntaxer()->isCommentNotFinished(syntaxState))
                return false;
            if (syntaxer()->isStringNotFinished(syntaxState)
                    && (key!='\'') && (key!='\"') && (key!='(') && (key!=')'))
                return false;
            if (( key=='<' || key =='>') && (mParser && !mParser->isIncludeLine(lineText())))
                return false;
            if ((key == '\'') && (attr->name() == "SYNS_AttrNumber"))
                return false;
        }
    }

    QuoteStatus status;
    switch(key.unicode()) {
    case '(':
        if (mEditorSettings->completeParenthese()) {
            return handleParentheseCompletion();
        }
        return false;
    case ')':
        if (selAvail())
            return false;
        if (mEditorSettings->completeParenthese() && mEditorSettings->overwriteSymbols()) {
            return handleParentheseSkip();
        }
        return false;
    case '[':
          if (mEditorSettings->completeBracket()) {
              return handleBracketCompletion();
          }
          return false;
    case ']':
        if (selAvail())
            return false;
        if (mEditorSettings->completeBracket() && mEditorSettings->overwriteSymbols()) {
            return handleBracketSkip();
        }
        return false;
    case '*':
        status = getQuoteStatus();
        if (mEditorSettings->completeComment() && (status == QuoteStatus::NotQuote)) {
            return handleMultilineCommentCompletion();
        }
        return false;
    case '{':
        if (mEditorSettings->completeBrace()) {
            return handleBraceCompletion();
        }
        return false;
    case '}':
        if (selAvail())
            return false;
        if (mEditorSettings->completeBrace() && mEditorSettings->overwriteSymbols()) {
            return handleBraceSkip();
        }
        return false;
    case '\'':
        if (mEditorSettings->completeSingleQuote()) {
            return handleSingleQuoteCompletion();
        }
        return false;
    case '\"':
        if (mEditorSettings->completeDoubleQuote()) {
            return handleDoubleQuoteCompletion();
        }
        return false;
    case '<':
        if (selAvail())
            return false;
        if (mEditorSettings->completeGlobalInclude()) { // #include <>
            return handleGlobalIncludeCompletion();
        }
        return false;
    case '>':
        if (selAvail())
            return false;
        if (mEditorSettings->completeGlobalInclude() && mEditorSettings->overwriteSymbols()) { // #include <>
            return handleGlobalIncludeSkip();
        }
        return false;
    case ';':
        if (selAvail())
            return false;
        if (mEditorSettings->overwriteSymbols()) {
            return handleSemiColonSkip();
        }
        return false;
    case ',':
        if (selAvail())
            return false;
        if (mEditorSettings->overwriteSymbols()) {
            return handlePeriodSkip();
        }
        return false;
    }
    return false;
}

bool Editor::handleParentheseCompletion()
{
    QuoteStatus status = getQuoteStatus();
    if (status == QuoteStatus::RawString || status == QuoteStatus::NotQuote) {
        if (selAvail() && status == QuoteStatus::NotQuote) {
            QString text=selText();
            beginEditing();
            processCommand(QSynedit::EditCommand::Input,'(');
            setSelText(text);
            processCommand(QSynedit::EditCommand::Input,')');
            endEditing();
        } else {
            beginEditing();
            processCommand(QSynedit::EditCommand::Input,'(');
            CharPos oldCaret = caretXY();
            processCommand(QSynedit::EditCommand::Input,')');
            setCaretXY(oldCaret);
            endEditing();
        }
        return true;
    }
    return false;
}

bool Editor::handleParentheseSkip()
{
      if (getCurrentChar() != ')')
          return false;
      QuoteStatus status = getQuoteStatus();
      if (status == QuoteStatus::RawStringEnd) {
          setCaretXY( CharPos{caretX() + 1, caretY()}); // skip over
          return true;
      }
      if (status != QuoteStatus::NotQuote)
          return false;

      if (lineCount()==0)
          return false;
      if (syntaxer()->supportBraceLevel()) {
          QSynedit::PSyntaxState lastLineState = document()->getSyntaxState(lineCount()-1);
          if (lastLineState->parenthesisLevel==0) {
              setCaretXY( CharPos{caretX() + 1, caretY()}); // skip over
              return true;
          }
      } else {
          CharPos pos = getMatchingBracket();
          if (pos.isValid()) {
              setCaretXY( CharPos{caretX() + 1, caretY()}); // skip over
              return true;
          }
      }
      return false;
}

bool Editor::handleBracketCompletion()
{
//    QuoteStatus status = getQuoteStatus();
//    if (status == QuoteStatus::RawString || status == QuoteStatus::NotQuote) {
    QuoteStatus status = getQuoteStatus();
    if (selAvail() && status == QuoteStatus::NotQuote) {
        QString text=selText();
        beginEditing();
        processCommand(QSynedit::EditCommand::Input,'[');
        setSelText(text);
        processCommand(QSynedit::EditCommand::Input,']');
        endEditing();
    } else {
        beginEditing();
        processCommand(QSynedit::EditCommand::Input,'[');
        CharPos oldCaret = caretXY();
        processCommand(QSynedit::EditCommand::Input,']');
        setCaretXY(oldCaret);
        endEditing();
    }
    return true;
        //    }
}

bool Editor::handleBracketSkip()
{
    if (getCurrentChar() != ']')
        return false;

    if (lineCount()==0)
        return false;
    if (syntaxer()->supportBraceLevel()) {
        QSynedit::PSyntaxState lastLineState = document()->getSyntaxState(lineCount()-1);
        if (lastLineState->bracketLevel==0) {
            setCaretXY( CharPos{caretX() + 1, caretY()}); // skip over
            return true;
        }
    } else {
        CharPos pos = getMatchingBracket();
        if (pos.isValid()) {
            setCaretXY( CharPos{caretX() + 1, caretY()}); // skip over
            return true;
        }
    }
    return false;
}

bool Editor::handleMultilineCommentCompletion()
{
    if ((caretX()-1>=0) && (caretX()-1 < lineText().length()) && (lineText()[caretX()] == '/')) {
        QString text=selText();
        beginEditing();
        processCommand(QSynedit::EditCommand::Input,'*');
        CharPos oldCaret;
        if (text.isEmpty())
            oldCaret = caretXY();
        else
            setSelText(text);
        processCommand(QSynedit::EditCommand::Input,'*');
        processCommand(QSynedit::EditCommand::Input,'/');
        if (text.isEmpty())
            setCaretXY(oldCaret);
        endEditing();
        return true;
    }
    return false;
}

bool Editor::handleBraceCompletion()
{
    bool addSemicolon=false;
    QString sLine = lineText().trimmed();
    int i= caretY();
    while ((sLine.isEmpty()) && (i>=0)) {
        sLine=lineText(i).trimmed();
        i--;
    }
    if (
        ( ( (sLine.startsWith("struct") && !sLine.endsWith(")"))
          || sLine.startsWith("class")
          || (sLine.startsWith("union") && !sLine.endsWith(")"))
          || sLine.startsWith("typedef")
          || sLine.startsWith("public")
          || sLine.startsWith("private")
          || (sLine.startsWith("enum") && !sLine.endsWith(")")) )
          && !sLine.contains(';')
        ) || sLine.endsWith('=')) {
        addSemicolon = true;
//        processCommand(QSynedit::EditCommand::Input,';');
    }

    beginEditing();
    if (!selAvail()) {
        processCommand(QSynedit::EditCommand::Input,'{');
        CharPos oldCaret = caretXY();
        processCommand(QSynedit::EditCommand::Input,'}');
        if (addSemicolon)
            processCommand(QSynedit::EditCommand::Input,';');
        setCaretXY(oldCaret);
    }  else {
        QString text = selText();
        CharPos oldSelBegin = selBegin();
        CharPos oldSelEnd = selEnd();
        bool shouldBreakLine = false;
        bool shouldAddEndLine = false;
        QString s1=lineText(oldSelBegin.line).left(oldSelBegin.ch).trimmed();

        if (s1.isEmpty() ) {
            QString s2 = lineText(oldSelEnd.line);
            if (s2.left(oldSelEnd.ch).trimmed().isEmpty()) {
                shouldBreakLine = true;
            } else if (oldSelEnd.ch >= trimRight(s2).length()) {
                shouldBreakLine = true;
            }
            if (shouldBreakLine)
                shouldAddEndLine = !s2.mid(oldSelEnd.ch).trimmed().isEmpty();
        }
        if (shouldBreakLine) {
            text = "{" + lineBreak() + text;
            if (!trimRight(text).endsWith(lineBreak())) {
                text.append(lineBreak());
            }
        } else {
            text = "{ "+text+" ";
        }
        if (addSemicolon)
            text.append("};");
        else
            text.append("}");
        if (shouldAddEndLine)
            text.append(lineBreak());
        setSelText(text);
    }
    endEditing();
    return true;
}

bool Editor::handleBraceSkip()
{
    if (getCurrentChar() != '}')
        return false;

    if (lineCount()==0)
        return false;

    if (syntaxer()->supportBraceLevel() && caretY()>=1) {
        QSynedit::PSyntaxState lastLineState = document()->getSyntaxState(caretY()-1);
        if (lastLineState->braceLevel==0) {
            bool oldInsertMode = insertMode();
            setInsertMode(false); //set mode to overwrite
            processCommand(QSynedit::EditCommand::Input,'}');
            setInsertMode(oldInsertMode);
            return true;
        }
    } else {
        CharPos pos = getMatchingBracket();
        if (pos.isValid()) {
            bool oldInsertMode = insertMode();
            setInsertMode(false); //set mode to overwrite
            processCommand(QSynedit::EditCommand::Input,'}');
            setInsertMode(oldInsertMode);
            return true;
        }
    }
    return false;
}

bool Editor::handleSemiColonSkip()
{
    if (getCurrentChar() != ';')
        return false;
    bool oldInsertMode = insertMode();
    setInsertMode(false); //set mode to overwrite
    processCommand(QSynedit::EditCommand::Input,';');
    setInsertMode(oldInsertMode);
    return true;
}

bool Editor::handlePeriodSkip()
{
    if (getCurrentChar() != ',')
        return false;

    bool oldInsertMode = insertMode();
    setInsertMode(false); //set mode to overwrite
    processCommand(QSynedit::EditCommand::Input,',');
    setInsertMode(oldInsertMode);
    return true;
}

bool Editor::handleSingleQuoteCompletion()
{
    QuoteStatus status = getQuoteStatus();
    QChar ch = getCurrentChar();
    if (ch == '\'') {
        if (status == QuoteStatus::SingleQuote && !selAvail()) {
            setCaretXY( CharPos{caretX() + 1, caretY()}); // skip over
            return true;
        }
    } else {
        if (status == QuoteStatus::NotQuote) {
            if (selAvail()) {
                QString text=selText();
                beginEditing();
                processCommand(QSynedit::EditCommand::Input,'\'');
                setSelText(text);
                processCommand(QSynedit::EditCommand::Input,'\'');
                endEditing();
                return true;
            }
            if (ch == 0 || syntaxer()->isWordBreakChar(ch) || syntaxer()->isSpaceChar(ch)) {
                // insert ''
                beginEditing();
                processCommand(QSynedit::EditCommand::Input,'\'');
                CharPos oldCaret = caretXY();
                processCommand(QSynedit::EditCommand::Input,'\'');
                setCaretXY(oldCaret);
                endEditing();
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
    if ((ch == '"') && (status == QuoteStatus::DoubleQuote || status == QuoteStatus::RawStringEnd)
            && !selAvail()) {
        setCaretXY( CharPos{caretX() + 1, caretY()}); // skip over
        return true;
    } else {
        if (status == QuoteStatus::NotQuote) {
            if (selAvail()) {
                QString text=selText();
                setSelText('"'+text+'"');
                return true;
            }
            if ((ch == 0)
                    || ( syntaxer()->isWordBreakChar(ch)
                             || syntaxer()->isSpaceChar(ch))) {
                // insert ""
                beginEditing();
                int oldCaretX = caretX();
                processCommand(QSynedit::EditCommand::Input,"\"\"");
                setCaretX(oldCaretX+1);
                endEditing();
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
    beginEditing();
    processCommand(QSynedit::EditCommand::Input,'<');
    CharPos oldCaret = caretXY();
    processCommand(QSynedit::EditCommand::Input,'>');
    setCaretXY(oldCaret);
    endEditing();
    return true;
}

bool Editor::handleGlobalIncludeSkip()
{
    if (getCurrentChar()!='>')
        return false;
    QString s= lineText().mid(1).trimmed();
    if (!s.startsWith("include"))  //it's not #include
        return false;
    CharPos pos = getMatchingBracket();
    if (pos.isValid()) {
        setCaretXY(CharPos{caretX()+1, caretY()}); // skip over
        return true;
    }
    return false;
}

bool Editor::handleCodeCompletion(QChar key)
{
    if (!mCompletionPopup || !mCompletionPopup->isEnabled())
        return false;
    if (mParser) {
        switch(key.unicode()) {
        case '.':
            processCommand(QSynedit::EditCommand::Input, key);
            showCompletion("",false,CodeCompletionType::Normal);
            return true;
        case '>':
            processCommand(QSynedit::EditCommand::Input, key);
            if ((caretX() > 2) && (lineText().length() >= 2) &&
                    (lineText()[caretX() - 3] == '-'))
                showCompletion("",false,CodeCompletionType::Normal);
            return true;
        case ':':
            processCommand(QSynedit::EditCommand::Input,':');
            //setSelText(key);
            if ((caretX() > 2) && (lineText().length() >= 2) &&
                    (lineText()[caretX() - 3] == ':'))
                showCompletion("",false,CodeCompletionType::Normal);
            return true;
        case '/':
        case '\\':
            processCommand(QSynedit::EditCommand::Input, key);
            if (mParser->isIncludeLine(lineText())) {
                showHeaderCompletion(false);
            }
            return true;
        default:
            return false;
        }
    }
    return false;
}

ParserLanguage Editor::calcParserLanguage() const
{
#ifdef ENABLE_SDCC
    if (mGetCompilerTypeForEditorFunc && mGetCompilerTypeForEditorFunc(this) == CompilerType::SDCC)
        return ParserLanguage::SDCC;
#endif
    switch(mFileType) {
    case FileType::CppSource:
        return ParserLanguage::CPlusPlus;
    case FileType::CSource:
        return ParserLanguage::C;
    case FileType::CCppHeader:
    {
        FileType contextFileType = getFileType(mContextFile);
        if (contextFileType==FileType::CppSource)
            return ParserLanguage::CPlusPlus;
        else if (contextFileType == FileType::CSource)
            return ParserLanguage::C;
    }
        return (mEditorSettings->defaultFileCpp())?ParserLanguage::CPlusPlus:ParserLanguage::C;
    default:
        return ParserLanguage::None;
    }
}

Editor::QuoteStatus Editor::getQuoteStatus()
{
    if (syntaxer()->language()==QSynedit::ProgrammingLanguage::CPP) {
        std::shared_ptr<QSynedit::CppSyntaxer> cppSyntaxer = std::dynamic_pointer_cast<QSynedit::CppSyntaxer>(syntaxer());
        QSynedit::PSyntaxState state;

        //raw string end must be determined with the following '"'
        QString token;
        QSynedit::PTokenAttribute attribute;
        if (getTokenAttriAtRowCol(caretXY(),token,attribute,state)
                && cppSyntaxer->isRawStringEnd(state))
            return QuoteStatus::RawStringEnd;

        QString s = lineText().mid(0,caretX());
        state = calcSyntaxStateAtLine(caretY(), s, false);
        if (syntaxer()->isStringNotFinished(state)) {
            if (cppSyntaxer->isStringEscaping(state))
                return QuoteStatus::DoubleQuoteEscape;
            else
                return QuoteStatus::DoubleQuote;
        }
        if (cppSyntaxer->isCharNotFinished(state)) {
            if (cppSyntaxer->isCharEscaping(state))
                return QuoteStatus::SingleQuoteEscape;
            else
                return QuoteStatus::SingleQuote;
        }
        if (cppSyntaxer->isRawStringNoEscape(state))
            return QuoteStatus::RawStringNoEscape;
        if (cppSyntaxer->isRawStringStart(state))
            return QuoteStatus::RawString;
    }
    return QuoteStatus::NotQuote;
}

void Editor::reparse()
{
    if (!mInited)
        return;
    if (!mCodeCompletionEnabled)
        return;
    if (!mParser)
        return;
    Q_ASSERT(syntaxer()->language() == QSynedit::ProgrammingLanguage::CPP);
    if (!mParser->enabled())
        return;
//    qDebug()<<"reparse "<<mFilename;
    //mParser->setEnabled(mCodeCompletionSettings->enabled());
    parseFileNonBlocking(mParser,mFilename, inProject(), mContextFile);
}

void Editor::reparseIfNeeded()
{
    if (needReparse()) {
        reparse();
    }
}

void Editor::reparseTodo()
{
    if (mEditorSettings->parseTodos())
        emit parseTodoRequested(mFilename, inProject());
}

void Editor::insertString(const QString &value, bool moveCursor)
{
    beginEditing();
    auto action = finally([this]{
        endEditing();
    });

    CharPos oldCursorPos = caretXY();
    setSelText(value);
    if (!moveCursor) {
        setCaretXY(oldCursorPos);
    }
}

void Editor::insertCodeSnippet(const QString &code)
{
    clearUserCodeInTabStops();
    mXOffsetSince = 0;
    mTabStopBegin = -1;
    mTabStopEnd = -1;
    mTabStopY =0;
    mLineBeforeTabStop = "";
    mLineAfterTabStop = "";
    // prevent lots of repaints
    beginEditing();
    auto action = finally([this]{
        endEditing();
    });
    if (selAvail())
        setSelText("");
    QStringList sl = textToLines(parseMacros(code, mGetMacroVarsFunc));
    int lastI=0;
//    int spaceCount = GetLeftSpacing(
//                leftSpaces(lineText()),true).length();
    QStringList newSl;
    for (int i=0;i<sl.count();i++) {
        int lastPos = 0;
        QString s = sl[i];
        if (i>0)
            lastPos = countLeadingWhitespaceChars(s);
        while (true) {
            int insertPos = s.indexOf(USER_CODE_IN_INSERT_POS);
            if (insertPos < 0) // no %INSERT% macro in this line now
                break;
            PTabStop p = std::make_shared<TabStop>();
            s.remove(insertPos, QString(USER_CODE_IN_INSERT_POS).length());
            //insertPos--;
            p->x = insertPos - lastPos;
            p->endX = p->x ;
            p->y = i - lastI;
            lastPos = insertPos;
            lastI = i;
            mUserCodeInTabStops.append(p);
        }
        lastPos = 0;
        if (i>0)
            lastPos = countLeadingWhitespaceChars(s);
        while (true) {
            int insertPos = s.indexOf(USER_CODE_IN_REPL_POS_BEGIN);
            if (insertPos < 0) // no %INSERT% macro in this line now
                break;
            PTabStop p = std::make_shared<TabStop>();
            s.remove(insertPos, QString(USER_CODE_IN_REPL_POS_BEGIN).length());
            //insertPos--;
            p->x = insertPos - lastPos;

            int insertEndPos = insertPos +
                    s.mid(insertPos).indexOf(USER_CODE_IN_REPL_POS_END);
            if (insertEndPos < insertPos) {
                p->endX = s.length();
            } else {
                s.remove(insertEndPos, QString(USER_CODE_IN_REPL_POS_END).length());
                //insertEndPos--;
                p->endX = insertEndPos - lastPos;
            }
            p->y=i-lastI;
            lastPos = insertEndPos;
            lastI = i;
            mUserCodeInTabStops.append(p);
        }
        newSl.append(s);
    }

    CharPos cursorPos = caretXY();
    QString s = linesToText(newSl);
    setSelText(s);
    if (mUserCodeInTabStops.count()>0) {
        setCaretXY(cursorPos); //restore cursor pos before insert
        mTabStopBegin = caretX();
        mTabStopEnd = caretX();
        popUserCodeInTabStops();
    }
//    if (!code.isEmpty()) {
//        mLastIdCharPressed = 0;
//    }
}

void Editor::print()
{
    QPrinter printer;

    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Print Document"));
    dialog.setOption(QAbstractPrintDialog::PrintCurrentPage,false);
    dialog.setOption(QAbstractPrintDialog::PrintPageRange,false);

    if (selAvail())
        dialog.setOption(QAbstractPrintDialog::PrintSelection,true);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QSynedit::QtSupportedHtmlExporter exporter(tabSize(), pCharsetInfoManager->getDefaultSystemEncoding());

    exporter.setTitle(QFileInfo(mFilename).fileName());
    exporter.setUseBackground(mEditorSettings->copyHTMLUseBackground());

    exporter.setFont(font());
    QSynedit::PSyntaxer hl = syntaxer();
    if (!mEditorSettings->copyHTMLUseEditorColor()) {
        hl = SyntaxerManager::copy(syntaxer());
        mColorManager->applySchemeToSyntaxer(hl,mEditorSettings->copyHTMLColorScheme());
    }
    exporter.setSyntaxer(hl);
    exporter.setOnFormatToken(std::bind(&Editor::onExportedFormatToken,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3,
                                        std::placeholders::_4,
                                        std::placeholders::_5
                                        ));

    if (dialog.testOption(QAbstractPrintDialog::PrintSelection))
        exporter.exportRange(document(),selBegin(),selEnd());
    else
        exporter.exportAll(document());

    QString html = exporter.text();
    QTextDocument doc;

    doc.setDefaultFont(font());
    doc.setHtml(html);
    doc.print(&printer);
}

void Editor::exportAsRTF(const QString &rtfFilename)
{
    QSynedit::RTFExporter exporter(tabSize(), pCharsetInfoManager->getDefaultSystemEncoding());
    exporter.setTitle(extractFileName(rtfFilename));
    exporter.setUseBackground(mEditorSettings->copyRTFUseBackground());
    exporter.setFont(font());
    QSynedit::PSyntaxer hl = syntaxer();
    if (!mEditorSettings->copyRTFUseEditorColor()) {
        hl = SyntaxerManager::copy(syntaxer());
        mColorManager->applySchemeToSyntaxer(hl,mEditorSettings->copyRTFColorScheme());
    }
    exporter.setSyntaxer(hl);
    exporter.setOnFormatToken(std::bind(&Editor::onExportedFormatToken,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3,
                                        std::placeholders::_4,
                                        std::placeholders::_5
                                        ));
    exporter.exportAll(document());
    exporter.saveToFile(rtfFilename);
}

void Editor::exportAsHTML(const QString &htmlFilename)
{
    QSynedit::HTMLExporter exporter(tabSize(), pCharsetInfoManager->getDefaultSystemEncoding());
    exporter.setTitle(extractFileName(htmlFilename));
    exporter.setUseBackground(mEditorSettings->copyHTMLUseBackground());
    exporter.setFont(font());
    QSynedit::PSyntaxer hl = syntaxer();
    if (!mEditorSettings->copyHTMLUseEditorColor()) {
        hl = SyntaxerManager::copy(syntaxer());
        mColorManager->applySchemeToSyntaxer(hl,mEditorSettings->copyHTMLColorScheme());
    }
    exporter.setSyntaxer(hl);
    exporter.setOnFormatToken(std::bind(&Editor::onExportedFormatToken,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3,
                                        std::placeholders::_4,
                                        std::placeholders::_5
                                        ));
    if (mEditorSettings->copyHTMLWithLineNumber()) {
        exporter.setExportLineNumber(true);
        exporter.setRecalcLineNumber(false);
        exporter.setLineNumberStartFromZero(mEditorSettings->gutterLineNumbersStartZero());
        exporter.setLineNumberColor(gutter().textColor());
        exporter.setLineNumberBackgroundColor(gutter().color());
    }
    exporter.exportAll(document());
    exporter.saveToFile(htmlFilename);
}

void Editor::showCompletion(const QString& preWord,bool autoComplete, CodeCompletionType type)
{
    if(!mCompletionPopup)
        return;
    if (mFunctionTooltip) mFunctionTooltip->hide();
    if (!mCodeCompletionEnabled)
        return;
    if (type==CodeCompletionType::KeywordsOnly) {
    } else {
        if (!mParser || !mParser->enabled())
            return;
    }

    if (completionPopupVisible()) // already in search, don't do it again
        return;

    QString word="";

    QString s;
    QSynedit::PTokenAttribute attr;
    CharPos pBeginPos, pEndPos;
    if (getTokenAttriAtRowCol(
                CharPos{caretX() - 1,
                caretY()}, s, attr)) {
        if (attr->tokenType() == QSynedit::TokenType::Preprocessor) {//Preprocessor
            word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpDirective);
            if (!word.startsWith('#')) {
                word = "";
            }
        } else if (attr->tokenType() == QSynedit::TokenType::Comment) { //Comment, javadoc tag
            word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpJavadoc);
            if (!word.startsWith('@')) {
                    return;
            }
        } else if (
                   (attr->tokenType() == QSynedit::TokenType::String) &&
                   (attr->tokenType() != QSynedit::TokenType::Character)) {
            return;
        } else if (type==CodeCompletionType::KeywordsOnly ) {
            if (syntaxer()->language()==QSynedit::ProgrammingLanguage::GNU_Assembly
                    && std::dynamic_pointer_cast<QSynedit::GASSyntaxer>(syntaxer())->prefixRegisterNames() )
                word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpATTASMKeywords);
            else if (fileType() == FileType::NASM && attr->tokenType() == QSynedit::TokenType::Preprocessor)
                word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpATTASMKeywords);
            else
                word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpKeywords);
        } else if (
                   (attr->tokenType() != QSynedit::TokenType::Symbol) &&
                   (attr->tokenType() != QSynedit::TokenType::Space) &&
                   (attr->tokenType() != QSynedit::TokenType::Keyword) &&
                   (attr->tokenType() != QSynedit::TokenType::Identifier)
                   ) {
            return;
        }
    }

    // Scan the current function body
    QSet<QString> keywords;

    if (syntaxer()->language() != QSynedit::ProgrammingLanguage::CPP ) {
        if (QSynedit::isAssemblyLanguage(syntaxer()->language())) {
            if (word.startsWith(".")) {
                keywords = syntaxer()->keywords(".");
            } else if (word.startsWith("%")) {
                keywords = syntaxer()->keywords("%");
            } else {
                keywords = syntaxer()->keywords("");
            }
        } else {
            int pos = word.lastIndexOf(".");
            if (pos>=0) {
                QString scopeWord=word.left(pos);
                word = word.mid(pos+1);
                QMap<QString, QSet<QString> > scopedKeywords = syntaxer()->scopedKeywords();
                keywords = scopedKeywords.value(scopeWord, QSet<QString>());
            } else
                keywords = syntaxer()->keywords();
        }
    } else {
        switch(calcParserLanguage()) {
        case ParserLanguage::CPlusPlus:
            for(auto it = CppKeywords.begin();it!=CppKeywords.end();++it) {
                keywords.insert(it.key());
            }
            break;
        case ParserLanguage::C:
            keywords = CKeywords;
            break;
#ifdef ENABLE_SDCC
        case ParserLanguage::SDCC:
            keywords = CKeywords;
            for(auto it = SDCCKeywords.begin();it!=SDCCKeywords.end();++it) {
                keywords.insert(it.key());
            }
            break;
#endif
        case ParserLanguage::None:
            Q_ASSERT(false); // shouldn't happen here
            break;
        }
        if (mEditorSettings->enableCustomCTypeKeywords()) {
            foreach (const QString& keyword, mEditorSettings->customCTypeKeywords()) {
                keywords.insert(keyword);
            }
        }
    }

    if (type == CodeCompletionType::KeywordsOnly && keywords.isEmpty())
        return;

    mCompletionPopup->setRecordUsage(mCodeCompletionSettings->recordUsage());
    mCompletionPopup->setSortByScope(mCodeCompletionSettings->sortByScope());
    mCompletionPopup->setShowKeywords(mCodeCompletionSettings->showKeywords());
    if (type!=CodeCompletionType::Normal) {
        mCompletionPopup->setShowCodeSnippets(false);
    } else {
        mCompletionPopup->setShowCodeSnippets(mCodeCompletionSettings->showCodeIns());
        if (mCodeCompletionSettings->showCodeIns() && mCodeSnippetsManager) {
            mCompletionPopup->setCodeSnippets(mCodeSnippetsManager->snippets());
        }
    }
    mCompletionPopup->setHideSymbolsStartWithUnderline(mCodeCompletionSettings->hideSymbolsStartsWithUnderLine());
    mCompletionPopup->setHideSymbolsStartWithTwoUnderline(mCodeCompletionSettings->hideSymbolsStartsWithTwoUnderLine());
    mCompletionPopup->setIgnoreCase(mCodeCompletionSettings->ignoreCase());
    QSize popSize = calcCompletionPopupSize();
    mCompletionPopup->resize(popSize);

    // Position it at the top of the next line
    QPoint popupPos = mapToGlobal(displayCoordToPixels(displayXY()));
    QSize  desktopSize = screen()->virtualSize();
    if (desktopSize.height() - popupPos.y() < mCompletionPopup->height() && popupPos.y() > mCompletionPopup->height())
        popupPos-=QPoint(0, mCompletionPopup->height()+2);
    else
        popupPos+=QPoint(0,textHeight()+2);

    if (desktopSize.width() -  popupPos.x() < mCompletionPopup->width() ) {
        popupPos.setX(std::max(0, desktopSize.width()-mCompletionPopup->width())-10);
    }

    mCompletionPopup->move(popupPos);

    //Set Font size;
    mCompletionPopup->setFont(font());
    mCompletionPopup->setLineHeightFactor(mEditorSettings->lineSpacing());
    // Redirect key presses to completion box if applicable
    //todo:
    mCompletionPopup->setKeypressedCallback([this](QKeyEvent *event)->bool{
        return onCompletionKeyPressed(event);
    });
    mCompletionPopup->setParser(mParser);
    if (mParser) {
        mCompletionPopup->setCurrentScope(
                    mParser->findScopeStatement(mFilename, caretY())
                    );
    }
    if (mFunctionTooltip) mFunctionTooltip->hide();
    mCompletionPopup->show();

    if (word.isEmpty()) {
        //word=getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpCompletion);
        QString memberOperator;
        QStringList memberExpression;
        CharPos pos{caretX()-1, caretY()};
        QStringList ownerExpression = getOwnerExpressionAndMemberAtPositionForCompletion(
                    pos,
                    memberOperator,
                    memberExpression);
        word = memberExpression.join("");
        mCompletionPopup->prepareSearch(
                    preWord,
                    ownerExpression,
                    memberOperator,
                    memberExpression,
                    mFilename,
                    caretY(),
                    type,
                    keywords);
    } else {
        QStringList memberExpression;
        memberExpression.append(word);
        mCompletionPopup->prepareSearch(preWord,
                                        QStringList(),
                                        "",
                                        memberExpression, mFilename, caretY(),type,keywords);
    }

    // Filter the whole statement list
    if (mCompletionPopup->search(word, autoComplete, mEditorSettings->colorScheme())) { //only one suggestion and it's not input while typing
        completionInsert(mCodeCompletionSettings->appendFunc());
    }
}

void Editor::showHeaderCompletion(bool autoComplete, bool forceShow)
{
    if (!mHeaderCompletionPopup)
        return;
    if (!mCodeCompletionEnabled)
        return;
//    if not devCodeCompletion.Enabled then
//      Exit;

    if (!forceShow && mHeaderCompletionPopup->isVisible()) // already in search, don't do it again
        return;

    CharPos  highlightPos{caretX()-1, caretY()};
    // Check if that line is highlighted as  comment
    QSynedit::PTokenAttribute attr;
    QString token;
    QSynedit::PSyntaxState syntaxState;
    if (getTokenAttriAtRowCol(highlightPos, token, attr, syntaxState)) {
        if (attr && attr->tokenType()==QSynedit::TokenType::Comment)
            return;
    }

    // Position it at the top of the next line
    QPoint p = displayCoordToPixels(displayXY());
    p.setY(p.y() + textHeight() + 2);
    mHeaderCompletionPopup->move(mapToGlobal(p));

    mHeaderCompletionPopup->setIgnoreCase(mCodeCompletionSettings->ignoreCase());

    QSize popSize = calcCompletionPopupSize();
    mHeaderCompletionPopup->resize(popSize);
    //Set Font size;
    mHeaderCompletionPopup->setFont(font());
    mHeaderCompletionPopup->setLineHeightFactor(mEditorSettings->lineSpacing());

    // Redirect key presses to completion box if applicable
    mHeaderCompletionPopup->setKeypressedCallback([this](QKeyEvent* event)->bool{
        return onHeaderCompletionKeyPressed(event);
    });
    mHeaderCompletionPopup->setParser(mParser);

    CharPos pBeginPos,pEndPos;
    QString word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos,
                                     WordPurpose::wpHeaderCompletionStart);

    if (word.isEmpty())
        return;

    if (!word.startsWith('"') && !word.startsWith('<'))
        return;

    if (word.lastIndexOf('"')>0 || word.lastIndexOf('>')>0)
        return;

    if (mFunctionTooltip) mFunctionTooltip->hide();
    mHeaderCompletionPopup->show();
    mHeaderCompletionPopup->setSearchLocal(word.startsWith('"'));
    word.remove(0,1);

    mHeaderCompletionPopup->prepareSearch(word, mFilename);

    // Filter the whole statement list
    if (mHeaderCompletionPopup->search(word, autoComplete, mEditorSettings->colorScheme())) //only one suggestion and it's not input while typing
        headerCompletionInsert(); // if only have one suggestion, just use it
}

void Editor::initAutoBackup()
{
    cleanAutoBackup();
    if (mAutoBackupEnabled)
        return;
    if (!mEditorSettings->enableEditTempBackup())
        return;
    if (readOnly())
        return;
    QFileInfo fileInfo(mFilename);
    if (fileInfo.isAbsolute()) {
        mBackupFile=new QFile(getFilePath(
                                extractFileDir(mFilename),
                                extractFileName(mFilename)+QString(".%1.editbackup").arg(QDateTime::currentSecsSinceEpoch())));
        if (mBackupFile->open(QFile::Truncate|QFile::WriteOnly)) {
            saveAutoBackup();
        } else {
            cleanAutoBackup();
        }
    } else {
        mBackupFile=new QFile(
                    getFilePath(QDir::currentPath(),
                        mFilename+QString(".%1.editbackup").arg(QDateTime::currentSecsSinceEpoch())));
        if (!mBackupFile->open(QFile::Truncate|QFile::WriteOnly)) {
            mBackupFile->setParent(nullptr);
            delete mBackupFile;
            mBackupFile=nullptr;
        }
    }
    if (mBackupFile) {
        mAutoBackupTimer.start();
    }
}

void Editor::saveAutoBackup()
{
    if (mBackupFile) {
        mBackupFile->reset();
        mBackupTime=QDateTime::currentDateTime();
        mBackupFile->write(text().toUtf8());
        mBackupFile->flush();
        //qDebug()<<mBackupTime<<mBackupFile->size()<<mBackupFile->fileName();
    }
}

void Editor::cleanAutoBackup()
{
    mAutoBackupTimer.stop();
    if (mBackupFile) {
        mBackupFile->close();
        mBackupFile->remove();
        delete mBackupFile;
        mBackupFile=nullptr;
    }
}

bool Editor::testInFunc(const CharPos& pos)
{
    int y=pos.line;
    int x=pos.ch;
    if (syntaxer()->language()!=QSynedit::ProgrammingLanguage::CPP)
        return false;
    startParseLine(syntaxer().get(), y);
    QSynedit::PSyntaxState state = syntaxer()->getState();
    while(!syntaxer()->eol()) {
        int start = syntaxer()->getTokenPos();
        QString token = syntaxer()->getToken();
        int end = start + token.length();
        if (end>=x)
            break;
        state = syntaxer()->getState();
        syntaxer()->next();
    }
//    qDebug()<<state.parenthesisLevel;
    return state->parenthesisLevel>0;


}

void Editor::completionInsert(bool appendFunc)
{
    Q_ASSERT(mCompletionPopup!=nullptr);
    PStatement statement = mCompletionPopup->selectedStatement();
    if (!statement)
        return;

    if (mCodeCompletionSettings->recordUsage()
            && statement->kind != StatementKind::UserCodeSnippet) {
        statement->usageCount+=1;
        emit symbolChoosed(statement->fullName, statement->usageCount);
    }

    QString funcAddOn = "";

// delete the part of the word that's already been typed ...
    CharPos caretPos = caretXY();
    CharPos pEnd = getTokenEnd(caretXY());
    CharPos pStart = prevWordBegin(caretXY());
    if (caretPos == pStart && caretPos.ch>0)
        pStart = getTokenBegin(CharPos{caretPos.ch-1,caretPos.line});
    setCaretAndSelection(pStart,pStart,pEnd);

    // if we are inserting a function,
    if (appendFunc) {
        if (statement->kind == StatementKind::Alias) {
            PStatement newStatement = mParser->findAliasedStatement(statement);
            while (newStatement && newStatement->kind==StatementKind::Alias) {
                newStatement = mParser->findAliasedStatement(newStatement);
            }
            if (newStatement)
                statement = newStatement;
        }
        if ( (statement->kind == StatementKind::Function
               && !IOManipulators.contains(statement->fullName))
                || statement->kind == StatementKind::Constructor
                || statement->kind == StatementKind::Destructor
                ||
                (statement->kind == StatementKind::Preprocessor
                  && !statement->args.isEmpty())) {
            QChar nextCh = charAt(nextNonSpaceChar(pEnd));
            if (nextCh=='(') {
                funcAddOn = "";
            } else if (isIdentChar(nextCh) || nextCh == '"'
                       || nextCh == '\'') {
                funcAddOn = '(';
            } else {
                funcAddOn = "()";
            }
        }
    }

    // ... by replacing the selection
    if (statement->kind == StatementKind::UserCodeSnippet) { // it's a user code template
        // insertUserCodeIn(Statement->value);
        //first move caret to the begin of the word to be replaced
        insertCodeSnippet(statement->value);
    } else {
        if (statement->kind == StatementKind::Keyword
                && !isIdentStartChar(statement->command[0])
                && statement->command[0] != charAt(pStart)) {
            setSelText(statement->command.mid(1));
        } else
            setSelText(statement->command + funcAddOn);

//        if (!funcAddOn.isEmpty())
//            mLastIdCharPressed = 0;

        // Move caret inside the ()'s, only when the user has something to do there...
        if (!funcAddOn.isEmpty()
                && (statement->args != "()")
                && (statement->args != "(void)")) {
            setCaretX(caretX() - funcAddOn.length()+1);
        } else {
            setCaretX(caretX());
        }
    }
    mCompletionPopup->hide();
}

void Editor::headerCompletionInsert()
{
    Q_ASSERT(mHeaderCompletionPopup!=nullptr);
    QString headerName = mHeaderCompletionPopup->selectedFilename(true);
    if (headerName.isEmpty()) {
        mHeaderCompletionPopup->hide();
        return;
    }

    // delete the part of the word that's already been typed ...
    CharPos p = caretXY();
    int posBegin = p.ch;
    int posEnd = p.ch;
    QString sLine = lineText();
    while ((posBegin>0) &&
           (sLine[posBegin-1]!='\"'
            && sLine[posBegin-1]!='<'
            && sLine[posBegin-1]!='/'))
        posBegin--;

    while ((posEnd < sLine.length()) &&
           (sLine[posEnd]!='\"'
            && sLine[posEnd]!='>'
            && sLine[posEnd]!='/'))
        posEnd++;
    setSelBeginEnd(CharPos{posBegin, p.line}, CharPos{posEnd, p.line});
    setSelText(headerName);

    setCaretX(caretX());

    if (headerName.endsWith("/")) {
        showHeaderCompletion(false,true);
    } else {
        mHeaderCompletionPopup->hide();
    }
}

bool Editor::onCompletionKeyPressed(QKeyEvent *event)
{
    Q_ASSERT(mCompletionPopup!=nullptr);
    bool processed = false;
    if (!mCompletionPopup->isEnabled())
        return false;
    QString oldPhrase = mCompletionPopup->memberPhrase();
    WordPurpose purpose = WordPurpose::wpCompletion;
    if (QSynedit::isAssemblyLanguage(syntaxer()->language())) {
        purpose = WordPurpose::wpATTASMKeywords;
    } else if (oldPhrase.startsWith('#')) {
        purpose = WordPurpose::wpDirective;
    } else if (oldPhrase.startsWith('@')) {
        purpose = WordPurpose::wpJavadoc;
    }
    QString phrase;
    CharPos pBeginPos,pEndPos;
    switch (event->key()) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
        //ignore it
        return true;
    case Qt::Key_Backspace:
        processCommand(QSynedit::EditCommand::DeleteLastChar, QChar()); // Simulate backspace in editor
        if (purpose == WordPurpose::wpCompletion) {
            phrase = getWordForCompletionSearch(caretXY(), mCompletionPopup->memberOperator()=="::");
        } else
            phrase = getWordAtPosition(this,caretXY(),
                                            pBeginPos,pEndPos,
                                            purpose);
        if (phrase.isEmpty()) {
            mCompletionPopup->hide();
        } else {
            mCompletionPopup->search(phrase, false, mEditorSettings->colorScheme());
        }
        return true;
    case Qt::Key_Escape:
        mCompletionPopup->hide();
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Tab:
        completionInsert(mCodeCompletionSettings->appendFunc());
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
        processCommand(QSynedit::EditCommand::Input, ch);
        if (purpose == WordPurpose::wpCompletion) {
            phrase = getWordForCompletionSearch(caretXY(),mCompletionPopup->memberOperator()=="::");
        } else
            phrase = getWordAtPosition(this,caretXY(),
                                            pBeginPos,pEndPos,
                                            purpose);
        mCompletionPopup->search(phrase, false, mEditorSettings->colorScheme());
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
    Q_ASSERT(mHeaderCompletionPopup!=nullptr);
    bool processed = false;
    if (!mHeaderCompletionPopup->isEnabled())
        return false;
    QString phrase;
    CharPos pBeginPos,pEndPos;
    switch (event->key()) {
    case Qt::Key_Backspace:
        processCommand(
                    QSynedit::EditCommand::DeleteLastChar,
                    QChar()); // Simulate backspace in editor
        phrase = getWordAtPosition(this,caretXY(),
                                   pBeginPos,pEndPos,
                                   WordPurpose::wpHeaderCompletion);
        mHeaderCompletionPopup->search(phrase, false, mEditorSettings->colorScheme());
        return true;
    case Qt::Key_Escape:
        mHeaderCompletionPopup->hide();
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Tab:
        headerCompletionInsert();
        //mHeaderCompletionPopup->hide();
        return true;
    case Qt::Key_Shift:
        return false;
    default:
        if (event->text().isEmpty()) {
            //stop completion
            mHeaderCompletionPopup->hide();
            keyPressEvent(event);
            return true;
        }
    }
    QChar ch = event->text().front();

    if (isIdentChar(ch) || ch == '.'
            || ch =='_' || ch=='+') {
        processCommand(QSynedit::EditCommand::Input, ch);
        phrase = getWordAtPosition(this,caretXY(),
                                            pBeginPos,pEndPos,
                                            WordPurpose::wpHeaderCompletion);
        mHeaderCompletionPopup->search(phrase, false, mEditorSettings->colorScheme());
        return true;
    } else {
        //stop completion
        mHeaderCompletionPopup->hide();
        keyPressEvent(event);
        return true;
    }
    return processed;
}

bool Editor::onCompletionInputMethod(QInputMethodEvent *event)
{
    bool processed = false;
    if (!completionPopupVisible())
        return processed;
    QString s=event->commitString();
    if (mCompletionPopup && mParser && !s.isEmpty()) {
        QString phrase = getWordForCompletionSearch(caretXY(),mCompletionPopup->memberOperator()=="::");
        mCompletionPopup->search(phrase, false, mEditorSettings->colorScheme());
        return true;
    }
    return processed;
}

Editor::TipType Editor::getTipType(QPoint point, CharPos& pos)
{
    // Only allow in the text area...
    if (pointToCharLine(point, pos)) {
        //qDebug()<<gutterWidth()<<charWidth()<<point.y()<<point.x()<<pos.line<<pos.ch;
        if (getSyntaxIssueAtPosition(pos)) {
            return TipType::Error;
        }

        QSynedit::PTokenAttribute attr;
        QString s;

        // Only allow hand tips in highlighted areas
        if (getTokenAttriAtRowCol(pos,s,attr)) {
            // Only allow Identifiers, Preprocessor directives, and selection
            if (attr) {
                if (dragging()) {
                    // do not allow when dragging selection
                } else if (mParser && mParser->isIncludeLine(lineText(pos.line))) {
                    return TipType::Include;
                } else if (selAvail() && inSelection(pos)) {
                        return TipType::Selection;
                } else if (attr->tokenType() == QSynedit::TokenType::Identifier) {
                    return TipType::Identifier;
                } else if (attr->tokenType() == QSynedit::TokenType::Number) {
                    return TipType::Number;
                } else if (attr->tokenType() == QSynedit::TokenType::Keyword) {
                    return TipType::Keyword;
                }
            }
        }
    }
    return TipType::None;
}

void Editor::cancelHint()
{
    // disable editor hint
    QToolTip::hideText();
    mCurrentWord="";
    mCurrentTipType=TipType::None;
}

QString Editor::getHeaderFileHint(const QString &s, bool fromNext)
{
    QString fileName = mParser->getHeaderFileName(mFilename, s, fromNext);
    if (fileExists(fileName)) {
        return fileName + " - " + tr("Ctrl+click for more info");
    }
    return "";
}

QString Editor::getParserHint(const QStringList& expression, const CharPos& p)
{
    if (!mParser)
        return "";
    // This piece of code changes the parser database, possibly making hints and code completion invalid...
    QString result;
    // Exit early, don't bother creating a stream (which is slow)
    PStatement statement = mParser->findStatementOf(
                mFilename,expression,
                p.line);
    statement = constructorToClass(statement, p);
    if (!statement)
        return result;
    if (statement->kind == StatementKind::Function
            || statement->kind == StatementKind::Constructor
            || statement->kind == StatementKind::Destructor) {
          result = getHintForFunction(statement,mFilename,p.line);
    } else if (statement->line>0) {
        QFileInfo fileInfo(statement->fileName);
        result = mParser->prettyPrintStatement(statement,mFilename, p.line) + " - "
                + QString("%1(%2) ").arg(fileInfo.fileName()).arg(statement->line+1)
                + tr("Ctrl+click for more info");
    } else {  // hard defines
        result = mParser->prettyPrintStatement(statement, mFilename);
    }
    return result;
}

void Editor::showDebugHint(const QString &s, int line)
{
    if (!mParser || !QFileInfo::exists(mFilename))
        return;
    PStatement statement = mParser->findStatementOf(mFilename,s,line);
    if (statement) {
        if (statement->kind != StatementKind::Variable
                && statement->kind != StatementKind::GlobalVariable
                && statement->kind != StatementKind::LocalVariable
                && statement->kind != StatementKind::Parameter) {
            return;
        }
    }
    if (mRequestEvalTipFunc && mRequestEvalTipFunc(this,s)) {
        if (mFunctionTooltip) {
            mFunctionTooltip->hide();
        }
        mCurrentDebugTipWord = s;
    }
}

QString Editor::getErrorHint(const PSyntaxIssue& issue)
{
    if (issue) {
        return issue->hint;
    } else {
        return "";
    }
}

QString Editor::getHintForFunction(const PStatement &statement, const QString& filename, int line)
{
    QFileInfo fileInfo(statement->fileName);
    QString result;
    result = mParser->prettyPrintStatement(statement,filename,line) + " - "
            + QString("%1(%2) ").arg(fileInfo.fileName()).arg(statement->line+1)
            + tr("Ctrl+click for more info");
    return result;
}

void Editor::updateFunctionTip(bool showTip)
{
    if (!mFunctionTooltip)
        return;
    if (completionPopupVisible()) {
        mFunctionTooltip->hide();
        return;
    }
    if (inputMethodOn()) {
        mFunctionTooltip->hide();
        return;
    }

    if (!mParser || !mParser->enabled())
        return;

    bool isFunction = false;
    auto action = finally([&isFunction,this]{
        if (!isFunction)
            mFunctionTooltip->hide();
    });
    const int maxLines=10;
    CharPos caretPos = caretXY();
    int currentLine = caretPos.line;
    int currentChar = caretPos.ch;
    CharPos functionNamePos{-1,-1};
    bool foundFunctionStart = false;
    int parenthesisLevel = 0;
    int braceLevel = 0;
    int bracketLevel = 0;
    int paramsCount = 1;
    int currentParamPos = 1;
    if (currentLine>=lineCount())
        return;

    QChar ch= charAt(prevNonSpaceChar(caretPos));
    if (ch!='(' && ch!=',')
        return;

    QSynedit::PTokenAttribute attr;
    QString token;
    QSynedit::PSyntaxState syntaxState;
    CharPos pos{caretX()-1, caretY()};
    if (getTokenAttriAtRowCol(pos, token, attr, syntaxState)) {
        if (syntaxer()->isStringNotFinished(syntaxState))
            return;
        if (syntaxer()->isCommentNotFinished(syntaxState))
            return;
        if (attr->tokenType() == QSynedit::TokenType::Character)
            return;
    }

    while (currentLine>=0) {
        QString line = document()->getLine(currentLine);
        if (currentLine!=caretPos.line)
            currentChar = line.length();
        QStringList tokens;
        QList<int> positions;
        startParseLine(syntaxer().get(), currentLine);
        while(!syntaxer()->eol()) {
            int start = syntaxer()->getTokenPos();
            QString token = syntaxer()->getToken();
            QSynedit::PTokenAttribute attr = syntaxer()->getTokenAttribute();
            if (start>=currentChar)
                break;

            if (attr->tokenType() != QSynedit::TokenType::Comment
                    && attr->tokenType() != QSynedit::TokenType::Space) {
                if (attr->tokenType() == QSynedit::TokenType::String)
                    token="\"\"";
                tokens.append(token);
                positions.append(start);
            }
            syntaxer()->next();
        }
        if (!foundFunctionStart) {
            for (int i=tokens.length()-1;i>=0;i--) {
                if (braceLevel>0) {
                    if (tokens[i]=="{") {
                        braceLevel--;
                    } else if (tokens[i]=="}") {
                        braceLevel++;
                    }
                } else if (bracketLevel>0) {
                    if (tokens[i]=="[") {
                        bracketLevel--;
                    } else if (tokens[i]=="]") {
                        bracketLevel++;
                    }
                }else if (parenthesisLevel>0){
                    if (tokens[i]==")") {
                        parenthesisLevel++;
                    } else if (tokens[i]=="(") {
                        parenthesisLevel--;
                    }
                } else {
                    if (tokens[i]=="(") {
                        // found start of function
                        foundFunctionStart = true;
                        if (i>0) {
                            functionNamePos.line = currentLine;
                            functionNamePos.ch = positions[i-1];
                        }
                        break;
                    } else if (tokens[i]=="[") {
                        //we are not in a function call
                        return;
                    } else if (tokens[i]=="{") {
                        //we are not in a function call
                        return;
                    } else if (tokens[i]==";") {
                        //we are not in a function call
                        return;
                    } else if (tokens[i]==")") {
                        parenthesisLevel++;
                    } else if (tokens[i]=="}") {
                        braceLevel++;
                    } else if (tokens[i]=="]") {
                        bracketLevel++;
                    } else if (tokens[i]==",") {
                        paramsCount++;
                    }
                }
            }
        } else {
            int i = tokens.length()-1;
            if (i>=0){
                if (!tokens[i].isEmpty() &&
                    isIdentStartChar(tokens[i].front())) {
                    functionNamePos.line = currentLine;
                    functionNamePos.ch = positions[i];
                    break;
                }
                // not a valid function
                return;
            }
        }
        if (functionNamePos.ch>=0)
            break;
        currentLine--;
        if (caretPos.line-currentLine>maxLines)
            break;
    }
    isFunction = functionNamePos.ch>=0;
    currentParamPos = paramsCount-1;
    if (!isFunction)
        return;
    CharPos pWordBegin, pWordEnd;

    QString s = getWordAtPosition(this, functionNamePos, pWordBegin,pWordEnd, WordPurpose::wpInformation);
    int x = pWordBegin.ch-1;
    QString line = lineText(pWordBegin.line);
    bool hasPreviousWord=false;
    while (x>=0) {
        QChar ch=line[x];
        if (ch == ' ' || ch == '\t') {
            x--;
            continue;
        }
        if (isIdentChar(ch)) {
            hasPreviousWord = true;
            break;
        }
        hasPreviousWord = false;
        break;
    }

    //handle class initializer
    if (x >= 0 && hasPreviousWord) {
        CharPos pos = pWordBegin;
        QSynedit::TokenType wordType;
        pos.ch = pWordBegin.ch;
        QString previousWord = getPreviousWordAtPositionForSuggestion(pos, wordType);
        if (wordType != QSynedit::TokenType::Identifier)
            return;
        PStatement statement = mParser->findStatementOf(
                    mFilename,
                    previousWord,
                    pos.line);
        if (statement) {
            PStatement typeStatement = mParser->findTypeDef(statement,mFilename);
            if (typeStatement && typeStatement->kind == StatementKind::Class) {
                s = previousWord;
                functionNamePos = pos;
            }
        }
    }

//    qDebug()<<QString("find word at %1:%2 - '%3'")
//              .arg(FuncStartXY.Line)
//              .arg(FuncStartXY.Char)
//              .arg(s);
    // Don't bother scanning the database when there's no identifier to scan for

    // Only do the cumbersome list filling when showing a new tooltip...
    if (s != mFunctionTooltip->functionFullName()
            && !mParser->parsing()) {
        mFunctionTooltip->clearTips();
        QList<PStatement> statements=mParser->getListOfFunctions(mFilename,
                                                                  s,
                                                                  functionNamePos.line);
//      qDebug()<<"finding function list:"<<s<<" - "<<statements.length();
        foreach (const PStatement statement, statements) {
            mFunctionTooltip->addTip(
                        statement->command,
                        statement->fullName,
                        statement->type,
                        statement->args,
                        statement->noNameArgs);
        }
    }

    // If we can't find it in our database, hide
    if (mFunctionTooltip->tipCount()<=0) {
        mFunctionTooltip->hide();
        return;
    }
    // Position it at the top of the next line
    QPoint p = displayCoordToPixels(displayXY());
    p+=QPoint(0,textHeight()+2);

    mFunctionTooltip->setFunctioFullName(s);
    mFunctionTooltip->guessFunction(paramsCount-1);
    mFunctionTooltip->setParamIndex(
                currentParamPos
                );
    int w = mFunctionTooltip->width();
    if (w+p.x() > clientWidth())
        p.setX(clientWidth()-w-2);
    mFunctionTooltip->move(mapToGlobal(p));
    cancelHint();
    if (showTip && hasFocus() && !completionPopupVisible() && !headerCompletionPopupVisible())
        mFunctionTooltip->show();
}

void Editor::clearUserCodeInTabStops()
{
    mUserCodeInTabStops.clear();
}

void Editor::popUserCodeInTabStops()
{
    if (mTabStopBegin < 0) {
      clearUserCodeInTabStops();
      return;
    }
    CharPos newCursorPos;
    int tabStopEnd;
    int tabStopBegin;
    if (mUserCodeInTabStops.count() > 0) {
        PTabStop p = mUserCodeInTabStops.front();
        // Update the cursor
        if (p->y ==0) {
            tabStopBegin = mTabStopEnd + p->x;
            tabStopEnd = mTabStopEnd + p->endX;
        } else {
            int n=countLeadingWhitespaceChars(lineText(caretY()+p->y));
//            qDebug()<<line<<n<<p->x;
            tabStopBegin = n+p->x;
            tabStopEnd = n+p->endX;
        }
        mTabStopY = caretY() + p->y;
        newCursorPos.line = mTabStopY;
        newCursorPos.ch = tabStopBegin;
        setCaretAndSelection(newCursorPos,newCursorPos,CharPos{tabStopEnd, mTabStopY});

        mTabStopBegin = tabStopBegin;
        mTabStopEnd = tabStopEnd;
        mLineBeforeTabStop = lineText().mid(0, mTabStopBegin) ;
        mLineAfterTabStop = lineText().mid(mTabStopEnd) ;
        mXOffsetSince=0;
        mUserCodeInTabStops.pop_front();
    }
}

void Editor::onExportedFormatToken(QSynedit::PSyntaxer syntaxer, int Line, int column, const QString &token, QSynedit::PTokenAttribute& attr)
{
    if (!syntaxer)
        return;
    if (token.isEmpty())
        return;
    //don't do this
    if (completionPopupVisible() || headerCompletionPopupVisible())
        return;

    if (mParser && (attr == syntaxer->identifierAttribute())) {
        CharPos p{column,Line};
        CharPos pBeginPos,pEndPos;
        QString s= getWordAtPosition(this,p, pBeginPos,pEndPos, WordPurpose::wpInformation);
//        qDebug()<<s;
        PStatement statement = mParser->findStatementOf(mFilename,
          s , p.line);
        while (statement && statement->kind == StatementKind::Alias)
            statement = mParser->findAliasedStatement(statement);
        StatementKind kind = getKindOfStatement(statement);
        if (kind == StatementKind::Unknown) {
            if ((pEndPos.line>=1)
              && (pEndPos.ch>=0)
              && (pEndPos.ch < lineText(pEndPos.line).length())
              && (lineText(pEndPos.line)[pEndPos.ch] == '(')) {
                kind = StatementKind::Function;
            } else {
                kind = StatementKind::Variable;
            }
        }
        QSynedit::CppSyntaxer* cppSyntaxer = dynamic_cast<QSynedit::CppSyntaxer*>(syntaxer.get());
        switch(kind) {
        case StatementKind::Function:
        case StatementKind::Constructor:
        case StatementKind::Destructor:
        case StatementKind::OverloadedOperator:
        case StatementKind::LiteralOperator:
            attr = cppSyntaxer->functionAttribute();
            break;
        case StatementKind::Class:
        case StatementKind::Typedef:
            attr = cppSyntaxer->classAttribute();
            break;
        case StatementKind::EnumClassType:
        case StatementKind::EnumType:
            break;
        case StatementKind::LocalVariable:
        case StatementKind::Parameter:
            attr = cppSyntaxer->localVarAttribute();
            break;
        case StatementKind::Variable:
            attr = cppSyntaxer->variableAttribute();
            break;
        case StatementKind::GlobalVariable:
            attr = cppSyntaxer->globalVarAttribute();
            break;
        case StatementKind::Enum:
        case StatementKind::Preprocessor:
            attr = cppSyntaxer->preprocessorAttribute();
            break;
        case StatementKind::Keyword:
            attr = cppSyntaxer->keywordAttribute();
            break;
        case StatementKind::Namespace:
        case StatementKind::NamespaceAlias:
            attr = cppSyntaxer->stringAttribute();
            break;
        default:
            break;
        }
    }
}

void Editor::onScrollBarValueChanged()
{
    if(mFunctionTooltip) mFunctionTooltip->hide();
}

void Editor::updateHoverLink(int line)
{
    setCursor(Qt::PointingHandCursor);
    if (mHoverModifiedLine!=line) invalidateLine(mHoverModifiedLine);
    mHoverModifiedLine=line;
    invalidateLine(mHoverModifiedLine);
}

void Editor::cancelHoverLink()
{
    if (mHoverModifiedLine != -1) {
        invalidateLine(mHoverModifiedLine);
        mHoverModifiedLine = -1;
    }
}

QSize Editor::calcCompletionPopupSize()
{
    int screenHeight = screen()->size().height();
    int screenWidth = screen()->size().width();
    int popWidth = std::min(mCodeCompletionSettings->widthInColumns() * charWidth(),
                            screenWidth / 2) + 4;
    int popHeight = std::min(mCodeCompletionSettings->heightInLines() * textHeight(),
                             (screenHeight / 2 - textHeight() * 2)) + 4;
    return QSize{popWidth, popHeight};
}

FileType Editor::fileType() const
{
    return mFileType;
}

void Editor::setFileType(FileType newFileType, bool parse)
{
    if (newFileType == FileType::None)
        newFileType = getFileType(mFilename);
    if (mFileType==newFileType)
        return;
    ParserLanguage oldLanguage = calcParserLanguage();
    QSynedit::PSyntaxer oldSyntaxer = syntaxer();
    mFileType = newFileType;
    setSyntaxer(SyntaxerManager::getSyntaxer(mFileType));
    setFormatter(SyntaxerManager::getFormatter(syntaxer()->language()));
    setUseCodeFolding(syntaxer()->supportFolding());
    applyColorScheme(mEditorSettings->colorScheme());
    if (parse && oldLanguage!=calcParserLanguage()) {
        setCppParser();
        reparse();
    }
    if (parse && syntaxer()->language() != oldSyntaxer->language()) {
        reparseTodo();
        checkSyntaxInBack();
    }
    emit statusChanged(QSynedit::StatusChange::Custom0);
}

void Editor::openFileInContext(const QString &filename, const CharPos& caretPos)
{
    FileType fileType = getFileType(filename);
    QString contextFile;
    if (!isC_CPPHeaderFile(fileType)
            && !isC_CPPSourceFile(fileType))
        fileType = FileType::CCppHeader;
    if (isC_CPPHeaderFile(fileType)) {
        if (isC_CPPSourceFile(mFileType)) {
            contextFile = mFilename;
        } else if (isC_CPPHeaderFile(mFileType) && !mContextFile.isEmpty()) {
            contextFile = mContextFile;
        }
    }

    emit openFileRequested(filename, fileType,contextFile, caretPos);
}

bool Editor::needReparse() const
{
    return mParser && ((isC_CPPHeaderFile(mFileType)
                        && !mContextFile.isEmpty()
                        && !mParser->isFileParsed(mContextFile))
                       || (!mParser->isFileParsed(mFilename)));
}

PStatement Editor::constructorToClass(PStatement statement, const CharPos& p)
{
    if (statement && statement->kind == StatementKind::Constructor) {
        QString token;
        int start;
        QSynedit::PTokenAttribute attri;
        if (getTokenAttriAtRowCol(p,token,start,attri)) {
            QString s = lineText(p.line);
            int pos = start+token.length()-1;
            while (pos<s.length() && CppParser::isSpaceChar(s[pos]))
                pos++;
            if (pos >= s.length() || (s[pos]!='(' && s[pos]!='{')) {
                return statement->parentScope.lock();
            }
        }
    }
    return statement;
}

int Editor::previousIdChars(const CharPos &pos)
{
    if (pos.ch == 0)
        return 0;
    QString token;
    QSynedit::PTokenAttribute attr;
    int start;
    if (getTokenAttriAtRowCol(CharPos{pos.ch-1, pos.line}, token, start, attr)) {
        if (attr->tokenType() == QSynedit::TokenType::Identifier
                || attr->tokenType() == QSynedit::TokenType::Keyword)
            return pos.ch - start;
    }
    return 0;
}

IconsManager *Editor::iconsManager() const
{
    return mIconsManager;
}

void Editor::setIconsManager(IconsManager *newIconsManager)
{
    if (mIconsManager!=newIconsManager) {
        mIconsManager = newIconsManager;
        invalidateGutter();
    }
}

ColorManager *Editor::colorManager() const
{
    return mColorManager;
}

void Editor::setColorManager(ColorManager *newColorManager)
{
    mColorManager = newColorManager;
}

const GetCppParserFunc &Editor::getCppParserFunc() const
{
    return mGetCppParserFunc;
}

void Editor::setGetCppParserFunc(const GetCppParserFunc &newGetCppParserFunc)
{
    mGetCppParserFunc = newGetCppParserFunc;
}

bool Editor::codeCompletionEnabled() const
{
    return mCodeCompletionEnabled;
}

const GetMacroVarsFunc &Editor::getMacroVarsFunc() const
{
    return mGetMacroVarsFunc;
}

void Editor::setGetMacroVarsFunc(const GetMacroVarsFunc &newGetMacroVarsFunc)
{
    mGetMacroVarsFunc = newGetMacroVarsFunc;
}

const GetReformatterFunc &Editor::getReformatterFunc() const
{
    return mGetReformatterFunc;
}

void Editor::setGetReformatterFunc(const GetReformatterFunc &newGetReformatterFunc)
{
    mGetReformatterFunc = newGetReformatterFunc;
}

#ifdef ENABLE_SDCC
const GetCompilerTypeForEditorFunc &Editor::getCompilerTypeForEditorFunc() const
{
    return mGetCompilerTypeForEditorFunc;
}

void Editor::setGetCompilerTypeForEditorFunc(const GetCompilerTypeForEditorFunc &newGetCompilerTypeForEditorFunc)
{
    mGetCompilerTypeForEditorFunc = newGetCompilerTypeForEditorFunc;
}
#endif

void Editor::setCodeCompletionSettings(const CodeCompletionSettings *newCodeCompletionSettings)
{
    mCodeCompletionSettings = newCodeCompletionSettings;
}

void Editor::setEditorSettings(const EditorSettings *newEditorSettings)
{
    mEditorSettings = newEditorSettings;
}

const CanShowEvalTipFunc &Editor::canShowEvalTipFunc() const
{
    return mCanShowEvalTipFunc;
}

void Editor::setCanShowEvalTipFunc(const CanShowEvalTipFunc &newCanShowEvalTipFunc)
{
    mCanShowEvalTipFunc = newCanShowEvalTipFunc;
}

QFileSystemWatcher *Editor::fileSystemWatcher() const
{
    return mFileSystemWatcher;
}

void Editor::setFileSystemWatcher(QFileSystemWatcher *newFileSystemWatcher)
{
    mFileSystemWatcher = newFileSystemWatcher;
}

CodeSnippetsManager *Editor::codeSnippetsManager() const
{
    return mCodeSnippetsManager;
}

void Editor::setCodeSnippetsManager(CodeSnippetsManager *newCodeSnippetsManager)
{
    mCodeSnippetsManager = newCodeSnippetsManager;
}

const EvalTipReadyCallback &Editor::evalTipReadyCallback() const
{
    return mEvalTipReadyCallback;
}

void Editor::setEvalTipReadyCallback(const EvalTipReadyCallback &newEvalTipReadyCallback)
{
    mEvalTipReadyCallback = newEvalTipReadyCallback;
}

const RequestEvalTipFunc &Editor::requestEvalTipFunc() const
{
    return mRequestEvalTipFunc;
}

void Editor::setRequestEvalTipFunc(const RequestEvalTipFunc &newRequestEvalTipFunc)
{
    mRequestEvalTipFunc = newRequestEvalTipFunc;
}

const GetFileStreamFunc &Editor::getFileStreamCallBack() const
{
    return mGetFileStreamFunc;
}

void Editor::setGetFileStreamCallBack(const GetFileStreamFunc &newGetFileStreamCallBack)
{
    mGetFileStreamFunc = newGetFileStreamCallBack;
}

const GetOpennedEditorFunc &Editor::getOpennedEditorFunc() const
{
    return mGetOpennedEditorFunc;
}

void Editor::setGetOpennedFunc(const GetOpennedEditorFunc &newOpennedEditorProviderCallBack)
{
    mGetOpennedEditorFunc = newOpennedEditorProviderCallBack;
}

const GetSharedParserrFunc &Editor::getSharedParserFunc() const
{
    return mGetSharedParserFunc;
}

void Editor::setGetSharedParserFunc(const GetSharedParserrFunc &newSharedParserProviderCallBack)
{
    mGetSharedParserFunc = newSharedParserProviderCallBack;
}

CodeCompletionPopup *Editor::completionPopup() const
{
    return mCompletionPopup;
}

void Editor::setCompletionPopup(CodeCompletionPopup *newCompletionPopup)
{
    mCompletionPopup = newCompletionPopup;
}

HeaderCompletionPopup *Editor::headerCompletionPopup() const
{
    return mHeaderCompletionPopup;
}

void Editor::setHeaderCompletionPopup(HeaderCompletionPopup *newHeaderCompletionPopup)
{
    mHeaderCompletionPopup = newHeaderCompletionPopup;
}

FunctionTooltipWidget *Editor::functionTooltip() const
{
    return mFunctionTooltip;
}

void Editor::setFunctionTooltip(FunctionTooltipWidget *newFunctionTooltip)
{
    mFunctionTooltip = newFunctionTooltip;
}

bool Editor::autoBackupEnabled() const
{
    return mAutoBackupEnabled;
}

void Editor::setAutoBackupEnabled(bool newEnableAutoBackup)
{
    mAutoBackupEnabled = newEnableAutoBackup;
    if (mAutoBackupEnabled) {
        initAutoBackup();
    } else {
        cleanAutoBackup();
    }
}

const QString &Editor::contextFile() const
{
    return mContextFile;
}

void Editor::setContextFile(const QString &newContextFile, bool parse)
{
    QString s = newContextFile.trimmed();
    if (mContextFile == s)
        return;
    ParserLanguage oldLanguage = calcParserLanguage();
    mContextFile = s;
    if (parse && oldLanguage!=calcParserLanguage()) {
        setCppParser();
        reparse();
    }
}

quint64 Editor::lastFocusOutTime() const
{
    return mLastFocusOutTime;
}

bool Editor::canAutoSave() const
{
    return mCanAutoSave;
}

void Editor::setCanAutoSave(bool newCanAutoSave)
{
    mCanAutoSave = newCanAutoSave;
}

const QDateTime &Editor::hideTime() const
{
    return mHideTime;
}

void Editor::setHideTime(const QDateTime &newHideTime)
{
    mHideTime = newHideTime;
}

const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &Editor::statementColors() const
{
    return mStatementColors;
}

void Editor::setStatementColors(const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &newStatementColors)
{
    mStatementColors = newStatementColors;
}

void Editor::setInProject(bool newInProject, bool parse)
{
    if (mInProject == newInProject)
        return;
    mInProject = newInProject;
    if (parse) {
        setCppParser();
        reparse();
    }
}

void Editor::gotoDeclaration(const CharPos &pos)
{
    if (!parser())
        return;
    // Exit early, don't bother creating a stream (which is slow)
    QStringList expression = getExpressionAtPosition(pos);

    // Find it's definition
    PStatement statement = parser()->findStatementOf(
                filename(),
                expression,
                pos.line);
    statement = constructorToClass(statement, pos);

    if (!statement) {
        return;
    }
    QString filename;
    int line;
    if (statement->fileName == mFilename && statement->line == pos.line) {
            filename = statement->definitionFileName;
            line = statement->definitionLine;
    } else {
        filename = statement->fileName;
        line = statement->line;
    }
    openFileInContext(filename, CharPos{0,line});
}

void Editor::gotoDefinition(const CharPos &pos)
{
    if (!parser())
        return;
    QStringList expression = getExpressionAtPosition(pos);

    // Find it's definition
    PStatement statement = parser()->findStatementOf(
                filename(),
                expression,
                pos.line);
    statement = constructorToClass(statement, pos);
    if (!statement) {
        return;
    }
    QString filename;
    int line;
    if (statement->definitionFileName == mFilename && statement->definitionLine == pos.line) {
            filename = statement->fileName;
            line = statement->line;
    } else {
        filename = statement->definitionFileName;
        line = statement->definitionLine;
    }
    openFileInContext(filename,CharPos{0,line});
}

QString getWordAtPosition(QSynedit::QSynEdit *editor, const CharPos &p, CharPos &pWordBegin, CharPos &pWordEnd, Editor::WordPurpose purpose)
{
    QString result = "";
    QString s;
    if (!editor->validInDoc(p)) {
        pWordBegin = p;
        pWordEnd = p;
        return "";
    }

    s = editor->lineText(p.line);
    int len = s.length();

    int wordBegin = p.ch - 1;
    int wordEnd = p.ch;

    // Copy forward until end of word
    if (purpose == Editor::WordPurpose::wpEvaluation
            || purpose == Editor::WordPurpose::wpInformation) {
        while (wordEnd < len) {
            if ((purpose == Editor::WordPurpose::wpEvaluation)
                    && (s[wordEnd ] == '[')) {
                if (!findComplement(s, '[', ']', wordEnd, 1))
                    break;
            } else if (editor->isIdentChar(s[wordEnd])) {
                wordEnd++;
            } else
                break;
        }
    }


    // Copy backward until % .
    if (purpose == Editor::WordPurpose::wpATTASMKeywords) {
        while ((wordBegin >= 0) && (wordBegin < len)) {
           if (editor->isIdentChar(s[wordBegin]))
               wordBegin--;
           else if (s[wordBegin] == '%') {
               wordBegin--;
               break;
           } else if (s[wordBegin] == '.') {
                   wordBegin--;
                   break;
           } else
               break;
        }
    }

    if (purpose == Editor::WordPurpose::wpKeywords) {
        while ((wordBegin >= 0) && (wordBegin < len)) {
           if (editor->isIdentChar(s[wordBegin])) {
               wordBegin--;
           } else if (s[wordBegin] == '.') {
               wordBegin--;
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
            if (editor->isIdentChar(s[wordBegin])) {
                wordBegin--;
            } else if (s[wordBegin] == '.'
                    || s[wordBegin] == '+') {
                wordBegin--;
            } else if (s[wordBegin] == '/'
                     || s[wordBegin] == '\\') {
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
                    wordBegin--; // step over mathing [
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
    wordBegin++;
    // Get end result
    result = s.mid(wordBegin, wordEnd - wordBegin);
    pWordBegin.line = p.line;
    pWordBegin.ch = wordBegin;
    pWordEnd.line = p.line;
    pWordEnd.ch = wordEnd;

    // last line still have part of word
    if (!result.isEmpty()
            && (
                result[0] == '.'
                || result[0] == '-')
            && (purpose == Editor::WordPurpose::wpCompletion
                || purpose == Editor::WordPurpose::wpEvaluation
                || purpose == Editor::WordPurpose::wpInformation)) {
        int i = wordBegin-1;
        int line=p.line;
        while (line>=0) {
            while (i>=0) {
                if (s[i] == ' '
                        || s[i] == '\t')
                    i--;
                else
                    break;
            }
            if (i<0) {
                line--;
                if (line>=0) {
                    s=editor->lineText(line);
                    i=s.length();
                    continue;
                } else
                    break;
            } else {
                CharPos highlightPos;
                CharPos pDummy;
                highlightPos.line = line;
                highlightPos.ch = i;
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
                    && (paramEnd == result.length()-1) ) {
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

QString Editor::getPreviousWordAtPositionForSuggestion(const CharPos &p, QSynedit::TokenType &wordType)
{
    wordType = QSynedit::TokenType::Default;
    QString result;
    if (!validInDoc(p)) {
        return "";
    }
    bool inFunc = testInFunc(p);

    int line = p.line;
    int ch = p.ch;
    QSynedit::CppSyntaxer syntaxer;
    if (line>=lineCount() || line<0)
        return "";
    QStringList tokenList;
    QList<QSynedit::TokenType> tokenTypeList;
    QString sLine = lineText(line);
    startParseLine(&syntaxer,line);
    while (!syntaxer.eol()) {
        QSynedit::PTokenAttribute attr = syntaxer.getTokenAttribute();
        QSynedit::TokenType tokenType = attr->tokenType();
        int start = syntaxer.getTokenPos();
        int end = start + syntaxer.getToken().length();
        QString token = syntaxer.getToken();
        if (start>=ch ) {
            break;
        }
        if (tokenType != QSynedit::TokenType::Comment
                && tokenType != QSynedit::TokenType::Space) {
            tokenList.append(token);
            tokenTypeList.append(tokenType);
        }
        if (end+1>=ch) {
            if (tokenType == QSynedit::TokenType::Identifier
                    || tokenType == QSynedit::TokenType::Keyword) {
                tokenList.pop_back();
                tokenTypeList.pop_back();
            }
        }
        syntaxer.next();
    }
    if (tokenList.isEmpty())
        return "";

    int bracketLevel=0;
    bool skipNextWord=false;
    int i;
    for (i=tokenList.size()-1;i>=0;i--) {
        if (tokenTypeList[i] == QSynedit::TokenType::Symbol
                && (tokenList[i] == ">"
                    || tokenList[i] == "]")) {
            bracketLevel++;
        } else if (tokenTypeList[i] == QSynedit::TokenType::Symbol
                && (tokenList[i] == "<"
                    || tokenList[i] == "[")) {
                       bracketLevel--;
            if (bracketLevel>0)
               bracketLevel--;
            else
               return "";
        } else if (bracketLevel==0) {
            //Differentiate multiple definition and function parameter define here
            if (tokenTypeList[i] == QSynedit::TokenType::Symbol
                            && (tokenList[i] == ",")) {
                if (inFunc) // in func, dont skip ','
                    break;
                else
                    skipNextWord=true;
            } else if (skipNextWord) {
                skipNextWord = false;
            } else {
                wordType = tokenTypeList[i];
                // if (tokenListType[i] == QSynedit::TokenType::Keyword)
                //     &&(CppTypeQualifiers.contains(tokenList[i])) {
                //     hasTypeQualifier = true;
                //     return "";
                // } else
                return tokenList[i];
            }
        }
    }
    return "";
}

QString Editor::getPreviousWordAtPositionForCompleteFunctionDefinition(const CharPos &p) const
{
    QString result;
    if (!validInDoc(p)) {
        return "";
    }

    QString s = lineText(p.line);
    int wordBegin;
    int wordEnd = p.ch-1;
    while (wordEnd > 0 &&  (isIdentChar(s[wordEnd]) || s[wordEnd] == ':')) {
        wordEnd--;
    }
    int bracketLevel=0;
    while (wordEnd > 0) {
        if (s[wordEnd] == '>'
               || s[wordEnd] == ']') {
            bracketLevel++;
        } else if (s[wordEnd] == '<'
                   || s[wordEnd] == '[') {
            if (bracketLevel>0)
                bracketLevel--;
            else
                break;
        } else if (bracketLevel==0) {
            if (s[wordEnd]=='*' || s[wordEnd]=='&' || s[wordEnd]==' ' || s[wordEnd]=='\t') {
                //do nothing
            } else if (isIdentChar(s[wordEnd]))
                break;
            else
                return ""; //error happened
        }
        wordEnd--;
    }
    if (wordEnd<=0)
        return "";

//        if (!isIdentChar(s[wordEnd]))
//            return "";

    wordBegin = wordEnd;
    while ((wordBegin >= 0) && isIdentChar(s[wordBegin]) ) {
        wordBegin--;
    }
    wordBegin++;

    if (wordBegin<s.length() && s[wordBegin]>='0' && s[wordBegin]<='9') // not valid word
        return "";

    result = s.mid(wordBegin, wordEnd - wordBegin+1);

    return result;
}

void Editor::reformat(bool doReparse)
{
    if (readOnly())
        return;
    if (!mGetReformatterFunc)
        return;
    std::unique_ptr<BaseReformatter> formatter = mGetReformatterFunc(this);
    if (!formatter)
        return;
    QString errorMessage;
    bool isOk;
    QString newContent = formatter->refomat(text(),errorMessage,isOk);
    if (!isOk) {
        if (!errorMessage.isEmpty()) {
            QMessageBox::critical(this,
                                  tr("Reformat Error"),
                                  errorMessage);
        }
        return;
    }
    if (newContent.isEmpty())
        return;
    replaceContent(newContent, doReparse);
}

void Editor::replaceContent(const QString &newContent, bool doReparse)
{
    int oldTopPos = topPos();
    CharPos mOldCaret = caretXY();

    beginEditing();
    QSynedit::EditorOptions oldOptions = getOptions();
    QSynedit::EditorOptions newOptions = oldOptions;
    newOptions.setFlag(QSynedit::EditorOption::AutoIndent,false);
    setOptions(newOptions);
    replaceAll(newContent);
    setCaretXY(ensureCharPosValid(mOldCaret));
    setTopPos(oldTopPos);
    setOptions(oldOptions);
    endEditing();

    if (doReparse) {
        reparse();
        checkSyntaxInBack();
        reparseTodo();
    }
}

void Editor::checkSyntaxInBack()
{
    if (!mInited)
        return;
    if (readOnly())
        return;
    emit syntaxCheckRequested(this);
}


const PCppParser &Editor::parser() const
{
    return mParser;
}

void Editor::tab()
{
    if (mUserCodeInTabStops.count()>0) {
        int oldLine = caretY();
        popUserCodeInTabStops();
        if (oldLine!=caretY()) {
            invalidateLine(oldLine);
        }
        invalidateLine(caretY());
        return;
    } else {
        if (mTabStopBegin >= 0) {
            mTabStopBegin = -1;
            setCaretXY(selEnd());
            invalidateLine(caretY());
            return;
        }
    }
    QSynEdit::tab();
}

void Editor::toggleBreakpoint(int line)
{
    if (hasBreakpoint(line)) {
        mBreakpointLines.remove(line);
        emit breakpointRemoved(this, line);
    } else {
        mBreakpointLines.insert(line);
        emit breakpointAdded(this, line);
    }

    invalidateGutterLine(line);
    invalidateLine(line);
}

void Editor::clearBreakpoints()
{
    emit breakpointsCleared(this);
    mBreakpointLines.clear();
    invalidate();
}

bool Editor::hasBreakpoint(int line)
{
    return mBreakpointLines.contains(line);
}

void Editor::toggleBookmark(int line)
{
    if (hasBookmark(line)) {
        removeBookmark(line);
    } else {
        addBookmark(line);
    }
}

void Editor::addBookmark(int line)
{
    mBookmarkLines.insert(line);
    invalidateGutterLine(line);
}

void Editor::removeBookmark(int line)
{
    mBookmarkLines.remove(line);
    invalidateGutterLine(line);
}

bool Editor::hasBookmark(int line) const
{
    return mBookmarkLines.contains(line);
}

void Editor::clearBookmarks()
{
    mBookmarkLines.clear();
    invalidateGutter();
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

void Editor::setActiveBreakpointFocus(int line, bool setFocus)
{
    if (line != mActiveBreakpointLine) {
        removeBreakpointFocus();

        // Put the caret at the active breakpoint
        mActiveBreakpointLine = line;

        // Invalidate new active line
        invalidateGutterLine(line);
        invalidateLine(line);
    }
}

void Editor::applySettings()
{
    Q_ASSERT(mEditorSettings!=nullptr);
    beginInternalChanges();
    QSynedit::EditorOptions options = QSynedit::EditorOption::AltSetsColumnMode
            | QSynedit::EditorOption::DragDropEditing | QSynedit::EditorOption::DropFiles
            | QSynedit::EditorOption::RightMouseMovesCursor
            | QSynedit::EditorOption::TabIndent
            | QSynedit::EditorOption::GroupUndo
            | QSynedit::EditorOption::SelectWordByDblClick;

    //options
    options.setFlag(QSynedit::EditorOption::ShowLeadingSpaces, mEditorSettings->showLeadingSpaces());
    options.setFlag(QSynedit::EditorOption::ShowTrailingSpaces, mEditorSettings->showTrailingSpaces());
    options.setFlag(QSynedit::EditorOption::ShowInnerSpaces, mEditorSettings->showInnerSpaces());
    options.setFlag(QSynedit::EditorOption::ShowLineBreaks, mEditorSettings->showLineBreaks());

    options.setFlag(QSynedit::EditorOption::AutoIndent,mEditorSettings->autoIndent());
    options.setFlag(QSynedit::EditorOption::TabsToSpaces,mEditorSettings->tabToSpaces());

    options.setFlag(QSynedit::EditorOption::KeepCaretX,mEditorSettings->keepCaretX());
    options.setFlag(QSynedit::EditorOption::EnhanceHomeKey,mEditorSettings->enhanceHomeKey());
    options.setFlag(QSynedit::EditorOption::EnhanceEndKey,mEditorSettings->enhanceEndKey());

    options.setFlag(QSynedit::EditorOption::AutoHideScrollbars,mEditorSettings->autoHideScrollbar());
    options.setFlag(QSynedit::EditorOption::ScrollPastEol,mEditorSettings->scrollPastEol());
    options.setFlag(QSynedit::EditorOption::ScrollPastEof,mEditorSettings->scrollPastEof());
    options.setFlag(QSynedit::EditorOption::HalfPageScroll,mEditorSettings->halfPageScroll());
    options.setFlag(QSynedit::EditorOption::InvertMouseScroll, false);

    options.setFlag(QSynedit::EditorOption::ShowRainbowColor,
                    mEditorSettings->rainbowParenthesis()
                    && syntaxer()->supportBraceLevel());
    options.setFlag(QSynedit::EditorOption::ForceMonospace,
                    mEditorSettings->forceFixedFontWidth());
    options.setFlag(QSynedit::EditorOption::LigatureSupport,
                    mEditorSettings->enableLigaturesSupport());
    setOptions(options);

    setTabSize(mEditorSettings->tabWidth());
    setInsertCaret(mEditorSettings->caretForInsert());
    setOverwriteCaret(mEditorSettings->caretForOverwrite());
    setCaretUseTextColor(mEditorSettings->caretUseTextColor());
    setCaretColor(mEditorSettings->caretColor());

    codeFolding().indentGuides = mEditorSettings->showIndentLines();
    codeFolding().fillIndents = mEditorSettings->fillIndents();
    codeFolding().rainbowIndentGuides = mEditorSettings->rainbowIndentGuides();
    codeFolding().rainbowIndents = mEditorSettings->rainbowIndents();

    QFont f=QFont();
    f.setFamily(mEditorSettings->fontName());
    f.setFamilies(mEditorSettings->fontFamiliesWithControlFont());
    f.setPixelSize(pointToPixel(mEditorSettings->fontSize()));
    f.setStyleStrategy(QFont::PreferAntialias);
    setFont(f);

    // QFont f=QFont(mEditorSettings->fontName());
    // f.setPixelSize(pointToPixel(mEditorSettings->fontSize()));
    // f.setStyleStrategy(QFont::PreferAntialias);
    // setFont(f);
    // QFont f2=QFont(mEditorSettings->nonAsciiFontName());
    // f2.setPixelSize(pointToPixel(mEditorSettings->fontSize()));
    // f2.setStyleStrategy(QFont::PreferAntialias);
    // setFontForNonAscii(f2);
    setLineSpacingFactor(mEditorSettings->lineSpacing());

    // Set gutter properties
    gutter().setLeftOffset(pointToPixel(mEditorSettings->fontSize()) + mEditorSettings->gutterLeftOffset());
    gutter().setRightOffset(pointToPixel(mEditorSettings->fontSize()) + mEditorSettings->gutterRightOffset());
    gutter().setBorderStyle(QSynedit::GutterBorderStyle::None);
    gutter().setUseFontStyle(mEditorSettings->gutterUseCustomFont());
    if (mEditorSettings->gutterUseCustomFont()) {
        f=QFont(mEditorSettings->gutterFontName());
        f.setPixelSize(pointToPixel(mEditorSettings->gutterFontSize()));
    } else {
        f=QFont(mEditorSettings->fontName());
        f.setPixelSize(pointToPixel(mEditorSettings->fontSize()));
    }
    f.setStyleStrategy(QFont::PreferAntialias);
    gutter().setFont(f);
    gutter().setDigitCount(mEditorSettings->gutterDigitsCount());
    gutter().setVisible(mEditorSettings->gutterVisible());
    gutter().setAutoSize(mEditorSettings->gutterAutoSize());
    gutter().setShowLineNumbers(mEditorSettings->gutterShowLineNumbers());
    gutter().setLeadingZeros(mEditorSettings->gutterAddLeadingZero());
    if (mEditorSettings->gutterLineNumbersStartZero())
        gutter().setLineNumberStart(0);
    else
        gutter().setLineNumberStart(1);
    //font color

    if (mEditorSettings->showRightEdgeLine()) {
        setRightEdge(mEditorSettings->rightEdgeWidth());
        setRightEdgeColor(mEditorSettings->rightEdgeLineColor());
    } else {
        setRightEdge(0);
    }

    if (syntaxer()->language() == QSynedit::ProgrammingLanguage::CPP) {
        QSet<QString> set;
        if (mEditorSettings->enableCustomCTypeKeywords()) {
            foreach(const QString& s, mEditorSettings->customCTypeKeywords())
                set.insert(s);
        }
#ifdef ENABLE_SDCC
        if (mGetCompilerTypeForEditorFunc && mGetCompilerTypeForEditorFunc(this) == CompilerType::SDCC) {
            for(auto it=SDCCKeywords.begin();it!=SDCCKeywords.end();++it)
                set.insert(it.key());
        }
#endif
        ((QSynedit::CppSyntaxer*)(syntaxer().get()))->setCustomTypeKeywords(set);
    }

    mCodeCompletionEnabled = mCodeCompletionSettings && mCodeCompletionSettings->enabled();

    initAutoBackup();

    setMouseWheelScrollSpeed(mEditorSettings->mouseWheelScrollSpeed());
    setMouseSelectionScrollSpeed(mEditorSettings->mouseSelectionScrollSpeed());

    applyColorScheme(mEditorSettings->colorScheme());
    invalidate();
    endInternalChanges();
}

static QSynedit::PTokenAttribute createRainbowAttribute(ColorManager *colorManager, const QString& attrName, const QString& schemeName, const QString& schemeItemName) {
    PColorSchemeItem item = colorManager->getItem(schemeName,schemeItemName);
    if (item) {
        QSynedit::PTokenAttribute attr = std::make_shared<QSynedit::TokenAttribute>(attrName,
                                                                                                QSynedit::TokenType::Default);
        attr->setForeground(item->foreground());
        attr->setBackground(item->background());
        return attr;
    }
    return QSynedit::PTokenAttribute();
}

void Editor::applyColorScheme(const QString& schemeName)
{
    QSynedit::EditorOptions options = getOptions();
    options.setFlag(QSynedit::EditorOption::ShowRainbowColor,
                    mEditorSettings->rainbowParenthesis()
                    && syntaxer()->supportBraceLevel());
    setOptions(options);
    codeFolding().rainbowIndentGuides = mEditorSettings->rainbowIndentGuides();
    codeFolding().rainbowIndents = mEditorSettings->rainbowIndents();
    Q_ASSERT(mColorManager!=nullptr);
    mColorManager->applySchemeToSyntaxer(syntaxer(),schemeName);
    if (mEditorSettings->rainbowParenthesis()) {
        QSynedit::PTokenAttribute attr0 =createRainbowAttribute(mColorManager, SYNS_AttrSymbol,
                                                               schemeName,COLOR_SCHEME_BRACE_1);
        QSynedit::PTokenAttribute attr1 =createRainbowAttribute(mColorManager,SYNS_AttrSymbol,
                                                               schemeName,COLOR_SCHEME_BRACE_2);
        QSynedit::PTokenAttribute attr2 =createRainbowAttribute(mColorManager,SYNS_AttrSymbol,
                                                               schemeName,COLOR_SCHEME_BRACE_3);
        QSynedit::PTokenAttribute attr3 =createRainbowAttribute(mColorManager,SYNS_AttrSymbol,
                                                               schemeName,COLOR_SCHEME_BRACE_4);
        setRainbowAttrs(attr0,attr1,attr2,attr3);
    }
    PColorSchemeItem item = mColorManager->getItem(schemeName,COLOR_SCHEME_ACTIVE_LINE);
    if (item) {
        setActiveLineColor(item->background());
    }
    item = mColorManager->getItem(schemeName,COLOR_SCHEME_GUTTER);
    if (item) {
        gutter().setTextColor(item->foreground());
        gutter().setColor(alphaBlend(palette().color(QPalette::Base), item->background()));
    }
    item = mColorManager->getItem(schemeName,COLOR_SCHEME_GUTTER_ACTIVE_LINE);
    if (item) {
        gutter().setActiveLineTextColor(item->foreground());
    }
    item = mColorManager->getItem(schemeName,COLOR_SCHEME_FOLD_LINE);
    if (item) {
        codeFolding().folderBarLinesColor = item->foreground();
    }
    item = mColorManager->getItem(schemeName,COLOR_SCHEME_INDENT_GUIDE_LINE);
    if (item) {
        codeFolding().indentGuidesColor = item->foreground();
    }
    item = mColorManager->getItem(schemeName,COLOR_SCHEME_ERROR);
    if (item) {
        this->mSyntaxErrorColor = item->foreground();
    }
    item = mColorManager->getItem(schemeName,COLOR_SCHEME_WARNING);
    if (item) {
        this->mSyntaxWarningColor = item->foreground();
    }
    item = mColorManager->getItem(schemeName,COLOR_SCHEME_SELECTION);
    if (item) {
        setSelectedForeground(item->foreground());
        setSelectedBackground(item->background());
    } else {
        this->setForegroundColor(palette().color(QPalette::HighlightedText));
        this->setBackgroundColor(palette().color(QPalette::Highlight));
    }
    item = mColorManager->getItem(schemeName,COLOR_SCHEME_ACTIVE_BREAKPOINT);
    if (item) {
        this->mActiveBreakpointForegroundColor = item->foreground();
        this->mActiveBreakpointBackgroundColor = item->background();
    }
    item = mColorManager->getItem(schemeName,COLOR_SCHEME_BREAKPOINT);
    if (item) {
        this->mBreakpointForegroundColor = item->foreground();
        this->mBreakpointBackgroundColor = item->background();
    }
    item = mColorManager->getItem(schemeName,COLOR_SCHEME_TEXT);
    if (item) {
        this->setForegroundColor(item->foreground());
        this->setBackgroundColor(alphaBlend(palette().color(QPalette::Base), item->background()));
    } else {
        this->setForegroundColor(palette().color(QPalette::Text));
        this->setBackgroundColor(palette().color(QPalette::Base));
    }
    item = mColorManager->getItem(schemeName,COLOR_SCHEME_CURRENT_HIGHLIGHTED_WORD);
    if (item) {
        mCurrentHighlighWordForeground = item->foreground();
        mCurrentHighlighWordBackground = item->background();
    } else {
        mCurrentHighlighWordForeground = selectedForeground();
        mCurrentHighlighWordBackground = selectedBackground();
    }
    invalidateAllNonTempLineWidth();
    invalidate();
}

void Editor::setAutoIndent(bool indent)
{
    QSynedit::EditorOptions opts = getOptions();
    opts.setFlag(QSynedit::EditorOption::AutoIndent,indent);
    setOptions(opts);
}

bool Editor::autoIndent()
{
    QSynedit::EditorOptions opts = getOptions();
    return opts.testFlag(QSynedit::EditorOption::AutoIndent);
}

QString Editor::caption() {
    QString result;
    result = QFileInfo(mFilename).fileName();
    if (this->modified()) {
        result.append("[*]");
    }
    if (this->readOnly()) {
        result.append("["+tr("Readonly")+"]");
    }
    result = result.replace("&","&&");
    return result;
}
