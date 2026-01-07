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
#include "uisettings.h"

#include <QRect>
#include <QApplication>
#include <QScreen>

UISettings::UISettings(SettingsPersistor *persistor):
    BaseSettings{persistor,SETTING_UI}
{

}

const QByteArray &UISettings::mainWindowGeometry() const
{
    return mMainWindowGeometry;
}

void UISettings::setMainWindowGeometry(const QByteArray &newMainWindowGeometry)
{
    mMainWindowGeometry = newMainWindowGeometry;
}

int UISettings::bottomPanelIndex() const
{
    return mBottomPanelIndex;
}

void UISettings::setBottomPanelIndex(int newBottomPanelIndex)
{
    mBottomPanelIndex = newBottomPanelIndex;
}

int UISettings::leftPanelIndex() const
{
    return mLeftPanelIndex;
}

void UISettings::setLeftPanelIndex(int newLeftPanelIndex)
{
    mLeftPanelIndex = newLeftPanelIndex;
}

bool UISettings::classBrowserShowInherited() const
{
    return mClassBrowserShowInherited;
}

void UISettings::setClassBrowserShowInherited(bool newClassBrowserShowInherited)
{
    mClassBrowserShowInherited = newClassBrowserShowInherited;
}

bool UISettings::showProblem() const
{
    return mShowProblem;
}

void UISettings::setShowProblem(bool newShowProblem)
{
    mShowProblem = newShowProblem;
}

int UISettings::settingsDialogSplitterPos() const
{
    return mSettingsDialogSplitterPos;
}

void UISettings::setSettingsDialogSplitterPos(int newSettingsDialogSplitterPos)
{
    mSettingsDialogSplitterPos = newSettingsDialogSplitterPos;
}

int UISettings::newProjectDialogWidth() const
{
    return mNewProjectDialogWidth;
}

void UISettings::setNewProjectDialogWidth(int newNewProjectDialogWidth)
{
    mNewProjectDialogWidth = newNewProjectDialogWidth;
}

int UISettings::newProjectDialogHeight() const
{
    return mNewProjectDialogHeight;
}

void UISettings::setNewProjectDialogHeight(int newNewProjectDialogHeight)
{
    mNewProjectDialogHeight = newNewProjectDialogHeight;
}

int UISettings::newClassDialogWidth() const
{
    return mNewClassDialogWidth;
}

void UISettings::setNewClassDialogWidth(int newNewClassDialogWidth)
{
    mNewClassDialogWidth = newNewClassDialogWidth;
}

int UISettings::newClassDialogHeight() const
{
    return mNewClassDialogHeight;
}

void UISettings::setNewClassDialogHeight(int newNewClassDialogHeight)
{
    mNewClassDialogHeight = newNewClassDialogHeight;
}

int  UISettings::newHeaderDialogHeight() const
{
    return mNewHeaderDialogHeight;
}

void  UISettings::setNewHeaderDialogHeight(int newNewFileDialogHeight)
{
    mNewHeaderDialogHeight = newNewFileDialogHeight;
}

const QSize &UISettings::messagesTabsSize() const
{
    return mMessagesTabsSize;
}

void UISettings::setMessagesTabsSize(const QSize &newMessagesTabsSize)
{
    mMessagesTabsSize = newMessagesTabsSize;
}

int UISettings::debugPanelIndex() const
{
    return mDebugPanelIndex;
}

void UISettings::setDebugPanelIndex(int newDebugPanelIndex)
{
    mDebugPanelIndex = newDebugPanelIndex;
}

int UISettings::problemOrder() const
{
    return mProblemOrder;
}

void UISettings::setProblemOrder(int newProblemOrder)
{
    mProblemOrder = newProblemOrder;
}

bool UISettings::openEditorsWhenReplace() const
{
    return mOpenEditorsWhenReplace;
}

void UISettings::setOpenEditorsWhenReplace(bool newOpenEditorsWhenReplace)
{
    mOpenEditorsWhenReplace = newOpenEditorsWhenReplace;
}

int UISettings::bookmarkOrder() const
{
    return mBookmarkOrder;
}

void UISettings::setBookmarkOrder(int newBookmarkOrder)
{
    mBookmarkOrder = newBookmarkOrder;
}

int UISettings::TODOOrder() const
{
    return mTODOOrder;
}

void UISettings::setTODOOrder(int newTODOOrder)
{
    mTODOOrder = newTODOOrder;
}

int UISettings::searchOrder() const
{
    return mSearchOrder;
}

void UISettings::setSearchOrder(int newSearchOrder)
{
    mSearchOrder = newSearchOrder;
}

int UISettings::debugOrder() const
{
    return mDebugOrder;
}

void UISettings::setDebugOrder(int newDebugOrder)
{
    mDebugOrder = newDebugOrder;
}

