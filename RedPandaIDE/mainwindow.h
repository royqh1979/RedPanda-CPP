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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileSystemWatcher>
#include <QMainWindow>
#include <QTimer>
#include <QFileSystemModel>
#include <QTcpServer>
#include <QElapsedTimer>
#include <QSortFilterProxyModel>
#include "common.h"
#include "widgets/searchresultview.h"
#include "widgets/classbrowser.h"
#include "widgets/codecompletionpopup.h"
#include "widgets/headercompletionpopup.h"
#include "widgets/functiontooltipwidget.h"
#include "caretlist.h"
#include "symbolusagemanager.h"
#include "codesnippetsmanager.h"
#include "todoparser.h"
#include "toolsmanager.h"
#include "widgets/labelwithmenu.h"
#include "widgets/bookmarkmodel.h"
#include "widgets/ojproblemsetmodel.h"
#include "widgets/customfilesystemmodel.h"
#include "customfileiconprovider.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum class CompileTarget {
    Invalid, None, File, Project, SyntaxCheck
};

enum class RunType {
    Normal,
    CurrentProblemCase,
    ProblemCases
};


class EditorList;
class QLabel;
class QComboBox;
class CompilerManager;
class Editor;
class Debugger;
class CPUDialog;
class QPlainTextEdit;
class SearchInFileDialog;
class SearchDialog;
class Project;
struct ProjectModelNode;
class ProjectUnit;
class ColorSchemeItem;
class VisitHistoryManager;

#define DPI_CHANGED_EVENT ((QEvent::Type)(QEvent::User+1))

class MainWindow : public QMainWindow
{
    Q_OBJECT


    enum class CompileIssuesState{
        CompilationResultFilled,
        Compiling,
        ProjectCompilationResultFilled,
        ProjectCompiling,
        SyntaxChecking,
        SyntaxCheckResultFilled,
        None
    };

    enum class CompileSuccessionTaskType {
        None,
        RunNormal,
        RunProblemCases,
        RunCurrentProblemCase,
        Debug,
        Profile
    };

    struct CompileSuccessionTask {
        CompileSuccessionTaskType type;
        QString execName;
        QStringList binDirs;
        bool isExecutable;
    };

    using PCompileSuccessionTask = std::shared_ptr<CompileSuccessionTask>;

    struct TabWidgetInfo {
        int order;
        QString text;
        QIcon icon;
    };
    using PTabWidgetInfo = std::shared_ptr<TabWidgetInfo>;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateForEncodingInfo(bool clear=false);
    void updateForEncodingInfo(const Editor* editor, bool clear=false);
    void updateStatusbarForLineCol(bool clear=false);
    void updateStatusbarForLineCol(const Editor* editor, bool clear=false);
    void updateForStatusbarModeInfo(bool clear=false);
    void updateForStatusbarModeInfo(const Editor* editor, bool clear=false);
    void updateStatusbarMessage(const QString& s);
    void setProjectCurrentFile(const QString& filename);
    void updateEditorSettings();
    void updateEditorBookmarks();
    void updateEditorBreakpoints();
    void updateEditorActions();
    void updateEncodingActions(const Editor *e);
    void updateEditorActions(const Editor *e);
    void updateProjectActions();
    void updateCompileActions();
    void updateCompileActions(const Editor* e);
    void updateEditorColorSchemes();
    void updateCompilerSet();
    void updateCompilerSet(const Editor* e);
    void updateDebuggerSettings();
    void updateActionIcons();
    void checkSyntaxInBack(Editor* e);
    bool parsing();
    bool compile(bool rebuild=false, CppCompileType compileType=CppCompileType::Normal);
    void runExecutable(
            const QString& exeName,
            const QString& filename,
            RunType runType,
            const QStringList& binDirs);
    void runExecutable(RunType runType = RunType::Normal);
    void debug();
    void showSearchPanel(bool showReplace = false);
    void showCPUInfoDialog();

    void setFilesViewRoot(const QString& path, bool setOpenFolder=false);

