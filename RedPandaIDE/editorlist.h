#ifndef EDITORLIST_H
#define EDITORLIST_H

#include <QObject>
#include <QTabWidget>
#include <QSplitter>
#include <QWidget>

class EditorList : public QObject
{
    Q_OBJECT
public:
    enum ShowType{
        lstNone,
        lstLeft,
        lstRight,
        lstBoth
    };
    explicit EditorList(QObject *parent = nullptr);



signals:

private:
    ShowType mLayout;
    QTabWidget *mLeftPageWidget;
    QTabWidget *mRightPageWidget;
    QSplitter *mSplitter;
    QWidget *mPanel;
    int mUpdateCount;



};

#endif // EDITORLIST_H
