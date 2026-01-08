/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#ifndef UI_SETTINGS_H
#define UI_SETTINGS_H
#include "basesettings.h"

#define SETTING_UI "UI"

class UISettings: public BaseSettings {
public:
    explicit UISettings(SettingsPersistor *persistor);

    const QByteArray &mainWindowState() const;
    void setMainWindowState(const QByteArray &newMainWindowState);

    const QByteArray &mainWindowGeometry() const;
    void setMainWindowGeometry(const QByteArray &newMainWindowGeometry);

    int bottomPanelIndex() const;
    void setBottomPanelIndex(int newBottomPanelIndex);
    int leftPanelIndex() const;
    void setLeftPanelIndex(int newLeftPanelIndex);

    bool classBrowserSortAlpha() const;
    void setClassBrowserSortAlpha(bool newClassBrowserSortAlpha);

    bool classBrowserSortType() const;
    void setClassBrowserSortType(bool newClassBrowserSortType);

    bool classBrowserShowInherited() const;
    void setClassBrowserShowInherited(bool newClassBrowserShowInherited);

    bool showToolbar() const;
    void setShowToolbar(bool newShowToolbar);

    bool showStatusBar() const;
    void setShowStatusBar(bool newShowStatusBar);

    bool showToolWindowBars() const;
    void setShowToolWindowBars(bool newShowToolWindowBars);

    bool showProject() const;
    void setShowProject(bool newShowProject);

    bool showWatch() const;
    void setShowWatch(bool newShowWatch);

    bool showStructure() const;
    void setShowStructure(bool newShowStructure);

    bool showFiles() const;
    void setShowFiles(bool newShowFiles);

    bool showProblemSet() const;
    void setShowProblemSet(bool newShowProblemSet);

    bool showIssues() const;
    void setShowIssues(bool newShowIssues);

    bool showCompileLog() const;
    void setShowCompileLog(bool newShowCompileLog);

    bool showDebug() const;
    void setShowDebug(bool newShowDebug);

    bool showSearch() const;
    void setShowSearch(bool newShowSearch);

    bool showTODO() const;
    void setShowTODO(bool newShowTODO);

    bool showBookmark() const;
    void setShowBookmark(bool newShowBookmark);

    bool showProblem() const;
    void setShowProblem(bool newShowProblem);

    int CPUDialogWidth() const;
    void setCPUDialogWidth(int newCPUDialogWidth);

    int CPUDialogHeight() const;
    void setCPUDialogHeight(int newCPUDialogHeight);

    int CPUDialogSplitterPos() const;
    void setCPUDialogSplitterPos(int newCPUDialogSplitterPos);

    int settingsDialogWidth() const;
    void setSettingsDialogWidth(int newSettingsDialogWidth);

    int settingsDialogHeight() const;
    void setSettingsDialogHeight(int newSettingsDialogHeight);

    int settingsDialogSplitterPos() const;
    void setSettingsDialogSplitterPos(int newSettingsDialogSplitterPos);

    int newProjectDialogWidth() const;
    void setNewProjectDialogWidth(int newNewProjectDialogWidth);

    int newProjectDialogHeight() const;
    void setNewProjectDialogHeight(int newNewProjectDialogHeight);

    int newClassDialogWidth() const;
    void setNewClassDialogWidth(int newNewClassDialogWidth);

    int newClassDialogHeight() const;
    void setNewClassDialogHeight(int newNewClassDialogHeight);

    int newHeaderDialogWidth() const;
    void setNewHeaderDialogWidth(int newNewFileDialogWidth);

    int newHeaderDialogHeight() const;
    void setNewHeaderDialogHeight(int newNewFileDialogHeight);

    bool shrinkExplorerTabs() const;
    void setShrinkExplorerTabs(bool newShrinkExplorerTabs);

    bool shrinkMessagesTabs() const;
    void setShrinkMessagesTabs(bool newShrinkMessagesTabs);

    const QSize &explorerTabsSize() const;
    void setExplorerTabsSize(const QSize &newExplorerTabsSize);

    const QSize &messagesTabsSize() const;
    void setMessagesTabsSize(const QSize &newMessagesTabsSize);

    int debugPanelIndex() const;
    void setDebugPanelIndex(int newDebugPanelIndex);

    int projectOrder() const;
    void setProjectOrder(int newProjectOrder);

    int watchOrder() const;
    void setWatchOrder(int newWatchOrder);

    int structureOrder() const;
    void setStructureOrder(int newStructureOrder);

    int filesOrder() const;
    void setFilesOrder(int newFilesOrder);

    int problemSetOrder() const;
    void setProblemSetOrder(int newProblemSetOrder);

    int issuesOrder() const;
    void setIssuesOrder(int newIssuesOrder);

    int compileLogOrder() const;
    void setCompileLogOrder(int newCompileLogOrder);

    int debugOrder() const;
    void setDebugOrder(int newDebugOrder);

    int searchOrder() const;
    void setSearchOrder(int newSearchOrder);

    int TODOOrder() const;
    void setTODOOrder(int newTODOOrder);

    int bookmarkOrder() const;
    void setBookmarkOrder(int newBookmarkOrder);

    int problemOrder() const;
    void setProblemOrder(int newProblemOrder);

    bool openEditorsWhenReplace() const;
    void setOpenEditorsWhenReplace(bool newOpenEditorsWhenReplace);

private:
    bool mOpenEditorsWhenReplace;
    QByteArray mMainWindowState;
    QByteArray mMainWindowGeometry;
    int mBottomPanelIndex;
    int mLeftPanelIndex;
    int mDebugPanelIndex;
    bool mClassBrowserSortAlpha;
    bool mClassBrowserSortType;
    bool mClassBrowserShowInherited;

    bool mShrinkExplorerTabs;
    bool mShrinkMessagesTabs;
    QSize mExplorerTabsSize;
    QSize mMessagesTabsSize;
    //view
    bool mShowToolbar;
    bool mShowStatusBar;
    bool mShowToolWindowBars;

    bool mShowProject;
    bool mShowWatch;
    bool mShowStructure;
    bool mShowFiles;
    bool mShowProblemSet;
    int mProjectOrder;
    int mWatchOrder;
    int mStructureOrder;
    int mFilesOrder;
    int mProblemSetOrder;

    bool mShowIssues;
    bool mShowCompileLog;
    bool mShowDebug;
    bool mShowSearch;
    bool mShowTODO;
    bool mShowBookmark;
    bool mShowProblem;
    int mIssuesOrder;
    int mCompileLogOrder;
    int mDebugOrder;
    int mSearchOrder;
    int mTODOOrder;
    int mBookmarkOrder;
    int mProblemOrder;

    //dialogs
    int mCPUDialogWidth;
    int mCPUDialogHeight;
    int mCPUDialogSplitterPos;
    int mSettingsDialogWidth;
    int mSettingsDialogHeight;
    int mSettingsDialogSplitterPos;
    int mNewProjectDialogWidth;
    int mNewProjectDialogHeight;
    int mNewClassDialogWidth;
    int mNewClassDialogHeight;
    int mNewHeaderDialogWidth;
    int mNewHeaderDialogHeight;

protected:
    void doSave() override;
    void doLoad() override;
};


#endif
//UI_SETTINGS_H