    void applySettings();
    void applyUISettings();
    QFileSystemWatcher* fileSystemWatcher();
    void initDocks();

    void removeActiveBreakpoints();
    void updateAppTitle();
    void updateAppTitle(const Editor* e);
    void addDebugOutput(const QString& text);
    void changeDebugOutputLastline(const QString& text);
    void updateDebugEval(const QString& value);
    void rebuildOpenedFileHisotryMenu();
    void updateClassBrowserForEditor(Editor* editor);
    void resetAutoSaveTimer();
    void updateShortcuts();
    bool saveLastOpens();
    void loadLastOpens();
    void updateTools();

    void openFiles(const QStringList& files);

    void newEditor(const QString& suffix="");

    QPlainTextEdit* txtLocals();

    QMenuBar* menuBar() const;

    CPUDialog *cpuDialog() const;

    Debugger *debugger() const;

    EditorList *editorList() const;

    SearchInFileDialog *searchInFilesDialog() const;

    SearchDialog *searchDialog() const;

    SearchResultModel* searchResultModel();

    const std::shared_ptr<CodeCompletionPopup> &completionPopup() const;

    const std::shared_ptr<HeaderCompletionPopup> &headerCompletionPopup() const;

    const std::shared_ptr<FunctionTooltipWidget> &functionTip() const;

    CaretList &caretList();
    void updateCaretActions();

    std::shared_ptr<Project> project();

    const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &statementColors() const;

    PSymbolUsageManager &symbolUsageManager();

    PCodeSnippetManager &codeSnippetManager();

    const PTodoParser &todoParser() const;

    const PToolsManager &toolsManager() const;

    bool shouldRemoveAllSettings() const;

    const PBookmarkModel &bookmarkModel() const;

    TodoModel* todoModel();

    Editor* openFile(QString filename, bool activate=true, QTabWidget* page=nullptr);
    void openProject(QString filename, bool openFiles = true);
    void changeOptions(const QString& widgetName=QString(), const QString& groupName=QString());
    void changeProjectOptions(const QString& widgetName=QString(), const QString& groupName=QString());

    bool openningFiles() const;

    QList<QAction*> listShortCutableActions();

    void switchCurrentStackTrace(int idx);

