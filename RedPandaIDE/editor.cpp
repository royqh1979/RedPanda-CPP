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
#include <Qsci/qscilexercpp.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

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


Editor::Editor(QWidget *parent, const QString& filename,
                  const QByteArray& encoding,
                  bool inProject, bool isNew,
                  QTabWidget* parentPageControl):
  QsciScintilla(parent),
  mFilename(filename),
  mEncodingOption(encoding),
  mInProject(inProject),
  mIsNew(isNew),
  mParentPageControl(parentPageControl)
{
    if (mFilename.isEmpty()) {
        mFilename = tr("untitled") + "1";
    }
    QFileInfo fileInfo(mFilename);
    if (mParentPageControl!=NULL) {
        mParentPageControl->addTab(this,QString());
        updateCaption();
    }
    if (!isNew) {
        loadFile();
    } else {
        if (mEncodingOption == ENCODING_AUTO_DETECT)
            mFileEncoding = ENCODING_ASCII;
        else
            mFileEncoding = mEncodingOption;
    }

    //
    QsciLexerCPP *lexer = new QsciLexerCPP();
    lexer->setHighlightEscapeSequences(true);
    lexer->setFoldComments(true);
    lexer->setDefaultFont(QFont("Consolas",12));
    this->setLexer(lexer);
    this->setAutoIndent(pSettings->editor().autoIndent());
    this->setFolding(FoldStyle::BoxedTreeFoldStyle,FoldMargin);
    this->setTabWidth(4);

    this->setCaretLineVisible(true);
    this->setCaretLineBackgroundColor(QColor(0xCCFFFF));
    this->setMatchedBraceForegroundColor(QColor("Red"));


    this->setBraceMatching(BraceMatch::SloppyBraceMatch);
    //行号显示区域
    setMarginType(LineNumberMargin, QsciScintilla::NumberMargin);
    setMarginLineNumbers(LineNumberMargin, true);
    setMarginWidth(LineNumberMargin,10);
    this->onLinesChanged(0,0);
    //断点设置区域
    setMarginType(MarkerMargin, QsciScintilla::SymbolMargin);
    setMarginLineNumbers(MarkerMargin, false);
    setMarginWidth(MarkerMargin,20);
    setMarginSensitivity(MarkerMargin, true);    //set the margin as a selection margin, which is clickable

//    connect(textEdit, SIGNAL(marginClicked(int, int, Qt::KeyboardModifiers)),this,
//            SLOT(on_margin_clicked(int, int, Qt::KeyboardModifiers)));

    //markers
    markerDefine(QsciScintilla::CircledPlus, ErrorMarker);
    setMarkerForegroundColor(QColor("BLACK"),ErrorMarker);
    setMarkerBackgroundColor(QColor("RED"),ErrorMarker);
    markerAdd(1,ErrorMarker);

    // connect will fail if use new function pointer syntax
//    connect(this, &QsciScintilla::modificationChanged,
//            this, &Editor::onModificationChanged);
//    connect(this , &QsciScintilla::cursorPositionChanged,
//            this, &Editor::onCursorPositionChanged);
    connect(this,SIGNAL(modificationChanged(bool)),
            this,SLOT(onModificationChanged(bool)));
    connect(this , SIGNAL(cursorPositionChanged(int,int)),
            this, SLOT(onCursorPositionChanged(int,int)));
    connect(this, SIGNAL(linesChanged(int,int)),
            this,SLOT(onLinesChanged(int, int)));

}

Editor::~Editor() {
    if (mParentPageControl!=NULL) {
        int index = mParentPageControl->indexOf(this);
        mParentPageControl->removeTab(index);
    }
    this->setParent(0);

    delete this->lexer();
    this->setLexer(NULL);
}

void Editor::loadFile() {
    QFile file(mFilename);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::information(pMainWindow,
                                 tr("Error"),
                                 QString(tr("Can't Open File %1:%2")).arg(mFilename).arg(file.errorString()));
    }
    QByteArray content=file.readAll();
    file.close();
    if (mEncodingOption == ENCODING_AUTO_DETECT) {
        mFileEncoding = GuessTextEncoding(content);
    } else {
        mFileEncoding = mEncodingOption;
    }
    if (mFileEncoding == ENCODING_UTF8) {
        this->setText(QString::fromUtf8(content));
    } else if (mFileEncoding == ENCODING_UTF8_BOM) {
        this->setText(QString::fromUtf8(content.mid(3)));
    } else if (mFileEncoding == ENCODING_ASCII) {
        this->setText(QString::fromLatin1(content));
    } else if (mFileEncoding == ENCODING_SYSTEM_DEFAULT) {
        this->setText(QString::fromLocal8Bit(content));
    }else {
        QTextCodec*codec = QTextCodec::codecForName(mFileEncoding);
        this->setText(codec->toUnicode(content));
    }
}

