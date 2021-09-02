#ifndef CARETLIST_H
#define CARETLIST_H
#include <QString>
#include <memory>
#include <QVector>
#include <QObject>

class Editor;

struct EditorCaret{
    Editor* editor;
    int line;
    int aChar;
};
using PEditorCaret = std::shared_ptr<EditorCaret>;

class CaretList:public QObject
{
    Q_OBJECT
public:
    explicit CaretList(QObject* parent = nullptr);
public:
    void addCaret(Editor *editor, int line, int aChar);
    bool hasPrevious() const;
    bool hasNext() const;
    PEditorCaret gotoAndGetPrevious();
    PEditorCaret gotoAndGetNext();
    void removeEditor(const Editor* editor);
    void reset();
    void pause();
    void unPause();
public slots:
    void linesDeleted(const Editor* editor, int firstLine, int count);
    void linesInserted(const Editor* editor, int firstLine, int count);
private:
    void removeCaret(int index);
private:
    QVector<PEditorCaret> mList;
    int mIndex;
    bool mPauseAdd;
};

#endif // CARETLIST_H
