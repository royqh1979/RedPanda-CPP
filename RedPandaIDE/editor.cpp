#include "editor.h"

#include <QtCore/QFileInfo>
#include <QFont>
#include <QTextCodec>
#include <QVariant>
#include <QWheelEvent>
#include <memory>
#include "settings.h"
#include "mainwindow.h"
#include <Qsci/qscilexercpp.h>

using namespace std;


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
    if (mParentPageControl!=NULL)
        mParentPageControl->addTab(this,fileInfo.fileName());
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
    this->setLexer(lexer);
    this->setAutoIndent(pSettings->value(EDITOR_AUTO_INDENT).toBool());

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
    QStringList strs;
    QFile file(mFilename);
    QByteArray ba=file.read(file.bytesAvailable());
    if (mEncodingOption == ENCODING_AUTO_DETECT) {
        mFileEncoding = GetFileEncodingType(ba);
    } else {
        mFileEncoding = mEncodingOption;
    }
    if (mFileEncoding == ENCODING_UTF8) {
        this->setText(QString::fromUtf8(ba));
    } else if (mFileEncoding == ENCODING_UTF8_BOM) {
        this->setText(QString::fromUtf8(ba.mid(3)));
    } else if (mFileEncoding == ENCODING_ASCII) {
        this->setText(QString::fromLatin1(ba));
    }else {
        QTextCodec*codec = QTextCodec::codecForName(mFileEncoding);
        this->setText(codec->toUnicode(ba));
    }
}

void Editor::saveFile(const QString &filename) {
    if (mEncodingOption!=ENCODING_AUTO_DETECT && mEncodingOption!=mFileEncoding)  {
        mFileEncoding = mEncodingOption;
    }
    if (mEncodingOption == ENCODING_AUTO_DETECT && mFileEncoding == ENCODING_ASCII) {
        if (!isTextAllAscii(this->text())) {
            mFileEncoding = pSettings->value(EDITOR_DEFAULT_ENCODING).toByteArray();
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
    } else {
        QTextCodec* codec = QTextCodec::codecForName(mFileEncoding);
        ba = codec->fromUnicode(this->text());
    }
    file.write(ba);
    file.close();
}

bool Editor::save(bool force, bool reparse) {

    return true;
}

const QByteArray& Editor::encodingOption() const {
    return mEncodingOption;
}
void Editor::setEncodingOption(const QByteArray& encoding) {
    mEncodingOption = encoding;
}
const QByteArray& Editor::fileEncoding() const {
    return mFileEncoding;
}
const QString& Editor::filename() {
    return mFilename;
}
bool Editor::inProject() const {
    return mInProject;
}
bool Editor::isNew() const {
    return mIsNew;
}

QTabWidget* Editor::pageControl() {
    return mParentPageControl;
}

void Editor::wheelEvent(QWheelEvent *event) {
    if ( (event->modifiers() & Qt::ControlModifier)!=0) {
        if (event->angleDelta().y()>0) {
            this->zoomIn();
        } else {
            this->zoomOut();
        }
    }
}
