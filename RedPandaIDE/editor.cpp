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
#include <qsynedit/document.h>
#include <qsynedit/syntaxer/cpp.h>
#include <qsynedit/syntaxer/asm.h>
#include <qsynedit/exporter/rtfexporter.h>
#include <qsynedit/exporter/htmlexporter.h>
#include <qsynedit/exporter/qtsupportedhtmlexporter.h>
#include <qsynedit/constants.h>
#include "settings.h"
#include "mainwindow.h"
#include "systemconsts.h"
#include "syntaxermanager.h"
#include "iconsmanager.h"
#include "debugger/debugger.h"
#include "editorlist.h"
#include <QDebug>
#include "project.h"
#include <qt_utils/charsetinfo.h>
#include "utils/escape.h"

QHash<ParserLanguage,std::weak_ptr<CppParser>> Editor::mSharedParsers;

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

Editor::Editor(QWidget *parent):
    Editor(parent,"untitled",ENCODING_AUTO_DETECT,nullptr,true,nullptr)
{
}

Editor::Editor(QWidget *parent, const QString& filename,
                  const QByteArray& encoding,
                  Project* pProject, bool isNew,
                  QTabWidget* parentPageControl):
  QSynEdit{parent},
  mInited{false},
  mEncodingOption{encoding},
  mFilename{filename},
  mParentPageControl{parentPageControl},
  mProject{pProject},
  mIsNew{isNew},
  mSyntaxErrorColor{Qt::red},
  mSyntaxWarningColor{"orange"},
  mLineCount{0},
  mActiveBreakpointLine{-1},
  mCurrentTipType{TipType::None},
  mSaving{false},
  mHoverModifiedLine{-1},
  mWheelAccumulatedDelta{0}
{
    mLastFocusOutTime = 0;
    mInited=false;
    mBackupFile=nullptr;
    mHighlightCharPos1 = QSynedit::BufferCoord{0,0};
    mHighlightCharPos2 = QSynedit::BufferCoord{0,0};
    mCurrentLineModified = false;
    mUseCppSyntax = pSettings->editor().defaultFileCpp();
    if (mFilename.isEmpty()) {
        mFilename = QString("untitled%1").arg(getNewFileNumber());
    }
    QFileInfo fileInfo(mFilename);
    QSynedit::PSyntaxer syntaxer;
    syntaxer = syntaxerManager.getSyntaxer(mFilename);
    if (syntaxer) {
        setSyntaxer(syntaxer);
        setFormatter(syntaxerManager.getFormatter(syntaxer->language()));
        setUseCodeFolding(true);
    } else {
        setUseCodeFolding(false);
    }

    if (mProject && mEncodingOption==ENCODING_PROJECT) {
        mEncodingOption=mProject->options().encoding;
    }
    mFileEncoding = ENCODING_ASCII;
    if (!isNew) {
        try {
            loadFile();
        } catch (FileError& e) {
            QMessageBox::critical(nullptr,
                                  tr("Error Load File"),
                                  e.reason());
        }
    }
    resolveAutoDetectEncodingOption();

    if (mProject) {
        if (syntaxer && syntaxer->language() == QSynedit::ProgrammingLanguage::CPP)
            mParser = mProject->cppParser();
    } else {
        initParser();
    }

    if (shouldOpenInReadonly()) {
        this->setModified(false);
        setReadOnly(true);
    }

    mCompletionPopup = pMainWindow->completionPopup();
    mHeaderCompletionPopup = pMainWindow->headerCompletionPopup();

    applySettings();
    applyColorScheme(pSettings->editor().colorScheme());

    //Initialize User Code Template stuff;
    mXOffsetSince =0;
    mTabStopY=-1;
    mTabStopBegin= -1;
    mTabStopEnd= -1;
    //mLineBeforeTabStop="";
    //mLineAfterTabStop = "";

    connect(this,&QSynEdit::statusChanged,this,&Editor::onStatusChanged);

    if (mParentPageControl)
        connect(this,&QSynEdit::gutterClicked,this,&Editor::onGutterClicked);

    onStatusChanged(QSynedit::StatusChange::OpenFile);

    setAttribute(Qt::WA_Hover,true);

    connect(this,&QSynEdit::linesDeleted,
            this, &Editor::onLinesDeleted);
    connect(this,&QSynEdit::linesInserted,
            this, &Editor::onLinesInserted);

    setContextMenuPolicy(Qt::CustomContextMenu);

    if (mParentPageControl)
        connect(this, &QWidget::customContextMenuRequested,
            pMainWindow, &MainWindow::onEditorContextMenu);

    mCanAutoSave = false;
    if (isNew && parentPageControl!=nullptr ) {
        FileType fileType = getFileType(filename);
        QString fileTemplate;
        switch (fileType) {
        case FileType::CSource:
            fileTemplate = pMainWindow->codeSnippetManager()->newCFileTemplate();
            break;
        case FileType::CppSource:
            fileTemplate = pMainWindow->codeSnippetManager()->newCppFileTemplate();
            break;
        case FileType::GAS:
            fileTemplate = pMainWindow->codeSnippetManager()->newGASFileTemplate();
            break;
        default:
            break;
        }
        if (!fileTemplate.isEmpty()) {
            insertCodeSnippet(fileTemplate);
            setCaretPosition(1,1);
            setModified(false);
        }
    }
    if (!isNew && parentPageControl) {
        resetBookmarks();
        resetBreakpoints();
    }

    mStatementColors = pMainWindow->statementColors();
    if (mParentPageControl) {
        //first showEvent triggered here
        mParentPageControl->addTab(this,"");
        updateCaption();
    }

    if (!inTab()) {
        setExtraKeystrokes();
    }

    if (inTab()) {
        connect(&mFunctionTipTimer, &QTimer::timeout,
            this, &Editor::onFunctionTipsTimer);
        mAutoBackupTimer.setInterval(1);
        connect(&mAutoBackupTimer, &QTimer::timeout,
            this, &Editor::onAutoBackupTimer);
    }

    connect(&mTooltipTimer, &QTimer::timeout,
            this, &Editor::onTooltipTimer);

    connect(horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &Editor::onScrollBarValueChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, &Editor::onScrollBarValueChanged);
    mInited=true;

    //show event is trigged when this is added to the qtabwidget
    if (!pMainWindow->openingFiles()
            && !pMainWindow->openingProject()) {
        reparse(false);
        checkSyntaxInBack();
        reparseTodo();
    }
}

Editor::~Editor() {
    //qDebug()<<"editor "<<mFilename<<" deleted";
    cleanAutoBackup();
}

void Editor::loadFile(QString filename) {
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

    this->document()->loadFromFile(filename,mEncodingOption,mFileEncoding);

    if (mProject) {
        PProjectUnit unit = mProject->findUnit(this);
        if (unit) {
            unit->setRealEncoding(mFileEncoding);
        }
    }

    //this->setModified(false);
    updateCaption();
    if (inTab())
        pMainWindow->updateForEncodingInfo(this);
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
    reparse(true);
    if (pSettings->editor().syntaxCheckWhenLineChanged()) {
        checkSyntaxInBack();
    }
    reparseTodo();
    saveAutoBackup();
}

void Editor::saveFile(QString filename) {
    QFile file(filename);
//    QByteArray encoding = mFileEncoding;
//    if (mEncodingOption != ENCODING_AUTO_DETECT || mFileEncoding==ENCODING_ASCII)
//        encoding = mEncodingOption;
    QByteArray encoding = mEncodingOption;
//  save backup
//    QString backupFilename=filename+".savebak";
//    int count=1;
//    while (fileExists(backupFilename)) {
//        backupFilename=filename+QString(".savebak%1").arg(count);
//        count++;
//    }
//    if (!fileExists(filename)) {
//        if (!stringToFile(text(),backupFilename)) {
//            if (QMessageBox::question(pMainWindow,tr("Error"),
//                                 tr("Can't generate temporary backup file '%1'.").arg(backupFilename)
//                                  +"<br />"
//                                  +tr("Continue to save?"),
//                                  QMessageBox::Yes | QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)
//                return;
//        }
//    } else if (!QFile::copy(filename,backupFilename)) {
//        if (QMessageBox::question(pMainWindow,tr("Error"),
//                             tr("Can't generate temporary backup file '%1'.").arg(backupFilename)
//                              +"<br />"
//                              +tr("Continue to save?"),
//                              QMessageBox::Yes | QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)
//            return;
//    }
    this->document()->saveToFile(file,encoding,
                              pSettings->editor().defaultEncoding(),
                              mFileEncoding);
    if (mProject) {
        PProjectUnit unit = mProject->findUnit(this);
        if (unit) {
            unit->setRealEncoding(mFileEncoding);
        }
    }
    if (isVisible() && inTab())
        pMainWindow->updateForEncodingInfo(this);
    emit fileSaved(filename, inProject());
//    QFile::remove(backupFilename);
}

void Editor::convertToEncoding(const QByteArray &encoding)
{
    mEncodingOption = encoding;
    setModified(true);
    save();
    if (mProject) {
        PProjectUnit unit = mProject->findUnit(this);
        if (unit) {
            unit->setEncoding(mEncodingOption);
            unit->setRealEncoding(mFileEncoding);
        }
    }
}

bool Editor::save(bool force, bool doReparse) {
    if (this->mIsNew && !force) {
        return saveAs();
    }    

    pMainWindow->fileSystemWatcher()->removePath(mFilename);
    try {
        if (pSettings->editor().autoFormatWhenSaved()) {
            reformat(false);
        } else if (pSettings->editor().removeTrailingSpacesWhenSaved()) {
            trimTrailingSpaces();
        }
        saveFile(mFilename);
        pMainWindow->fileSystemWatcher()->addPath(mFilename);
        setModified(false);
        mIsNew = false;
        updateCaption();
    } catch (FileError& exception) {
        if (!force) {
            QMessageBox::critical(pMainWindow,tr("Error"),
                                 exception.reason());
        }
        pMainWindow->fileSystemWatcher()->addPath(mFilename);
        return false;
    }

    if (doReparse && isVisible()) {
        reparse(false);
        if (pSettings->editor().syntaxCheckWhenSave())
            checkSyntaxInBack();
        reparseTodo();
    }

    return true;
}

