/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "editorlist.h"
#include "editor.h"
#include <QMessageBox>
#include <QVariant>
#include <mainwindow.h>
#include <QFileInfo>
#include "settings.h"
#include "project.h"
#include "systemconsts.h"
#include "visithistorymanager.h"
#include <QApplication>

EditorList::EditorList(QTabWidget* leftPageWidget,
      QTabWidget* rightPageWidget,
      QSplitter* splitter,
      QWidget* panel,
      QObject* parent):
    QObject(parent),
    mLayout(LayoutShowType::lstLeft),
    mLeftPageWidget(leftPageWidget),
    mRightPageWidget(rightPageWidget),
    mSplitter(splitter),
    mPanel(panel),
    mUpdateCount(0)
{

}

Editor* EditorList::newEditor(const QString& filename, const QByteArray& encoding,
                 Project *pProject, bool newFile,
                 QTabWidget* page) {
    QTabWidget * parentPageControl = nullptr;
    if (page == nullptr)
        parentPageControl = getNewEditorPageControl();
    else
        parentPageControl = page;
    if (fileExists(filename)) {
        pMainWindow->fileSystemWatcher()->addPath(filename);
    }

    // parentPageControl takes the owner ship
    Editor * e = new Editor(parentPageControl,filename,encoding,pProject,newFile,parentPageControl);
    connect(e, &Editor::renamed, this, &EditorList::onEditorRenamed);
    updateLayout();
    connect(e,&Editor::fileSaved,
            pMainWindow, &MainWindow::onFileSaved);
    return e;
}

QTabWidget*  EditorList::getNewEditorPageControl() const {
    return getFocusedPageControl();
}

QTabWidget* EditorList::getFocusedPageControl() const {
    //todo:
    switch(mLayout) {
    case LayoutShowType::lstLeft:
        return mLeftPageWidget;
    case LayoutShowType::lstRight:
        return mRightPageWidget;
    case LayoutShowType::lstBoth: {
        Editor* editor = dynamic_cast<Editor*>(mRightPageWidget->currentWidget());
        if (editor && editor->hasFocus())
            return mRightPageWidget;
        return mLeftPageWidget;
    }
    default:
        return nullptr;
    }
}

void EditorList::showLayout(LayoutShowType layout)
{
    if (layout == mLayout)
        return;
    mLayout = layout;
    // Apply widths if layout does not change
    switch(mLayout) {
    case LayoutShowType::lstLeft:
        mLeftPageWidget->setVisible(true);
        mRightPageWidget->setVisible(false);
        break;
    case LayoutShowType::lstRight:
        mLeftPageWidget->setVisible(false);
        mRightPageWidget->setVisible(true);
        break;
    case LayoutShowType::lstBoth:
        mLeftPageWidget->setVisible(true);
        mRightPageWidget->setVisible(true);
        {
            QList<int> sizes=mSplitter->sizes();
            int total = sizes[0]+sizes[1];
            sizes[0] = total / 2;
            sizes[1] = total - sizes[0];
            mSplitter->setSizes(sizes);
        }
        break;
    }
}

void EditorList::doRemoveEditor(Editor *e)
{
    QTabWidget* parentPage=e->pageControl();
    int index = parentPage->indexOf(e);
    parentPage->removeTab(index);
    pMainWindow->fileSystemWatcher()->removePath(e->filename());
    pMainWindow->caretList().removeEditor(e);
    pMainWindow->updateCaretActions();
    e->setParent(nullptr);
    delete e;
}

void EditorList::onEditorRenamed(const QString &oldFilename, const QString &newFilename, bool firstSave)
{
    emit editorRenamed(oldFilename, newFilename, firstSave);
}

QTabWidget *EditorList::rightPageWidget() const
{
    return mRightPageWidget;
}

QTabWidget *EditorList::leftPageWidget() const
{
    return mLeftPageWidget;
}

Editor* EditorList::getEditor(int index, QTabWidget* tabsWidget) const {
    QTabWidget* selectedWidget;
    if (tabsWidget == nullptr) {
        selectedWidget = getFocusedPageControl();
    } else {
        selectedWidget = tabsWidget;
    }
    if (!selectedWidget)
        return nullptr;
    if (index == -1) {
        index = selectedWidget->currentIndex();
    }
    if (index<0 || index >= selectedWidget->count()) {
        return nullptr;
    }
    return (Editor*)selectedWidget->widget(index);
}