    static CompileSuccessionTaskType runTypeToCompileSuccessionTaskType(RunType runType);

public slots:
    void logToolsOutput(const QString& msg);
    void onCompileIssue(PCompileIssue issue);
    void clearToolsOutput();
    void clearTodos();
    void onCompileStarted();
    void onProjectCompileStarted();
    void onSyntaxCheckStarted();
    void onCompileFinished(QString filename, bool isCheckSyntax);
    void onCompileErrorOccured(const QString& reason);
    void onRunErrorOccured(const QString& reason);
    void onRunFinished();
    void onRunPausingForFinish();
    void onRunProblemFinished();
    void onOJProblemCaseStarted(const QString& id, int current, int total);
    void onOJProblemCaseFinished(const QString& id, int current, int total);
    void onOJProblemCaseNewOutputGetted(const QString& id, const QString& line);
    void onOJProblemCaseResetOutput(const QString& id, const QString& line);
    void cleanUpCPUDialog();
    void onDebugCommandInput(const QString& command);
    void onDebugEvaluateInput();
    void onDebugMemoryAddressInput();
    void onParserProgress(const QString& fileName, int total, int current);
    void onStartParsing();
    void onEndParsing(int total, int updateView);
    void onEvalValueReady(const QString& value);
    void onLocalsReady(const QStringList& value);
    void onEditorContextMenu(const QPoint& pos);
    void onEditorRightTabContextMenu(const QPoint& pos);
    void onEditorLeftTabContextMenu(const QPoint& pos);
    void onEditorTabContextMenu(QTabWidget* tabWidget, const QPoint& pos);
    void disableDebugActions();
    void enableDebugActions();
    void stopDebugForNoSymbolTable();
    void onTodoParsingFile(const QString& filename);
    void onTodoParseStarted();
    void onTodoFound(const QString& filename, int lineNo, int ch, const QString& line);
    void onTodoParseFinished();
    void onWatchpointHitted(const QString& var, const QString& oldVal, const QString& newVal);
    void setActiveBreakpoint(QString FileName, int Line, bool setFocus);
    void updateDPI(int oldDPI, int newDPI);
    void onFileSaved(const QString& path, bool inProject);

private:
    int calIconSize(const QString &fontName, int fontPointSize);
    void hideAllSearchDialogs();
    void prepareSearchDialog();
    void prepareSearchInFilesDialog();
    void prepareProjectForCompile();
    void closeProject(bool refreshEditor);
    void updateProjectView();
    CompileTarget getCompileTarget();
    bool debugInferiorhasBreakpoint();
    void stretchMessagesPanel(bool open);
    void stretchExplorerPanel(bool open);
    void prepareDebugger();
    void doAutoSave(Editor *e);
    void createCustomActions();
    void initToolButtons();
    void buildContextMenus();
    void buildEncodingMenu();
    void buildNewlineMenu();
    void maximizeEditor();
    QStringList getBinDirsForCurrentEditor();
    QStringList getDefaultCompilerSetBinDirs();
    void openShell(const QString& folder, const QString& shellCommand, const QStringList& binDirs);
    QAction* createAction(const QString& text,
                          QWidget* parent,
                          QKeySequence shortcut=QKeySequence(),
                          Qt::ShortcutContext shortcutContext = Qt::ShortcutContext::WidgetWithChildrenShortcut
                          );
    QAction* createShortcutCustomableAction(
            const QString& text,
            const QString& objectName,
            QKeySequence shortcut=QKeySequence());
    void scanActiveProject(bool parse=false);
    void showSearchReplacePanel(bool show);
    void clearIssues();
    void doCompileRun(RunType runType);
    void doGenerateAssembly();
    void updateProblemCaseOutput(POJProblemCase problemCase);
    void applyCurrentProblemCaseChanges();
    void showHideInfosTab(QWidget *widget, bool show);
    void showHideMessagesTab(QWidget *widget, bool show);
    void prepareTabInfosData();
    void prepareTabMessagesData();
    void newProjectUnitFile(const QString& suffix="");
    void fillProblemCaseInputAndExpected(const POJProblemCase &problemCase);

    void doFilesViewRemoveFile(const QModelIndex& index);

    void setProjectViewCurrentNode(std::shared_ptr<ProjectModelNode> node);
    void setProjectViewCurrentUnit(std::shared_ptr<ProjectUnit> unit);

    void reparseNonProjectEditors();
    QString switchHeaderSourceTarget(Editor *editor);

private slots:
    void setupSlotsForProject();
    void onProjectUnitAdded(const QString &filename);
    void onProjectUnitRemoved(const QString &filename);
    void onProjectUnitRenamed(const QString &oldFilename, const QString& newFilename);
    void onProjectViewNodeRenamed();
    void setDockExplorerToArea(const Qt::DockWidgetArea &area);
    void setDockMessagesToArea(const Qt::DockWidgetArea &area);
#ifdef ENABLE_VCS
    void updateVCSActions();
#endif
    void invalidateProjectProxyModel();
    void onEditorRenamed(const QString &oldFilename, const QString &newFilename, bool firstSave);
    void onAutoSaveTimeout();
    void onFileChanged(const QString &path);
    void onDirChanged(const QString &path);
    void onFilesViewPathChanged();
    void onWatchViewContextMenu(const QPoint& pos);
    void onBookmarkContextMenu(const QPoint& pos);
    void onTableIssuesContextMenu(const QPoint& pos);
    void onSearchViewContextMenu(const QPoint& pos);
    void onBreakpointsViewContextMenu(const QPoint& pos);
    void onProjectViewContextMenu(const QPoint& pos);
    void onClassBrowserContextMenu(const QPoint& pos);
    void onDebugConsoleContextMenu(const QPoint& pos);
    void onFileEncodingContextMenu(const QPoint& pos);
    void onFilesViewContextMenu(const QPoint& pos);
    void onLstProblemSetContextMenu(const QPoint& pos);
    void onTableProblemCasesContextMenu(const QPoint& pos);
    void onToolsOutputContextMenu(const QPoint&pos);

