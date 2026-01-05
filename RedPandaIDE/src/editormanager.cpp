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
#include "editormanager.h"
#include "editor.h"
#include <QMessageBox>
#include <QVariant>
#include "mainwindow.h"
#include <QFileInfo>
#include "settings.h"
#include "project.h"
#include "systemconsts.h"
#include "visithistorymanager.h"
#include "debugger/debugger.h"
#include <QApplication>

EditorManager::EditorManager(QTabWidget* leftPageWidget,
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

Editor* EditorManager::newEditor(const QString& filename, const QByteArray& encoding,
                              FileType fileType, const QString& contextFile,
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
    Editor * e = new Editor(parentPageControl);
    e->setGetSharedParserFunc(std::bind(&EditorManager::sharedParser,this,std::placeholders::_1));
    e->setGetOpennedFunc(std::bind(&EditorManager::getOpenedEditor,this,std::placeholders::_1));
    e->setGetFileStreamCallBack(std::bind(
                                    &EditorManager::getContentFromOpenedEditor,this,
                                    std::placeholders::_1, std::placeholders::_2));
    e->setCanShowEvalTipFunc(std::bind(&EditorManager::debuggerReadyForEvalTip,this));
    e->setRequestEvalTipFunc(std::bind(&EditorManager::requestEvalTip,this,
                                       std::placeholders::_1, std::placeholders::_2));
    e->setEvalTipReadyCallback(std::bind(&EditorManager::onEditorTipEvalValueReady,
                                         this, std::placeholders::_1));

    e->setCodeSnippetsManager(pMainWindow->codeSnippetManager());
    e->setFileSystemWatcher(pMainWindow->fileSystemWatcher());
    e->applySettings();
    e->setEncodingOption(encoding);
    e->setFilename(filename);
    if (!newFile) {
        e->loadFile(filename);
        e->setFileType(fileType);
        e->setContextFile(contextFile);
    }
    e->setProject(pProject);

    if (!newFile) {
        e->resetBookmarks(pMainWindow->bookmarkModel());
        e->resetBreakpoints(pMainWindow->debugger()->breakpointModel().get());
    }
    e->setStatementColors(pMainWindow->statementColors());
    QString fileTemplate;
    switch (e->fileType()) {
    case FileType::CSource:
        fileTemplate = pMainWindow->codeSnippetManager()->newCFileTemplate();
        break;
    case FileType::CppSource:
        fileTemplate = pMainWindow->codeSnippetManager()->newCppFileTemplate();
        break;
    case FileType::ATTASM:
        fileTemplate = pMainWindow->codeSnippetManager()->newGASFileTemplate();
        break;
    default:
        break;
    }
    if (!fileTemplate.isEmpty()) {
        e->insertCodeSnippet(fileTemplate);
        e->setCaretPosition(e->fileBegin());
        e->setModified(false);
    }
    e->setAutoBackupEnabled(true);
    parentPageControl->addTab(e, e->caption());
    updateLayout();
    e->setFunctionTooltip(pMainWindow->functionTip());
    e->setCompletionPopup(pMainWindow->completionPopup());
    e->setHeaderCompletionPopup(pMainWindow->headerCompletionPopup());

    connect(e, &Editor::breakpointAdded, this, &EditorManager::onBreakpointAdded);
    connect(e, &Editor::breakpointRemoved, this, &EditorManager::onBreakpointRemoved);
    connect(e, &Editor::breakpointsCleared, this, &EditorManager::onBreakpointsCleared);
    connect(e, &Editor::showOccured, this, &EditorManager::onEditorShown);
    connect(e, &Editor::fileSaving, this, &EditorManager::onFileSaving);
    connect(e, &Editor::fileSaved, this, &EditorManager::onFileSaved);
    connect(e, &Editor::fileRenamed, this, &EditorManager::onFileRenamed);
    connect(e, &Editor::linesDeleted, this, &EditorManager::onEditorLinesRemoved);
    connect(e, &Editor::linesInserted, this, &EditorManager::onEditorLinesInserted);
    connect(e, &Editor::lineMoved, this, &EditorManager::onEditorLineMoved);
    connect(e, &Editor::statusChanged, this, &EditorManager::onEditorStatusChanged);
    connect(e, &Editor::fontSizeChangedByWheel, this, &EditorManager::onEditorFontSizeChangedByWheel);

    connect(e, &Editor::syntaxCheckRequested, pMainWindow, &MainWindow::checkSyntaxInBack);
    connect(e, &Editor::parseTodoRequested, pMainWindow->todoParser().get(), &TodoParser::parseFile);
    connect(e, &Editor::updateEncodingInfoRequested, pMainWindow, &MainWindow::updateForEncodingInfo);
    connect(e, &Editor::focusInOccured, pMainWindow, &MainWindow::refreshInfosForEditor);
    connect(e, &Editor::closeOccured, pMainWindow, &MainWindow::removeInfosForEditor);
    connect(e, &Editor::hideOccured, pMainWindow, &MainWindow::removeInfosForEditor);
    connect(e, &QWidget::customContextMenuRequested, pMainWindow, &MainWindow::onEditorContextMenu);
    connect(e, &Editor::openFileRequested, pMainWindow, &MainWindow::onOpenFileRequested);
    connect(e, &Editor::symbolChoosed, pMainWindow->symbolUsageManager(), &SymbolUsageManager::updateUsage);

    if (!pMainWindow->openingFiles()
            && !pMainWindow->openingProject()) {
        if (e->inProject()) {
            e->reparse(false);
            e->reparseTodo();
        }
        //checkSyntaxInBack();
    }
    return e;
}

QTabWidget*  EditorManager::getNewEditorPageControl() const {
    return getFocusedPageControl();
}

QTabWidget* EditorManager::getFocusedPageControl() const {
    switch(mLayout) {
    case LayoutShowType::lstLeft:
        return mLeftPageWidget;
    case LayoutShowType::lstRight:
        return mRightPageWidget;
    case LayoutShowType::lstBoth: {
        Editor* rightEditor = dynamic_cast<Editor*>(mRightPageWidget->currentWidget());
        if (!rightEditor)
            return mLeftPageWidget;
        if (rightEditor->hasFocus())
            return mRightPageWidget;
        Editor *leftEditor = dynamic_cast<Editor*>(mLeftPageWidget->currentWidget());
        if (!leftEditor)
            return mRightPageWidget;
        if (leftEditor->hasFocus())
            return mLeftPageWidget;
        if (rightEditor->lastFocusOutTime() > leftEditor->lastFocusOutTime())
            return mRightPageWidget;
        return mLeftPageWidget;
    }
    default:
        return nullptr;
    }
}

void EditorManager::showLayout(LayoutShowType layout)
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

void EditorManager::doRemoveEditor(Editor *e)
{
    QTabWidget* parentPage=findPageControlForEditor(e);
    if (parentPage) {
        int index = parentPage->indexOf(e);
        parentPage->removeTab(index);
    }
    pMainWindow->fileSystemWatcher()->removePath(e->filename());
    pMainWindow->caretList().removeEditor(e);
    pMainWindow->updateCaretActions();
    e->setParent(nullptr);
    delete e;
}

void EditorManager::updateEditorTabCaption(Editor* e)
{
    QTabWidget *parentWidget = mLeftPageWidget;
    int index = mLeftPageWidget->indexOf(e);
    if (index==-1) {
        index = mRightPageWidget->indexOf(e);
        parentWidget = mRightPageWidget;
    }
    if (index==-1)
        return;
    parentWidget->setTabText(index,e->caption());
    parentWidget->setTabToolTip(index, e->filename());
}

void EditorManager::onBreakpointAdded(const Editor *e, int line)
{
    pMainWindow->debugger()->addBreakpoint(line,e->filename(),e->inProject());
}

void EditorManager::onBreakpointRemoved(const Editor *e, int line)
{
    pMainWindow->debugger()->removeBreakpoint(line,e->filename(), e->inProject());
}

void EditorManager::onBreakpointsCleared(const Editor *e)
{
    pMainWindow->debugger()->deleteBreakpoints(e->filename(), e->inProject());
}

void EditorManager::onEditorShown(Editor *e)
{
    Q_ASSERT(e!=nullptr);
    if (e->parser() && !pMainWindow->isClosingAll()
            && !pMainWindow->isQuitting()) {
        if (!pMainWindow->openingFiles() && !pMainWindow->openingProject()) {
            if (pSettings->codeCompletion().clearWhenEditorHidden()
                && pSettings->codeCompletion().shareParser()
                && !e->inProject()) {
                e->resetParserIfNeeded();
            }
            e->reparseIfNeeded();
        }
    }
    pMainWindow->debugger()->setIsForProject(e->inProject());
    pMainWindow->bookmarkModel()->setIsForProject(e->inProject());
    pMainWindow->todoModel()->setIsForProject(e->inProject());

    if (!pMainWindow->isClosingAll()
                && !pMainWindow->isQuitting()
            && !pMainWindow->openingFiles()
            && !pMainWindow->openingProject()) {
        if (!e->inProject() || !pMainWindow->closingProject()) {
            e->checkSyntaxInBack();
            e->reparseTodo();
        }
    }
    if (e->inProject() && !pMainWindow->closingProject()) {
        pMainWindow->setProjectCurrentFile(e->filename());
    }
}

void EditorManager::onFileSaving(Editor *e, const QString &filename)
{
    Q_UNUSED(e);
    Q_UNUSED(filename);
}

void EditorManager::onFileSaved(Editor *e, const QString &filename)
{
    pMainWindow->onFileSaved(filename, e->inProject());
}

void EditorManager::onFileRenamed(Editor *e, const QString &oldFilename, const QString &newFilename)
{
    pMainWindow->getOJProblemSetModel()->updateProblemAnswerFilename(oldFilename, newFilename);
    if (!e->inProject()) {
        pMainWindow->bookmarkModel()->renameBookmarkFile(oldFilename,newFilename,false);
        pMainWindow->debugger()->breakpointModel()->renameBreakpointFilenames(oldFilename,newFilename,false);
    }
}

void EditorManager::onFileSaveError(Editor *e, const QString& filename, const QString& reason)
{
    Q_UNUSED(e);
    Q_UNUSED(reason);
    Q_UNUSED(filename);
}

void EditorManager::onEditorLinesInserted(int startLine, int count)
{
    Editor * e = static_cast<Editor *>(sender());
    pMainWindow->caretList().onLinesInserted(e,startLine,count);
    pMainWindow->debugger()->breakpointModel()->onFileInsertLines(e->filename(), startLine,count, e->inProject());
    pMainWindow->bookmarkModel()->onFileInsertLines(e->filename(), startLine,count, e->inProject());
    e->resetBreakpoints(pMainWindow->debugger()->breakpointModel().get());
    e->resetBookmarks(pMainWindow->bookmarkModel());
}

void EditorManager::onEditorLinesRemoved(int startLine, int count)
{
    Editor * e = static_cast<Editor *>(sender());
    pMainWindow->caretList().onLinesDeleted(e,startLine,count);
    pMainWindow->debugger()->breakpointModel()->onFileDeleteLines(e->filename(),startLine,count,e->inProject());
    pMainWindow->bookmarkModel()->onFileDeleteLines(e->filename(),startLine,count,e->inProject());
    e->resetBreakpoints(pMainWindow->debugger()->breakpointModel().get());
    e->resetBookmarks(pMainWindow->bookmarkModel());
}

void EditorManager::onEditorLineMoved(int fromLine, int toLine)
{
    Editor * e = static_cast<Editor *>(sender());
    pMainWindow->caretList().onLinesMoved(e, fromLine, toLine);
    pMainWindow->debugger()->breakpointModel()->onFileLineMoved(e->filename(),fromLine,toLine,e->inProject());
    pMainWindow->bookmarkModel()->onFileDeleteLines(e->filename(),fromLine,toLine,e->inProject());

    e->resetBreakpoints(pMainWindow->debugger()->breakpointModel().get());
    e->resetBookmarks(pMainWindow->bookmarkModel());
}

void EditorManager::onEditorStatusChanged(QSynedit::StatusChanges changes)
{
    Editor *e = static_cast<Editor *>(sender());
    if (changes.testFlag(QSynedit::StatusChange::CaretX)
            || changes.testFlag(QSynedit::StatusChange::CaretY)) {
        pMainWindow->updateStatusbarForLineCol(e);
    }
    if (changes.testFlag(QSynedit::StatusChange::InsertMode) || changes.testFlag(QSynedit::StatusChange::ReadOnlyChanged)) {
        pMainWindow->updateForStatusbarModeInfo(e);
    }
    if (changes.testFlag(QSynedit::StatusChange::ModifyChanged)
            || changes.testFlag(QSynedit::StatusChange::ReadOnlyChanged)
            || changes.testFlag(QSynedit::StatusChange::Custom)) {
        updateEditorTabCaption(e);
    }
    if (changes.testFlag(QSynedit::StatusChange::ModifyChanged)
        || changes.testFlag(QSynedit::StatusChange::Modified)
        || changes.testFlag(QSynedit::StatusChange::Selection)
        || changes.testFlag(QSynedit::StatusChange::ReadOnlyChanged)
            || changes.testFlag(QSynedit::StatusChange::Custom)) {
        pMainWindow->updateEditorActions(e);
    }

    if (changes.testFlag(QSynedit::StatusChange::CaretY)) {
        pMainWindow->caretList().addCaret(e,e->caretY(),e->caretX());
        pMainWindow->updateCaretActions();
    }
}

void EditorManager::onEditorFontSizeChangedByWheel(int newSize)
{
    pSettings->editor().setFontSize(newSize);
    pSettings->editor().save();
    pMainWindow->updateEditorSettings();
}


QTabWidget *EditorManager::rightPageWidget() const
{
    return mRightPageWidget;
}

PCppParser EditorManager::sharedParser(ParserLanguage language)
{
    PCppParser parser;
    if (mSharedParsers.contains(language)) {
        parser=mSharedParsers[language].lock();
    }
    if (!parser) {
        parser = std::make_shared<CppParser>();
        parser->setLanguage(language);
        parser->setOnGetFileStream(
                    std::bind(
                        &EditorManager::getContentFromOpenedEditor,this,
                        std::placeholders::_1, std::placeholders::_2));
        resetCppParser(parser);
        parser->setEnabled(true);
        mSharedParsers.insert(language,parser);
    }
    return parser;
}

QTabWidget *EditorManager::leftPageWidget() const
{
    return mLeftPageWidget;
}

Editor* EditorManager::getEditor(int index, QTabWidget* tabsWidget) const {
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

bool EditorManager::closeEditor(Editor* editor, bool transferFocus, bool force) {
    QMutexLocker locker(&mMutex);
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
            activeEditor(editor,true);
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

bool EditorManager::swapEditor(Editor *editor)
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(editor!=nullptr);
    beginUpdate();
    auto action = finally([this](){
        endUpdate();
    });
    //remember old index
    QTabWidget* fromPageControl = findPageControlForEditor(editor);
    if (fromPageControl == mLeftPageWidget) {
        mLeftPageWidget->removeTab(mLeftPageWidget->indexOf(editor));
        mRightPageWidget->addTab(editor, editor->caption());
    } else {
        mRightPageWidget->removeTab(mRightPageWidget->indexOf(editor));
        mLeftPageWidget->addTab(editor, editor->caption());
    }
    updateLayout();
    activeEditor(editor, true);
    return true;
}

void EditorManager::saveAll()
{
    for (int i=0;i<pageCount();i++) {
        Editor * e= (*this)[i];
        if (e->modified())
            e->save();
    }
}

bool EditorManager::saveAllForProject()
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

bool EditorManager::projectEditorsModified()
{
    for (int i=0;i<pageCount();i++) {
        Editor * e= (*this)[i];
        if (e->modified() && e->inProject()) {
            return true;
        }
    }
    return false;
}

void EditorManager::clearProjectEditorsModified()
{
    for (int i=0;i<pageCount();i++) {
        Editor * e= (*this)[i];
        if (e->inProject()) {
            e->setModified(false);
        }
    }
}

void EditorManager::beginUpdate() {
    if (mUpdateCount==0) {
        mPanel->setUpdatesEnabled(false);
    }
    mUpdateCount++;
}

void EditorManager::endUpdate() {
    mUpdateCount--;
    if (mUpdateCount==0) {
        mPanel->setUpdatesEnabled(true);
        mPanel->update();
    }
}

void EditorManager::applySettings()
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

void EditorManager::applyColorSchemes(const QString& name)
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

bool EditorManager::isFileOpened(const QString &fullfilepath) const
{
    QFileInfo fileInfo(fullfilepath);
    QString filename = fileInfo.absoluteFilePath();
    for (int i=0;i<mLeftPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mLeftPageWidget->widget(i));
        if (e->filename().compare(filename)==0 || e->filename().compare(fullfilepath)==0)
            return true;
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mRightPageWidget->widget(i));
        if (e->filename().compare(filename)==0 || e->filename().compare(fullfilepath)==0)
            return true;
    }
    return false;
}

