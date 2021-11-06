#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileSystemWatcher>
#include <QMainWindow>
#include <QTimer>
#include <QFileSystemModel>
#include <QTcpServer>
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
class SearchDialog;
class Project;
class ColorSchemeItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

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
        QString filename;
    };

    using PCompileSuccessionTask = std::shared_ptr<CompileSuccessionTask>;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateForEncodingInfo();
    void updateStatusbarForLineCol();
    void updateForStatusbarModeInfo();
    void updateStatusbarMessage(const QString& s);
    void updateEditorSettings();
    void updateEditorActions();
    void updateProjectActions();
    void updateCompileActions();
    void updateEditorColorSchemes();
    void updateCompilerSet();
    void updateDebuggerSettings();
    void checkSyntaxInBack(Editor* e);
    bool compile(bool rebuild=false);
    void runExecutable(const QString& exeName, const QString& filename=QString(),RunType runType = RunType::Normal);
    void runExecutable(RunType runType = RunType::Normal);
    void debug();
    void showSearchPanel(bool showReplace = false);

    void applySettings();
    void applyUISettings();
    QFileSystemWatcher* fileSystemWatcher();

    void removeActiveBreakpoints();
    void setActiveBreakpoint(QString FileName, int Line, bool setFocus=true);
    void updateAppTitle();
    void addDebugOutput(const QString& text);
    void changeDebugOutputLastline(const QString& text);
    void updateDebugEval(const QString& value);
    void rebuildOpenedFileHisotryMenu();
    void updateClassBrowserForEditor(Editor* editor);
    void resetAutoSaveTimer();
    void updateShortcuts();
    void saveLastOpens();
    void loadLastOpens();
    void updateTools();

    void openFiles(const QStringList& files);

    void newEditor();

    QPlainTextEdit* txtLocals();

    Ui::MainWindow* mainWidget() const;

    CPUDialog *cpuDialog() const;

    Debugger *debugger() const;

    EditorList *editorList() const;

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

    void openFile(const QString& filename, QTabWidget* page=nullptr);
    void openProject(const QString& filename);
    void changeOptions(const QString& widgetName=QString(), const QString& groupName=QString());

public slots:
    void onCompileLog(const QString& msg);
    void onCompileIssue(PCompileIssue issue);
    void onCompileStarted();
    void onCompileFinished(bool isCheckSyntax);
    void onCompileErrorOccured(const QString& reason);
    void onRunErrorOccured(const QString& reason);
    void onRunFinished();
    void onRunProblemFinished();
    void onOJProblemCaseStarted(const QString& id, int current, int total);
    void onOJProblemCaseFinished(const QString& id, int current, int total);
    void cleanUpCPUDialog();
    void onDebugCommandInput(const QString& command);
    void onDebugEvaluateInput();
    void onDebugMemoryAddressInput();
    void onParserProgress(const QString& fileName, int total, int current);
    void onStartParsing();
    void onEndParsing(int total, int updateView);
    void onEvalValueReady(const QString& value);
    void onMemoryExamineReady(const QStringList& value);
    void onLocalsReady(const QStringList& value);
    void onEditorContextMenu(const QPoint& pos);
    void onEditorRightTabContextMenu(const QPoint& pos);
    void onEditorLeftTabContextMenu(const QPoint& pos);
    void onEditorTabContextMenu(QTabWidget* tabWidget, const QPoint& pos);
    void disableDebugActions();
    void enableDebugActions();
    void onTodoParseStarted(const QString& filename);
    void onTodoParsing(const QString& filename, int lineNo, int ch, const QString& line);
    void onTodoParseFinished();

private:
    void prepareProjectForCompile();
    void closeProject(bool refreshEditor);
    void updateProjectView();
    CompileTarget getCompileTarget();
    bool debugInferiorhasBreakpoint();
    void setupActions();
    void openCloseBottomPanel(bool open);
    void openCloseLeftPanel(bool open);
    void prepareDebugger();
    void doAutoSave(Editor *e);
    void buildContextMenus();
    void buildEncodingMenu();
    void maximizeEditor();
    void openShell(const QString& folder, const QString& shellCommand);
    QAction* createActionFor(const QString& text,
                             QWidget* parent,
                             QKeySequence shortcut=QKeySequence());
    void scanActiveProject(bool parse=false);
    void includeOrSkipDirs(const QStringList& dirs, bool skip);
    void showSearchReplacePanel(bool show);
    void setFilesViewRoot(const QString& path);
    void clearIssues();
    void doCompileRun(RunType runType);
    void updateProblemCaseOutput(POJProblemCase problemCase);
    void applyCurrentProblemCaseChanges();

