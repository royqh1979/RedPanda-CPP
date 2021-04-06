#ifndef EDITOR_H
#define EDITOR_H

#include <QObject>
#include <utils.h>
#include <QTabWidget>
#include <Qsci/qsciscintilla.h>

class Editor : public QObject
{
    Q_OBJECT
public:
    explicit Editor(QObject *parent, const QString& filename,
                    FileEncodingType encodingType,
                    bool inProject, bool isNew,
                    QTabWidget* parentPageControl);

    FileEncodingType encodingType() const;
    void setFileEncodingType(FileEncodingType type);
    FileEncodingType fileEncoding() const;
    const QString& filename();
    bool inProject() const;
    bool isNew() const;

    void loadFile();

signals:

private:
    FileEncodingType mEncodingType; // the encoding type set by the user
    FileEncodingType mFileEncoding; // the real encoding of the file (auto detected)
    QString mFilename;
    QTabWidget* mParentPageControl;
    bool mInProject;
    bool mIsNew;
    QsciScintilla* mTextEdit;
};

#endif // EDITOR_H