bool EditorManager::hasFilename(const QString &filename) const
{
    for (int i=0;i<mLeftPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mLeftPageWidget->widget(i));
        QFileInfo fileInfo(e->filename());
        QString name = fileInfo.fileName();
        if (name.compare(filename, PATH_SENSITIVITY)==0 )
            return true;
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        Editor* e = static_cast<Editor*>(mRightPageWidget->widget(i));
        QFileInfo fileInfo(e->filename());
        QString name = fileInfo.fileName();
        if (name.compare(filename, PATH_SENSITIVITY)==0 )
            return true;
    }
    return false;
}

int EditorManager::pageCount() const
{
    return mLeftPageWidget->count()+mRightPageWidget->count();
}

void EditorManager::selectNextPage()
{
    QTabWidget * pageControl = getFocusedPageControl();
    if (pageControl && pageControl->count()>0) {
        pageControl->setCurrentIndex(
                    (pageControl->currentIndex()+1) % pageControl->count()
                    );
    }
}

void EditorManager::selectPreviousPage()
{
    QTabWidget * pageControl = getFocusedPageControl();
    if (pageControl && pageControl->count()>0) {
        pageControl->setCurrentIndex(
                    (pageControl->currentIndex()+pageControl->count()-1) % pageControl->count()
                    );
    }
}