int UISettings::compileLogOrder() const
{
    return mCompileLogOrder;
}

void UISettings::setCompileLogOrder(int newCompileLogOrder)
{
    mCompileLogOrder = newCompileLogOrder;
}

int UISettings::issuesOrder() const
{
    return mIssuesOrder;
}

void UISettings::setIssuesOrder(int newIssuesOrder)
{
    mIssuesOrder = newIssuesOrder;
}

int UISettings::problemSetOrder() const
{
    return mProblemSetOrder;
}

void UISettings::setProblemSetOrder(int newProblemSetOrder)
{
    mProblemSetOrder = newProblemSetOrder;
}

int UISettings::filesOrder() const
{
    return mFilesOrder;
}

void UISettings::setFilesOrder(int newFilesOrder)
{
    mFilesOrder = newFilesOrder;
}

int UISettings::structureOrder() const
{
    return mStructureOrder;
}

void UISettings::setStructureOrder(int newStructureOrder)
{
    mStructureOrder = newStructureOrder;
}

int UISettings::watchOrder() const
{
    return mWatchOrder;
}

void UISettings::setWatchOrder(int newWatchOrder)
{
    mWatchOrder = newWatchOrder;
}

int UISettings::projectOrder() const
{
    return mProjectOrder;
}

void UISettings::setProjectOrder(int newProjectOrder)
{
    mProjectOrder = newProjectOrder;
}

const QSize &UISettings::explorerTabsSize() const
{
    return mExplorerTabsSize;
}

void UISettings::setExplorerTabsSize(const QSize &newExplorerTabsSize)
{
    mExplorerTabsSize = newExplorerTabsSize;
}

bool UISettings::shrinkMessagesTabs() const
{
    return mShrinkMessagesTabs;
}

void UISettings::setShrinkMessagesTabs(bool newShrinkMessagesTabs)
{
    mShrinkMessagesTabs = newShrinkMessagesTabs;
}

bool UISettings::shrinkExplorerTabs() const
{
    return mShrinkExplorerTabs;
}

void UISettings::setShrinkExplorerTabs(bool newShrinkExplorerTabs)
{
    mShrinkExplorerTabs = newShrinkExplorerTabs;
}

int  UISettings::newHeaderDialogWidth() const
{
    return mNewHeaderDialogWidth;
}

void  UISettings::setNewHeaderDialogWidth(int newNewFileDialogWidth)
{
    mNewHeaderDialogWidth = newNewFileDialogWidth;
}

int UISettings::settingsDialogHeight() const
{
    return mSettingsDialogHeight;
}

void UISettings::setSettingsDialogHeight(int newSettingsDialogHeight)
{
    mSettingsDialogHeight = newSettingsDialogHeight;
}

int UISettings::settingsDialogWidth() const
{
    return mSettingsDialogWidth;
}

void UISettings::setSettingsDialogWidth(int newSettingsDialogWidth)
{
    mSettingsDialogWidth = newSettingsDialogWidth;
}

int UISettings::CPUDialogSplitterPos() const
{
    return mCPUDialogSplitterPos;
}

void UISettings::setCPUDialogSplitterPos(int newCPUDialogSplitterPos)
{
    mCPUDialogSplitterPos = newCPUDialogSplitterPos;
}

int UISettings::CPUDialogHeight() const
{
    return mCPUDialogHeight;
}

void UISettings::setCPUDialogHeight(int newCPUDialogHeight)
{
    mCPUDialogHeight = newCPUDialogHeight;
}

int UISettings::CPUDialogWidth() const
{
    return mCPUDialogWidth;
}

void UISettings::setCPUDialogWidth(int newCPUDialogWidth)
{
    mCPUDialogWidth = newCPUDialogWidth;
}

bool UISettings::showBookmark() const
{
    return mShowBookmark;
}

void UISettings::setShowBookmark(bool newShowBookmark)
{
    mShowBookmark = newShowBookmark;
}

bool UISettings::showTODO() const
{
    return mShowTODO;
}

void UISettings::setShowTODO(bool newShowTODO)
{
    mShowTODO = newShowTODO;
}

bool UISettings::showSearch() const
{
    return mShowSearch;
}

void UISettings::setShowSearch(bool newShowSearch)
{
    mShowSearch = newShowSearch;
}

bool UISettings::showDebug() const
{
    return mShowDebug;
}

void UISettings::setShowDebug(bool newShowDebug)
{
    mShowDebug = newShowDebug;
}

bool UISettings::showCompileLog() const
{
    return mShowCompileLog;
}

void UISettings::setShowCompileLog(bool newShowCompileLog)
{
    mShowCompileLog = newShowCompileLog;
}

bool UISettings::showIssues() const
{
    return mShowIssues;
}

void UISettings::setShowIssues(bool newShowIssues)
{
    mShowIssues = newShowIssues;
}

