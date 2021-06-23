#include "editorlist.h"
#include "editor.h"
#include <QMessageBox>
#include <QVariant>
#include <mainwindow.h>
#include <iconv.h>
#include <QDebug>
#include <QFileInfo>

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
    beginUpdate();
    auto end = finally([this] {
        this->endUpdate();
    });
    if (editor == NULL)
        return false;
    if (force) {
        editor->save(true,false);
    } else if ( (editor->modified()) && (!editor->empty())) {
        // ask user if he wants to save
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(pMainWindow,QObject::tr("Save"),
                                      QString(QObject::tr("Save changes to %1?")).arg(editor->filename()),
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

void EditorList::applySettings()
{
    for (int i=0;i<mLeftPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mLeftPageWidget->widget(i));
        e->applySettings();
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mRightPageWidget->widget(i));
        e->applySettings();
    }
}

void EditorList::applyColorSchemes(const QString& name)
{
    for (int i=0;i<mLeftPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mLeftPageWidget->widget(i));
        e->applyColorScheme(name);
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mRightPageWidget->widget(i));
        e->applyColorScheme(name);
    }
}

bool EditorList::isFileOpened(const QString &name)
{
    QFileInfo fileInfo(name);
    QString filename = fileInfo.absoluteFilePath();
    for (int i=0;i<mLeftPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mLeftPageWidget->widget(i));
        if (e->filename().compare(filename)==0)
            return true;
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mRightPageWidget->widget(i));
        if (e->filename().compare(filename)==0)
            return true;
    }
    return false;
}

bool EditorList::closeAll(bool force) {
    beginUpdate();
    auto end = finally([this] {
        this->endUpdate();
    });
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

Editor* EditorList::getOpenedEditorByFilename(const QString &filename)
{
    for (int i=0;i<mLeftPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mLeftPageWidget->widget(i));
        if (e->filename() == filename) {
            return e;
        }
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mRightPageWidget->widget(i));
        if (e->filename() == filename) {
            return e;
        }
    }
    return nullptr;
}

Editor *EditorList::getEditorByFilename(const QString &filename)
{
    //check if an editor is already openned
    Editor* e=getOpenedEditorByFilename(filename);
    if (e!=nullptr)
        return e;
    //Todo: check if is in the project

    //Create a new editor
    return newEditor(filename,ENCODING_AUTO_DETECT,false,false);
}
