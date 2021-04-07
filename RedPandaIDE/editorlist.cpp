#include "editorlist.h"
#include "editor.h"

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

