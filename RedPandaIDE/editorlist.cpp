#include "editorlist.h"
#include "editor.h"
#include <QMessageBox>
#include <QVariant>
#include <mainwindow.h>
#include <QDebug>
#include <QFileInfo>
#include "settings.h"

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
    if (!filename.isEmpty() && QFile(filename).exists()) {
        pMainWindow->fileSystemWatcher()->addPath(filename);
    }
    return new Editor(parentPageControl,filename,encoding,inProject,newFile,parentPageControl);
    updateLayout();
}

QTabWidget*  EditorList::getNewEditorPageControl() const {
    //todo: return widget depends on layout
    return mLeftPageWidget;
}

QTabWidget* EditorList::getFocusedPageControl() const {
    //todo:
    return mLeftPageWidget;
}

void EditorList::showLayout(LayoutShowType layout)
{
    if (layout == mLayout)
        return;
    mLayout = layout;
    // Apply widths if layout does not change
    switch(mLayout) {
    case LayoutShowType::lstLeft:
    case LayoutShowType::lstNone:
        mLeftPageWidget->setVisible(true);
        mRightPageWidget->setVisible(false);
        mSplitter->setVisible(false);
        break;
    case LayoutShowType::lstRight:
        mLeftPageWidget->setVisible(false);
        mRightPageWidget->setVisible(true);
        mSplitter->setVisible(false);
        break;
    case LayoutShowType::lstBoth:
        mLeftPageWidget->setVisible(true);
        mRightPageWidget->setVisible(true);
        mSplitter->setVisible(true);
    }
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

    if (pSettings->history().addToOpenedFiles(editor->filename())) {
        pMainWindow->rebuildOpenedFileHisotryMenu();
        updateLayout();
    }

    pMainWindow->fileSystemWatcher()->removePath(editor->filename());
    delete editor;

    editor = getEditor();
    if (!force)
        pMainWindow->updateClassBrowserForEditor(editor);
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
        if (e->filename().compare(filename)==0 || e->filename().compare(name)==0)
            return true;
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mRightPageWidget->widget(i));
        if (e->filename().compare(filename)==0 || e->filename().compare(name)==0)
            return true;
    }
    return false;
}

int EditorList::pageCount()
{
    return mLeftPageWidget->count()+mRightPageWidget->count();
}

void EditorList::selectNextPage()
{
    QTabWidget * pageControl = getFocusedPageControl();
    if (pageControl && pageControl->count()>0) {
        pageControl->setCurrentIndex(
                    (pageControl->currentIndex()+1) % pageControl->count()
                    );
    }
}

void EditorList::selectPreviousPage()
{
    QTabWidget * pageControl = getFocusedPageControl();
    if (pageControl && pageControl->count()>0) {
        pageControl->setCurrentIndex(
                    (pageControl->currentIndex()+pageControl->count()-1) % pageControl->count()
                    );
    }
}

Editor *EditorList::operator[](int index)
{
    if (index>=0 && index<mLeftPageWidget->count()) {
        return static_cast<Editor*>(mLeftPageWidget->widget(index));
    }
    index -= mLeftPageWidget->count();
    if (index>=0 && index<mRightPageWidget->count()) {
        return static_cast<Editor*>(mRightPageWidget->widget(index));
    }
    return nullptr;
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

void EditorList::forceCloseEditor(Editor *editor)
{
    beginUpdate();
    delete editor;
    // Force layout update when creating, destroying or moving editors
    updateLayout();
    endUpdate();
}

Editor* EditorList::getOpenedEditorByFilename(const QString &filename)
{
    QFileInfo fileInfo(filename);
    QString fullname = fileInfo.absoluteFilePath();
    for (int i=0;i<mLeftPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mLeftPageWidget->widget(i));
        if (e->filename().compare(filename)==0 || e->filename().compare(fullname)==0) {
            return e;
        }
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mRightPageWidget->widget(i));
        if (e->filename().compare(filename)==0 || e->filename().compare(fullname)==0) {
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
    QFileInfo fileInfo(filename);
    QString fullname = fileInfo.absoluteFilePath();
    if (fileInfo.exists())
        return newEditor(fullname,ENCODING_AUTO_DETECT,false,false);
    return nullptr;
}

bool EditorList::getContentFromOpenedEditor(const QString &filename, QStringList &buffer)
{
    Editor * e= getOpenedEditorByFilename(filename);
    if (!e)
        return false;
    buffer = e->lines()->contents();
    return true;
}

void EditorList::getVisibleEditors(Editor *&left, Editor *&right)
{
    switch(mLayout) {
    case LayoutShowType::lstNone:
        left = nullptr;
        right = nullptr;
        break;
    case LayoutShowType::lstLeft:
        left = getEditor(-1,mLeftPageWidget);
        right = nullptr;
        break;
    case LayoutShowType::lstRight:
        left = nullptr;
        right = getEditor(-1,mRightPageWidget);
        break;
    case LayoutShowType::lstBoth:
        left = getEditor(-1,mLeftPageWidget);
        right = getEditor(-1,mRightPageWidget);
        break;
    }
}

void EditorList::updateLayout()
{
    if (mLeftPageWidget->count() ==0 && mRightPageWidget->count() == 0)
        showLayout(LayoutShowType::lstNone);
    else if (mLeftPageWidget->count() > 0 && mRightPageWidget->count() == 0)
        showLayout(LayoutShowType::lstLeft);
    else if (mLeftPageWidget->count() ==0 && mRightPageWidget->count() > 0)
        showLayout(LayoutShowType::lstRight);
    else
        showLayout(LayoutShowType::lstBoth);
}
