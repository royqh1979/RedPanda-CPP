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
                    const QByteArray& encoding,
                    bool inProject, bool isNew,
                    QTabWidget* parentPageControl);

    ~Editor();

    const QByteArray& encodingOption() const;
    void setEncodingOption(const QByteArray& encoding);
    const QByteArray& fileEncoding() const;
    const QString& filename();
    bool inProject() const;
    bool isNew() const;

    void loadFile();
    void saveFile(const QString& filename);
    bool save();

    QsciScintilla* textEdit();
signals:

private:
    QByteArray mEncodingOption; // the encoding type set by the user
    QByteArray mFileEncoding; // the real encoding of the file (auto detected)
    QString mFilename;
    QTabWidget* mParentPageControl;
    bool mInProject;
    bool mIsNew;
    QsciScintilla* mTextEdit;
};

#endif // EDITOR_H
