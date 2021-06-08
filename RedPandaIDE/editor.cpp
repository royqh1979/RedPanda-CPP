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
#include "qsynedit/highlighter/cpp.h"
#include "HighlighterManager.h"


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

Editor::Editor(QWidget *parent, const QString& filename,
                  const QByteArray& encoding,
                  bool inProject, bool isNew,
                  QTabWidget* parentPageControl):
  SynEdit(parent),
  mFilename(filename),
  mEncodingOption(encoding),
  mInProject(inProject),
  mIsNew(isNew),
  mParentPageControl(parentPageControl)
{
    if (mFilename.isEmpty()) {
        newfileCount++;
        mFilename = tr("untitled%1").arg(newfileCount);
    }
    QFileInfo fileInfo(mFilename);
    if (mParentPageControl!=NULL) {
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
}

Editor::~Editor() {
    if (mParentPageControl!=NULL) {
        int index = mParentPageControl->indexOf(this);
        mParentPageControl->removeTab(index);
    }
    this->setParent(0);

}

void Editor::loadFile() {
    QFile file(mFilename);
//    if (!file.open(QFile::ReadOnly)) {
//        QMessageBox::information(pMainWindow,
//                                 tr("Error"),
//                                 QString(tr("Can't Open File %1:%2")).arg(mFilename).arg(file.errorString()));
//    }
    this->lines()->LoadFromFile(file,mEncodingOption,mFileEncoding);
    this->setModified(false);
    updateCaption();
    pMainWindow->updateStatusBarForEncoding();
}

void Editor::saveFile(const QString &filename) {
//    if (mEncodingOption!=ENCODING_AUTO_DETECT && mEncodingOption!=mFileEncoding)  {
//        mFileEncoding = mEncodingOption;
//    }
//    if (mEncodingOption == ENCODING_AUTO_DETECT && mFileEncoding == ENCODING_ASCII) {
//        if (!isTextAllAscii(this->text())) {
//            mFileEncoding = pSettings->editor().defaultEncoding();
//        }
//        pMainWindow->updateStatusBarForEncoding();
//        //todo: update status bar, and set fileencoding using configurations
//    }
    QFile file(filename);
    this->lines()->SaveToFile(file,mEncodingOption,mFileEncoding);
    pMainWindow->updateStatusBarForEncoding();
//    QByteArray ba;
//    if (mFileEncoding == ENCODING_UTF8) {
//        ba = this->text().toUtf8();
//    } else if (mFileEncoding == ENCODING_UTF8_BOM) {
//            ba.resize(3);
//            ba[0]=0xEF;
//            ba[1]=0xBB;
//            ba[2]=0xBF;
//            ba.append(this->text().toUtf8());
//    } else if (mFileEncoding == ENCODING_ASCII) {
//        ba = this->text().toLatin1();
//    } else if (mFileEncoding == ENCODING_SYSTEM_DEFAULT) {
//        ba = this->text().toLocal8Bit();
//    } else {
//        QTextCodec* codec = QTextCodec::codecForName(mFileEncoding);
//        ba = codec->fromUnicode(this->text());
//    }
//    if (file.open(QFile::WriteOnly)) {
//        if (file.write(ba)<0) {
//            throw SaveException(QString(tr("Failed to Save file %1: %2")).arg(filename).arg(file.errorString()));
//        }
//        file.close();
//    } else {
//        throw SaveException(QString(tr("Failed to Open file %1: %2")).arg(filename).arg(file.errorString()));
//    }
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
    if (this->modified()|| force) {
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
        QFont oldFont = font();
        int size = oldFont.pointSize();
        if (event->angleDelta().y()>0) {
//            size = std::max(5,size-1);
//            oldFont.setPointSize(oldFont.pointSize());
//            this->setFont(oldFont);
            this->zoomIn();
        } else {
//            size = std::min(size+1,50);
//            oldFont.setPointSize(oldFont.pointSize());
//            this->setFont(oldFont);
            this->zoomOut();
        }
        onLinesChanged(0,0);
    }
}

void Editor::onModificationChanged(bool) {
    updateCaption();
}

void Editor::onCursorPositionChanged(int line, int index) {
    pMainWindow->updateStatusBarForEditingInfo(line,index+1,lines()->count(),lines()->getTextLength());
}

void Editor::onLinesChanged(int startLine, int count) {
    qDebug()<<"Editor lines changed"<<lines()->count();
    qDebug()<<startLine<<count;
}

void Editor::applySettings()
{
    SynEditorOptions options = eoAltSetsColumnMode |
            eoDragDropEditing | eoDropFiles |  eoKeepCaretX | eoTabsToSpaces |
            eoRightMouseMovesCursor | eoScrollByOneLess | eoTabIndent | eoHideShowScrollbars;
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
    //todo: show indent line
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
        if (this->modified()) {
            caption.append("[*]");
        }
        mParentPageControl->setTabText(index,caption);
    } else {
        mParentPageControl->setTabText(index,newCaption);
    }

}
