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

Editor* EditorList::NewEditor(const QString& filename, FileEncodingType encoding,
                 bool inProject, bool newFile,
                 QTabWidget* page) {
    QTabWidget * parentPageControl = NULL;
    if (page == NULL)
        parentPageControl = GetNewEditorPageControl();
    else
        parentPageControl = page;
    return new Editor(parentPageControl,filename,encoding,inProject,newFile,parentPageControl);
    //UpdateLayout;
}

QTabWidget*  EditorList::GetNewEditorPageControl() {
    //todo: return widget depends on layout
    return mLeftPageWidget;
}

Editor* EditorList::GetEditor(int index, QTabWidget* tabsWidget) const {
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

bool EditorList::CloseEditor(Editor* editor, bool transferFocus, bool force) {
    delete editor;
    return true;
}
