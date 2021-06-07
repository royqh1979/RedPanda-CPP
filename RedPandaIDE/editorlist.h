#ifndef EDITORLIST_H
#define EDITORLIST_H

#include <QTabWidget>
#include <QSplitter>
#include <QWidget>
#include "utils.h"

class Editor;
class EditorList
{
public:
    enum class ShowType{
        lstNone,
        lstLeft,
        lstRight,
        lstBoth
    };

    explicit EditorList(QTabWidget* leftPageWidget,
                        QTabWidget* rightPageWidget,
                        QSplitter* splitter,
                        QWidget* panel);

    Editor* newEditor(const QString& filename, const QByteArray& encoding,
                     bool inProject, bool newFile,
                     QTabWidget* page=NULL);

    Editor* getEditor(int index=-1, QTabWidget* tabsWidget=NULL) const;

    bool closeEditor(Editor* editor, bool transferFocus=true, bool force=false);

    bool closeAll(bool force = false);

    Editor* getOpenedEditorByFilename(const QString& filename);

    Editor* getEditorByFilename(const QString& filename);

    void beginUpdate();
    void endUpdate();
    void applySettings();

private:
    QTabWidget* getNewEditorPageControl() const;
    QTabWidget* getFocusedPageControl() const;


private:
    ShowType mLayout;
    QTabWidget *mLeftPageWidget;
    QTabWidget *mRightPageWidget;
    QSplitter *mSplitter;
    QWidget *mPanel;
    int mUpdateCount;



};

#endif // EDITORLIST_H
