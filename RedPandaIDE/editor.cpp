#include "editor.h"

#include <QtCore/QFileInfo>


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
    QFileInfo fileInfo(mFilename);
    mParentPageControl->addTab(mTextEdit,fileInfo.fileName());
    if (!isNew) {
        loadFile();
    } else {
        mFileEncoding = etAscii;
    }

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
            mTextEdit->setText(UTF8toQString(ba));
            break;
        case etUTF8Bom:
            mTextEdit->setText(UTF8toQString(ba.mid(3)));
            break;
        default:
            mTextEdit->setText(QString(ba));
    }
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