    void onProblemSetIndexChanged(const QModelIndex &current, const QModelIndex &previous);
    void onProblemCaseIndexChanged(const QModelIndex &current, const QModelIndex &previous);
    void onProblemNameChanged(int index);
    void onProblemRunCurrentCase();
    void onProblemBatchSetCases();
    void onNewProblemConnection();
    void updateProblemTitle();
    void onEditorClosed();
    void onToolsOutputClear();
    void onToolsOutputCopy();
    void onToolsOutputSelectAll();

    void onShowInsertCodeSnippetMenu();

    void onFilesViewCreateFolderFolderLoaded(const QString& path);
    void onFilesViewCreateFolder();
    void onFilesViewCreateFile();
    void onFilesViewRemoveFiles();
    void onFilesViewRename();

    void onNewProblemSet();

    void onProblemProperties();
    void onProblemOpenSource();
    void onProblemRename();
    void onProblemGotoUrl();

    void onRenameProblemSet();
    void onBookmarkRemove();
    void onBookmarkRemoveAll();
    void onBookmarkModify();
    void onDebugConsoleShowDetailLog();
    void onDebugConsolePaste();
    void onDebugConsoleSelectAll();
    void onDebugConsoleCopy();
    void onDebugConsoleClear();
    void onFilesViewOpenInExplorer();
    void onFilesViewOpenInTerminal();
    void onFilesViewOpenWithExternal();
    void onFilesViewOpen();
    void onClassBrowserGotoDeclaration();
    void onClassBrowserGotoDefinition();
    void onClassBrowserShowInherited();
    void onClassBrowserSortByType();
    void onClassBrowserSortByName();
    void onClassBrowserChangeScope();
    void onClassBrowserRefreshStart();
    void onClassBrowserRefreshEnd();

    void onProjectSwitchCustomViewMode();
    void onProjectSwitchFileSystemViewMode();
    void onProjectRemoveFolder();
    void onProjectRenameFolder();
    void onProjectAddFolder();
    void onProjectRenameUnit();
    void onBreakpointRemove();
    void onBreakpointViewRemoveAll();
    void onBreakpointViewProperty();
    void onSearchViewClearAll();
    void onSearchViewClear();
    void onTableIssuesClear();
    void onTableIssuesCopyAll();
    void onTableIssuesCopy();

    void on_actionNew_triggered();

    void on_EditorTabsLeft_tabCloseRequested(int index);
    void on_EditorTabsRight_tabCloseRequested(int index);

    void onFileSystemModelLayoutChanged();

    void on_actionOpen_triggered();

    void on_actionSave_triggered();

    void on_actionSaveAs_triggered();

    void on_actionOptions_triggered();

    // qt will auto bind slots with the prefix "on_"
    void onCompilerSetChanged(int index);

    void on_actionCompile_triggered();

    void on_actionRun_triggered();

    void on_actionUndo_triggered();

    void on_actionRedo_triggered();

    void on_actionCut_triggered();

    void on_actionSelectAll_triggered();

    void on_actionCopy_triggered();

    void on_actionPaste_triggered();

    void on_actionIndent_triggered();

    void on_actionUnIndent_triggered();

    void on_actionToggleComment_triggered();

    void on_actionUnfoldAll_triggered();

    void on_actionFoldAll_triggered();

    void on_tableIssues_doubleClicked(const QModelIndex &index);

    void on_actionEncode_in_ANSI_triggered();

    void on_actionEncode_in_UTF_8_triggered();

    void on_actionAuto_Detect_triggered();

    void on_actionConvert_to_ANSI_triggered();

    void on_actionConvert_to_UTF_8_triggered();

    void on_tabMessages_tabBarClicked(int index);

    void on_actionRebuild_triggered();

    void on_actionStop_Execution_triggered();

    void on_actionDebug_triggered();

