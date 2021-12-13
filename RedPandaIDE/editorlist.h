#ifndef EDITORLIST_H
#define EDITORLIST_H

#include <QTabWidget>
#include <QSplitter>
#include <QWidget>
#include "utils.h"

class Editor;
class EditorList : public QObject
{
    Q_OBJECT
public:
    enum class LayoutShowType{
        lstLeft,
        lstRight,
        lstBoth
    };

    explicit EditorList(QTabWidget* leftPageWidget,
                        QTabWidget* rightPageWidget,
                        QSplitter* splitter,
                        QWidget* panel, QObject* parent = nullptr);

    Editor* newEditor(const QString& filename, const QByteArray& encoding,
                     bool inProject, bool newFile,
                     QTabWidget* page=nullptr);

    Editor* getEditor(int index=-1, QTabWidget* tabsWidget=nullptr) const;

    bool closeEditor(Editor* editor, bool transferFocus=true, bool force=false);

    bool swapEditor(Editor* editor);

    bool closeAll(bool force = false);

    void forceCloseEditor(Editor* editor);

    Editor* getOpenedEditorByFilename(QString filename);

    Editor* getEditorByFilename(QString filename);

    bool getContentFromOpenedEditor(const QString& filename, QStringList& buffer);

    void getVisibleEditors(Editor*& left, Editor*& right);
    void updateLayout();

    void beginUpdate();
    void endUpdate();
    void applySettings();
    void applyColorSchemes(const QString& name);
    bool isFileOpened(const QString& name);
    int pageCount();
    void selectNextPage();
    void selectPreviousPage();

    Editor* operator[](int index);

    QTabWidget *leftPageWidget() const;

    QTabWidget *rightPageWidget() const;

signals:
    void editorClosed();

private:
    QTabWidget* getNewEditorPageControl() const;
    QTabWidget* getFocusedPageControl() const;
    void showLayout(LayoutShowType layout);


private:
    LayoutShowType mLayout;
    QTabWidget *mLeftPageWidget;
    QTabWidget *mRightPageWidget;
    QSplitter *mSplitter;
    QWidget *mPanel;
    int mUpdateCount;



};

#endif // EDITORLIST_H