void Editor::saveFile(const QString &filename) {
    if (mEncodingOption!=ENCODING_AUTO_DETECT && mEncodingOption!=mFileEncoding)  {
        mFileEncoding = mEncodingOption;
    }
    if (mEncodingOption == ENCODING_AUTO_DETECT && mFileEncoding == ENCODING_ASCII) {
        if (!isTextAllAscii(this->text())) {
            mFileEncoding = pSettings->editor().defaultEncoding();
        }
        pMainWindow->updateStatusBarForEncoding();
        //todo: update status bar, and set fileencoding using configurations
    }
    QFile file(filename);
    QByteArray ba;
    if (mFileEncoding == ENCODING_UTF8) {
        ba = this->text().toUtf8();
    } else if (mFileEncoding == ENCODING_UTF8_BOM) {
            ba.resize(3);
            ba[0]=0xEF;
            ba[1]=0xBB;
            ba[2]=0xBF;
            ba.append(this->text().toUtf8());
    } else if (mFileEncoding == ENCODING_ASCII) {
        ba = this->text().toLatin1();
    } else if (mFileEncoding == ENCODING_SYSTEM_DEFAULT) {
        ba = this->text().toLocal8Bit();
    } else {
        QTextCodec* codec = QTextCodec::codecForName(mFileEncoding);
        ba = codec->fromUnicode(this->text());
    }
    if (file.open(QFile::WriteOnly)) {
        if (file.write(ba)<0) {
            throw SaveException(QString(tr("Failed to Save file %1: %2")).arg(filename).arg(file.errorString()));
        }
        file.close();
    } else {
        throw SaveException(QString(tr("Failed to Open file %1: %2")).arg(filename).arg(file.errorString()));
    }
}

bool Editor::save(bool force, bool reparse) {
    if (this->mIsNew) {
        return saveAs();
    }
    QFileInfo info(mFilename);
    //is this file writable;
    if (!force && !info.isWritable()) {
        QMessageBox::information(pMainWindow,tr("Fail"),
                                 QString(QObject::tr("File %s is not writable!")));
        return false;
    }
    if (this->isModified() || force) {
        try {
            saveFile(mFilename);
            setModified(false);
        }  catch (SaveException& exception) {
            QMessageBox::information(pMainWindow,tr("Fail"),
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
        saveFile(mFilename);
        mFilename = newName;
        mIsNew = false;
        setModified(false);
    }  catch (SaveException& exception) {
        QMessageBox::information(pMainWindow,tr("Fail"),
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
    this->mParentPageControl->setCurrentWidget(this);
    this->setFocus();
}

const QByteArray& Editor::encodingOption() const noexcept{
    return mEncodingOption;
}
void Editor::setEncodingOption(const QByteArray& encoding) noexcept{
    mEncodingOption = encoding;
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
        if (event->angleDelta().y()>0) {
            this->zoomIn();
        } else {
            this->zoomOut();
        }
        onLinesChanged(0,0);
    }
}

void Editor::onModificationChanged(bool) {
    updateCaption();
}

void Editor::onCursorPositionChanged(int line, int index) {
    pMainWindow->updateStatusBarForEditingInfo(line,index+1,lines(),text().length());
    long pos = getCursorPosition();
    long start = SendScintilla(SCI_WORDSTARTPOSITION,pos,false);
    long end = SendScintilla(SCI_WORDENDPOSITION,pos,false);

    qDebug()<<start<<end<<text(start,end);
}

void Editor::onLinesChanged(int startLine, int count) {
    this->setMarginWidth(0,QString("0%1").arg(lines()));
    qDebug()<<"Editor lines changed"<<lines();
    qDebug()<<startLine<<count;
}

void Editor::updateCaption(const QString& newCaption) {
    if (mParentPageControl==NULL) {
        return;
    }
    int index = mParentPageControl->indexOf(this);
    if (index==-1)
        return;
    if (newCaption.isEmpty()) {
        QString caption = QFileInfo(mFilename).fileName();
        if (this->isModified()) {
            caption.append("[*]");
        }
        mParentPageControl->setTabText(index,caption);
    } else {
        mParentPageControl->setTabText(index,newCaption);
    }

}