    void on_actionStep_Over_triggered();

    void on_actionStep_Into_triggered();

    void on_actionStep_Out_triggered();

    void on_actionRun_To_Cursor_triggered();

    void on_actionContinue_triggered();

    void on_actionAdd_Watch_triggered();

    void on_actionView_CPU_Window_triggered();

    void on_actionExit_triggered();

    void on_actionFind_triggered();

    void on_actionFind_in_files_triggered();

    void on_actionReplace_triggered();

    void on_actionFind_Next_triggered();

    void on_actionFind_Previous_triggered();

    void on_cbSearchHistory_currentIndexChanged(int index);

    void on_btnSearchAgain_clicked();
    void on_actionRemove_Watch_triggered();

    void on_actionRemove_All_Watches_triggered();

    void on_actionModify_Watch_triggered();

    void on_actionReformat_Code_triggered();

    void on_actionBack_triggered();

    void on_actionForward_triggered();

    void on_tabExplorer_tabBarClicked(int index);

    void on_EditorTabsLeft_tabBarDoubleClicked(int index);
    void on_EditorTabsRight_tabBarDoubleClicked(int index);

    void on_actionClose_triggered();

    void on_actionClose_All_triggered();

    void on_actionMaximize_Editor_triggered();

    void on_actionNext_Editor_triggered();

    void on_actionPrevious_Editor_triggered();

    void on_actionToggle_Breakpoint_triggered();

    void on_actionClear_all_breakpoints_triggered();

    void on_actionBreakpoint_property_triggered();

    void on_actionGoto_Declaration_triggered();

    void on_actionGoto_Definition_triggered();

    void on_actionFind_references_triggered();

    void on_actionOpen_Containing_Folder_triggered();

    void on_actionOpen_Terminal_triggered();

    void on_actionFile_Properties_triggered();

    void on_searchView_doubleClicked(const QModelIndex &index);

    void on_tblStackTrace_doubleClicked(const QModelIndex &index);

    void on_tblBreakpoints_doubleClicked(const QModelIndex &index);

    void on_projectView_doubleClicked(const QModelIndex &index);

    void on_actionClose_Project_triggered();

    void on_actionProject_options_triggered();

    void on_actionNew_Project_triggered();

    void on_actionSaveAll_triggered();

    void on_actionProject_New_File_triggered();

    void on_actionAdd_to_project_triggered();

    void on_actionRemove_from_project_triggered();

    void on_actionView_Makefile_triggered();

    void on_actionMakeClean_triggered();

    void on_actionProject_Open_Folder_In_Explorer_triggered();

    void on_actionProject_Open_In_Terminal_triggered();

    void on_classBrowser_doubleClicked(const QModelIndex &index);

    void on_EditorTabsLeft_currentChanged(int index);
    void on_EditorTabsRight_currentChanged(int index);

    void on_tableTODO_doubleClicked(const QModelIndex &index);

    void on_actionAbout_triggered();

    void on_actionRename_Symbol_triggered();

    void on_btnReplace_clicked();

    void on_btnCancelReplace_clicked();

    void on_actionPrint_triggered();

    void on_actionExport_As_RTF_triggered();

    void on_actionExport_As_HTML_triggered();

    void on_actionMove_To_Other_View_triggered();

    void on_actionC_C_Reference_triggered();

    void on_actionEGE_Manual_triggered();

    void on_actionAdd_bookmark_triggered();

    void on_actionRemove_Bookmark_triggered();

    void on_tableBookmark_doubleClicked(const QModelIndex &index);

    void on_actionModify_Bookmark_Description_triggered();

    void on_actionLocate_in_Files_View_triggered();

    void on_treeFiles_doubleClicked(const QModelIndex &index);

    void on_actionOpen_Folder_triggered();

    void on_actionRun_Parameters_triggered();

    void onAddProblem();

    void onRemoveProblem();

    void onSaveProblemSet();

    void onLoadProblemSet();

    void onAddProblemCase();

    void onProblemRunAllCases();

    void on_actionC_Reference_triggered();