void EditorManager::activeEditor(Editor *e, bool focus)
{
    if (e==nullptr)
        return;
    QTabWidget * pageControl = findPageControlForEditor(e);
    if (pageControl!=nullptr) {
        pageControl->setCurrentWidget(e);
        if (focus)
            e->setFocus();
    }
}

void EditorManager::activeEditorAndSetCaret(Editor *e, QSynedit::CharPos pos)
{
    if (e) {
        e->setCaretPosition(pos);
        activeEditor(e,true);
    }
}

Editor *EditorManager::operator[](int index)
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

bool EditorManager::closeAll(bool force) {
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

bool EditorManager::closeOthers(Editor *editor)
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

void EditorManager::forceCloseEditor(Editor *editor)
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    doRemoveEditor(editor);
    // Force layout update when creating, destroying or moving editors
    updateLayout();
    endUpdate();
    emit editorClosed();
}

Editor* EditorManager::getOpenedEditor(const QString &filename) const
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

bool EditorManager::getContentFromOpenedEditor(const QString &filename, QStringList &buffer) const
{
    if(mMutex.tryLock(0)){
        auto action = finally([this](){
            mMutex.unlock();
        });
        if (pMainWindow->isQuitting())
            return false;
        Editor * e= getOpenedEditor(filename);
        if (!e)
            return false;
        buffer = e->content();
        return true;
    } else
        return false;
}

