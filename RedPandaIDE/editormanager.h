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

class Project;
class Editor;
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

    void saveAll();
    bool saveAllForProject();

    bool projectEditorsModified();
    void clearProjectEditorsModified();

    bool closeAll(bool force = false);
    bool closeOthers(Editor* editor);

    void forceCloseEditor(Editor* editor);

    Editor* getOpenedEditorByFilename(QString filename) const;

    bool getContentFromOpenedEditor(const QString& filename, QStringList& buffer) const;

    void getVisibleEditors(Editor*& left, Editor*& right) const;
    void updateLayout();


    QTabWidget *findPageControlForEditor(Editor *e);

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

signals:
    void editorCreated(Editor *);
    void editorClosed();
    void editorRenamed(const QString& oldFilename, const QString& newFilename, bool firstSave);
    void editorOpenned();

private:
    QTabWidget* getNewEditorPageControl() const;
    QTabWidget* getFocusedPageControl() const;
    void showLayout(LayoutShowType layout);
    void doRemoveEditor(Editor* e);
private slots:
    void onEditorRenamed(const QString& oldFilename, const QString& newFilename, bool firstSave);
    void onEditorCaptionUpdated(Editor* e);
private:
    LayoutShowType mLayout;
    QTabWidget *mLeftPageWidget;
    QTabWidget *mRightPageWidget;
    QSplitter *mSplitter;
    QWidget *mPanel;
    int mUpdateCount;
    mutable QRecursiveMutex mMutex;
};

#endif // EDITORLIST_H