    void onRemoveProblemCases();

    void onOpenProblemAnswerFile();

    void on_actionTool_Window_Bars_triggered();

    void on_actionStatus_Bar_triggered();

    void on_actionProject_triggered();

    void on_actionWatch_triggered();

    void on_actionStructure_triggered();

    void on_actionFiles_triggered();

    void on_actionProblem_Set_triggered();

    void on_actionIssues_triggered();

    void on_actionTools_Output_triggered();

    void on_actionDebug_Window_triggered();

    void on_actionSearch_triggered();

    void on_actionTODO_triggered();

    void on_actionBookmark_triggered();

    void on_actionProblem_triggered();

    void on_actionDelete_Line_triggered();

    void on_actionDuplicate_Line_triggered();

    void on_actionDelete_Word_triggered();

    void on_actionDelete_to_EOL_triggered();

    void on_actionDelete_to_BOL_triggered();

    void onOpenCaseValidationOptions();

    void on_actionInterrupt_triggered();

    void on_actionDelete_Last_Word_triggered();

    void on_actionDelete_to_Word_End_triggered();

    void on_actionNew_Class_triggered();

    void on_actionNew_Header_triggered();

#ifdef ENABLE_VCS
    void on_actionGit_Create_Repository_triggered();

    void on_actionGit_Add_Files_triggered();

    void on_actionGit_Commit_triggered();

    void on_actionGit_Restore_triggered();

    void on_actionGit_Branch_triggered();

    void on_actionGit_Merge_triggered();

    void on_actionGit_Log_triggered();

    void on_actionGit_Remotes_triggered();

    void on_actionGit_Fetch_triggered();

    void on_actionGit_Pull_triggered();

    void on_actionGit_Push_triggered();
#endif
    void on_actionWebsite_triggered();

    void on_actionFilesView_Hide_Non_Support_Files_toggled(bool arg1);

    void on_actionToggle_Block_Comment_triggered();

    void on_actionMatch_Bracket_triggered();

    void on_btnProblemCaseInputFileName_clicked();

    void on_btnProblemCaseClearExpectedOutputFileName_clicked();

    void on_btnProblemCaseClearInputFileName_clicked();

    void on_btnProblemCaseExpectedOutputFileName_clicked();

    void on_txtProblemCaseOutput_cursorPositionChanged();

    void on_txtProblemCaseExpected_cursorPositionChanged();

    void on_txtProblemCaseInput_cursorPositionChanged();

    void on_actionMove_Selection_Up_triggered();

    void on_actionMove_Selection_Down_triggered();

    void on_actionConvert_to_UTF_8_BOM_triggered();

    void on_actionEncode_in_UTF_8_BOM_triggered();

    void on_actionCompiler_Options_triggered();

    void on_dockExplorer_dockLocationChanged(const Qt::DockWidgetArea &area);

    void on_dockMessages_dockLocationChanged(const Qt::DockWidgetArea &area);

    void on_actionToggle_Explorer_Panel_triggered();

    void on_actionToggle_Messages_Panel_triggered();

    void on_actionRaylib_Manual_triggered();

    void on_actionSelect_Word_triggered();

    void on_actionGo_to_Line_triggered();

    void on_actionNew_Template_triggered();

    void on_actionGoto_block_start_triggered();

    void on_actionGoto_block_end_triggered();

    void on_actionSwitchHeaderSource_triggered();

    void on_actionGenerate_Assembly_triggered();

    void onImportFPSProblemSet();

    void on_actionTrim_trailing_spaces_triggered();

    void onExportFPSProblemSet();

    void on_actionToggle_Readonly_triggered();

    void on_actionSubmit_Issues_triggered();

    void on_actionDocument_triggered();

    void on_actionNew_GAS_File_triggered();

    void on_actionGNU_Assembler_Manual_triggered();

#ifdef ARCH_X86_64
    void on_actionx86_Assembly_Language_Reference_Manual_triggered();
#endif
#ifdef ARCH_X86
    void on_actionIA_32_Assembly_Language_Reference_Manual_triggered();
#endif