bool EditorList::closeEditor(Editor* editor, bool transferFocus, bool force) {
    if (editor == nullptr)
        return false;
    if (force) {
        editor->save(true,false);
    } else if ( (editor->modified()) && (!editor->empty())) {
        // ask user if he wants to save
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(editor,QObject::tr("Save"),
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

    beginUpdate();
//    if (transferFocus && (editor->pageControl()->currentWidget()==editor)) {
//        //todo: activate & focus the previous editor
//    }

    if (editor->inProject() && pMainWindow->project()) {
        if (fileExists(pMainWindow->project()->directory())) {
            PProjectUnit unit = pMainWindow->project()->findUnit(editor);
            pMainWindow->project()->closeUnit(unit);
        } else {
            editor->setProject(nullptr);
        }
    } else {
        if (!editor->isNew() && pMainWindow->visitHistoryManager()->addFile(editor->filename())) {
            pMainWindow->rebuildOpenedFileHisotryMenu();
        }
        editor->clearBreakpoints();
        doRemoveEditor(editor);
    }
    updateLayout();
    if (!force && transferFocus) {
        editor = getEditor();
        if (editor) {
            editor->activate();
            pMainWindow->updateClassBrowserForEditor(editor);
        }
    }
    editor = getEditor();
    if (!editor) {
        pMainWindow->updateClassBrowserForEditor(nullptr);
    }
    emit editorClosed();
    endUpdate();
    return true;
}

bool EditorList::swapEditor(Editor *editor)
{
    Q_ASSERT(editor!=nullptr);
    beginUpdate();
    auto action = finally([this](){
        endUpdate();
    });
    //remember old index
    QTabWidget* fromPageControl = editor->pageControl();
    if (fromPageControl == mLeftPageWidget) {
        editor->setPageControl(mRightPageWidget);
    } else {
        editor->setPageControl(mLeftPageWidget);
    }
    updateLayout();
    editor->activate();
    return true;
}

void EditorList::saveAll()
{
    for (int i=0;i<pageCount();i++) {
        Editor * e= (*this)[i];
        if (e->modified())
            e->save();
    }
}

bool EditorList::saveAllForProject()
{
    for (int i=0;i<pageCount();i++) {
        Editor * e= (*this)[i];
        if (e->modified() && e->inProject()) {
            if (!e->save())
                return false;
        }
    }
    return true;
}

bool EditorList::projectEditorsModified()
{
    for (int i=0;i<pageCount();i++) {
        Editor * e= (*this)[i];
        if (e->modified() && e->inProject()) {
            return true;
        }
    }
    return false;
}

void EditorList::clearProjectEditorsModified()
{
    for (int i=0;i<pageCount();i++) {
        Editor * e= (*this)[i];
        if (e->inProject()) {
            e->setModified(false);
        }
    }
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
//    beginUpdate();
//    auto end = finally([this] {
//        this->endUpdate();
//    });
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

bool EditorList::closeOthers(Editor *editor)
{
    QList<Editor*> editors;
    for (int i=0;i<mLeftPageWidget->count();i++) {
        editors.append(static_cast<Editor*>(mLeftPageWidget->widget(i)));
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        editors.append(static_cast<Editor*>(mRightPageWidget->widget(i)));
    }
    for (Editor* e: editors ) {
        if (e!=editor) {
            if (!closeEditor(e,false,false)) {
                return false;
            }
        }
    }
    return true;
}

void EditorList::forceCloseEditor(Editor *editor)
{
    beginUpdate();
    doRemoveEditor(editor);
    // Force layout update when creating, destroying or moving editors
    updateLayout();
    endUpdate();
    emit editorClosed();
}

Editor* EditorList::getOpenedEditorByFilename(QString filename)
{
    if (filename.isEmpty())
        return nullptr;
    for (int i=0;i<mLeftPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mLeftPageWidget->widget(i));
        if (!e)
            continue;
        if (e->filename().compare(filename, PATH_SENSITIVITY)==0) {
            return e;
        }
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mRightPageWidget->widget(i));
        if (!e)
            continue;
        if (e->filename().compare(filename)==0) {
            return e;
        }
    }
    return nullptr;
}

bool EditorList::getContentFromOpenedEditor(const QString &filename, QStringList &buffer)
{
    if (pMainWindow->isQuitting())
        return false;
    Editor * e= getOpenedEditorByFilename(filename);
    if (!e)
        return false;
    buffer = e->contents();
    return true;
}

void EditorList::getVisibleEditors(Editor *&left, Editor *&right)
{
    switch(mLayout) {
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
    if (mRightPageWidget->count() == 0)
        showLayout(LayoutShowType::lstLeft);
    else if (mLeftPageWidget->count() ==0)
        showLayout(LayoutShowType::lstRight);
    else
        showLayout(LayoutShowType::lstBoth);
}