bool Editor::saveAs(const QString &name, bool fromProject){
    QString newName = name;
    QString oldName = mFilename;
    bool firstSave = isNew();
    if (name.isEmpty()) {
        QString selectedFileFilter;
        QString defaultExt;
        defaultExt=QFileInfo(oldName).suffix();
        if (defaultExt.isEmpty()) {
            if (pSettings->editor().defaultFileCpp()) {
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
        newName = dialog.selectedFiles()[0];
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

    if (pMainWindow->editorList()->getOpenedEditorByFilename(newName)) {
        QMessageBox::critical(pMainWindow,tr("Error"),
                              tr("File %1 already opened!").arg(newName));
        return false;
    }
    // Update project information
    if (mProject && !fromProject) {
        PProjectUnit unit = mProject->findUnit(newName);
        if (!unit) {
            setProject(nullptr);
        }
    }

    clearSyntaxIssues();
    pMainWindow->fileSystemWatcher()->removePath(mFilename);
    if (pSettings->codeCompletion().enabled() && mParser && !inProject()) {
        mParser->invalidateFile(mFilename);
    }

    if (pSettings->editor().autoFormatWhenSaved()) {
        reformat(false);
    } else if (pSettings->editor().removeTrailingSpacesWhenSaved()) {
        trimTrailingSpaces();
    }
    try {
        mFilename = newName;
        saveFile(mFilename);
        mIsNew = false;
        setModified(false);
    }  catch (FileError& exception) {
        QMessageBox::critical(pMainWindow,tr("Error"),
                                 exception.reason());
        return false;
    }
    if (mProject && !fromProject) {
        mProject->associateEditor(this);
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

    //update (reassign syntaxer)
    QSynedit::PSyntaxer newSyntaxer = syntaxerManager.getSyntaxer(mFilename);
    if (newSyntaxer) {
        setUseCodeFolding(true);
        setFormatter(syntaxerManager.getFormatter(newSyntaxer->language()));
    } else {
        setUseCodeFolding(false);
        setFormatter(syntaxerManager.getFormatter(QSynedit::ProgrammingLanguage::Unknown));
    }
    setSyntaxer(newSyntaxer);

    if (!newSyntaxer || newSyntaxer->language() != QSynedit::ProgrammingLanguage::CPP) {
        mSyntaxIssues.clear();
    }
    applyColorScheme(pSettings->editor().colorScheme());

    if (!inProject()) {
        initParser();
        reparse(false);
        reparseTodo();
    }

    if (pSettings->editor().syntaxCheckWhenSave())
        checkSyntaxInBack();


    if (!shouldOpenInReadonly()) {
        setReadOnly(false);
    }
    updateCaption();

    emit renamed(oldName, newName , firstSave);

    initAutoBackup();
    return true;
}

void Editor::setFilename(const QString &newName)
{
    if (mFilename == newName)
        return;
    if (pMainWindow->editorList()->getOpenedEditorByFilename(newName)) {
        return;
    }
    QString oldName = mFilename;
    // Update project information
    if (mProject) {
        PProjectUnit unit = mProject->findUnit(oldName);
        if (unit) {
            mProject->renameUnit(unit, newName);
        }
    }

    clearSyntaxIssues();
    pMainWindow->fileSystemWatcher()->removePath(oldName);
    if (pSettings->codeCompletion().enabled() && mParser && !inProject()) {
        mParser->invalidateFile(oldName);
    }

    mFilename = newName;
    if (mProject) {
        mProject->associateEditor(this);
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

    //update (reassign syntaxer)
    QSynedit::PSyntaxer newSyntaxer = syntaxerManager.getSyntaxer(mFilename);
    if (newSyntaxer) {
        setUseCodeFolding(true);
        setFormatter(syntaxerManager.getFormatter(newSyntaxer->language()));
    } else {
        setUseCodeFolding(false);
        setFormatter(syntaxerManager.getFormatter(QSynedit::ProgrammingLanguage::Unknown));
    }
    setSyntaxer(newSyntaxer);

    if (!newSyntaxer || newSyntaxer->language() != QSynedit::ProgrammingLanguage::CPP) {
        mSyntaxIssues.clear();
    }
    applyColorScheme(pSettings->editor().colorScheme());

    if (!inProject()) {
        initParser();
        reparse(false);
        reparseTodo();
    }

    if (pSettings->editor().syntaxCheckWhenSave())
        checkSyntaxInBack();

    updateCaption();

    emit renamed(oldName, newName , true);

    initAutoBackup();
    return;
}

void Editor::activate(bool focus)
{
    if (mParentPageControl)
        mParentPageControl->setCurrentWidget(this);
    if (focus)
        setFocus();
}

const QByteArray& Editor::encodingOption() const noexcept{
    return mEncodingOption;
}
void Editor::setEncodingOption(const QByteArray& encoding) noexcept{
    QByteArray newEncoding=encoding;
    if (mProject && encoding==ENCODING_PROJECT)
        newEncoding=mProject->options().encoding;
    if (mEncodingOption == newEncoding)
        return;
    mEncodingOption = newEncoding;
    if (!isNew()) {
        try {
            loadFile();
        } catch (FileError& e) {
            QMessageBox::critical(nullptr,
                                  tr("Error Load File"),
                                  e.reason());
        }
    } else if (inTab())
        pMainWindow->updateForEncodingInfo(this);
    resolveAutoDetectEncodingOption();
    if (mProject) {
        PProjectUnit unit = mProject->findUnit(this);
        if (unit) {
            unit->setEncoding(mEncodingOption);
            unit->setRealEncoding(mFileEncoding);
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
    return mProject!=nullptr;
}
bool Editor::isNew() const noexcept {
    return mIsNew;
}

QTabWidget* Editor::pageControl() noexcept{
    return mParentPageControl;
}
static int findWidgetInPageControl(QTabWidget* pageControl, QWidget* child) {
    for (int i=0;i<pageControl->count();i++) {
        if (pageControl->widget(i)==child)
            return i;
    }
    return -1;
}
void Editor::setPageControl(QTabWidget *newPageControl)
{
    if (mParentPageControl==newPageControl)
        return;
    if (inTab()) {
        int index = findWidgetInPageControl(mParentPageControl,this);
        if (index>=0)
            mParentPageControl->removeTab(index);
    }
    mParentPageControl= newPageControl;
    if (newPageControl!=nullptr) {
        mParentPageControl->addTab(this,"");
        updateCaption();
    }
}

void Editor::undoSymbolCompletion(int pos)
{
    QSynedit::PTokenAttribute attr;
    QString token;
    QSynedit::SyntaxState syntaxState;

    if (!pSettings->editor().removeSymbolPairs())
        return;
    QSynedit::BufferCoord coord{pos, caretY()};
    if (!getTokenAttriAtRowCol(coord, token, attr, syntaxState))
        return;
    if (syntaxer()->isCommentNotFinished(syntaxState.state))
        return ;
    //convert caret x to string index;
    pos--;

    if (pos<0 || pos+1>=lineText().length())
        return;
    QChar deletedChar = lineText().at(pos);
    QChar nextChar = lineText().at(pos+1);
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
    if ( (pSettings->editor().completeBracket() && (deletedChar == '[') && (nextChar == ']')) ||
         (pSettings->editor().completeParenthese() && (deletedChar == '(') && (nextChar == ')')) ||
         (pSettings->editor().completeGlobalInclude() && (deletedChar == '<') && (nextChar == '>')) ||
         (pSettings->editor().completeBrace() && (deletedChar == '{') && (nextChar == '}')) ||
         (pSettings->editor().completeSingleQuote() && (deletedChar == '\'') && (nextChar == '\'')) ||
         (pSettings->editor().completeDoubleQuote() && (deletedChar == '\"') && (nextChar == '\"'))) {
         processCommand(QSynedit::EditCommand::DeleteChar);
    }
}

void Editor::wheelEvent(QWheelEvent *event) {
    if ( (event->modifiers() & Qt::ControlModifier)!=0) {
        int size = pSettings->editor().fontSize();
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
            pSettings->editor().setFontSize(size);
            pSettings->editor().save();
            pMainWindow->updateEditorSettings();
        }
        event->accept();
        return;
    }
    QSynEdit::wheelEvent(event);
}

void Editor::focusInEvent(QFocusEvent *event)
{
    QSynEdit::focusInEvent(event);
    if (inTab()) {
        pMainWindow->updateClassBrowserForEditor(this);
        pMainWindow->updateAppTitle(this);
        pMainWindow->updateEditorActions(this);
        pMainWindow->updateForEncodingInfo(this);
        pMainWindow->updateStatusbarForLineCol(this);
        pMainWindow->updateForStatusbarModeInfo(this);
    }
}

void Editor::focusOutEvent(QFocusEvent *event)
{
    QSynEdit::focusOutEvent(event);
    //pMainWindow->updateClassBrowserForEditor(nullptr);
    if (!pMainWindow->isQuitting()) {
        pMainWindow->functionTip()->hide();
    }
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
            && !mCompletionPopup->isVisible()
            && !mHeaderCompletionPopup->isVisible()
            ) {
        //setMouseTracking(true);
        handled=true;
        QMouseEvent mouseEvent{
            QEvent::MouseMove,
                    mapFromGlobal(QCursor::pos()),
                    Qt::NoButton,
                    Qt::NoButton,
                    Qt::ControlModifier};
        mouseMoveEvent( &mouseEvent );
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
            QString sLine = lineText().mid(0,caretX()-1).trimmed();
            QSynedit::SyntaxState state = calcSyntaxStateAtLine(caretY()-1, sLine);
            if (syntaxer()->isCommentNotFinished(state.state)) {
                if (sLine=="/**") { //javadoc style docstring
                    sLine = lineText().mid(caretX()-1).trimmed();
                    if (sLine=="*/") {
                        QSynedit::BufferCoord p = caretXY();
                        setBlockBegin(p);
                        p.ch = lineText().length()+1;
                        setBlockEnd(p);
                        setSelText("");
                    }
                    handled = true;
                    QStringList insertString;
                    insertString.append("");
                    PStatement function;
                    if (mParser)
                        function = mParser->findFunctionAt(mFilename,caretY()+1);
                    if (function) {
                        QStringList params;
                        QString funcName = function->command;
                        bool isVoid = (function->type  == "void");
                        foreach (const PStatement& child, function->children) {
                            if (child->kind == StatementKind::Parameter)
                                params.append(child->command);
                        }
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
                        sLine=lineBreak()+"* ";
                        insertString(sLine,false);
                        QSynedit::BufferCoord p = caretXY();
                        p.line++;
                        p.ch = lineText(p.line).length()+1;
                        if (right>0) {
                            p.ch -=right+1;
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
            setBlockEnd(caretXY());
            invalidateLine(caretY());
            clearUserCodeInTabStops();
        }
        pMainWindow->functionTip()->hide();
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
        if (pMainWindow->functionTip()->isVisible()) {
            handled = true;
            pMainWindow->functionTip()->previousTip();
        } else {
            clearUserCodeInTabStops();
        }
        return;
    case Qt::Key_Down:
        if (pMainWindow->functionTip()->isVisible()) {
            handled = true;
            pMainWindow->functionTip()->nextTip();
        } else {
            clearUserCodeInTabStops();
        }
        return;
    case Qt::Key_Delete:
        // remove completed character
        if (!selAvail()) {
            undoSymbolCompletion(caretX());
        }
        return;
    case Qt::Key_Backspace:
        // remove completed character
        if (!selAvail()) {
            undoSymbolCompletion(caretX()-1);
        }
        return;
    }

    QString t = event->text();
    if (t.isEmpty())
        return;

    if (activeSelectionMode()==QSynedit::SelectionMode::Column)
        return;

    QChar ch = t[0];
    QSynedit::BufferCoord ws=wordStart();
    int idCharPressed=caretX()-ws.ch;
    if (isIdentStartChar(ch)) {
        idCharPressed++;
        if (pSettings->codeCompletion().enabled()
                && pSettings->codeCompletion().showCompletionWhileInput()
                && idCharPressed>=pSettings->codeCompletion().minCharRequired()) {
            if (mParser) {
                if (mParser->isIncludeLine(lineText())) {
                    // is a #include line
                    processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                    showHeaderCompletion(false);
                    handled=true;
                    return;
                } else {
                    bool hasTypeQualifier;
                    QString lastWord = getPreviousWordAtPositionForSuggestion(ws, hasTypeQualifier);
                    if (mParser && (!lastWord.isEmpty() || hasTypeQualifier)) {
                        if (lastWord.isEmpty()) {
                            Q_ASSERT(hasTypeQualifier);
                            processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                            showCompletion(lastWord,false, CodeCompletionType::Types);
                            handled=true;
                            return;
                        } else if ( lastWord == "typedef" ) {
                            processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                            showCompletion(lastWord,false, CodeCompletionType::Types);
                            handled=true;
                            return;
                        } else if (lastWord == "using") {
                            processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                            showCompletion(lastWord,false, CodeCompletionType::ComplexKeyword);
                            handled=true;
                            return;
                        } else if (lastWord == "namespace") {
                            processCommand(QSynedit::EditCommand::Char,ch,nullptr);
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
                                processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                                showCompletion(lastWord,false,CodeCompletionType::FunctionWithoutDefinition);
                                handled=true;
                                return;
                            }
                            if (lastWord == "long" ||
                                    lastWord == "short" ||
                                    lastWord == "signed" ||
                                    lastWord == "unsigned") {
                                processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                                showCompletion(lastWord,false, CodeCompletionType::ComplexKeyword);
                                handled=true;
                                return;
                            }


                            //last word is a type keyword, this is a var or param define, and dont show suggestion
                            return;
                        } else if (lastWord == "#ifdef" || lastWord == "#ifndef" || lastWord == "#undef") {
                            processCommand(QSynedit::EditCommand::Char,ch,nullptr);
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
                                processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                                showCompletion("",false,CodeCompletionType::FunctionWithoutDefinition);
                                handled=true;
                                return;
                            }

                            //last word is a typedef/class/struct, this is a var or param define, and dont show suggestion
                            return;
                        }
                    }
                    lastWord = getPreviousWordAtPositionForCompleteFunctionDefinition(ws);
                    if (mParser && !lastWord.isEmpty()) {
                        PStatement currentScope = mParser->findScopeStatement(mFilename,caretY());
                        while(currentScope && currentScope->kind==StatementKind::Block) {
                            currentScope = currentScope->parentScope.lock();
                        }
                        if (!currentScope || currentScope->kind == StatementKind::Namespace) {
                            //may define a function
                            processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                            showCompletion("",false,CodeCompletionType::FunctionWithoutDefinition);
                            handled=true;
                            return;
                        }
                    }
                    processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                    showCompletion("",false,CodeCompletionType::Normal);
                    handled=true;
                    return;
                }
            } else {
                //show keywords
                processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                showCompletion("",false,CodeCompletionType::KeywordsOnly);
                handled=true;
                return;
            }
        }
    } else {
        if (pSettings->codeCompletion().enabled()
                && pSettings->codeCompletion().showCompletionWhileInput() ) {
            if (mParser && mParser->isIncludeLine(lineText())
                    && ch.isDigit()) {
                // is a #include line
                processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                showHeaderCompletion(false);
                handled=true;
                return;
            } else if (syntaxer()->language()==QSynedit::ProgrammingLanguage::CPP) {
                //preprocessor ?
                if ((idCharPressed==0) && (ch=='#') && lineText().isEmpty()) {
                    processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                    showCompletion("",false,CodeCompletionType::Normal);
                    handled=true;
                    return;
                }
                //javadoc directive?
                if  ((idCharPressed==0) && (ch=='@') &&
                      lineText().trimmed().startsWith('*')) {
                    processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                    showCompletion("",false,CodeCompletionType::Normal);
                    handled=true;
                    return;
                }
            } else if (syntaxer()->language()==QSynedit::ProgrammingLanguage::LUA) {
                if (ch=='.') {
                    processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                    showCompletion("",false,CodeCompletionType::KeywordsOnly);
                    handled=true;
                    return;
                }
            } else if (syntaxer()->language()==QSynedit::ProgrammingLanguage::ATTAssembly) {
                if ((idCharPressed==0) && (ch=='.')) {
                    processCommand(QSynedit::EditCommand::Char,ch,nullptr);
                    showCompletion("",false,CodeCompletionType::KeywordsOnly);
                    handled=true;
                    return;
                }
                if ((idCharPressed==0) && (ch=='%')) {
                    processCommand(QSynedit::EditCommand::Char,ch,nullptr);
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
    if (pSettings->codeCompletion().enabled())
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
    if(event->modifiers() == Qt::ControlModifier) {
        cancelHint();

        QSynedit::BufferCoord p;
        TipType reason = getTipType(event->pos(),p);
        if (reason == TipType::Preprocessor) {
            QString sLine = lineText(p.line);
            if (mParser->isIncludeNextLine(sLine) || mParser->isIncludeLine(sLine))
                updateHoverLink(p.line);
        } else if (reason == TipType::Identifier) {
            updateHoverLink(p.line);
        } else {
            cancelHoverLink();
        }
        return;
    }
    QSynedit::QSynEdit::mouseMoveEvent(event);
}

void Editor::onGutterPaint(QPainter &painter, int aLine, int X, int Y)
{
    IconsManager::PPixmap icon;

    if (mActiveBreakpointLine == aLine) {
        icon = pIconsManager->getPixmap(IconsManager::GUTTER_ACTIVEBREAKPOINT);
    } else if (hasBreakpoint(aLine)) {
        icon = pIconsManager->getPixmap(IconsManager::GUTTER_BREAKPOINT);
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
                icon = pIconsManager->getPixmap(IconsManager::GUTTER_SYNTAX_ERROR);
            } else {
                icon = pIconsManager->getPixmap(IconsManager::GUTTER_SYNTAX_WARNING);
            }
        } else if (hasBookmark(aLine)) {
            icon = pIconsManager->getPixmap(IconsManager::GUTTER_BOOKMARK);
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
        p->beginX = pos1+2;
        p->endX = pos2+1;
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
            int pos1=sLine.indexOf("<")+1;
            int pos2=sLine.indexOf(">",pos1);
            if (pos1>0 && pos2>0 && pos1<aChar && aChar<pos2) {
                style = syntaxer()->identifierAttribute()->styles();
                foreground = syntaxer()->identifierAttribute()->foreground();
                background = syntaxer()->identifierAttribute()->background();
            }
            return;
        } else if (mParser->enabled() && attr->tokenType() == QSynedit::TokenType::Identifier) {
            //Syntax color for different identifier types
            QSynedit::BufferCoord p{aChar,line};

            StatementKind kind;
            if (mParser->parsing()){
                kind=mIdentCache.value(QString("%1 %2").arg(aChar).arg(token),StatementKind::Unknown);
            } else {
                QStringList expression = getExpressionAtPosition(p);
                PStatement statement = parser()->findStatementOf(
                            filename(),
                            expression,
                            p.line);
                while (statement && statement->kind == StatementKind::Alias)
                    statement = mParser->findAliasedStatement(statement);
                kind = getKindOfStatement(statement);
                mIdentCache.insert(QString("%1 %2").arg(aChar).arg(token),kind);
            }
            if (kind == StatementKind::Unknown) {
                QSynedit::BufferCoord pBeginPos,pEndPos;
                QString s= getWordAtPosition(this,p, pBeginPos,pEndPos, WordPurpose::wpInformation);
                if ((pEndPos.line>=1)
                  && (pEndPos.ch>=0)
                  && (pEndPos.ch+1 < lineText(pEndPos.line).length())
                  && (lineText(pEndPos.line)[pEndPos.ch+1] == '(')) {
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
                QSynedit::BufferCoord p;
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
                   && pSettings->editor().highlightMathingBraces()) {
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
            && pSettings->editor().enableTooltips()
            && !mCompletionPopup->isVisible()
            && !pMainWindow->functionTip()->isVisible()
            && !mHeaderCompletionPopup->isVisible()) {
        cancelHint();
        mTooltipTimer.stop();
        if (pSettings->editor().tipsDelay()>0) {
            mTooltipTimer.setSingleShot(true);
            mTooltipTimer.start(pSettings->editor().tipsDelay());
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

void Editor::mouseReleaseEvent(QMouseEvent *event)
{
    // if ctrl+clicked
    if ((event->modifiers() == Qt::ControlModifier)
            && (event->button() == Qt::LeftButton)) {
        QSynedit::BufferCoord p;
        if (mParser && pointToCharLine(event->pos(),p)) {
            cancelHoverLink();
            QString sLine = lineText(p.line);
            if (mParser->isIncludeNextLine(sLine)) {
                QString filename = mParser->getHeaderFileName(mFilename,sLine, true);
                pMainWindow->openFile(filename);
                return;
            } if (mParser->isIncludeLine(sLine)) {
                QString filename = mParser->getHeaderFileName(mFilename,sLine);
                pMainWindow->openFile(filename);
                return;
            } else if (mParser->enabled()) {
                gotoDefinition(p);
                return;
            }
        }
    }
    QSynedit::QSynEdit::mouseReleaseEvent(event);
}

void Editor::inputMethodEvent(QInputMethodEvent *event)
{
    QSynedit::QSynEdit::inputMethodEvent(event);  
    QString s = event->commitString();
    if (s.isEmpty())
        return;
    if (mCompletionPopup->isVisible()) {
        onCompletionInputMethod(event);
        return;
    } else {
        if (pSettings->codeCompletion().enabled()
                && pSettings->codeCompletion().showCompletionWhileInput() ) {
            QSynedit::BufferCoord ws=wordStart();
            int idCharPressed=caretX()-ws.ch;
            idCharPressed += s.length();
            if (idCharPressed>=pSettings->codeCompletion().minCharRequired()) {
                bool hasTypeQualifier;
                QString lastWord = getPreviousWordAtPositionForSuggestion(caretXY(), hasTypeQualifier);
                if (mParser && !lastWord.isEmpty()) {
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
    if (pMainWindow->functionTip())
        pMainWindow->functionTip()->hide();
    if (inTab()) {
        pMainWindow->updateForEncodingInfo(true);
        pMainWindow->updateStatusbarForLineCol(true);
        pMainWindow->updateForStatusbarModeInfo(true);
    }
}

void Editor::showEvent(QShowEvent */*event*/)
{
//    if (pSettings->codeCompletion().clearWhenEditorHidden()
//            && !inProject()) {
////        initParser();
//    }
    if (mParser && !pMainWindow->isClosingAll()
            && !pMainWindow->isQuitting()
            && !mParser->isFileParsed(mFilename)
            ) {
        connect(mParser.get(),
                &CppParser::onEndParsing,
                this,
                &Editor::onEndParsing);
        if (!pMainWindow->openingFiles() && !pMainWindow->openingProject())
            reparse(false);
    }
    if (inTab()) {
        pMainWindow->debugger()->setIsForProject(inProject());
        pMainWindow->bookmarkModel()->setIsForProject(inProject());
        pMainWindow->todoModel()->setIsForProject(inProject());
    }

    if (!pMainWindow->isClosingAll()
                && !pMainWindow->isQuitting()
            && !pMainWindow->openingFiles()
            && !pMainWindow->openingProject()) {
        if (!inProject() || !pMainWindow->closingProject()) {
            checkSyntaxInBack();
            reparseTodo();
        }
    }
    if (inTab()) {
        pMainWindow->updateClassBrowserForEditor(this);
        pMainWindow->updateAppTitle(this);
        pMainWindow->updateEditorActions(this);
        pMainWindow->updateForEncodingInfo(this);
        pMainWindow->updateStatusbarForLineCol(this);
        pMainWindow->updateForStatusbarModeInfo(this);
    }
    if (inProject() && !pMainWindow->closingProject()) {
        pMainWindow->setProjectCurrentFile(mFilename);
    }

    setHideTime(QDateTime::currentDateTime());
}

void Editor::hideEvent(QHideEvent */*event*/)
{
//    if (pSettings->codeCompletion().clearWhenEditorHidden()
//            && !inProject() && mParser
//            && !pMainWindow->isMinimized()) {
//        //recreate a parser, to totally clean memories the parser uses;
//        if (!pMainWindow->openingFiles() && !pMainWindow->openingProject())
//            resetCppParser(mParser);
//    }
    if (mParser) {
        disconnect(mParser.get(),
                &CppParser::onEndParsing,
                this,
                &Editor::onEndParsing);
    }
    if (!pMainWindow->isQuitting()) {
        pMainWindow->updateForEncodingInfo(nullptr);
        pMainWindow->updateStatusbarForLineCol(nullptr);
        pMainWindow->updateForStatusbarModeInfo(nullptr);
    }
    setHideTime(QDateTime::currentDateTime());
}

void Editor::resizeEvent(QResizeEvent *event)
{
    QSynedit::QSynEdit::resizeEvent(event);
    pMainWindow->functionTip()->setMinWidth(width()*3/4);
    pMainWindow->functionTip()->hide();
}

void Editor::copyToClipboard()
{
    switch(pSettings->editor().copyWithFormatAs()) {
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
    exporter.setUseBackground(pSettings->editor().copyHTMLUseBackground());
    exporter.setFont(font());
    QSynedit::PSyntaxer hl = syntaxer();
    if (!pSettings->editor().copyHTMLUseEditorColor()) {
        hl = syntaxerManager.copy(syntaxer());
        syntaxerManager.applyColorScheme(hl,pSettings->editor().copyHTMLColorScheme());
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

    if (pSettings->editor().copyHTMLWithLineNumber()) {
        exporter.setExportLineNumber(true);
        exporter.setRecalcLineNumber(pSettings->editor().copyHTMLRecalcLineNumber());
        exporter.setLineNumberStartFromZero(pSettings->editor().gutterLineNumbersStartZero());
        exporter.setLineNumberColor(gutter().textColor());
        exporter.setLineNumberBackgroundColor(gutter().color());
    }
    exporter.exportRange(document(),blockBegin(),blockEnd());

    //clipboard takes the owner ship
    QMimeData * mimeData = new QMimeData;

    //sethtml will convert buffer to QString , which will cause encoding trouble
    mimeData->setData(exporter.clipboardFormat(),exporter.buffer());
    mimeData->setText(selText());

    QGuiApplication::clipboard()->clear();
    QGuiApplication::clipboard()->setMimeData(mimeData);
}

void Editor::setCaretPosition(int line, int aChar)
{
    this->uncollapseAroundLine(line);
    this->setCaretXYCentered(QSynedit::BufferCoord{aChar,line});
}

void Editor::setCaretPositionAndActivate(int line, int aChar)
{
    this->uncollapseAroundLine(line);
    if (!this->hasFocus())
        this->activate();
    this->setCaretXYCentered(QSynedit::BufferCoord{aChar,line});
}

void Editor::addSyntaxIssues(int line, int startChar, int endChar, CompileIssueType errorType, const QString &hint)
{
    PSyntaxIssue pError;
    QSynedit::BufferCoord p;
    QString token;
    int start;
    QSynedit::PTokenAttribute attr;
    PSyntaxIssueList lst;
    if ((line<1) || (line>lineCount()))
        return;
    pError = std::make_shared<SyntaxIssue>();
    p.ch = startChar;
    p.line = line;
    if (startChar >= lineText(line).length()) {
        start = 1;
        token = lineText(line);
    } else if (endChar < 1) {
        if (!getTokenAttriAtRowColEx(p,token,start,attr))
            return;
    } else {
        start = startChar;
        token = lineText(line).mid(start-1,endChar-startChar);
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
    QSynedit::BufferCoord p;
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
    QSynedit::BufferCoord p;
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

Editor::PSyntaxIssue Editor::getSyntaxIssueAtPosition(const QSynedit::BufferCoord &pos)
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
    if ((!changes.testFlag(QSynedit::StatusChange::ReadOnly)
            && !changes.testFlag(QSynedit::StatusChange::InsertMode)
            && (lineCount()!=mLineCount)
            && (lineCount()!=0) && ((mLineCount>0) || (lineCount()>1)))
            ||
        (mCurrentLineModified
            && !changes.testFlag(QSynedit::StatusChange::ReadOnly)
            && changes.testFlag(QSynedit::StatusChange::CaretY))) {
        mCurrentLineModified = false;
        if (!changes.testFlag(QSynedit::StatusChange::OpenFile)) {
            reparse(false);
            if (pSettings->editor().syntaxCheckWhenLineChanged())
                checkSyntaxInBack();
            reparseTodo();
        }
//        if (pSettings->codeCompletion().clearWhenEditorHidden()
//                && changes.testFlag(SynStatusChange::scOpenFile)) {
//        } else{
//            reparse();
//        }
    }
    mLineCount = lineCount();
    if (changes.testFlag(QSynedit::StatusChange::ModifyChanged)) {
        updateCaption();
    }
    if (changes.testFlag(QSynedit::StatusChange::Modified)) {
        mCurrentLineModified = true;
        if (inTab())
            mCanAutoSave = true;
    }

    if (changes.testFlag(QSynedit::StatusChange::CaretX)
            || changes.testFlag(QSynedit::StatusChange::CaretY)) {
        if (pSettings->editor().highlightMathingBraces()) {
            invalidateLine(mHighlightCharPos1.line);
            invalidateLine(mHighlightCharPos2.line);
        }
        mHighlightCharPos1 = QSynedit::BufferCoord{0,0};
        mHighlightCharPos2 = QSynedit::BufferCoord{0,0};
        if (mTabStopBegin >=0) {
            if (mTabStopY==caretY()) {
                if (mLineAfterTabStop.isEmpty()) {
                    if (lineText().startsWith(mLineBeforeTabStop))
                        mTabStopBegin = mLineBeforeTabStop.length()+1;
                    mTabStopEnd = lineText().length()+1;
                } else {
                    if (lineText().startsWith(mLineBeforeTabStop)
                        && lineText().endsWith(mLineAfterTabStop))
                        mTabStopBegin = mLineBeforeTabStop.length()+1;
                    mTabStopEnd = lineText().length()
                            - mLineAfterTabStop.length()+1;
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
        } else if (!selAvail() && pSettings->editor().highlightMathingBraces()){
            // Is there a bracket char before us?
            int lineLength = lineText().length();
            int ch = caretX() - 2;
            QSynedit::BufferCoord coord{0,0};
            if (ch>=0 && ch<lineLength &&  isBraceChar(lineText()[ch]) ) {
                coord.ch = ch+1;
                coord.line = caretY();
            }
            //or after us?
            ch = caretX()-1;
            if (ch>=0 && ch<lineLength &&  isBraceChar(lineText()[ch]) ) {
                coord.ch = ch+1;
                coord.line = caretY();
            }
            QSynedit::PTokenAttribute attr;
            QString token;
            if (getTokenAttriAtRowCol(coord,token,attr)
                    && attr->tokenType() == QSynedit::TokenType::Operator) {
                QSynedit::BufferCoord complementCharPos = getMatchingBracketEx(coord);
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

    // scSelection includes anything caret related
    if (changes.testFlag(QSynedit::StatusChange::Selection)) {
        if (!selAvail() && pSettings->editor().highlightCurrentWord()) {
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
        } else if (selAvail() && blockBegin() == wordStart()
                   && blockEnd() == wordEnd()){
            mCurrentHighlightedWord = selText();
        } else {
            mCurrentHighlightedWord = "";
        }

        if (mOldHighlightedWord != mCurrentHighlightedWord) {
            invalidate();
            mOldHighlightedWord = mCurrentHighlightedWord;
        }
        pMainWindow->updateStatusbarForLineCol(this);

        // Update the function tip
        if (pSettings->editor().showFunctionTips()) {
            updateFunctionTip(false);
            mFunctionTipTimer.stop();
            if (pSettings->editor().tipsDelay()>0)
                mFunctionTipTimer.start(500);
            else
                onFunctionTipsTimer();
        }
    }

    if (changes.testFlag(QSynedit::StatusChange::InsertMode) || changes.testFlag(QSynedit::StatusChange::ReadOnly))
        pMainWindow->updateForStatusbarModeInfo();

    pMainWindow->updateEditorActions();

    if (changes.testFlag(QSynedit::StatusChange::CaretY) && inTab()) {
        pMainWindow->caretList().addCaret(this,caretY(),caretX());
        pMainWindow->updateCaretActions();
    }

    if (changes.testFlag(QSynedit::StatusChange::ReadOnly)) {
        if (!readOnly())
            initAutoBackup();
    }
}

void Editor::onGutterClicked(Qt::MouseButton button, int , int , int line)
{
    if (button == Qt::LeftButton) {
        FileType fileType=getFileType(mFilename);
        if (fileType==FileType::CSource
                || fileType==FileType::CHeader
                || fileType==FileType::CppSource
                || fileType==FileType::CppHeader
                || fileType==FileType::GAS
                )
            toggleBreakpoint(line);
    }
    mGutterClickedLine = line;
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
    disconnect(pMainWindow->debugger(), &Debugger::evalValueReady,
               this, &Editor::onTipEvalValueReady);
}

void Editor::onLinesDeleted(int first, int count)
{
    pMainWindow->caretList().linesDeleted(this,first,count);
    pMainWindow->debugger()->breakpointModel()->onFileDeleteLines(mFilename,first,count,inProject());
    pMainWindow->bookmarkModel()->onFileDeleteLines(mFilename,first,count, inProject());
    resetBreakpoints();
    resetBookmarks();
    if (!pSettings->editor().syntaxCheckWhenLineChanged()) {
        //todo: update syntax issues
    }
}

void Editor::onLinesInserted(int first, int count)
{
    pMainWindow->caretList().linesInserted(this,first,count);
    pMainWindow->debugger()->breakpointModel()->onFileInsertLines(mFilename,first,count, inProject());
    pMainWindow->bookmarkModel()->onFileInsertLines(mFilename,first,count, inProject());
    resetBreakpoints();
    resetBookmarks();
    if (!pSettings->editor().syntaxCheckWhenLineChanged()) {
        //todo: update syntax issues
    }
}

void Editor::onFunctionTipsTimer()
{
    mFunctionTipTimer.stop();
    updateFunctionTip(true);
}

void Editor::onAutoBackupTimer()
{
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

    QSynedit::BufferCoord p;
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
    bool isIncludeLine = false;
    bool isIncludeNextLine = false;
    QSynedit::BufferCoord pBeginPos,pEndPos;
    QString s;
    QStringList expression;
    switch (reason) {
    case TipType::Preprocessor:
        // When hovering above a preprocessor line, determine if we want to show an include or a identifier hint
        if (mParser) {
            s = lineText(p.line);
            isIncludeNextLine = mParser->isIncludeNextLine(s);
            if (!isIncludeNextLine)
                isIncludeLine = mParser->isIncludeLine(s);
            if (!isIncludeNextLine &&!isIncludeLine)
                s = wordAtRowCol(p);
        }
        break;
    case TipType::Identifier:
        if (pMainWindow->debugger()->executing() && !pMainWindow->debugger()->inferiorRunning()) {
            s = getWordAtPosition(this,p, pBeginPos,pEndPos, WordPurpose::wpEvaluation); // debugging
        } else if (!mCompletionPopup->isVisible()
                 && !mHeaderCompletionPopup->isVisible()) {
            expression = getExpressionAtPosition(p);
            s = expression.join(""); // information during coding
        }
        break;
    case TipType::Keyword:
        if (QSynedit::isAssemblyLanguage(syntaxer()->language())) {
            if (!mCompletionPopup->isVisible()
                 && !mHeaderCompletionPopup->isVisible()) {
                s = wordAtRowCol(p);
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
        if (!mCompletionPopup->isVisible()
             && !mHeaderCompletionPopup->isVisible()) {
            QSynedit::PTokenAttribute attr;
            int start;
            if (getTokenAttriAtRowColEx(p,s,start,attr)) {
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
    case TipType::Preprocessor:
        if (isIncludeNextLine || isIncludeLine) {
            if (pSettings->editor().enableHeaderToolTips())
                hint = getFileHint(s, isIncludeNextLine);
        } else if (//devEditor.ParserHints and
                 !mCompletionPopup->isVisible()
                 && !mHeaderCompletionPopup->isVisible()) {
            if (pSettings->editor().enableIdentifierToolTips())
                hint = getParserHint(QStringList(),s,p.line);
        }
        break;
    case TipType::Identifier:
    case TipType::Selection:
        if (!mCompletionPopup->isVisible()
                && !mHeaderCompletionPopup->isVisible()) {
            if (pMainWindow->debugger()->executing()
                    && (pSettings->editor().enableDebugTooltips())) {
                if (inTab()) {
                    showDebugHint(s,p.line);
                }
            } else if (pSettings->editor().enableIdentifierToolTips()) {
                hint = getParserHint(expression, s, p.line);
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
        if (pSettings->editor().enableIdentifierToolTips()) {
            if (QSynedit::isAssemblyLanguage(syntaxer()->language())) {
                hint = QSynedit::ASMSyntaxer::Instructions.value(s.toLower(),"");
            }
        }
        break;
    case TipType::Error:
        if (pSettings->editor().enableIssueToolTips())
            hint = getErrorHint(pError);
        break;
    default:
        break;
    }
//        qDebug()<<"hint:"<<hint;
    if (!hint.isEmpty()) {
        //            QApplication* app = dynamic_cast<QApplication *>(QApplication::instance());
        //            if (app->keyboardModifiers().testFlag(Qt::ControlModifier)) {
        if (pMainWindow->functionTip()->isVisible()) {
            pMainWindow->functionTip()->hide();
        }
        QToolTip::showText(mapToGlobal(pos),hint,this);
    }
}

void Editor::onEndParsing()
{
    mIdentCache.clear();
    document()->invalidateAllNonTempLineWidth();
    invalidate();
}

void Editor::resolveAutoDetectEncodingOption()
{
    if (mEncodingOption==ENCODING_AUTO_DETECT) {
        if (mFileEncoding==ENCODING_ASCII)
            mEncodingOption=pSettings->editor().defaultEncoding();
        else
            mEncodingOption=mFileEncoding;
    }
}

bool Editor::isBraceChar(QChar ch)
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
    if (mProject && mProject->findUnit(mFilename))
        return false;
    return pSettings->editor().readOnlySytemHeader()
                && mParser && (mParser->isSystemHeaderFile(mFilename) || mParser->isProjectHeaderFile(mFilename));
}

void Editor::resetBookmarks()
{
    mBookmarkLines=pMainWindow->bookmarkModel()->bookmarksInFile(mFilename,inProject());
    invalidate();
}

void Editor::resetBreakpoints()
{
    mBreakpointLines.clear();
    foreach (const PBreakpoint& breakpoint,
             pMainWindow->debugger()->breakpointModel()->breakpoints(inProject())) {
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

void Editor::insertLine()
{
    processCommand(QSynedit::EditCommand::InsertLine,QChar(),nullptr);
}

void Editor::breakLine()
{
    processCommand(QSynedit::EditCommand::LineBreak,QChar(),nullptr);
}

void Editor::deleteWord()
{
    processCommand(QSynedit::EditCommand::DeleteWord,QChar(),nullptr);
}

void Editor::deleteToWordStart()
{
    processCommand(QSynedit::EditCommand::DeleteWordStart,QChar(),nullptr);
}

void Editor::deleteToWordEnd()
{
    processCommand(QSynedit::EditCommand::DeleteWordEnd,QChar(),nullptr);
}

void Editor::deleteLine()
{
    processCommand(QSynedit::EditCommand::DeleteLine,QChar(),nullptr);
}

void Editor::duplicateLine()
{
    processCommand(QSynedit::EditCommand::DuplicateLine,QChar(),nullptr);
}

void Editor::deleteToEOL()
{
    processCommand(QSynedit::EditCommand::DeleteEOL,QChar(),nullptr);
}

void Editor::deleteToBOL()
{
    processCommand(QSynedit::EditCommand::DeleteBOL,QChar(),nullptr);
}

void Editor::gotoBlockStart()
{
    processCommand(QSynedit::EditCommand::BlockStart,QChar(),nullptr);
}

void Editor::gotoBlockEnd()
{
    processCommand(QSynedit::EditCommand::BlockEnd,QChar(),nullptr);
}

void Editor::showCodeCompletion()
{
    if (!pSettings->codeCompletion().enabled())
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
        const QSynedit::BufferCoord &pos,
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
        const QSynedit::BufferCoord &pos)
{
    QStringList result;
    if (!parser())
        return result;
    int line = pos.line-1;
    int ch = pos.ch-1;
    int symbolMatchingLevel = 0;
    LastSymbolType lastSymbolType=LastSymbolType::None;
    QSynedit::CppSyntaxer syntaxer;
    while (true) {
        if (line>=lineCount() || line<0)
            break;
        QStringList tokens;
        if (line==0) {
            syntaxer.resetState();
        } else {
            syntaxer.setState(document()->getSyntaxState(line-1));
        }
        QString sLine = document()->getLine(line);
        syntaxer.setLine(sLine,line-1);
        while (!syntaxer.eol()) {
            int start = syntaxer.getTokenPos();
            QString token = syntaxer.getToken();
            int endPos = start + token.length()-1;
            if (start>ch) {
                break;
            }
            QSynedit::PTokenAttribute attr = syntaxer.getTokenAttribute();
            if ( (line == pos.line-1)
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

QString Editor::getWordForCompletionSearch(const QSynedit::BufferCoord &pos,bool permitTilde)
{
    QString result = "";
    QString s;

    s = lineText(pos.line);
    int len = s.length();

    int wordBegin = pos.ch - 1 - 1; //BufferCoord::Char starts with 1
    int wordEnd = pos.ch - 1 - 1;

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
    if (lineText().length()<caretX())
        return QChar();
    else
        return lineText().at(caretX()-1);
}

bool Editor::handleSymbolCompletion(QChar key)
{
    if (!pSettings->editor().completeSymbols())
        return false;
    if (!insertMode())
        return false;

    //todo: better methods to detect current caret type
    if (caretX() <= 1) {
        if (caretY()>1) {
            if (syntaxer()->isCommentNotFinished(document()->getSyntaxState(caretY() - 2).state))
                return false;
            if (syntaxer()->isStringNotFinished(document()->getSyntaxState(caretY() - 2).state)
                    && (key!='\"') && (key!='\''))
                return false;
        }
    } else {
        QSynedit::BufferCoord  HighlightPos = QSynedit::BufferCoord{caretX()-1, caretY()};
        // Check if that line is highlighted as  comment
        QSynedit::PTokenAttribute attr;
        QString token;
        QSynedit::SyntaxState syntaxState;
        if (getTokenAttriAtRowCol(HighlightPos, token, attr, syntaxState)) {
            if (syntaxer()->isCommentNotFinished(syntaxState.state))
                return false;
            if (syntaxer()->isStringNotFinished(syntaxState.state)
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
        if (pSettings->editor().completeParenthese()) {
            return handleParentheseCompletion();
        }
        return false;
    case ')':
        if (selAvail())
            return false;
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
        if (selAvail())
            return false;
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
        if (selAvail())
            return false;
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
        if (selAvail())
            return false;
        if (pSettings->editor().completeGlobalInclude()) { // #include <>
            return handleGlobalIncludeCompletion();
        }
        return false;
    case '>':
        if (selAvail())
            return false;
        if (pSettings->editor().completeGlobalInclude() && pSettings->editor().overwriteSymbols()) { // #include <>
            return handleGlobalIncludeSkip();
        }
        return false;
    case ';':
        if (selAvail())
            return false;
        if (pSettings->editor().overwriteSymbols()) {
            return handleSemiColonSkip();
        }
        return false;
    case ',':
        if (selAvail())
            return false;
        if (pSettings->editor().overwriteSymbols()) {
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
            processCommand(QSynedit::EditCommand::Char,'(');
            setSelText(text);
            processCommand(QSynedit::EditCommand::Char,')');
            endEditing();
        } else {
            beginEditing();
            processCommand(QSynedit::EditCommand::Char,'(');
            QSynedit::BufferCoord oldCaret = caretXY();
            processCommand(QSynedit::EditCommand::Char,')');
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
      if (status == QuoteStatus::RawStringNoEscape) {
          setCaretXY( QSynedit::BufferCoord{caretX() + 1, caretY()}); // skip over
          return true;
      }
      if (status != QuoteStatus::NotQuote)
          return false;

      if (lineCount()==0)
          return false;
      if (syntaxer()->supportBraceLevel()) {
          QSynedit::SyntaxState lastLineState = document()->getSyntaxState(lineCount()-1);
          if (lastLineState.parenthesisLevel==0) {
              setCaretXY( QSynedit::BufferCoord{caretX() + 1, caretY()}); // skip over
              return true;
          }
      } else {
          QSynedit::BufferCoord pos = getMatchingBracket();
          if (pos.line != 0) {
              setCaretXY( QSynedit::BufferCoord{caretX() + 1, caretY()}); // skip over
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
        processCommand(QSynedit::EditCommand::Char,'[');
        setSelText(text);
        processCommand(QSynedit::EditCommand::Char,']');
        endEditing();
    } else {
        beginEditing();
        processCommand(QSynedit::EditCommand::Char,'[');
        QSynedit::BufferCoord oldCaret = caretXY();
        processCommand(QSynedit::EditCommand::Char,']');
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
        QSynedit::SyntaxState lastLineState = document()->getSyntaxState(lineCount()-1);
        if (lastLineState.bracketLevel==0) {
            setCaretXY( QSynedit::BufferCoord{caretX() + 1, caretY()}); // skip over
            return true;
        }
    } else {
        QSynedit::BufferCoord pos = getMatchingBracket();
        if (pos.line != 0) {
            setCaretXY( QSynedit::BufferCoord{caretX() + 1, caretY()}); // skip over
            return true;
        }
    }
    return false;
}

bool Editor::handleMultilineCommentCompletion()
{
    if ((caretX()-2>=0) && (caretX()-2 < lineText().length()) && (lineText()[caretX() - 2] == '/')) {
        QString text=selText();
        beginEditing();
        processCommand(QSynedit::EditCommand::Char,'*');
        QSynedit::BufferCoord oldCaret;
        if (text.isEmpty())
            oldCaret = caretXY();
        else
            setSelText(text);
        processCommand(QSynedit::EditCommand::Char,'*');
        processCommand(QSynedit::EditCommand::Char,'/');
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
    int i= caretY()-2;
    while ((sLine.isEmpty()) && (i>=0)) {
        sLine=document()->getLine(i).trimmed();
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
//        processCommand(QSynedit::EditCommand::Char,';');
    }

    beginEditing();
    if (!selAvail()) {
        processCommand(QSynedit::EditCommand::Char,'{');
        QSynedit::BufferCoord oldCaret = caretXY();
        processCommand(QSynedit::EditCommand::Char,'}');
        if (addSemicolon)
            processCommand(QSynedit::EditCommand::Char,';');
        setCaretXY(oldCaret);
    }  else {
        QString text = selText();
        QSynedit::BufferCoord oldSelBegin = blockBegin();
        QSynedit::BufferCoord oldSelEnd = blockEnd();
        bool shouldBreakLine = false;
        bool shouldAddEndLine = false;
        QString s1=lineText(oldSelBegin.line).left(oldSelBegin.ch-1).trimmed();

        if (s1.isEmpty() ) {
            QString s2 = lineText(oldSelEnd.line);
            if (s2.left(oldSelEnd.ch-1).trimmed().isEmpty()) {
                shouldBreakLine = true;
            } else if (oldSelEnd.ch > trimRight(s2).length()) {
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
            text = "{"+text;
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

    if (syntaxer()->supportBraceLevel()) {
        QSynedit::SyntaxState lastLineState = document()->getSyntaxState(lineCount()-1);
        if (lastLineState.braceLevel==0) {
            bool oldInsertMode = insertMode();
            setInsertMode(false); //set mode to overwrite
            processCommand(QSynedit::EditCommand::Char,'}');
            setInsertMode(oldInsertMode);
            return true;
        }
    } else {
        QSynedit::BufferCoord pos = getMatchingBracket();
        if (pos.line != 0) {
            bool oldInsertMode = insertMode();
            setInsertMode(false); //set mode to overwrite
            processCommand(QSynedit::EditCommand::Char,'}');
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
    processCommand(QSynedit::EditCommand::Char,';');
    setInsertMode(oldInsertMode);
    return true;
}

bool Editor::handlePeriodSkip()
{
    if (getCurrentChar() != ',')
        return false;

    bool oldInsertMode = insertMode();
    setInsertMode(false); //set mode to overwrite
    processCommand(QSynedit::EditCommand::Char,',');
    setInsertMode(oldInsertMode);
    return true;
}

bool Editor::handleSingleQuoteCompletion()
{
    QuoteStatus status = getQuoteStatus();
    QChar ch = getCurrentChar();
    if (ch == '\'') {
        if (status == QuoteStatus::SingleQuote && !selAvail()) {
            setCaretXY( QSynedit::BufferCoord{caretX() + 1, caretY()}); // skip over
            return true;
        }
    } else {
        if (status == QuoteStatus::NotQuote) {
            if (selAvail()) {
                QString text=selText();
                beginEditing();
                processCommand(QSynedit::EditCommand::Char,'\'');
                setSelText(text);
                processCommand(QSynedit::EditCommand::Char,'\'');
                endEditing();
                return true;
            }
            if (ch == 0 || syntaxer()->isWordBreakChar(ch) || syntaxer()->isSpaceChar(ch)) {
                // insert ''
                beginEditing();
                processCommand(QSynedit::EditCommand::Char,'\'');
                QSynedit::BufferCoord oldCaret = caretXY();
                processCommand(QSynedit::EditCommand::Char,'\'');
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
    if (ch == '"') {
        if ((status == QuoteStatus::DoubleQuote || status == QuoteStatus::RawStringEnd)
            && !selAvail()) {
            setCaretXY( QSynedit::BufferCoord{caretX() + 1, caretY()}); // skip over
            return true;
        }
    } else {
        if (status == QuoteStatus::NotQuote) {
            if (selAvail()) {
                QString text=selText();
                beginEditing();
                processCommand(QSynedit::EditCommand::Char,'"');
                setSelText(text);
                processCommand(QSynedit::EditCommand::Char,'"');
                endEditing();
                return true;
            }
            if ((ch == 0)
                    || ( syntaxer()->isWordBreakChar(ch)
                             || syntaxer()->isSpaceChar(ch))) {
                // insert ""
                beginEditing();
                processCommand(QSynedit::EditCommand::Char,'"');
                QSynedit::BufferCoord oldCaret = caretXY();
                processCommand(QSynedit::EditCommand::Char,'"');
                setCaretXY(oldCaret);
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
    processCommand(QSynedit::EditCommand::Char,'<');
    QSynedit::BufferCoord oldCaret = caretXY();
    processCommand(QSynedit::EditCommand::Char,'>');
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
    QSynedit::BufferCoord pos = getMatchingBracket();
    if (pos.line != 0) {
        setCaretXY(QSynedit::BufferCoord{caretX() + 1, caretY()}); // skip over
        return true;
    }
    return false;
}

bool Editor::handleCodeCompletion(QChar key)
{
    if (!mCompletionPopup->isEnabled())
        return false;
    if (mParser) {
        switch(key.unicode()) {
        case '.':
            processCommand(QSynedit::EditCommand::Char, key);
            showCompletion("",false,CodeCompletionType::Normal);
            return true;
        case '>':
            processCommand(QSynedit::EditCommand::Char, key);
            if ((caretX() > 2) && (lineText().length() >= 2) &&
                    (lineText()[caretX() - 3] == '-'))
                showCompletion("",false,CodeCompletionType::Normal);
            return true;
        case ':':
            processCommand(QSynedit::EditCommand::Char,':',nullptr);
            //setSelText(key);
            if ((caretX() > 2) && (lineText().length() >= 2) &&
                    (lineText()[caretX() - 3] == ':'))
                showCompletion("",false,CodeCompletionType::Normal);
            return true;
        case '/':
        case '\\':
            processCommand(QSynedit::EditCommand::Char, key);
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

void Editor::initParser()
{
    if (pSettings->codeCompletion().enabled()
        && (isCFile(mFilename) || isHFile(mFilename))) {
        if (pSettings->codeCompletion().shareParser()) {
            mParser = sharedParser(calcParserLanguage());
        } else {
            mParser = std::make_shared<CppParser>();
            mParser->setLanguage(calcParserLanguage());
            mParser->setOnGetFileStream(
                        std::bind(
                            &EditorList::getContentFromOpenedEditor,pMainWindow->editorList(),
                            std::placeholders::_1, std::placeholders::_2));
            resetCppParser(mParser);
            mParser->setEnabled(
                        pSettings->codeCompletion().enabled() &&
                        (syntaxer()->language() == QSynedit::ProgrammingLanguage::CPP));
        }
    } else {
        mParser = nullptr;
    }
}

ParserLanguage Editor::calcParserLanguage()
{
#ifdef ENABLE_SDCC
    Settings::PCompilerSet pSet;
    if (inProject()) {
        pSet = pSettings->compilerSets().getSet(mProject->options().compilerSet);
    } else if (!inProject()) {
        pSet = pSettings->compilerSets().defaultSet();
    }
    if (pSet && pSet->compilerType()==CompilerType::SDCC)
        return ParserLanguage::SDCC;
#endif
    return mUseCppSyntax?ParserLanguage::CPlusPlus:ParserLanguage::C;
}

Editor::QuoteStatus Editor::getQuoteStatus()
{
    QuoteStatus Result = QuoteStatus::NotQuote;
    if (syntaxer()->language()==QSynedit::ProgrammingLanguage::CPP) {
        QString s = lineText().mid(0,caretX()-1);
        QSynedit::SyntaxState state = calcSyntaxStateAtLine(caretY()-1, s);
        std::shared_ptr<QSynedit::CppSyntaxer> cppSyntaxer = std::dynamic_pointer_cast<QSynedit::CppSyntaxer>(syntaxer());
        if (syntaxer()->isStringNotFinished(state.state)) {
            if (cppSyntaxer->isStringToNextLine(state.state))
                return QuoteStatus::DoubleQuoteEscape;
            else
                return QuoteStatus::DoubleQuote;
        }
        if (cppSyntaxer->isCharNotFinished(state.state)) {
            if (cppSyntaxer->isCharEscaping(state.state))
                return QuoteStatus::SingleQuoteEscape;
            else
                return QuoteStatus::SingleQuote;
        }
        if (cppSyntaxer->isRawStringNoEscape(state.state))
            return QuoteStatus::RawStringNoEscape;
        if (cppSyntaxer->isRawStringStart(state.state))
            return QuoteStatus::RawString;
        if (cppSyntaxer->isRawStringEnd(state.state))
            return QuoteStatus::RawStringEnd;
        return QuoteStatus::NotQuote;
    } else {
        return QuoteStatus::NotQuote;
    }
    return Result;
}

void Editor::reparse(bool resetParser)
{
    if (!mInited)
        return;
    if (!inTab())
        return;
    if (!pSettings->codeCompletion().enabled())
        return;
    if (syntaxer()->language() != QSynedit::ProgrammingLanguage::CPP
             && syntaxer()->language() != QSynedit::ProgrammingLanguage::GLSL)
        return;
    if (!mParser)
        return;
    if (!mParser->enabled())
        return;
    //qDebug()<<"reparse "<<mFilename;
    //mParser->setEnabled(pSettings->codeCompletion().enabled());
    if (!inProject()) {
        ParserLanguage language = calcParserLanguage();
        if (pSettings->codeCompletion().shareParser()) {
            if (language!=mParser->language()) {
                mParser->invalidateFile(mFilename);
                mParser=sharedParser(language);
            }
        } else {
            if (language!=mParser->language()) {
                mParser->setLanguage(language);
                resetCppParser(mParser);
            } else if (resetParser) {
                resetCppParser(mParser);
            }
        }
    }
    parseFile(mParser,mFilename, inProject());
}

void Editor::reparseTodo()
{
    if (!mInited)
        return;
    if (!inTab())
        return;
    if (pSettings->editor().parseTodos())
        pMainWindow->todoParser()->parseFile(mFilename, inProject());
}

void Editor::insertString(const QString &value, bool moveCursor)
{
    beginEditing();
    auto action = finally([this]{
        endEditing();
    });

    QSynedit::BufferCoord oldCursorPos = caretXY();
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
    QStringList sl = textToLines(parseMacros(code));
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

    QSynedit::BufferCoord cursorPos = caretXY();
    QString s = linesToText(newSl);
//        if EndsStr(#13#10,s) then
//          Delete(s,Length(s)-1,2)
//        else if EndsStr(#10, s) then
//          Delete(s,Length(s),1);
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
    exporter.setUseBackground(pSettings->editor().copyHTMLUseBackground());

    exporter.setFont(font());
    QSynedit::PSyntaxer hl = syntaxer();
    if (!pSettings->editor().copyHTMLUseEditorColor()) {
        hl = syntaxerManager.copy(syntaxer());
        syntaxerManager.applyColorScheme(hl,pSettings->editor().copyHTMLColorScheme());
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
        exporter.exportRange(document(),blockBegin(),blockEnd());
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
    exporter.setUseBackground(pSettings->editor().copyRTFUseBackground());
    exporter.setFont(font());
    QSynedit::PSyntaxer hl = syntaxer();
    if (!pSettings->editor().copyRTFUseEditorColor()) {
        hl = syntaxerManager.copy(syntaxer());
        syntaxerManager.applyColorScheme(hl,pSettings->editor().copyRTFColorScheme());
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
    exporter.setUseBackground(pSettings->editor().copyHTMLUseBackground());
    exporter.setFont(font());
    QSynedit::PSyntaxer hl = syntaxer();
    if (!pSettings->editor().copyHTMLUseEditorColor()) {
        hl = syntaxerManager.copy(syntaxer());
        syntaxerManager.applyColorScheme(hl,pSettings->editor().copyHTMLColorScheme());
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
    if (pSettings->editor().copyHTMLWithLineNumber()) {
        exporter.setExportLineNumber(true);
        exporter.setRecalcLineNumber(false);
        exporter.setLineNumberStartFromZero(pSettings->editor().gutterLineNumbersStartZero());
        exporter.setLineNumberColor(gutter().textColor());
        exporter.setLineNumberBackgroundColor(gutter().color());
    }
    exporter.exportAll(document());
    exporter.saveToFile(htmlFilename);
}

void Editor::showCompletion(const QString& preWord,bool autoComplete, CodeCompletionType type)
{
    if (pMainWindow->functionTip()->isVisible()) {
        pMainWindow->functionTip()->hide();
    }
    if (!pSettings->codeCompletion().enabled())
        return;
    if (type==CodeCompletionType::KeywordsOnly) {
    } else {
        if (!mParser || !mParser->enabled())
            return;
    }

    if (mCompletionPopup->isVisible()) // already in search, don't do it again
        return;

    QString word="";

    QString s;
    QSynedit::PTokenAttribute attr;
    QSynedit::BufferCoord pBeginPos, pEndPos;
    if (getTokenAttriAtRowCol(
                QSynedit::BufferCoord{caretX() - 1,
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
            if (syntaxer()->language()==QSynedit::ProgrammingLanguage::ATTAssembly)
                word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpATTASMKeywords);
            else
                word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpKeywords);
        } else if (
                   (attr->tokenType() != QSynedit::TokenType::Operator) &&
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
        if (syntaxer()->language()==QSynedit::ProgrammingLanguage::ATTAssembly) {
            if (word.startsWith("."))
                keywords = QSynedit::ASMSyntaxer::ATTDirectives;
            else if (word.startsWith("%"))
                keywords = QSynedit::ASMSyntaxer::ATTRegisters;
            else {
                keywords = QSynedit::ASMSyntaxer::InstructionNames;
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
        }
        if (pSettings->editor().enableCustomCTypeKeywords()) {
            foreach (const QString& keyword, pSettings->editor().customCTypeKeywords()) {
                keywords.insert(keyword);
            }
        }
    }

    if (type == CodeCompletionType::KeywordsOnly && keywords.isEmpty())
        return;

    mCompletionPopup->setRecordUsage(pSettings->codeCompletion().recordUsage());
    mCompletionPopup->setSortByScope(pSettings->codeCompletion().sortByScope());
    mCompletionPopup->setShowKeywords(pSettings->codeCompletion().showKeywords());
    if (type!=CodeCompletionType::Normal) {
        mCompletionPopup->setShowCodeSnippets(false);
    } else {
        mCompletionPopup->setShowCodeSnippets(pSettings->codeCompletion().showCodeIns());
        if (pSettings->codeCompletion().showCodeIns()) {
            mCompletionPopup->setCodeSnippets(pMainWindow->codeSnippetManager()->snippets());
        }
    }
    mCompletionPopup->setHideSymbolsStartWithUnderline(pSettings->codeCompletion().hideSymbolsStartsWithUnderLine());
    mCompletionPopup->setHideSymbolsStartWithTwoUnderline(pSettings->codeCompletion().hideSymbolsStartsWithTwoUnderLine());
    mCompletionPopup->setIgnoreCase(pSettings->codeCompletion().ignoreCase());
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
    mCompletionPopup->setLineHeightFactor(pSettings->editor().lineSpacing());
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
    pMainWindow->functionTip()->hide();
    mCompletionPopup->show();

    if (word.isEmpty()) {
        //word=getWordAtPosition(this,caretXY(),pBeginPos,pEndPos, WordPurpose::wpCompletion);
        QString memberOperator;
        QStringList memberExpression;
        QSynedit::BufferCoord pos = caretXY();
        pos.ch--;
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
    if (mCompletionPopup->search(word, autoComplete)) { //only one suggestion and it's not input while typing
        completionInsert(pSettings->codeCompletion().appendFunc());
    }
}

void Editor::showHeaderCompletion(bool autoComplete, bool forceShow)
{
    if (!pSettings->codeCompletion().enabled())
        return;
//    if not devCodeCompletion.Enabled then
//      Exit;

    if (!forceShow && mHeaderCompletionPopup->isVisible()) // already in search, don't do it again
        return;

    QSynedit::BufferCoord  HighlightPos = QSynedit::BufferCoord{caretX()-1, caretY()};
    // Check if that line is highlighted as  comment
    QSynedit::PTokenAttribute attr;
    QString token;
    QSynedit::SyntaxState syntaxState;
    if (getTokenAttriAtRowCol(HighlightPos, token, attr, syntaxState)) {
        if (attr && attr->tokenType()==QSynedit::TokenType::Comment)
            return;
    }

    // Position it at the top of the next line
    QPoint p = displayCoordToPixels(displayXY());
    p.setY(p.y() + textHeight() + 2);
    mHeaderCompletionPopup->move(mapToGlobal(p));

    mHeaderCompletionPopup->setIgnoreCase(pSettings->codeCompletion().ignoreCase());

    QSize popSize = calcCompletionPopupSize();
    mHeaderCompletionPopup->resize(popSize);
    //Set Font size;
    mHeaderCompletionPopup->setFont(font());
    mHeaderCompletionPopup->setLineHeightFactor(pSettings->editor().lineSpacing());

    // Redirect key presses to completion box if applicable
    mHeaderCompletionPopup->setKeypressedCallback([this](QKeyEvent* event)->bool{
        return onHeaderCompletionKeyPressed(event);
    });
    mHeaderCompletionPopup->setParser(mParser);

    QSynedit::BufferCoord pBeginPos,pEndPos;
    QString word = getWordAtPosition(this,caretXY(),pBeginPos,pEndPos,
                                     WordPurpose::wpHeaderCompletionStart);

    if (word.isEmpty())
        return;

    if (!word.startsWith('"') && !word.startsWith('<'))
        return;

    if (word.lastIndexOf('"')>0 || word.lastIndexOf('>')>0)
        return;

    pMainWindow->functionTip()->hide();
    mHeaderCompletionPopup->show();
    mHeaderCompletionPopup->setSearchLocal(word.startsWith('"'));
    word.remove(0,1);

    mHeaderCompletionPopup->prepareSearch(word, mFilename);

    // Filter the whole statement list
    if (mHeaderCompletionPopup->search(word, autoComplete)) //only one suggestion and it's not input while typing
        headerCompletionInsert(); // if only have one suggestion, just use it
}

void Editor::initAutoBackup()
{
    if (!inTab())
        return;
    cleanAutoBackup();
    if (!pSettings->editor().enableEditTempBackup())
        return;
    if (readOnly())
        return;
    QFileInfo fileInfo(mFilename);
    if (fileInfo.isAbsolute()) {
        mBackupFile=new QFile(extractFileDir(mFilename)
                              +QDir::separator()
                              +extractFileName(mFilename)+QString(".%1.editbackup").arg(QDateTime::currentSecsSinceEpoch()));
        if (mBackupFile->open(QFile::Truncate|QFile::WriteOnly)) {
            saveAutoBackup();
        } else {
            cleanAutoBackup();
        }
    } else {
        mBackupFile=new QFile(
                    includeTrailingPathDelimiter(QDir::currentPath())
                    +mFilename
                    +QString(".%1.editbackup").arg(QDateTime::currentSecsSinceEpoch()));
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

bool Editor::testInFunc(const QSynedit::BufferCoord& pos)
{
    int y=pos.line-1;
    int x=pos.ch;
    if (syntaxer()->language()!=QSynedit::ProgrammingLanguage::CPP)
        return false;
    if (y==0)
        syntaxer()->resetState();
    else
        syntaxer()->setState(document()->getSyntaxState(y-1));
    syntaxer()->setLine(document()->getLine(y),y);
    QSynedit::SyntaxState state = syntaxer()->getState();
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
    return state.parenthesisLevel>0;


}

void Editor::completionInsert(bool appendFunc)
{
    PStatement statement = mCompletionPopup->selectedStatement();
    if (!statement)
        return;

    if (pSettings->codeCompletion().recordUsage()
            && statement->kind != StatementKind::UserCodeSnippet) {
        statement->usageCount+=1;
        pMainWindow->symbolUsageManager()->updateUsage(statement->fullName,
                                                         statement->usageCount);
    }

    QString funcAddOn = "";

// delete the part of the word that's already been typed ...
    QSynedit::BufferCoord p = wordEnd();
    QSynedit::BufferCoord pStart = wordStart();
    if (syntaxer()->language()==QSynedit::ProgrammingLanguage::ATTAssembly) {
        if (statement->command.startsWith(".")
                || statement->command.startsWith("%"))
            pStart.ch--;
    }
    setCaretAndSelection(pStart,pStart,p);

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
            QChar nextCh = nextNonSpaceChar(caretY()-1,p.ch-1);
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
        if (
                (statement->kind == StatementKind::Keyword
                 || statement->kind == StatementKind::Preprocessor)
                && (statement->command.startsWith('#')
                    || statement->command.startsWith('@'))
                ) {

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
    QString headerName = mHeaderCompletionPopup->selectedFilename(true);
    if (headerName.isEmpty()) {
        mHeaderCompletionPopup->hide();
        return;
    }

    // delete the part of the word that's already been typed ...
    QSynedit::BufferCoord p = caretXY();
    int posBegin = p.ch-1;
    int posEnd = p.ch-1;
    QString sLine = lineText();
    while ((posBegin>0) &&
           (isIdentChar(sLine[posBegin-1]) || (sLine[posBegin-1]=='.') || (sLine[posBegin-1]=='+')))
        posBegin--;

    while ((posEnd < sLine.length())
           && (isIdentChar(sLine[posEnd]) || (sLine[posEnd]=='.') || (sLine[posBegin-1]=='+')))
        posEnd++;
    p.ch = posBegin+1;
    setBlockBegin(p);
    p.ch = posEnd+1;
    setBlockEnd(p);

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
    bool processed = false;
    if (!mCompletionPopup->isEnabled())
        return false;
    QString oldPhrase = mCompletionPopup->memberPhrase();
    WordPurpose purpose = WordPurpose::wpCompletion;
    if (syntaxer()->language()==QSynedit::ProgrammingLanguage::ATTAssembly) {
        purpose = WordPurpose::wpATTASMKeywords;
    } else if (oldPhrase.startsWith('#')) {
        purpose = WordPurpose::wpDirective;
    } else if (oldPhrase.startsWith('@')) {
        purpose = WordPurpose::wpJavadoc;
    }
    QString phrase;
    QSynedit::BufferCoord pBeginPos,pEndPos;
    switch (event->key()) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
        //ignore it
        return true;
    case Qt::Key_Backspace:
        processCommand(
                    QSynedit::EditCommand::DeleteLastChar,
                    QChar(), nullptr); // Simulate backspace in editor
        if (purpose == WordPurpose::wpCompletion) {
            phrase = getWordForCompletionSearch(caretXY(), mCompletionPopup->memberOperator()=="::");
        } else
            phrase = getWordAtPosition(this,caretXY(),
                                            pBeginPos,pEndPos,
                                            purpose);
        if (phrase.isEmpty()) {
            mCompletionPopup->hide();
        } else {
            mCompletionPopup->search(phrase, false);
        }
        return true;
    case Qt::Key_Escape:
        mCompletionPopup->hide();
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
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
        processCommand(QSynedit::EditCommand::Char, ch);
        if (purpose == WordPurpose::wpCompletion) {
            phrase = getWordForCompletionSearch(caretXY(),mCompletionPopup->memberOperator()=="::");
        } else
            phrase = getWordAtPosition(this,caretXY(),
                                            pBeginPos,pEndPos,
                                            purpose);
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
    if (!mHeaderCompletionPopup->isEnabled())
        return false;
    QString phrase;
    QSynedit::BufferCoord pBeginPos,pEndPos;
    switch (event->key()) {
    case Qt::Key_Backspace:
        processCommand(
                    QSynedit::EditCommand::DeleteLastChar,
                    QChar(), nullptr); // Simulate backspace in editor
        phrase = getWordAtPosition(this,caretXY(),
                                   pBeginPos,pEndPos,
                                   WordPurpose::wpHeaderCompletion);
        mHeaderCompletionPopup->search(phrase, false);
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
        processCommand(QSynedit::EditCommand::Char, ch);
        phrase = getWordAtPosition(this,caretXY(),
                                            pBeginPos,pEndPos,
                                            WordPurpose::wpHeaderCompletion);
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

bool Editor::onCompletionInputMethod(QInputMethodEvent *event)
{
    bool processed = false;
    if (!mCompletionPopup->isVisible())
        return processed;
    QString s=event->commitString();
    if (mParser && !s.isEmpty()) {
        QString phrase = getWordForCompletionSearch(caretXY(),mCompletionPopup->memberOperator()=="::");
        mCompletionPopup->search(phrase, false);
        return true;
    }
    return processed;
}

Editor::TipType Editor::getTipType(QPoint point, QSynedit::BufferCoord& pos)
{
    // Only allow in the text area...
    if (pointToCharLine(point, pos)) {
        //qDebug()<<gutterWidth()<<charWidth()<<point.y()<<point.x()<<pos.line<<pos.ch;
        if (!pMainWindow->debugger()->executing()
                && getSyntaxIssueAtPosition(pos)) {
            return TipType::Error;
        }

        QSynedit::PTokenAttribute attr;
        QString s;

        // Only allow hand tips in highlighted areas
        if (getTokenAttriAtRowCol(pos,s,attr)) {
            // Only allow Identifiers, Preprocessor directives, and selection
            if (attr) {
                if (selAvail()) {
                    // do not allow when dragging selection
                    if (isPointInSelection(pos))
                        return TipType::Selection;
                } else if (attr->tokenType() == QSynedit::TokenType::Identifier) {
                    return TipType::Identifier;
                } else if (mParser && mParser->isIncludeLine(lineText(pos.line))) {
                    return TipType::Preprocessor;
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

QString Editor::getFileHint(const QString &s, bool fromNext)
{
    QString fileName = mParser->getHeaderFileName(mFilename, s, fromNext);
    if (fileExists(fileName)) {
        return fileName + " - " + tr("Ctrl+click for more info");
    }
    return "";
}

QString Editor::getParserHint(const QStringList& expression,const QString &/*s*/, int line)
{
    if (!mParser)
        return "";
    // This piece of code changes the parser database, possibly making hints and code completion invalid...
    QString result;
    // Exit early, don't bother creating a stream (which is slow)
    PStatement statement = mParser->findStatementOf(
                mFilename,expression,
                line);
    if (!statement)
        return result;
    if (statement->kind == StatementKind::Function
            || statement->kind == StatementKind::Constructor
            || statement->kind == StatementKind::Destructor) {
          result = getHintForFunction(statement,mFilename,line);
    } else if (statement->line>0) {
        QFileInfo fileInfo(statement->fileName);
        result = mParser->prettyPrintStatement(statement,mFilename, line) + " - "
                + QString("%1(%2) ").arg(fileInfo.fileName()).arg(statement->line)
                + tr("Ctrl+click for more info");
    } else {  // hard defines
        result = mParser->prettyPrintStatement(statement, mFilename);
    }
    return result;
}

void Editor::showDebugHint(const QString &s, int line)
{
    if (!mParser)
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
    if (pMainWindow->debugger()->commandRunning())
        return;
    if (pMainWindow->functionTip()->isVisible()) {
        pMainWindow->functionTip()->hide();
    }
    connect(pMainWindow->debugger(), &Debugger::evalValueReady,
               this, &Editor::onTipEvalValueReady);
    mCurrentDebugTipWord = s;
    pMainWindow->debugger()->evalExpression(s);
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
            + QString("%1(%2) ").arg(fileInfo.fileName()).arg(statement->line)
            + tr("Ctrl+click for more info");
//    const StatementMap& children = mParser->statementList().childrenStatements(scopeStatement);
//    foreach (const PStatement& childStatement, children){
//        if (statement->command == childStatement->command
//                && statement->kind == childStatement->kind) {
//            if ((line < childStatement->line) &&
//                    childStatement->fileName == filename)
//                continue;
//            if (!result.isEmpty())
//                result += "\n";
//            QFileInfo fileInfo(childStatement->fileName);
//            result = mParser->prettyPrintStatement(childStatement,filename,line) + " - "
//                    + QString("%1(%2) ").arg(fileInfo.fileName()).arg(childStatement->line)
//                    + tr("Ctrl+click for more info");
//        }
//    }
    return result;
}

void Editor::updateFunctionTip(bool showTip)
{
    if (mCompletionPopup->isVisible()) {
        pMainWindow->functionTip()->hide();
        return;
    }
    if (inputMethodOn()) {
        pMainWindow->functionTip()->hide();
        return;
    }

    if (!mParser || !mParser->enabled())
        return;

    bool isFunction = false;
    auto action = finally([&isFunction]{
        if (!isFunction)
            pMainWindow->functionTip()->hide();
    });
    const int maxLines=10;
    QSynedit::BufferCoord caretPos = caretXY();
    int currentLine = caretPos.line-1;
    int currentChar = caretPos.ch-1;
    QSynedit::BufferCoord functionNamePos{-1,-1};
    bool foundFunctionStart = false;
    int parenthesisLevel = 0;
    int braceLevel = 0;
    int bracketLevel = 0;
    int paramsCount = 1;
    int currentParamPos = 1;
    if (currentLine>=lineCount())
        return;

    QChar ch=lastNonSpaceChar(currentLine,currentChar);
    if (ch!='(' && ch!=',')
        return;

    QSynedit::PTokenAttribute attr;
    QString token;
    QSynedit::SyntaxState syntaxState;
    QSynedit::BufferCoord pos = caretPos;
    pos.ch--;
    if (getTokenAttriAtRowCol(pos, token, attr, syntaxState)) {
        if (syntaxer()->isStringNotFinished(syntaxState.state))
            return;
        if (syntaxer()->isCommentNotFinished(syntaxState.state))
            return;
        if (attr->tokenType() == QSynedit::TokenType::Character)
            return;
    }

    while (currentLine>=0) {
        QString line = document()->getLine(currentLine);
        if (currentLine!=caretPos.line-1)
            currentChar = line.length();
        QStringList tokens;
        QList<int> positions;
        if (currentLine==0)
            syntaxer()->resetState();
        else
            syntaxer()->setState(
                            document()->getSyntaxState(currentLine-1));
        syntaxer()->setLine(line,currentLine);
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
                            functionNamePos.line = currentLine+1;
                            functionNamePos.ch = positions[i-1]+1;
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
                    functionNamePos.line = currentLine+1;
                    functionNamePos.ch = positions[i]+1;
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
    QSynedit::BufferCoord pWordBegin, pWordEnd;

    QString s = getWordAtPosition(this, functionNamePos, pWordBegin,pWordEnd, WordPurpose::wpInformation);
    int x = pWordBegin.ch-1-1;
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
        QSynedit::BufferCoord pos = pWordBegin;
        bool hasTypeQualifier;
        pos.ch = pWordBegin.ch;
        QString previousWord = getPreviousWordAtPositionForSuggestion(pos, hasTypeQualifier);

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
    if (s != pMainWindow->functionTip()->functionFullName()
            && !mParser->parsing()) {
        pMainWindow->functionTip()->clearTips();
        QList<PStatement> statements=mParser->getListOfFunctions(mFilename,
                                                                  s,
                                                                  functionNamePos.line);
//      qDebug()<<"finding function list:"<<s<<" - "<<statements.length();
        foreach (const PStatement statement, statements) {
            pMainWindow->functionTip()->addTip(
                        statement->command,
                        statement->fullName,
                        statement->type,
                        statement->args,
                        statement->noNameArgs);
        }
    }

    // If we can't find it in our database, hide
    if (pMainWindow->functionTip()->tipCount()<=0) {
        pMainWindow->functionTip()->hide();
        return;
    }
    // Position it at the top of the next line
    QPoint p = displayCoordToPixels(displayXY());
    p+=QPoint(0,textHeight()+2);

    pMainWindow->functionTip()->setFunctioFullName(s);
    pMainWindow->functionTip()->guessFunction(paramsCount-1);
    pMainWindow->functionTip()->setParamIndex(
                currentParamPos
                );
    int w = pMainWindow->functionTip()->width();
    if (w+p.x() > clientWidth())
        p.setX(clientWidth()-w-2);
    pMainWindow->functionTip()->move(mapToGlobal(p));
    cancelHint();
    if (showTip)
        pMainWindow->functionTip()->show();
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
    QSynedit::BufferCoord newCursorPos;
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
            tabStopBegin = n+p->x+1;
            tabStopEnd = n+p->endX+1;
        }
        mTabStopY = caretY() + p->y;
        newCursorPos.line = mTabStopY;
        newCursorPos.ch = tabStopBegin;
        setCaretXY(newCursorPos);
        setBlockBegin(newCursorPos);
        newCursorPos.ch = tabStopEnd;
        setBlockEnd(newCursorPos);

        mTabStopBegin = tabStopBegin;
        mTabStopEnd = tabStopEnd;
        mLineBeforeTabStop = lineText().mid(0, mTabStopBegin-1) ;
        mLineAfterTabStop = lineText().mid(mTabStopEnd-1) ;
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
    if (mCompletionPopup->isVisible() || mHeaderCompletionPopup->isVisible())
        return;

    if (mParser && (attr == syntaxer->identifierAttribute())) {
        QSynedit::BufferCoord p{column,Line};
        QSynedit::BufferCoord pBeginPos,pEndPos;
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
    pMainWindow->functionTip()->hide();
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
    int popWidth = std::min(pSettings->codeCompletion().widthInColumns() * charWidth(),
                            screenWidth / 2) + 4;
    int popHeight = std::min(pSettings->codeCompletion().heightInLines() * textHeight(),
                             (screenHeight / 2 - textHeight() * 2)) + 4;
    return QSize{popWidth, popHeight};
}

quint64 Editor::lastFocusOutTime() const
{
    return mLastFocusOutTime;
}

PCppParser Editor::sharedParser(ParserLanguage language)
{
    PCppParser parser;
    if (mSharedParsers.contains(language)) {
        parser=mSharedParsers[language].lock();
    }
    if (!parser) {
        parser=std::make_shared<CppParser>();
        parser->setLanguage(language);
        parser->setOnGetFileStream(
                    std::bind(
                        &EditorList::getContentFromOpenedEditor,pMainWindow->editorList(),
                        std::placeholders::_1, std::placeholders::_2));
        resetCppParser(parser);
        parser->setEnabled(true);
        mSharedParsers.insert(language,parser);
    }
    return parser;
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

bool Editor::useCppSyntax() const
{
    return mUseCppSyntax;
}

void Editor::setUseCppSyntax(bool newUseCppSyntax)
{
    mUseCppSyntax = newUseCppSyntax;
}

void Editor::setProject(Project *pProject)
{
    if (mProject == pProject)
        return;
    mProject = pProject;
    if (mProject) {
        if (syntaxer()->language() == QSynedit::ProgrammingLanguage::CPP) {
            mParser = mProject->cppParser();
            if (isVisible()) {
                if (mParser && mParser->parsing()) {
                    connect(mParser.get(),
                            &CppParser::onEndParsing,
                            this,
                            &Editor::onEndParsing);
                } else {
                    invalidate();
                }
            }
        }
    } else {
        initParser();
    }
}

void Editor::gotoDeclaration(const QSynedit::BufferCoord &pos)
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
    Editor *e = pMainWindow->openFile(filename);
    if (e) {
        e->setCaretPositionAndActivate(line,1);
    }
}

void Editor::gotoDefinition(const QSynedit::BufferCoord &pos)
{
    if (!parser())
        return;
    QStringList expression = getExpressionAtPosition(pos);

    // Find it's definition
    PStatement statement = parser()->findStatementOf(
                filename(),
                expression,
                pos.line);

    if (!statement) {
        // pMainWindow->updateStatusbarMessage(tr("Symbol '%1' not found!").arg(phrase));
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
    Editor *e = pMainWindow->editorList()->getOpenedEditorByFilename(filename);
    if (!e)
        e = pMainWindow->openFile(filename);
    if (e) {
        e->setCaretPositionAndActivate(line,1);
    }
}

QString getWordAtPosition(QSynedit::QSynEdit *editor, const QSynedit::BufferCoord &p, QSynedit::BufferCoord &pWordBegin, QSynedit::BufferCoord &pWordEnd, Editor::WordPurpose purpose)
{
    QString result = "";
    QString s;
    if ((p.line<1) || (p.line>editor->lineCount())) {
        pWordBegin = p;
        pWordEnd = p;
        return "";
    }

    s = editor->lineText(p.line);
    int len = s.length();

    int wordBegin = p.ch - 1 - 1; //BufferCoord::Char starts with 1
    int wordEnd = p.ch - 1 - 1;

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

    // Get end result
    result = s.mid(wordBegin+1, wordEnd - wordBegin);
    pWordBegin.line = p.line;
    pWordBegin.ch = wordBegin+1;
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
        int i = wordBegin;
        int line=p.line;
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
                    s=editor->lineText(line);
                    i=s.length();
                    continue;
                } else
                    break;
            } else {
                QSynedit::BufferCoord highlightPos;
                QSynedit::BufferCoord pDummy;
                highlightPos.line = line;
                highlightPos.ch = i+1;
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

QString Editor::getPreviousWordAtPositionForSuggestion(const QSynedit::BufferCoord &p, bool &hasTypeQualifier)
{
    hasTypeQualifier = false;
    QString result;
    if ((p.line<1) || (p.line>lineCount())) {
        return "";
    }
    bool inFunc = testInFunc(p);

    QString s = lineText(p.line);
    int wordBegin;
    int wordEnd = p.ch-2;
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
              if (bracketLevel>0)
                  bracketLevel--;
              else
                  return "";
          } else if (bracketLevel==0) {
              //Differentiate multiple definition and function parameter define here
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
        while ((wordBegin >= 0) && (isIdentChar(s[wordBegin]) || s[wordBegin]==':') ) {
            wordBegin--;
        }
        if (wordBegin<0 || s[wordBegin]!='#')
            wordBegin++;

        if (s[wordBegin]>='0' && s[wordBegin]<='9') // not valid word
            return "";

        result = s.mid(wordBegin, wordEnd - wordBegin+1);
        if (CppTypeQualifiers.contains(result))
            hasTypeQualifier = true;
        else if (!skipNextWord)
            break;
        // if ((result != "const") && !skipNextWord)
        //     break;
        wordEnd = wordBegin-1;
    }
    return result;
}

QString Editor::getPreviousWordAtPositionForCompleteFunctionDefinition(const QSynedit::BufferCoord &p)
{
    QString result;
    if ((p.line<1) || (p.line>lineCount())) {
        return "";
    }

    QString s = lineText(p.line);
    int wordBegin;
    int wordEnd = p.ch-2;
    if (wordEnd >= s.length())
        wordEnd = s.length()-1;
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
    if (wordEnd<0)
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
    const QString &astyle = pSettings->environment().AStylePath();
    if (!fileExists(astyle)) {
        QMessageBox::critical(this,
                              tr("astyle not found"),
                              tr("Can't find astyle in \"%1\".").arg(astyle));
        return;
    }
    //we must remove all breakpoints and syntax issues
//    onLinesDeleted(1,lineCount());
    QByteArray content = text().toUtf8();
    QStringList args = pSettings->codeFormatter().getArguments();
    QString command = escapeCommandForPlatformShell(extractFileName(astyle), args);
    pMainWindow->logToolsOutput(tr("Reformatting content using astyle..."));
    pMainWindow->logToolsOutput("------------------");
    pMainWindow->logToolsOutput(tr("- Astyle: %1").arg(astyle));
    pMainWindow->logToolsOutput(tr("- Command: %1").arg(command));
    auto [newContent, astyleError, processError] =
        runAndGetOutput(astyle, extractFileDir(astyle), args, content, true);
    if (!astyleError.isEmpty()) {
#ifdef Q_OS_WIN
        QString msg = QString::fromLocal8Bit(astyleError);
#else
        QString msg = QString::fromUtf8(astyleError);
#endif
        pMainWindow->logToolsOutput(msg);
    }
    if (!processError.isEmpty())
        pMainWindow->logToolsOutput(processError);
    if (newContent.isEmpty())
        return;
    replaceContent(QString::fromUtf8(newContent), doReparse);
}

void Editor::replaceContent(const QString &newContent, bool doReparse)
{
    int oldTopPos = topPos();
    QSynedit::BufferCoord mOldCaret = caretXY();

    beginEditing();
    addLeftTopToUndo();
    addCaretToUndo();

    QSynedit::EditorOptions oldOptions = getOptions();
    QSynedit::EditorOptions newOptions = oldOptions;
    newOptions.setFlag(QSynedit::EditorOption::AutoIndent,false);
    setOptions(newOptions);
    replaceAll(newContent);
    setCaretXY(mOldCaret);
    setTopPos(oldTopPos);
    setOptions(oldOptions);
    endEditing();

    if (doReparse && !pMainWindow->isQuitting() && !pMainWindow->isClosingAll()
            && !(inProject() && pMainWindow->closingProject())) {
        reparse(true);
        checkSyntaxInBack();
        reparseTodo();
        pMainWindow->updateEditorActions();
    }

}

void Editor::checkSyntaxInBack()
{
    if (!mInited)
        return;
    if (!inTab())
        return;
    if (readOnly())
        return;
    pMainWindow->checkSyntaxInBack(this);
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
            setCaretXY(blockEnd());
            invalidateLine(caretY());
            return;
        }
    }
    QSynEdit::tab();
}

int Editor::gutterClickedLine() const
{
    return mGutterClickedLine;
}

void Editor::toggleBreakpoint(int line)
{
    if (hasBreakpoint(line)) {
        mBreakpointLines.remove(line);
        if (inTab())
            pMainWindow->debugger()->removeBreakpoint(line,this);
    } else {
        mBreakpointLines.insert(line);
        if (inTab())
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

void Editor::modifyBreakpointProperty(int line)
{
    int index;
    PBreakpoint breakpoint = pMainWindow->debugger()->breakpointAt(line,this,&index);
    if (!breakpoint)
        return;
    bool isOk;
    QString s=QInputDialog::getText(this,
                              tr("Break point condition"),
                              tr("Enter the condition of the breakpoint:"),
                            QLineEdit::Normal,
                            breakpoint->condition,&isOk);
    if (isOk) {
        pMainWindow->debugger()->setBreakPointCondition(index,s,inProject());
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
    incPaintLock();
    QSynedit::EditorOptions options = QSynedit::EditorOption::AltSetsColumnMode
            | QSynedit::EditorOption::DragDropEditing | QSynedit::EditorOption::DropFiles
            | QSynedit::EditorOption::RightMouseMovesCursor
            | QSynedit::EditorOption::TabIndent
            | QSynedit::EditorOption::GroupUndo
            | QSynedit::EditorOption::SelectWordByDblClick;

    //options
    options.setFlag(QSynedit::EditorOption::ShowLeadingSpaces, pSettings->editor().showLeadingSpaces());
    options.setFlag(QSynedit::EditorOption::ShowTrailingSpaces, pSettings->editor().showTrailingSpaces());
    options.setFlag(QSynedit::EditorOption::ShowInnerSpaces, pSettings->editor().showInnerSpaces());
    options.setFlag(QSynedit::EditorOption::ShowLineBreaks, pSettings->editor().showLineBreaks());

    options.setFlag(QSynedit::EditorOption::AutoIndent,pSettings->editor().autoIndent());
    options.setFlag(QSynedit::EditorOption::TabsToSpaces,pSettings->editor().tabToSpaces());

    options.setFlag(QSynedit::EditorOption::KeepCaretX,pSettings->editor().keepCaretX());
    options.setFlag(QSynedit::EditorOption::EnhanceHomeKey,pSettings->editor().enhanceHomeKey());
    options.setFlag(QSynedit::EditorOption::EnhanceEndKey,pSettings->editor().enhanceEndKey());

    options.setFlag(QSynedit::EditorOption::AutoHideScrollbars,pSettings->editor().autoHideScrollbar());
    options.setFlag(QSynedit::EditorOption::ScrollPastEol,pSettings->editor().scrollPastEol());
    options.setFlag(QSynedit::EditorOption::ScrollPastEof,pSettings->editor().scrollPastEof());
    options.setFlag(QSynedit::EditorOption::HalfPageScroll,pSettings->editor().halfPageScroll());
    options.setFlag(QSynedit::EditorOption::InvertMouseScroll, false);

    options.setFlag(QSynedit::EditorOption::ShowRainbowColor,
                    pSettings->editor().rainbowParenthesis()
                    && syntaxer()->supportBraceLevel());
    options.setFlag(QSynedit::EditorOption::ForceMonospace,
                    pSettings->editor().forceFixedFontWidth());
    options.setFlag(QSynedit::EditorOption::LigatureSupport,
                    pSettings->editor().enableLigaturesSupport());
    setOptions(options);

    setTabSize(pSettings->editor().tabWidth());
    setInsertCaret(pSettings->editor().caretForInsert());
    setOverwriteCaret(pSettings->editor().caretForOverwrite());
    setCaretUseTextColor(pSettings->editor().caretUseTextColor());
    setCaretColor(pSettings->editor().caretColor());

    codeFolding().indentGuides = pSettings->editor().showIndentLines();
    codeFolding().indentGuidesColor = pSettings->editor().indentLineColor();
    codeFolding().fillIndents = pSettings->editor().fillIndents();

    QFont f=QFont();
    f.setFamily(pSettings->editor().fontName());
    f.setFamilies(pSettings->editor().fontFamiliesWithControlFont());
    f.setPixelSize(pointToPixel(pSettings->editor().fontSize()));
    f.setStyleStrategy(QFont::PreferAntialias);
    setFont(f);

    // QFont f=QFont(pSettings->editor().fontName());
    // f.setPixelSize(pointToPixel(pSettings->editor().fontSize()));
    // f.setStyleStrategy(QFont::PreferAntialias);
    // setFont(f);
    // QFont f2=QFont(pSettings->editor().nonAsciiFontName());
    // f2.setPixelSize(pointToPixel(pSettings->editor().fontSize()));
    // f2.setStyleStrategy(QFont::PreferAntialias);
    // setFontForNonAscii(f2);
    setLineSpacingFactor(pSettings->editor().lineSpacing());

    // Set gutter properties
    gutter().setLeftOffset(pointToPixel(pSettings->editor().fontSize()) + pSettings->editor().gutterLeftOffset());
    gutter().setRightOffset(pointToPixel(pSettings->editor().fontSize()) + pSettings->editor().gutterRightOffset());
    gutter().setBorderStyle(QSynedit::GutterBorderStyle::None);
    gutter().setUseFontStyle(pSettings->editor().gutterUseCustomFont());
    if (pSettings->editor().gutterUseCustomFont()) {
        f=QFont(pSettings->editor().gutterFontName());
        f.setPixelSize(pointToPixel(pSettings->editor().gutterFontSize()));
    } else {
        f=QFont(pSettings->editor().fontName());
        f.setPixelSize(pointToPixel(pSettings->editor().fontSize()));
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

    if (syntaxer()->language() == QSynedit::ProgrammingLanguage::CPP) {
        QSet<QString> set;
        if (pSettings->editor().enableCustomCTypeKeywords()) {
            foreach(const QString& s, pSettings->editor().customCTypeKeywords())
                set.insert(s);
        }
#ifdef ENABLE_SDCC
        if (!inProject() && pSettings->compilerSets().defaultSet()
               && pSettings->compilerSets().defaultSet()->compilerType()==CompilerType::SDCC) {
            for(auto it=SDCCKeywords.begin();it!=SDCCKeywords.end();++it)
                set.insert(it.key());
        }
#endif
        ((QSynedit::CppSyntaxer*)(syntaxer().get()))->setCustomTypeKeywords(set);
    }

    initAutoBackup();

    setMouseWheelScrollSpeed(pSettings->editor().mouseWheelScrollSpeed());
    setMouseSelectionScrollSpeed(pSettings->editor().mouseSelectionScrollSpeed());
    invalidate();
    decPaintLock();
}

static QSynedit::PTokenAttribute createRainbowAttribute(const QString& attrName, const QString& schemeName, const QString& schemeItemName) {
    PColorSchemeItem item = pColorManager->getItem(schemeName,schemeItemName);
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
                    pSettings->editor().rainbowParenthesis()
                    && syntaxer()->supportBraceLevel());
    setOptions(options);
    syntaxerManager.applyColorScheme(syntaxer(),schemeName);
    if (pSettings->editor().rainbowParenthesis()) {
        QSynedit::PTokenAttribute attr0 =createRainbowAttribute(SYNS_AttrSymbol,
                                                               schemeName,COLOR_SCHEME_BRACE_1);
        QSynedit::PTokenAttribute attr1 =createRainbowAttribute(SYNS_AttrSymbol,
                                                               schemeName,COLOR_SCHEME_BRACE_2);
        QSynedit::PTokenAttribute attr2 =createRainbowAttribute(SYNS_AttrSymbol,
                                                               schemeName,COLOR_SCHEME_BRACE_3);
        QSynedit::PTokenAttribute attr3 =createRainbowAttribute(SYNS_AttrSymbol,
                                                               schemeName,COLOR_SCHEME_BRACE_4);
        setRainbowAttrs(attr0,attr1,attr2,attr3);
    }
    PColorSchemeItem item = pColorManager->getItem(schemeName,COLOR_SCHEME_ACTIVE_LINE);
    if (item) {
        setActiveLineColor(item->background());
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_GUTTER);
    if (item) {
        gutter().setTextColor(item->foreground());
        gutter().setColor(alphaBlend(palette().color(QPalette::Base), item->background()));
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_GUTTER_ACTIVE_LINE);
    if (item) {
        gutter().setActiveLineTextColor(item->foreground());
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
    } else {
        this->setForegroundColor(palette().color(QPalette::HighlightedText));
        this->setBackgroundColor(palette().color(QPalette::Highlight));
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
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_TEXT);
    if (item) {
        this->setForegroundColor(item->foreground());
        this->setBackgroundColor(alphaBlend(palette().color(QPalette::Base), item->background()));
    } else {
        this->setForegroundColor(palette().color(QPalette::Text));
        this->setBackgroundColor(palette().color(QPalette::Base));
    }
    item = pColorManager->getItem(schemeName,COLOR_SCHEME_CURRENT_HIGHLIGHTED_WORD);
    if (item) {
        mCurrentHighlighWordForeground = item->foreground();
        mCurrentHighlighWordBackground = item->background();
    } else {
        mCurrentHighlighWordForeground = selectedForeground();
        mCurrentHighlighWordBackground = selectedBackground();
    }
    document()->invalidateAllNonTempLineWidth();
    invalidate();
}

void Editor::updateCaption(const QString& newCaption) {
    if (!inTab()) {
        return;
    }
    int index = mParentPageControl->indexOf(this);
    if (index==-1)
        return;
    QString caption;
    if (newCaption.isEmpty()) {
        caption = QFileInfo(mFilename).fileName();
        if (this->modified()) {
            caption.append("[*]");
        }
        if (this->readOnly()) {
            caption.append("["+tr("Readonly")+"]");
        }        
    } else {
        caption = newCaption;
    }
    caption = caption.replace("&","&&");
    mParentPageControl->setTabText(index,caption);
    mParentPageControl->setTabToolTip(index, mFilename);
}