void EditorManager::getVisibleEditors(Editor *&left, Editor *&right) const
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

void EditorManager::updateLayout()
{
    if (mRightPageWidget->count() == 0)
        showLayout(LayoutShowType::lstLeft);
    else if (mLeftPageWidget->count() ==0)
        showLayout(LayoutShowType::lstRight);
    else
        showLayout(LayoutShowType::lstBoth);
}

MainWindow *EditorManager::mainWindow()
{
    return pMainWindow;
}

QTabWidget *EditorManager::findPageControlForEditor(Editor *e)
{
    if (mLeftPageWidget->indexOf(e)!=-1)
        return mLeftPageWidget;
    if (mRightPageWidget->indexOf(e)!=-1)
        return mRightPageWidget;
    return nullptr;
}

void EditorManager::updateEditorBookmarks()
{
    for (int i=0;i<mLeftPageWidget->count();i++) {
        Editor * e = static_cast<Editor*>(mLeftPageWidget->widget(i));
        e->resetBookmarks(pMainWindow->bookmarkModel());
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        Editor * e = static_cast<Editor*>(mRightPageWidget->widget(i));
        e->resetBookmarks(pMainWindow->bookmarkModel());
    }
}

void EditorManager::updateEditorBreakpoints()
{
    for (int i=0;i<mLeftPageWidget->count();i++) {
        Editor * e = static_cast<Editor*>(mLeftPageWidget->widget(i));
        e->resetBreakpoints(pMainWindow->debugger()->breakpointModel().get());
    }
    for (int i=0;i<mRightPageWidget->count();i++) {
        Editor * e = static_cast<Editor*>(mRightPageWidget->widget(i));
        e->resetBreakpoints(pMainWindow->debugger()->breakpointModel().get());
    }
}

bool EditorManager::debuggerReadyForEvalTip()
{
    return (pMainWindow->debugger()->executing() && !pMainWindow->debugger()->inferiorRunning());
}

bool EditorManager::requestEvalTip(Editor *e, const QString &s)
{
    if (pMainWindow->debugger()->commandRunning())
        return false;
    connect(pMainWindow->debugger(), &Debugger::evalValueReady,
               e, &Editor::onTipEvalValueReady);
    pMainWindow->debugger()->evalExpression(s);
    return true;
}

void EditorManager::onEditorTipEvalValueReady(Editor *e)
{
    disconnect(pMainWindow->debugger(), &Debugger::evalValueReady,
               e, &Editor::onTipEvalValueReady);
}
