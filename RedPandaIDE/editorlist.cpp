#include "editorlist.h"
#include "editor.h"
#include <QVariant>

EditorList::EditorList(QTabWidget* leftPageWidget,
      QTabWidget* rightPageWidget,
      QSplitter* splitter,
      QWidget* panel):
    mLeftPageWidget(leftPageWidget),
    mRightPageWidget(rightPageWidget),
    mSplitter(splitter),
    mPanel(panel),
    mUpdateCount(0)
{

}

Editor* EditorList::newEditor(const QString& filename, const QByteArray& encoding,
                 bool inProject, bool newFile,
                 QTabWidget* page) {
    QTabWidget * parentPageControl = NULL;
    if (page == NULL)
        parentPageControl = getNewEditorPageControl();
    else
        parentPageControl = page;
    return new Editor(parentPageControl,filename,encoding,inProject,newFile,parentPageControl);
    //UpdateLayout;
}

QTabWidget*  EditorList::getNewEditorPageControl() {
    //todo: return widget depends on layout
    return mLeftPageWidget;
}

Editor* EditorList::getEditor(int index, QTabWidget* tabsWidget) const {
    QTabWidget* selectedWidget;
    if (tabsWidget == NULL) {
        selectedWidget = mLeftPageWidget; // todo: get focused widget
    } else {
        selectedWidget = tabsWidget;
    }
    QWidget* textEdit;
    if (index == -1) {
        textEdit = selectedWidget->currentWidget();
    } else {
        textEdit =selectedWidget->widget(index);
    }
    QVariant pop = textEdit->property("editor");
    Editor *editor = (Editor*)pop.value<intptr_t>();
    return editor;
}

bool EditorList::closeEditor(Editor* editor, bool transferFocus, bool force) {
    delete editor;
    return true;
}