    void on_actionAdd_Watchpoint_triggered();

    void on_actionNew_Text_File_triggered();

    void on_actionPage_Up_triggered();

    void on_actionPage_Down_triggered();

    void on_actionGoto_Line_Start_triggered();

    void on_actionGoto_Line_End_triggered();

    void on_actionGoto_File_Start_triggered();

    void on_actionGoto_File_End_triggered();

    void on_actionPage_Up_and_Select_triggered();

    void on_actionPage_Down_and_Select_triggered();

    void on_actionGoto_Page_Start_triggered();

    void on_actionGoto_Page_End_triggered();

    void on_actionGoto_Page_Start_and_Select_triggered();

    void on_actionGoto_Page_End_and_Select_triggered();

    void on_actionGoto_Line_Start_and_Select_triggered();

    void on_actionGoto_Line_End_and_Select_triggered();

    void on_actionGoto_File_Start_and_Select_triggered();

    void on_actionGoto_File_End_and_Select_triggered();

    void on_actionClose_Others_triggered();

    void on_actionOI_Wiki_triggered();

    void on_actionTurtle_Graphics_Manual_triggered();

    void on_cbProblemCaseValidateType_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    bool mFullInitialized;
    EditorList *mEditorList;
    QLabel *mFileInfoStatus;
    LabelWithMenu *mFileEncodingStatus;
    QLabel *mFileModeStatus;
    QMenu *mMenuEncoding;
    QMenu *mMenuNewline;
    QMenu *mMenuExport;
    QMenu *mMenuEncodingList;
    QMenu *mMenuRecentFiles;
    QMenu *mMenuRecentProjects;
    QMenu *mMenuNew;
    QMenu *mMenuInsertCodeSnippet;
    QComboBox *mCompilerSet;
    std::shared_ptr<CompilerManager> mCompilerManager;
    std::shared_ptr<Debugger> mDebugger;
    CPUDialog *mCPUDialog;
    SearchInFileDialog *mSearchInFilesDialog;
    SearchDialog *mSearchDialog;
    bool mQuitting;
    bool mOpeningFiles;
    bool mOpeningProject;
    bool mClosingProject;
    QElapsedTimer mParserTimer;
    QFileSystemWatcher mFileSystemWatcher;
    std::shared_ptr<Project> mProject;
    Qt::DockWidgetArea mMessagesDockLocation;

    std::shared_ptr<CodeCompletionPopup> mCompletionPopup;
    std::shared_ptr<HeaderCompletionPopup> mHeaderCompletionPopup;
    std::shared_ptr<FunctionTooltipWidget> mFunctionTip;

    std::shared_ptr<VisitHistoryManager> mVisitHistoryManager;

    TodoModel mTodoModel;
    SearchResultModel mSearchResultModel;
    PBookmarkModel mBookmarkModel;
    PSearchResultListModel mSearchResultListModel;
    PSearchResultTreeModel mSearchResultTreeModel;
    PSearchResultTreeViewDelegate mSearchViewDelegate;
    ClassBrowserModel mClassBrowserModel;
    std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > mStatementColors;
    PSymbolUsageManager mSymbolUsageManager;
    PCodeSnippetManager mCodeSnippetManager;
    PTodoParser mTodoParser;
    PToolsManager mToolsManager;
    CustomFileSystemModel mFileSystemModel;
    CustomFileIconProvider mFileSystemModelIconProvider;
    OJProblemSetModel mOJProblemSetModel;
    OJProblemModel mOJProblemModel;
    int mOJProblemSetNameCounter;

    QString mClassBrowserCurrentStatement;
    QString mFilesViewNewCreatedFolder;
    QString mFilesViewNewCreatedFile;

    bool mCheckSyntaxInBack;
    bool mShouldRemoveAllSettings;
    PCompileSuccessionTask mCompileSuccessionTask;

    QMap<QWidget*, PTabWidgetInfo> mTabInfosData;
    QMap<QWidget*, PTabWidgetInfo> mTabMessagesData;