private slots:
    void onAutoSaveTimeout();
    void onFileChanged(const QString& path);

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
    void onProblemSetIndexChanged(const QModelIndex &current, const QModelIndex &previous);
    void onProblemCaseIndexChanged(const QModelIndex &current, const QModelIndex &previous);
    void onProblemNameChanged(int index);
    void onNewProblemConnection();
    void onEditorClosed();

    void onShowInsertCodeSnippetMenu();

    void on_actionNew_triggered();

    void on_EditorTabsLeft_tabCloseRequested(int index);
    void on_EditorTabsRight_tabCloseRequested(int index);

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

    void on_tabMessages_currentChanged(int index);

    void on_tabMessages_tabBarDoubleClicked(int index);

    void on_actionCompile_Run_triggered();

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

    void on_tabInfos_tabBarClicked(int index);

    void on_splitterInfos_splitterMoved(int pos, int index);

    void on_splitterMessages_splitterMoved(int pos, int index);

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

    void on_btnNewProblemSet_clicked();

    void on_btnAddProblem_clicked();

    void on_btnRemoveProblem_clicked();

    void on_btnSaveProblemSet_clicked();

    void on_btnLoadProblemSet_clicked();

    void on_btnAddProblemCase_clicked();

    void on_btnRunAllProblemCases_clicked();

    void on_actionC_Reference_triggered();

    void on_btnRemoveProblemCase_clicked();

    void on_btnOpenProblemAnswer_clicked();

private:
    Ui::MainWindow *ui;
    EditorList *mEditorList;
    QLabel *mFileInfoStatus;
    LabelWithMenu *mFileEncodingStatus;
    QLabel *mFileModeStatus;
    QMenu *mMenuEncoding;
    QMenu *mMenuExport;
    QMenu *mMenuEncodingList;
    QMenu *mMenuRecentFiles;
    QMenu *mMenuRecentProjects;
    QMenu *mMenuNew;
    QMenu *mMenuInsertCodeSnippet;
    QComboBox *mCompilerSet;
    CompilerManager *mCompilerManager;
    Debugger *mDebugger;
    CPUDialog *mCPUDialog;
    SearchDialog *mSearchDialog;
    bool mQuitting;
    QElapsedTimer mParserTimer;
    QFileSystemWatcher mFileSystemWatcher;
    std::shared_ptr<Project> mProject;

    std::shared_ptr<CodeCompletionPopup> mCompletionPopup;
    std::shared_ptr<HeaderCompletionPopup> mHeaderCompletionPopup;
    std::shared_ptr<FunctionTooltipWidget> mFunctionTip;

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
    QFileSystemModel mFileSystemModel;
    OJProblemSetModel mOJProblemSetModel;
    OJProblemModel mOJProblemModel;
    int mOJProblemSetNameCounter;

    bool mCheckSyntaxInBack;
    bool mOpenClosingBottomPanel;
    int mBottomPanelHeight;
    bool mBottomPanelOpenned;
    bool mOpenClosingLeftPanel;
    int mLeftPanelWidth;
    bool mLeftPanelOpenned;
    bool mShouldRemoveAllSettings;
    PCompileSuccessionTask mCompileSuccessionTask;

    QTimer mAutoSaveTimer;

    CaretList mCaretList;

    bool mClosing;
    bool mSystemTurnedOff;
    QPoint mEditorContextMenuPos;
    QTcpServer mTcpServer;
    QColor mErrorColor;

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

    //actions for class browser
    QAction * mClassBrowser_Sort_By_Type;
    QAction * mClassBrowser_Sort_By_Name;
    QAction * mClassBrowser_Show_Inherited;
    QAction * mClassBrowser_goto_declaration;
    QAction * mClassBrowser_goto_definition;
    QWidget * mClassBrowserToolbar;

    //actions for files view
    QAction * mFilesView_Open;
    QAction * mFilesView_OpenWithExternal;
    QAction * mFilesView_OpenInTerminal;
    QAction * mFilesView_OpenInExplorer;
    QWidget * mFilesViewToolbar;

    //action for debug console
    QAction * mDebugConsole_ShowCommandLog;
    QAction * mDebugConsole_Clear;
    QAction * mDebugConsole_Copy;
    QAction * mDebugConsole_Paste;
    QAction * mDebugConsole_SelectAll;
    //action for bookmarks
    QAction * mBookmark_Remove;
    QAction * mBookmark_RemoveAll;
    QAction * mBookmark_Modify;

    //action for problem set
    QAction * mProblem_Properties;

   // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent *event) override;
};

extern MainWindow* pMainWindow;
#endif // MAINWINDOW_H