bool UISettings::showProblemSet() const
{
    return mShowProblemSet;
}

void UISettings::setShowProblemSet(bool newShowProblemSet)
{
    mShowProblemSet = newShowProblemSet;
}

bool UISettings::showFiles() const
{
    return mShowFiles;
}

void UISettings::setShowFiles(bool newShowFiles)
{
    mShowFiles = newShowFiles;
}

bool UISettings::showStructure() const
{
    return mShowStructure;
}

void UISettings::setShowStructure(bool newShowStructure)
{
    mShowStructure = newShowStructure;
}

bool UISettings::showWatch() const
{
    return mShowWatch;
}

void UISettings::setShowWatch(bool newShowWatch)
{
    mShowWatch = newShowWatch;
}

bool UISettings::showProject() const
{
    return mShowProject;
}

void UISettings::setShowProject(bool newShowProject)
{
    mShowProject = newShowProject;
}

bool UISettings::showToolWindowBars() const
{
    return mShowToolWindowBars;
}

void UISettings::setShowToolWindowBars(bool newShowToolWindowBars)
{
    mShowToolWindowBars = newShowToolWindowBars;
}

bool UISettings::showStatusBar() const
{
    return mShowStatusBar;
}

void UISettings::setShowStatusBar(bool newShowStatusBar)
{
    mShowStatusBar = newShowStatusBar;
}

bool UISettings::showToolbar() const
{
    return mShowToolbar;
}

void UISettings::setShowToolbar(bool newShowToolbar)
{
    mShowToolbar = newShowToolbar;
}

bool UISettings::classBrowserSortType() const
{
    return mClassBrowserSortType;
}

void UISettings::setClassBrowserSortType(bool newClassBrowserSortType)
{
    mClassBrowserSortType = newClassBrowserSortType;
}

bool UISettings::classBrowserSortAlpha() const
{
    return mClassBrowserSortAlpha;
}

void UISettings::setClassBrowserSortAlpha(bool newClassBrowserSortAlpha)
{
    mClassBrowserSortAlpha = newClassBrowserSortAlpha;
}

const QByteArray &UISettings::mainWindowState() const
{
    return mMainWindowState;
}

void UISettings::setMainWindowState(const QByteArray &newMainWindowState)
{
    mMainWindowState = newMainWindowState;
}

void UISettings::doSave()
{
    saveValue("open_editor_when_batch_replace",mOpenEditorsWhenReplace);

    saveValue("main_window_state",mMainWindowState);
    saveValue("main_window_geometry",mMainWindowGeometry);
    saveValue("bottom_panel_index",mBottomPanelIndex);
    saveValue("left_panel_index",mLeftPanelIndex);
    saveValue("debug_panel_index",mDebugPanelIndex);
    saveValue("class_browser_sort_alphabetically",mClassBrowserSortAlpha);
    saveValue("class_browser_sort_by_type",mClassBrowserSortType);
    saveValue("class_browser_show_inherited",mClassBrowserShowInherited);

    saveValue("shrink_explorer_tabs",mShrinkExplorerTabs);
    saveValue("shrink_messages_tabs",mShrinkMessagesTabs);
    saveValue("explorer_tabs_size", mExplorerTabsSize);
    saveValue("messages_tabs_size",mMessagesTabsSize);

    //view
    saveValue("show_toolbar", mShowToolbar);
    saveValue("show_statusbar", mShowStatusBar);
    saveValue("show_tool_windowbars", mShowToolWindowBars);

    saveValue("show_project", mShowProject);
    saveValue("show_watch", mShowWatch);
    saveValue("show_structure", mShowStructure);
    saveValue("show_file", mShowFiles);
    saveValue("show_problem_set", mShowProblemSet);

    saveValue("show_issues", mShowIssues);
    saveValue("show_compile_log", mShowCompileLog);
    saveValue("show_debug", mShowDebug);
    saveValue("show_search", mShowSearch);
    saveValue("show_todo", mShowTODO);
    saveValue("show_bookmark", mShowBookmark);
    saveValue("show_problem", mShowProblem);

    saveValue("project_order", mProjectOrder);
    saveValue("watch_order", mWatchOrder);
    saveValue("structure_order", mStructureOrder);
    saveValue("files_order", mFilesOrder);
    saveValue("problemset_order", mProblemSetOrder);
    saveValue("issues_order", mIssuesOrder);
    saveValue("compilelog_order", mCompileLogOrder);
    saveValue("debug_order", mDebugOrder);
    saveValue("search_order", mSearchOrder);
    saveValue("todo_order", mTODOOrder);
    saveValue("bookmark_order", mBookmarkOrder);
    saveValue("problem_order", mProblemOrder);

    //dialogs
    saveValue("cpu_dialog_width", mCPUDialogWidth);
    saveValue("cpu_dialog_height", mCPUDialogHeight);
    saveValue("cpu_dialog_splitter", mCPUDialogSplitterPos);
    saveValue("settings_dialog_width", mSettingsDialogWidth);
    saveValue("settings_dialog_height", mSettingsDialogHeight);
    saveValue("settings_dialog_splitter", mSettingsDialogSplitterPos);
    saveValue("new_project_dialog_width", mNewProjectDialogWidth);
    saveValue("new_project_dialog_height", mNewProjectDialogHeight);
    saveValue("new_class_dialog_width", mNewClassDialogWidth);
    saveValue("new_class_dialog_height", mNewClassDialogHeight);
    saveValue("new_header_dialog_width", mNewHeaderDialogWidth);
    saveValue("new_header_dialog_height", mNewHeaderDialogHeight);
}

