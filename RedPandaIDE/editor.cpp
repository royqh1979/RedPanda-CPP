#include "editor.h"

#include <QtCore/QFileInfo>
#include <QVariant>
#include <memory>

using namespace std;


Editor::Editor(QObject *parent, const QString& filename,
                  FileEncodingType encodingType,
                  bool inProject, bool isNew,
                  QTabWidget* parentPageControl):
  QObject(parent),
  mFilename(filename),
  mEncodingType(encodingType),
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
        if (mEncodingType == etAuto)
            mFileEncoding = etAscii;
        else
            mFileEncoding = mEncodingType;
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
    if (mEncodingType == etAuto) {
        mFileEncoding = GetFileEncodingType(ba);
    } else {
        mFileEncoding = mEncodingType;
    }
    switch(mFileEncoding) {
        case etUTF8:
            mTextEdit->setText(QString::fromUtf8(ba));
            break;
        case etUTF8Bom:
            mTextEdit->setText(QString::fromUtf8(ba.mid(3)));
            break;
        default:
            mTextEdit->setText(QString::fromLocal8Bit(ba));
    }
}

void Editor::saveFile(const QString &filename) {
    if (mEncodingType!=etAuto && mEncodingType!=mFileEncoding)  {
        mFileEncoding = mEncodingType;
    }
    if (mEncodingType ==etAuto && mFileEncoding == etAscii) {
        if (!isTextAllAscii(mTextEdit->text())) {
            mFileEncoding = etAnsi;
        }
        //todo: update status bar, and set fileencoding using configurations
    }
    QFile file(filename);
    QByteArray ba;
    switch(mFileEncoding) {
        case etUTF8:
            ba = mTextEdit->text().toUtf8();
            break;
        case etUTF8Bom:
            ba.resize(3);
            ba[0]=0xEF;
            ba[1]=0xBB;
            ba[2]=0xBF;
            ba.append(mTextEdit->text().toUtf8());
            break;
        default:
            ba = mTextEdit->text().toLocal8Bit();
    }
    file.write(ba);
    file.close();
}

FileEncodingType Editor::encodingType() const {
    return mEncodingType;
}
void Editor::setFileEncodingType(FileEncodingType type) {
    mEncodingType = type;
}
FileEncodingType Editor::fileEncoding() const {
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