    QTimer mAutoSaveTimer;

    CaretList mCaretList;

    bool mClosing;
    bool mClosingAll;
    bool mOpenningFiles;
    bool mSystemTurnedOff;
    QPoint mEditorContextMenuPos;
    QTcpServer mTcpServer;
    QColor mErrorColor;
    CompileIssuesState mCompileIssuesState;

    QSet<QString> mFilesChangedNotifying;

    //actions for compile issue table
    QAction * mTableIssuesCopyAction;
    QAction * mTableIssuesCopyAllAction;
    QAction * mTableIssuesClearAction;

    //actions for search result view
    QAction * mSearchViewClearAction;
    QAction * mSearchViewClearAllAction;

    //actions for breakpoint view
    QAction * mBreakpointViewPropertyAction;
    QAction * mBreakpointViewRemoveAllAction;
    QAction * mBreakpointViewRemoveAction;

    //actions for project view
    QAction * mProject_Add_Folder;
    QAction * mProject_Rename_Unit;
    QAction * mProject_Rename_Folder;
    QAction * mProject_Remove_Folder;
    QAction * mProject_SwitchFileSystemViewMode;
    QAction * mProject_SwitchCustomViewMode;

    //actions for class browser
    QAction * mClassBrowser_Sort_By_Type;
    QAction * mClassBrowser_Sort_By_Name;
    QAction * mClassBrowser_Show_Inherited;
    QAction * mClassBrowser_goto_declaration;
    QAction * mClassBrowser_goto_definition;
    QAction * mClassBrowser_Show_CurrentFile;
    QAction * mClassBrowser_Show_WholeProject;
    QWidget * mClassBrowserToolbar;

    //actions for files view
    QAction * mFilesView_Open;
    QAction * mFilesView_OpenWithExternal;
    QAction * mFilesView_OpenInTerminal;
    QAction * mFilesView_OpenInExplorer;
    QAction * mFilesView_CreateFolder;
    QAction * mFilesView_CreateFile;
    QAction * mFilesView_RemoveFile;
    QAction * mFilesView_Rename;

    //action for debug console
    QAction * mDebugConsole_ShowDetailLog;
    QAction * mDebugConsole_Clear;
    QAction * mDebugConsole_Copy;
    QAction * mDebugConsole_Paste;
    QAction * mDebugConsole_SelectAll;
    //action for bookmarks
    QAction * mBookmark_Remove;
    QAction * mBookmark_RemoveAll;
    QAction * mBookmark_Modify;

    //action for problem set
    QAction * mProblemSet_New;
    QAction * mProblemSet_Rename;
    QAction * mProblemSet_Save;
    QAction * mProblemSet_Load;
    QAction * mProblemSet_ImportFPS;
    QAction * mProblemSet_ExportFPS;
    QAction * mProblemSet_AddProblem;
    QAction * mProblemSet_RemoveProblem;

    //action for problem
    QAction * mProblem_OpenSource;
    QAction * mProblem_Properties;
    QAction * mProblem_Rename;
    QAction * mProblem_GotoUrl;


    //action for problem cases
    QAction * mProblem_AddCase;
    QAction * mProblem_RemoveCases;
    QAction * mProblem_OpenAnswer;
    QAction * mProblem_CaseValidationOptions;

    QAction * mProblem_RunCurrentCase;
    QAction * mProblem_RunAllCases;
    QAction * mProblem_batchSetCases;

    //action for tools output
    QAction * mToolsOutput_Clear;
    QAction * mToolsOutput_SelectAll;
    QAction * mToolsOutput_Copy;

    QSortFilterProxyModel *mProjectProxyModel;

   // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent *event) override;


    // QObject interface
public:
    bool event(QEvent *event) override;
    bool isClosingAll() const;
    bool isQuitting() const;
    const std::shared_ptr<VisitHistoryManager> &visitHistoryManager() const;
    bool closingProject() const;
    bool openingFiles() const;
    bool openingProject() const;
};

extern MainWindow* pMainWindow;
#endif // MAINWINDOW_H
