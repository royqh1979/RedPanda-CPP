#include "editorlist.h"
#include "editor.h"
#include <QMessageBox>
#include <QVariant>
#include <mainwindow.h>
#include <iconv.h>

EditorList::UpdateLocker::UpdateLocker(EditorList* editorList): mEditorList(editorList){
    mEditorList->beginUpdate();
}

EditorList::UpdateLocker::~UpdateLocker() {
    mEditorList->endUpdate();
}

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

QTabWidget*  EditorList::getNewEditorPageControl() const {
    //todo: return widget depends on layout
    return mLeftPageWidget;
}

QTabWidget* EditorList::getFocusedPageControl() const {
    //todo:
    return mLeftPageWidget;
}

Editor* EditorList::getEditor(int index, QTabWidget* tabsWidget) const {
    QTabWidget* selectedWidget;
    if (tabsWidget == NULL) {
        selectedWidget = getFocusedPageControl(); // todo: get focused widget
    } else {
        selectedWidget = tabsWidget;
    }
    if (index == -1) {
        index = selectedWidget->currentIndex();
    }
    if (index<0 || index >= selectedWidget->count()) {
        return NULL;
    }
    return (Editor*)selectedWidget->widget(index);
}

bool EditorList::closeEditor(Editor* editor, bool transferFocus, bool force) {
    UpdateLocker locker(this); // use RAII to correctly pause/resume update of the panel widget
    if (editor == NULL)
        return false;
    if (force) {
        editor->save(true,false);
    } else if ( (editor->isModified()) && (!editor->text().isEmpty())) {
        // ask user if he wants to save
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(pMainWindow,QObject::tr("Save"),QObject::tr("Save changes to %s?"),
                                      QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
        if (reply == QMessageBox::Cancel) {
            return false;
        } else if (reply == QMessageBox::Yes) {
            if (!editor->save(false,false)) {
                return false;
            }
        }
    }

    if (transferFocus && (editor->pageControl()->currentWidget()==editor)) {
        //todo: activate & focus the previous editor
    }

    delete editor;
    return true;
}

void EditorList::beginUpdate() {
    if (mUpdateCount==0) {
        mPanel->setUpdatesEnabled(false);
    }
    mUpdateCount++;
}

void EditorList::endUpdate() {
    mUpdateCount--;
    if (mUpdateCount==0) {
        mPanel->setUpdatesEnabled(true);
        mPanel->update();
    }
}

bool EditorList::closeAll(bool force) {
    UpdateLocker locker(this);
    while (mLeftPageWidget->count()>0) {
        if (!closeEditor(getEditor(0,mLeftPageWidget),false,force)) {
            return false;
        }
    }
    while (mRightPageWidget->count()>0) {
        if (!closeEditor(getEditor(0,mRightPageWidget),false,force)) {
            return false;
        }
    }

    return true;
}
