#include "editor.h"

#include <QtCore/QFileInfo>
#include <QTextCodec>
#include <QVariant>
#include <memory>
#include "settings.h"
#include "mainwindow.h"

using namespace std;


Editor::Editor(QObject *parent, const QString& filename,
                  const QByteArray& encoding,
                  bool inProject, bool isNew,
                  QTabWidget* parentPageControl):
  QObject(parent),
  mFilename(filename),
  mEncodingOption(encoding),
  mInProject(inProject),
  mIsNew(isNew),
  mParentPageControl(parentPageControl)
{
    mTextEdit = new QsciScintilla();
    if (mFilename.isEmpty()) {
        mFilename = tr("untitled") + "1";
    }
    QFileInfo fileInfo(mFilename);
    mParentPageControl->addTab(mTextEdit,fileInfo.fileName());
    if (!isNew) {
        loadFile();
    } else {
        if (mEncodingOption == ENCODING_AUTO_DETECT)
            mFileEncoding = ENCODING_ASCII;
        else
            mFileEncoding = mEncodingOption;
    }
    mTextEdit->setProperty("editor",QVariant::fromValue<intptr_t>((intptr_t)this));
}

Editor::~Editor() {
    int index = mParentPageControl->indexOf(mTextEdit);
    mParentPageControl->removeTab(index);
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
        mTextEdit->setText(QString::fromUtf8(ba));
    } else if (mFileEncoding == ENCODING_UTF8_BOM) {
        mTextEdit->setText(QString::fromUtf8(ba.mid(3)));
    } else if (mFileEncoding == ENCODING_ASCII) {
        mTextEdit->setText(QString::fromLatin1(ba));
    }else {
        QTextCodec*codec = QTextCodec::codecForName(mFileEncoding);
        mTextEdit->setText(codec->toUnicode(ba));
    }
}

void Editor::saveFile(const QString &filename) {
    if (mEncodingOption!=ENCODING_AUTO_DETECT && mEncodingOption!=mFileEncoding)  {
        mFileEncoding = mEncodingOption;
    }
    if (mEncodingOption == ENCODING_AUTO_DETECT && mFileEncoding == ENCODING_ASCII) {
        if (!isTextAllAscii(mTextEdit->text())) {
            mFileEncoding = pSettings->value(EDITOR_DEFAULT_ENCODING).toByteArray();
        }
        pMainWindow->updateStatusBarForEncoding();
        //todo: update status bar, and set fileencoding using configurations
    }
    QFile file(filename);
    QByteArray ba;
    if (mFileEncoding == ENCODING_UTF8) {
        ba = mTextEdit->text().toUtf8();
    } else if (mFileEncoding == ENCODING_UTF8_BOM) {
            ba.resize(3);
            ba[0]=0xEF;
            ba[1]=0xBB;
            ba[2]=0xBF;
            ba.append(mTextEdit->text().toUtf8());
    } else if (mFileEncoding == ENCODING_ASCII) {
        ba = mTextEdit->text().toLatin1();
    } else {
        QTextCodec* codec = QTextCodec::codecForName(mFileEncoding);
        ba = codec->fromUnicode(mTextEdit->text());
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
QsciScintilla* Editor::textEdit() {
    return mTextEdit;
}