void UISettings::doLoad()
{
    mOpenEditorsWhenReplace=boolValue("open_editor_when_batch_replace",true);

    mMainWindowState = value("main_window_state",QByteArray()).toByteArray();
    mMainWindowGeometry = value("main_window_geometry",QByteArray()).toByteArray();
    mBottomPanelIndex = intValue("bottom_panel_index",0);
    mLeftPanelIndex = intValue("left_panel_index",0);
    mDebugPanelIndex = intValue("debug_panel_index",0);

    mClassBrowserSortAlpha = boolValue("class_browser_sort_alphabetically",true);
    mClassBrowserSortType = boolValue("class_browser_sort_by_type",true);
    mClassBrowserShowInherited = boolValue("class_browser_show_inherited",true);

    mShrinkExplorerTabs = boolValue("shrink_explorer_tabs",false);
    mShrinkMessagesTabs = boolValue("shrink_messages_tabs",false);
    mExplorerTabsSize = sizeValue("explorer_tabs_size",QSize(300,600));
    mMessagesTabsSize = sizeValue("messages_tabs_size",QSize(450,150));

    //view
    mShowToolbar = boolValue("show_toolbar",true);
    mShowStatusBar = boolValue("show_statusbar",true);
    mShowToolWindowBars = boolValue("show_tool_windowbars",true);

    mShowProject = boolValue("show_project",true);
    mShowWatch = boolValue("show_watch",true);
    mShowStructure = boolValue("show_structure",true);
    mShowFiles = boolValue("show_file",true);
    mShowProblemSet = boolValue("show_problem_set",true);

    mShowIssues = boolValue("show_issues",true);
    mShowCompileLog = boolValue("show_compile_log",true);
    mShowDebug = boolValue("show_debug",true);
    mShowSearch = boolValue("show_search",true);
    mShowTODO = boolValue("show_todo",true);
    mShowBookmark = boolValue("show_bookmark",true);
    mShowProblem = boolValue("show_problem",true);

    mProjectOrder = intValue("project_order",1);
    mWatchOrder = intValue("watch_order",2);
    mStructureOrder = intValue("structure_order",3);
    mFilesOrder = intValue("files_order",0);
    mProblemSetOrder = intValue("problemset_order",4);

    mIssuesOrder = intValue("issues_order",0);
    mCompileLogOrder = intValue("compilelog_order",1);
    mDebugOrder = intValue("debug_order",2);
    mSearchOrder = intValue("search_order",3);
    mTODOOrder = intValue("todo_order",4);
    mBookmarkOrder = intValue("bookmark_order",5);
    mProblemOrder = intValue("problem_order",6);

    //dialogs
    QRect geometry = qApp->primaryScreen()->geometry();
    int width = geometry.width();
    int height = geometry.height();

    mCPUDialogWidth = intValue("cpu_dialog_width", 977 * width / 1920);
    mCPUDialogHeight = intValue("cpu_dialog_height", 622 * height / 1080);
    mCPUDialogSplitterPos = intValue("cpu_dialog_splitter", 500 * width / 1920);
    mSettingsDialogWidth = intValue("settings_dialog_width", 977 * width / 1920);
    mSettingsDialogHeight = intValue("settings_dialog_height", 622 * height / 1080);
    mSettingsDialogSplitterPos = intValue("settings_dialog_splitter", 300 * width / 1920);

    mNewProjectDialogWidth = intValue("new_project_dialog_width", 900 * width / 1920);
    mNewProjectDialogHeight = intValue("new_project_dialog_height", 600 * height / 1080);
    mNewClassDialogWidth = intValue("new_class_dialog_width", 642 * width / 1920);
    mNewClassDialogHeight = intValue("new_class_dialog_height", 300 * height / 1080);
    mNewHeaderDialogWidth = intValue("new_header_dialog_width", 642 * width / 1920);
    mNewHeaderDialogHeight = intValue("new_header_dialog_height", 300 * height / 1080);
}
