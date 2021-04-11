#ifndef EDITOR_H
#define EDITOR_H

#include <QObject>
#include <utils.h>
#include <QTabWidget>
#include <Qsci/qsciscintilla.h>

class Editor : public QsciScintilla
{
    Q_OBJECT
public:
    explicit Editor(QWidget *parent, const QString& filename,
                    const QByteArray& encoding,
                    bool inProject, bool isNew,QTabWidget* parentPageControl);

    ~Editor();

    //tell the compiler to prohibit copy/moving editor objects ( we should only use pointers to the editor object)
    Editor(const Editor&) = delete;
    Editor(const Editor&&) = delete;
    Editor& operator=(const Editor&) = delete;
    Editor& operator=(const Editor&&) = delete;

    const QByteArray& encodingOption() const;
    void setEncodingOption(const QByteArray& encoding);
    const QByteArray& fileEncoding() const;
    const QString& filename();
    bool inProject() const;
    bool isNew() const;

    void loadFile();
    void saveFile(const QString& filename);
    bool save(bool force=false, bool reparse=true);
    bool saveAs();

    QTabWidget* pageControl();

    void updateCaption(const QString& newCaption=QString());

signals:


protected slots:
    void onModificationChanged(bool status);

private:
    QByteArray mEncodingOption; // the encoding type set by the user
    QByteArray mFileEncoding; // the real encoding of the file (auto detected)
    QString mFilename;
    QTabWidget* mParentPageControl;
    bool mInProject;
    bool mIsNew;


    // QWidget interface
protected:
    void wheelEvent(QWheelEvent *event) override;
};

#endif // EDITOR_H
