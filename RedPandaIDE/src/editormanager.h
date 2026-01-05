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
#ifndef EDITORMANAGER_H
#define EDITORMANAGER_H

#include <QTabWidget>
#include <QSplitter>
#include <QWidget>
#include <QRecursiveMutex>
#include "utils.h"
#include "editor.h"

class MainWindow;
class Project;
class EditorManager : public QObject
{
    Q_OBJECT
public:
    enum class LayoutShowType{
        lstLeft,
        lstRight,
        lstBoth
    };

    explicit EditorManager(QTabWidget* leftPageWidget,
                        QTabWidget* rightPageWidget,
                        QSplitter* splitter,
                        QWidget* panel, QObject* parent = nullptr);

    Editor* newEditor(const QString& filename, const QByteArray& encoding,
                      FileType fileType, const QString& contextFile,
                     Project *pProject, bool newFile,
                     QTabWidget* page=nullptr);

    Editor* getEditor(int index=-1, QTabWidget* tabsWidget=nullptr) const;

    bool closeEditor(Editor* editor, bool transferFocus=true, bool force=false);

    bool swapEditor(Editor* editor);
    void activeEditor(Editor *e, bool focus);
    void activeEditorAndSetCaret(Editor *e, QSynedit::CharPos pos);

    void saveAll();
    bool saveAllForProject();

    bool projectEditorsModified();
    void clearProjectEditorsModified();

    bool closeAll(bool force = false);
    bool closeOthers(Editor* editor);

    void forceCloseEditor(Editor* editor);

    Editor* getOpenedEditor(const QString &filename) const;

    bool getContentFromOpenedEditor(const QString& filename, QStringList& buffer) const;

    void getVisibleEditors(Editor*& left, Editor*& right) const;
    void updateLayout();

    MainWindow * mainWindow();


    QTabWidget *findPageControlForEditor(Editor *e);

    void updateEditorBookmarks();
    void updateEditorBreakpoints();

    bool debuggerReadyForEvalTip();
    bool requestEvalTip(Editor *e, const QString& s);
    void onEditorTipEvalValueReady(Editor *e);

    void beginUpdate();
    void endUpdate();
    void applySettings();
    void applyColorSchemes(const QString& name);
    bool isFileOpened(const QString& fullfilepath) const;
    bool hasFilename(const QString& filename) const;
    int pageCount() const;
    void selectNextPage();
    void selectPreviousPage();

    Editor* operator[](int index);

    QTabWidget *leftPageWidget() const;

    QTabWidget *rightPageWidget() const;

    PCppParser sharedParser(ParserLanguage language);

signals:
    void editorClosed();
    void editorOpenned();
private:
    QTabWidget* getNewEditorPageControl() const;
    QTabWidget* getFocusedPageControl() const;
    void showLayout(LayoutShowType layout);
    void doRemoveEditor(Editor* e);
private slots:
    void updateEditorTabCaption(Editor* e);
    void onBreakpointAdded(const Editor* e, int line);
    void onBreakpointRemoved(const Editor* e, int line);
    void onBreakpointsCleared(const Editor* e);
    void onEditorShown(Editor *e);
    void onFileSaving(Editor *e, const QString& filename);
    void onFileSaved(Editor *e, const QString& filename);
    void onFileRenamed(Editor *e, const QString &oldFilename, const QString &newFilename);
    void onFileSaveError(Editor *e, const QString& filename, const QString& reason);
    void onEditorLinesInserted(int startLine, int count);
    void onEditorLinesRemoved(int startLine, int count);
    void onEditorLineMoved(int fromLine, int toLine);
    void onEditorStatusChanged(QSynedit::StatusChanges changes);
    void onEditorFontSizeChangedByWheel(int newSize);
private:
    LayoutShowType mLayout;
    QTabWidget *mLeftPageWidget;
    QTabWidget *mRightPageWidget;
    QSplitter *mSplitter;
    QWidget *mPanel;
    int mUpdateCount;
    QHash<ParserLanguage,std::weak_ptr<CppParser>> mSharedParsers;
    mutable QRecursiveMutex mMutex;
};

#endif // EDITORLIST_H
