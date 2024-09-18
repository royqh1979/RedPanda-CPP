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
#include <QGuiApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QScreen>
#include <QStyleFactory>
#include <QTcpSocket>
#include <QTemporaryFile>
#include <QTextBlock>
#include <QTranslator>
#include <QFileIconProvider>
#include <QMimeDatabase>
#include <QMimeType>
#include <QToolTip>
#include <QCompleter>
#include <QUuid>
#include <QScrollBar>
#include <QTextDocumentFragment>
#include <QActionGroup>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editorlist.h"
#include "editor.h"
#include "systemconsts.h"
#include "settings.h"
#include "qsynedit/constants.h"
#include "debugger/debugger.h"
#include "utils/escape.h"
#include "utils/parsearg.h"
#include "widgets/cpudialog.h"
#include "widgets/filepropertiesdialog.h"
#include "widgets/filenameeditdelegate.h"
#include "project.h"
#include "projecttemplate.h"
#include "widgets/newprojectdialog.h"
#include <qt_utils/charsetinfo.h>
#include "widgets/aboutdialog.h"
#include "shortcutmanager.h"
#include "colorscheme.h"
#include "thememanager.h"
#include "widgets/darkfusionstyle.h"
#include "widgets/lightfusionstyle.h"
#include "problems/problemcasevalidator.h"
#include "problems/freeprojectsetformat.h"
#include "widgets/ojproblempropertywidget.h"
#include "iconsmanager.h"
#include "widgets/newclassdialog.h"
#include "widgets/newheaderdialog.h"
#ifdef ENABLE_LUA_ADDON
#include "addon/executor.h"
#include "addon/runtime.h"
#endif
#ifdef ENABLE_VCS
#include "vcs/gitmanager.h"
#include "vcs/gitrepository.h"
#include "vcs/gitbranchdialog.h"
#include "vcs/gitmergedialog.h"
#include "vcs/gitlogdialog.h"
#include "vcs/gitremotedialog.h"
#include "vcs/gituserconfigdialog.h"
#endif
#include "widgets/infomessagebox.h"
#include "widgets/newtemplatedialog.h"
#include "visithistorymanager.h"
#include "widgets/projectalreadyopendialog.h"
#include "widgets/searchdialog.h"


#include "settingsdialog/settingsdialog.h"
#include "compiler/compilermanager.h"
#include <qsynedit/document.h>
#include "cpprefacter.h"

#include "widgets/newprojectunitdialog.h"
#include "widgets/searchinfiledialog.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

static int findTabIndex(QTabWidget* tabWidget , QWidget* w) {
    for (int i=0;i<tabWidget->count();i++) {
        if (w==tabWidget->widget(i))
            return i;
    }    
    return -1;
}

MainWindow* pMainWindow;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow{parent},
      ui{new Ui::MainWindow},
      mFullInitialized{false},
      mSearchInFilesDialog{nullptr},
      mSearchDialog{nullptr},
      mQuitting{false},
      mOpeningFiles{false},
      mOpeningProject{false},
      mClosingProject{false},
      mCheckSyntaxInBack{false},
      mShouldRemoveAllSettings{false},
      mClosing{false},
      mClosingAll{false},
      mOpenningFiles{false},
      mSystemTurnedOff{false},
      mCompileIssuesState{CompileIssuesState::None}

{
    ui->setupUi(this);
    ui->cbProblemCaseValidateType->blockSignals(true);
    ui->cbProblemCaseValidateType->addItem(tr("Exact"));
    ui->cbProblemCaseValidateType->addItem(tr("Ignore leading/trailing spaces"));
    ui->cbProblemCaseValidateType->addItem(tr("Ignore spaces"));
    ui->cbProblemCaseValidateType->blockSignals(false);
    addActions( this->findChildren<QAction *>(QString(), Qt::FindChildrenRecursively));

    //custom actions
    createCustomActions();

    // status bar

    //statusBar takes the owner ships
    mFileInfoStatus=new QLabel();
    mFileEncodingStatus = new LabelWithMenu();
    mFileModeStatus = new QLabel();

    mFileInfoStatus->setStyleSheet("margin-left:5px; margin-right:5px");
    mFileEncodingStatus->setStyleSheet("margin-left:5px; margin-right:5px");
    mFileModeStatus->setStyleSheet("margin-left:5px; margin-right:5px");
    prepareTabInfosData();
    prepareTabMessagesData();
    ui->statusbar->insertPermanentWidget(0,mFileModeStatus);
    ui->statusbar->insertPermanentWidget(0,mFileEncodingStatus);
    ui->statusbar->insertPermanentWidget(0,mFileInfoStatus);
    //delete in the destructor
    mEditorList = new EditorList(ui->EditorTabsLeft,
                                 ui->EditorTabsRight,
                                 ui->splitterEditorPanel,
                                 ui->EditorPanel);
    connect(mEditorList, &EditorList::editorRenamed,
            this, &MainWindow::onEditorRenamed);
    connect(mEditorList, &EditorList::editorClosed,
               this, &MainWindow::onEditorClosed);
    mProject = nullptr;
    //delete in the destructor
    mProjectProxyModel = new ProjectModelSortFilterProxy();
    QItemSelectionModel *m=ui->projectView->selectionModel();
    ui->projectView->setModel(mProjectProxyModel);
    delete m;
    mProjectProxyModel->setDynamicSortFilter(false);
    ui->EditorTabsRight->setVisible(false);

    mVisitHistoryManager = std::make_shared<VisitHistoryManager>(
                includeTrailingPathDelimiter(pSettings->dirs().config())
                                                                 +DEV_HISTORY_FILE);
    mVisitHistoryManager->load();

    //toolbar takes the owner
    mCompilerSet = new QComboBox();
    mCompilerSet->setMinimumWidth(200);
    mCompilerSet->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui->toolbarCompilerSet->insertWidget(ui->actionCompiler_Options, mCompilerSet);
    ui->toolbarCompilerSet->insertSeparator(ui->actionCompiler_Options);
    connect(mCompilerSet,QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCompilerSetChanged);
    //updateCompilerSet();

    mCompilerManager = std::make_shared<CompilerManager>();
    mDebugger = std::make_shared<Debugger>();

    m=ui->tblBreakpoints->selectionModel();
    ui->tblBreakpoints->setModel(mDebugger->breakpointModel().get());
    delete m;
    ui->tblBreakpoints->setTextElideMode(Qt::ElideRight);
    ui->tblBreakpoints->setWordWrap(false);
    connect(ui->tblBreakpoints, &QTableView::doubleClicked,
            this, &MainWindow::onBreakpointTableDoubleClicked);

    m=ui->tblStackTrace->selectionModel();
    ui->tblStackTrace->setModel(mDebugger->backtraceModel().get());
    delete m;
    ui->tblStackTrace->setTextElideMode(Qt::ElideRight);
    ui->tblStackTrace->setWordWrap(false);

    m=ui->watchView->selectionModel();
    ui->watchView->setModel(mDebugger->watchModel().get());
    delete m;

    m=ui->tblMemoryView->selectionModel();
    ui->tblMemoryView->setModel(mDebugger->memoryModel().get());
    delete m;

    ui->tblMemoryView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    try {
        mDebugger->loadForNonproject(includeTrailingPathDelimiter(pSettings->dirs().config())
                                           +DEV_DEBUGGER_FILE);
    } catch (FileError &e) {
        QMessageBox::warning(nullptr,
                             tr("Error"),
                             e.reason());
    }

    //mainmenu takes the owner
    mMenuNew = new QMenu(this);
    mMenuNew->setTitle(tr("New"));
    mMenuNew->addAction(ui->actionNew);
    mMenuNew->addAction(ui->actionNew_GAS_File);
    mMenuNew->addAction(ui->actionNew_Text_File);
    mMenuNew->addAction(ui->actionNew_Project);
    mMenuNew->addSeparator();
    mMenuNew->addAction(ui->actionNew_Template);
    mMenuNew->addSeparator();
    mMenuNew->addAction(ui->actionNew_Class);
    mMenuNew->addAction(ui->actionNew_Header);

    ui->menuFile->insertMenu(ui->actionOpen,mMenuNew);

    mMenuExport = new QMenu(tr("Export"));
    mMenuExport->addAction(ui->actionExport_As_RTF);
    mMenuExport->addAction(ui->actionExport_As_HTML);
    ui->menuFile->insertMenu(ui->actionPrint,mMenuExport);

    buildEncodingMenu();

    mMenuRecentProjects = new QMenu();
    mMenuRecentProjects->setTitle(tr("Recent Projects"));
    ui->menuFile->insertMenu(ui->actionExit, mMenuRecentProjects);

    mMenuRecentFiles = new QMenu();
    mMenuRecentFiles->setTitle(tr("Recent Files"));
    ui->menuFile->insertMenu(ui->actionExit, mMenuRecentFiles);
    ui->menuFile->insertSeparator(ui->actionExit);
    rebuildOpenedFileHisotryMenu();

    mMenuInsertCodeSnippet = new QMenu();
    mMenuInsertCodeSnippet->setTitle(tr("Insert Snippet"));
    ui->menuCode->insertMenu(ui->actionTrim_trailing_spaces,mMenuInsertCodeSnippet);
    ui->menuCode->insertSeparator(ui->actionTrim_trailing_spaces);
    connect(mMenuInsertCodeSnippet,&QMenu::aboutToShow,
            this, &MainWindow::onShowInsertCodeSnippetMenu);

    mCPUDialog = nullptr;

    ui->cbReplaceInHistory->completer()->setCaseSensitivity(Qt::CaseSensitive);
    ui->cbEvaluate->completer()->setCaseSensitivity(Qt::CaseSensitive);
    ui->cbMemoryAddress->completer()->setCaseSensitivity(Qt::CaseSensitive);
    ui->cbFilesPath->completer()->setCaseSensitivity(Qt::CaseInsensitive);

    connect(ui->debugConsole,&QConsole::commandInput,this,&MainWindow::onDebugCommandInput);
    connect(ui->cbEvaluate->lineEdit(), &QLineEdit::returnPressed,
            this, &MainWindow::onDebugEvaluateInput);
    connect(ui->cbMemoryAddress->lineEdit(), &QLineEdit::returnPressed,
            this, &MainWindow::onDebugMemoryAddressInput);

    mTodoParser = std::make_shared<TodoParser>();
    mSymbolUsageManager = std::make_shared<SymbolUsageManager>();
    try {
        mSymbolUsageManager->load();
    } catch (FileError &e) {
        QMessageBox::warning(nullptr,
                         tr("Error"),
                         e.reason());
    }

    mCodeSnippetManager = std::make_shared<CodeSnippetsManager>();
    try {
        mCodeSnippetManager->load();
    } catch (FileError &e) {
        QMessageBox::warning(nullptr,
                             tr("Error"),
                             e.reason());
    }
    mToolsManager = std::make_shared<ToolsManager>();
    try {
        mToolsManager->load();
    } catch (FileError &e) {
        QMessageBox::warning(nullptr,
                             tr("Error"),
                             e.reason());
    }
    mBookmarkModel = std::make_shared<BookmarkModel>();
    try {
        mBookmarkModel->loadBookmarks(includeTrailingPathDelimiter(pSettings->dirs().config())
                         +DEV_BOOKMARK_FILE);
    } catch (FileError &e) {
        QMessageBox::warning(nullptr,
                             tr("Error"),
                             e.reason());
    }

    m=ui->tableBookmark->selectionModel();
    ui->tableBookmark->setModel(mBookmarkModel.get());
    delete m;

    mSearchResultTreeModel = std::make_shared<SearchResultTreeModel>(&mSearchResultModel);
    mSearchResultListModel = std::make_shared<SearchResultListModel>(&mSearchResultModel);
    mSearchViewDelegate = std::make_shared<SearchResultTreeViewDelegate>(mSearchResultTreeModel);

    ui->cbSearchHistory->setModel(mSearchResultListModel.get());

    m=ui->searchView->selectionModel();
    ui->searchView->setModel(mSearchResultTreeModel.get());
    delete m;
    ui->searchView->setItemDelegate(mSearchViewDelegate.get());
    m=ui->tableTODO->selectionModel();
    ui->tableTODO->setModel(&mTodoModel);
    delete m;
    connect(mSearchResultTreeModel.get() , &QAbstractItemModel::modelReset,
            ui->searchView,&QTreeView::expandAll);
    ui->replacePanel->setVisible(false);
    ui->tabProblem->setEnabled(false);

    //problem set
    mOJProblemSetNameCounter=1;
    mOJProblemSetModel.rename(tr("Problem Set %1").arg(mOJProblemSetNameCounter));

    m=ui->lstProblemSet->selectionModel();
    ui->lstProblemSet->setModel(&mOJProblemSetModel);
    delete m;

    m=ui->tblProblemCases->selectionModel();
    ui->tblProblemCases->setModel(&mOJProblemModel);
    delete m;
    connect(ui->lstProblemSet->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, &MainWindow::onProblemSetIndexChanged);
    connect(ui->tblProblemCases->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, &MainWindow::onProblemCaseIndexChanged);
    connect(&mOJProblemSetModel, &OJProblemSetModel::problemNameChanged,
            this , &MainWindow::onProblemNameChanged);
    ui->pbProblemCases->setVisible(false);
    connect(&mCCHandler, &CompetitiveCompanionHandler::newProblemReceived,
            this, &MainWindow::onNewProblemReceived);

    connect(&mOJProblemModel, &OJProblemModel::dataChanged,
            this, &MainWindow::updateProblemTitle);
    try {
        int currentIndex=-1;
        mOJProblemSetModel.load(currentIndex);
        if (currentIndex>=0) {
            QModelIndex index = mOJProblemSetModel.index(currentIndex,0);
            ui->lstProblemSet->setCurrentIndex(index);
            ui->lstProblemSet->scrollTo(index);
        }
    } catch (FileError& e) {
        QMessageBox::warning(nullptr,
                             tr("Error"),
                             e.reason());
    }

    //files view
    m=ui->treeFiles->selectionModel();
    ui->treeFiles->setModel(&mFileSystemModel);
    delete m;
    // connect(&mFileSystemModel, &QFileSystemModel::layoutChanged,
    //         this, &MainWindow::onFileSystemModelLayoutChanged, Qt::QueuedConnection);
    connect(&mFileSystemModel, &QFileSystemModel::fileRenamed,
            this, &MainWindow::onFileSystemModelLayoutChanged, Qt::QueuedConnection);
    connect(&mFileSystemModel, &QFileSystemModel::fileRenamed,
            this, &MainWindow::onFileRenamedInFileSystemModel);
    mFileSystemModel.setReadOnly(false);
    mFileSystemModel.setIconProvider(&mFileSystemModelIconProvider);

    mFileSystemModel.setNameFilters(pSystemConsts->defaultFileNameFilters());
    mFileSystemModel.setNameFilterDisables(true);
    //setFilesViewRoot(pSettings->environment().currentFolder());
    for (int i=1;i<mFileSystemModel.columnCount();i++) {
        ui->treeFiles->hideColumn(i);
    }
    FilenameEditDelegate *filenameEditDelegate = new FilenameEditDelegate(ui->treeFiles);
    ui->treeFiles->setItemDelegate(filenameEditDelegate);
    connect(ui->cbFilesPath->lineEdit(),&QLineEdit::returnPressed,
            this,&MainWindow::onFilesViewPathChanged);
    connect(ui->cbFilesPath, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFilesViewPathChanged);

    //class browser
    ui->classBrowser->setUniformRowHeights(true);
    m=ui->classBrowser->selectionModel();
    ui->classBrowser->setModel(&mClassBrowserModel);
    delete m;

    connect(&mClassBrowserModel, &ClassBrowserModel::refreshStarted,
            this, &MainWindow::onClassBrowserRefreshStart);
    connect(&mClassBrowserModel, &ClassBrowserModel::refreshEnd,
            this, &MainWindow::onClassBrowserRefreshEnd);

    connect(&mFileSystemWatcher,&QFileSystemWatcher::fileChanged,
            this, &MainWindow::onFileChanged);
    connect(&mFileSystemWatcher,&QFileSystemWatcher::directoryChanged,
            this, &MainWindow::onDirChanged);

    mStatementColors = std::make_shared<QHash<StatementKind, PColorSchemeItem> >();
    mCompletionPopup = std::make_shared<CodeCompletionPopup>();
    mCompletionPopup->setColors(mStatementColors);
    mHeaderCompletionPopup = std::make_shared<HeaderCompletionPopup>();
    mFunctionTip = std::make_shared<FunctionTooltipWidget>();

    mClassBrowserModel.setColors(mStatementColors);

    connect(&mAutoSaveTimer, &QTimer::timeout,
            this, &MainWindow::onAutoSaveTimeout);
    resetAutoSaveTimer();

    connect(ui->menuFile, &QMenu::aboutToShow,
            this,&MainWindow::rebuildOpenedFileHisotryMenu);

    connect(ui->menuProject, &QMenu::aboutToShow,
            this, &MainWindow::updateProjectActions);

#ifdef ARCH_X86
        ui->actionIA_32_Assembly_Language_Reference_Manual->setVisible(true);
        ui->actionx86_Assembly_Language_Reference_Manual->setVisible(false);
#elif defined(ARCH_X86_64)
        ui->actionIA_32_Assembly_Language_Reference_Manual->setVisible(false);
        ui->actionx86_Assembly_Language_Reference_Manual->setVisible(true);
#else
        ui->actionIA_32_Assembly_Language_Reference_Manual->setVisible(false);
        ui->actionx86_Assembly_Language_Reference_Manual->setVisible(false);
#endif
    ui->actionEGE_Manual->setVisible(pSettings->environment().language()=="zh_CN");
    ui->actionOI_Wiki->setVisible(pSettings->environment().language()=="zh_CN");
    ui->actionTurtle_Graphics_Manual->setVisible(pSettings->environment().language()=="zh_CN");
    ui->actionDocument->setVisible(pSettings->environment().language()=="zh_CN");

    connect(ui->EditorTabsLeft, &EditorsTabWidget::middleButtonClicked,
            this, &MainWindow::on_EditorTabsLeft_tabCloseRequested);

    connect(ui->EditorTabsRight, &EditorsTabWidget::middleButtonClicked,
            this, &MainWindow::on_EditorTabsRight_tabCloseRequested);

#ifdef ENABLE_VCS
    //git menu
    connect(ui->menuGit, &QMenu::aboutToShow,
            this, &MainWindow::updateVCSActions);
#endif
    //set action group name (show in the option / environment / shortcuts)
    ui->actionNew->setData(mMenuNew->title());
    ui->actionNew_GAS_File->setData(mMenuNew->title());
    ui->actionNew_Text_File->setData(mMenuNew->title());
    ui->actionNew_Project->setData(mMenuNew->title());

    ui->actionRemove_Watch->setData(tr("Debug"));
    ui->actionRemove_All_Watches->setData(tr("Debug"));
    ui->actionModify_Watch->setData(tr("Debug"));
    ui->actionRemove_All_Watches->setData(tr("Debug"));
    ui->actionBreakpoint_property->setData(tr("Debug"));

    initToolButtons();
    buildContextMenus();
    updateAppTitle();
    initEditorActions();
    //applySettings();
    applyUISettings();
    initDocks();
    updateProjectView();
    updateEditorActions();
    updateCaretActions();
    updateEditorColorSchemes();
    updateTools();
    updateShortcuts();
    updateEditorSettings();
    //updateEditorBookmarks();
}

MainWindow::~MainWindow()
{
    mQuitting=true;
    if (mProject)
        mProject=nullptr;
    delete mProjectProxyModel;
    delete mEditorList;
    delete ui;
}

void MainWindow::updateForEncodingInfo(bool clear)
{
    Editor * editor = mEditorList->getEditor();
    updateForEncodingInfo(editor,clear);
}

void MainWindow::updateForEncodingInfo(const Editor* editor, bool clear) {
    if (!clear && editor!=nullptr) {
        if (editor->encodingOption() != editor->fileEncoding()) {
            mFileEncodingStatus->setText(
                        QString(" %1(%2) ")
                        .arg(QString(editor->encodingOption())
                             ,QString(editor->fileEncoding())));
        } else {
            mFileEncodingStatus->setText(
                        QString(" %1 ")
                        .arg(QString(editor->encodingOption()))
                        );
        }
        //ui->actionAuto_Detect->setChecked(editor->encodingOption() == ENCODING_AUTO_DETECT);
        ui->actionEncode_in_ANSI->setChecked(editor->encodingOption() == ENCODING_SYSTEM_DEFAULT);
        ui->actionEncode_in_UTF_8->setChecked(editor->encodingOption() == ENCODING_UTF8);
        ui->actionEncode_in_UTF_8_BOM->setChecked(editor->encodingOption() == ENCODING_UTF8_BOM);
    } else {
        mFileEncodingStatus->setText("");
        //ui->actionAuto_Detect->setChecked(false);
        ui->actionEncode_in_ANSI->setChecked(false);
        ui->actionEncode_in_UTF_8->setChecked(false);
        ui->actionEncode_in_UTF_8_BOM->setChecked(false);
    }
}

void MainWindow::updateStatusbarForLineCol(bool clear)
{
    Editor* e = mEditorList->getEditor();
    updateStatusbarForLineCol(e,clear);
}

void MainWindow::updateEditorSettings()
{
    pIconsManager->updateEditorGutterIcons(
                pSettings->environment().iconSet(),
                calIconSize(pSettings->editor().fontName(),pSettings->editor().fontSize())
                );
    mEditorList->applySettings();
}

void MainWindow::updateEditorBookmarks()
{
    for (int i=0;i<mEditorList->pageCount();i++) {
        Editor * e=(*mEditorList)[i];
        e->resetBookmarks();
    }
}

void MainWindow::updateEditorBreakpoints()
{
    for (int i=0;i<mEditorList->pageCount();i++) {
        Editor * e=(*mEditorList)[i];
        e->resetBreakpoints();
    }
}

void MainWindow::updateEditorActions()
{
    Editor* e = mEditorList->getEditor();
    updateEditorActions(e);
}

void MainWindow::updateEncodingActions(const Editor *e)
{
    if (e==nullptr) {
        ui->actionAuto_Detect->setEnabled(false);
        ui->actionEncode_in_ANSI->setEnabled(false);
        ui->actionEncode_in_UTF_8->setEnabled(false);
        ui->actionEncode_in_UTF_8_BOM->setEnabled(false);
        mMenuEncoding->setEnabled(false);
        ui->actionConvert_to_ANSI->setEnabled(false);
        ui->actionConvert_to_UTF_8->setEnabled(false);
        ui->actionConvert_to_UTF_8_BOM->setEnabled(false);
    } else {
        ui->actionAuto_Detect->setEnabled(true);
        ui->actionEncode_in_ANSI->setEnabled(true);
        ui->actionEncode_in_UTF_8->setEnabled(true);
        ui->actionEncode_in_UTF_8_BOM->setEnabled(true);
        mMenuEncoding->setEnabled(true);
        ui->actionConvert_to_ANSI->setEnabled(e->encodingOption()!=ENCODING_SYSTEM_DEFAULT
                && e->fileEncoding()!=ENCODING_SYSTEM_DEFAULT);
        ui->actionConvert_to_UTF_8->setEnabled(e->encodingOption()!=ENCODING_UTF8 && e->fileEncoding()!=ENCODING_UTF8);
        ui->actionConvert_to_UTF_8_BOM->setEnabled(e->encodingOption()!=ENCODING_UTF8_BOM && e->fileEncoding()!=ENCODING_UTF8_BOM);
    }
}

void MainWindow::updateEditorActions(const Editor *e)
{
    ui->menuCode->menuAction()->setVisible(mEditorList->pageCount()>0);
    ui->menuEdit->menuAction()->setVisible(mEditorList->pageCount()>0);
    ui->menuSelection->menuAction()->setVisible(mEditorList->pageCount()>0);
    ui->menuRefactor->menuAction()->setVisible(mEditorList->pageCount()>0);

    //it's not a compile action, but put here for convinience
    ui->actionSaveAll->setEnabled(
                (mProject!=nullptr || mEditorList->pageCount()>0));
    if (e==nullptr || !e->hasFocus()) {
        ui->actionCopy->setEnabled(false);
        ui->actionCut->setEnabled(false);
        ui->actionFoldAll->setEnabled(false);
        ui->actionIndent->setEnabled(false);
        ui->actionPaste->setEnabled(false);
        ui->actionRedo->setEnabled(false);
        ui->actionSave->setEnabled(false);
        ui->actionSaveAs->setEnabled(false);
        ui->actionExport_As_HTML->setEnabled(false);
        ui->actionExport_As_RTF->setEnabled(false);
        ui->actionPrint->setEnabled(false);
        ui->actionToggleComment->setEnabled(false);
        ui->actionToggle_Block_Comment->setEnabled(false);
        ui->actionUnIndent->setEnabled(false);
        ui->actionUndo->setEnabled(false);
        ui->actionUnfoldAll->setEnabled(false);
        ui->actionDelete_Line->setEnabled(false);
        ui->actionDelete_Word->setEnabled(false);
        ui->actionDuplicate_Line->setEnabled(false);
        ui->actionDelete_to_BOL->setEnabled(false);
        ui->actionDelete_to_EOL->setEnabled(false);
        ui->actionDelete_to_Word_End->setEnabled(false);
        ui->actionDelete_Last_Word->setEnabled(false);

        ui->menuMove_Caret->setEnabled(false);
        ui->actionPage_Up->setEnabled(false);
        ui->actionPage_Down->setEnabled(false);
        ui->actionGoto_Line_Start->setEnabled(false);
        ui->actionGoto_Line_End->setEnabled(false);
        ui->actionGoto_File_Start->setEnabled(false);
        ui->actionGoto_File_End->setEnabled(false);
        ui->actionGoto_Page_Start->setEnabled(false);
        ui->actionGoto_Page_End->setEnabled(false);

        ui->actionSelectAll->setEnabled(false);
        ui->actionSelect_Word->setEnabled(false);
        ui->actionMove_Selection_Up->setEnabled(false);
        ui->actionMove_Selection_Down->setEnabled(false);
        ui->actionPage_Up_and_Select->setEnabled(false);
        ui->actionPage_Down_and_Select->setEnabled(false);
        ui->actionGoto_Line_Start_and_Select->setEnabled(false);
        ui->actionGoto_Line_End_and_Select->setEnabled(false);
        ui->actionGoto_File_Start_and_Select->setEnabled(false);
        ui->actionGoto_File_End_and_Select->setEnabled(false);
        ui->actionGoto_Page_Start_and_Select->setEnabled(false);
        ui->actionGoto_Page_End_and_Select->setEnabled(false);

        ui->actionFind->setEnabled(false);
        ui->actionReplace->setEnabled(false);
        ui->actionFind_Next->setEnabled(false);
        ui->actionFind_Previous->setEnabled(false);

        //code
        ui->actionReformat_Code->setEnabled(false);

        ui->actionClose->setEnabled(false);
        ui->actionClose_All->setEnabled(false);
        ui->actionClose_Others->setEnabled(false);

        ui->actionToggle_Bookmark->setEnabled(false);
        ui->actionModify_Bookmark_Description->setEnabled(false);

        ui->actionMatch_Bracket->setEnabled(false);
        ui->actionGo_to_Line->setEnabled(false);
        ui->actionGoto_block_start->setEnabled(false);
        ui->actionGoto_block_end->setEnabled(false);
        ui->actionTrim_trailing_spaces->setEnabled(false);
        mMenuInsertCodeSnippet->setEnabled(false);

        ui->actionRename_Symbol->setEnabled(false);

        ui->actionLocate_in_Files_View->setEnabled(false);
        ui->actionToggle_Readonly->setEnabled(false);

        ui->actionGoto_Declaration->setEnabled(false);
        ui->actionGoto_Definition->setEnabled(false);
        ui->actionFind_references->setEnabled(false);

        ui->actionMove_To_Other_View->setEnabled(false);
    } else {
        ui->actionCopy->setEnabled(true);
        ui->actionCut->setEnabled(true);
        ui->actionFoldAll->setEnabled(e->lineCount()>0);
        ui->actionIndent->setEnabled(!e->readOnly());
        ui->actionPaste->setEnabled(!e->readOnly());
        ui->actionRedo->setEnabled(e->canRedo());
        ui->actionUndo->setEnabled(e->canUndo());
        ui->actionSave->setEnabled(!e->readOnly());
        ui->actionSaveAs->setEnabled(true);
        ui->actionExport_As_HTML->setEnabled(true);
        ui->actionExport_As_RTF->setEnabled(true);
        ui->actionPrint->setEnabled(true);
        ui->actionToggleComment->setEnabled(!e->readOnly() && e->lineCount()>0);
        ui->actionToggle_Block_Comment->setEnabled(!e->readOnly() && e->selAvail());
        ui->actionUnIndent->setEnabled(!e->readOnly() && e->lineCount()>0);
        ui->actionUnfoldAll->setEnabled(e->lineCount()>0);
        ui->actionDelete_Line->setEnabled(!e->readOnly() && e->lineCount()>0);
        ui->actionDelete_Word->setEnabled(!e->readOnly() && e->lineCount()>0);
        ui->actionDuplicate_Line->setEnabled(!e->readOnly() && e->lineCount()>0);
        ui->actionDelete_to_BOL->setEnabled(!e->readOnly() && e->lineCount()>0);
        ui->actionDelete_to_EOL->setEnabled(!e->readOnly() && e->lineCount()>0);
        ui->actionDelete_to_Word_End->setEnabled(!e->readOnly() && e->lineCount()>0);
        ui->actionDelete_Last_Word->setEnabled(!e->readOnly() && e->lineCount()>0);

        ui->menuMove_Caret->setEnabled(true);
        ui->actionPage_Up->setEnabled(true);
        ui->actionPage_Down->setEnabled(true);
        ui->actionGoto_Line_Start->setEnabled(true);
        ui->actionGoto_Line_End->setEnabled(true);
        ui->actionGoto_File_Start->setEnabled(true);
        ui->actionGoto_File_End->setEnabled(true);
        ui->actionGoto_Page_Start->setEnabled(true);
        ui->actionGoto_Page_End->setEnabled(true);

        ui->actionSelectAll->setEnabled(e->lineCount()>0);
        ui->actionSelect_Word->setEnabled(true);
        ui->actionMove_Selection_Up->setEnabled(true);
        ui->actionMove_Selection_Down->setEnabled(true);
        ui->actionPage_Up_and_Select->setEnabled(true);
        ui->actionPage_Down_and_Select->setEnabled(true);
        ui->actionGoto_Line_Start_and_Select->setEnabled(true);
        ui->actionGoto_Line_End_and_Select->setEnabled(true);
        ui->actionGoto_File_Start_and_Select->setEnabled(true);
        ui->actionGoto_File_End_and_Select->setEnabled(true);
        ui->actionGoto_Page_Start_and_Select->setEnabled(true);
        ui->actionGoto_Page_End_and_Select->setEnabled(true);

        ui->actionFind->setEnabled(true);
        ui->actionReplace->setEnabled(true);
        ui->actionFind_Next->setEnabled(true);
        ui->actionFind_Previous->setEnabled(true);

        //code
        ui->actionReformat_Code->setEnabled(isCFile(e->filename()) || isHFile(e->filename()));

        ui->actionClose->setEnabled(true);
        ui->actionClose_All->setEnabled(true);
        ui->actionClose_Others->setEnabled(mEditorList->pageCount()>1);

        int line = e->caretY();
        ui->actionToggle_Bookmark->setEnabled(e->lineCount()>0);
        ui->actionModify_Bookmark_Description->setEnabled(e->hasBookmark(line));

        ui->actionMatch_Bracket->setEnabled(true);
        ui->actionGo_to_Line->setEnabled(true);
        ui->actionGoto_block_start->setEnabled(true);
        ui->actionGoto_block_end->setEnabled(true);
        ui->actionTrim_trailing_spaces->setEnabled(true);
        mMenuInsertCodeSnippet->setEnabled(true);

        ui->actionRename_Symbol->setEnabled(true);

        ui->actionLocate_in_Files_View->setEnabled(!e->isNew());
        ui->actionToggle_Readonly->setEnabled(!e->modified());

        //these actions needs parser
        ui->actionGoto_Declaration->setEnabled(e->parser()!=nullptr);
        ui->actionGoto_Definition->setEnabled(e->parser()!=nullptr);
        ui->actionFind_references->setEnabled(e->parser()!=nullptr);
        ui->actionMove_To_Other_View->setEnabled(editorList()->pageCount()>1);
    }

    updateEncodingActions(e);
    updateCompileActions(e);
    updateCompilerSet(e);
}


void MainWindow::updateProjectActions()
{
    bool hasProject = (mProject != nullptr);
    ui->menuProject->menuAction()->setVisible(hasProject);

    ui->actionNew_Template->setEnabled(hasProject);
    ui->actionView_Makefile->setEnabled(hasProject);
    ui->actionProject_New_File->setEnabled(hasProject);
    ui->actionAdd_to_project->setEnabled(hasProject);
    ui->actionRemove_from_project->setEnabled(hasProject && ui->projectView->selectionModel()->selectedIndexes().count()>0);
    ui->actionMakeClean->setEnabled(hasProject);
    ui->actionProject_options->setEnabled(hasProject);
    ui->actionClose_Project->setEnabled(hasProject);
    ui->actionNew_Class->setEnabled(hasProject);
    ui->actionNew_Header->setEnabled(hasProject);
    ui->actionProject_Open_Folder_In_Explorer->setEnabled(hasProject);
    ui->actionProject_Open_In_Terminal->setEnabled(hasProject);
    updateCompileActions();
}

void MainWindow::updateCompileActions() {
    updateCompileActions(mEditorList->getEditor());
}

void MainWindow::updateCompileActions(const Editor *e)
{
    if (mCompilerManager->compiling()
            //|| mCompilerManager->backgroundSyntaxChecking()
            || mCompilerManager->running() || mDebugger->executing()) {
        ui->actionCompile->setEnabled(false);
        ui->actionRun->setEnabled(false);
        ui->actionRebuild->setEnabled(false);
        ui->actionGenerate_Assembly->setEnabled(false);
        ui->actionDebug->setEnabled(false);
        mProblem_RunAllCases->setEnabled(false);
    } else {
        bool forProject=false;
        bool canRun = false;
        bool canDebug = false;
        bool canCompile = false;
        bool canGenerateAssembly=false;
        Settings::PCompilerSet set=pSettings->compilerSets().getSet(mCompilerSet->currentIndex());
        if (set) {
            if (e) {
                if (!e->inProject()) {
                    FileType fileType = getFileType(e->filename());
                    switch(fileType) {
                    case FileType::CSource:
                        canCompile = set->canCompileC();
                        //qDebug()<<(int)set->compilerType();
#ifdef ENABLE_SDCC
                        if (set->compilerType()!=CompilerType::SDCC)
#endif
                        {
                            canGenerateAssembly = canCompile;
                            canRun = canCompile ;
                        }
                        //qDebug()<<canCompile<<canRun;
                        canDebug = set->canDebug();
                        break;
                    case FileType::CppSource:
                        canCompile = set->canCompileCPP();
                        canGenerateAssembly = canCompile;
                        canRun = canCompile;
                        canDebug = set->canDebug();
                        break;
                    case FileType::GAS:
                        if (set->compilerType()==CompilerType::GCC
                                || set->compilerType()==CompilerType::GCC_UTF8) {
                            canCompile = true;
                            canRun = canCompile;
                            canDebug = set->canDebug();
                        }
                        break;
                    }
                } else {
                     forProject = (mProject!=nullptr);
                }
            }  else {
                forProject = (mProject!=nullptr);
            }
            if (forProject) {
                canCompile = true;
                canRun = (mProject->options().type !=ProjectType::DynamicLib)
                        && (mProject->options().type !=ProjectType::StaticLib);
                canDebug = set->canDebug() && canRun;
                if (e) {
                    FileType fileType = getFileType(e->filename());
                    if (fileType == FileType::CSource
                            || fileType == FileType::CppSource) {
                        canGenerateAssembly = true;
                    }
                }
            }
        }
        ui->actionCompile->setEnabled(canCompile);
        ui->actionRun->setEnabled(canRun);
        ui->actionRebuild->setEnabled(canCompile);
        ui->actionGenerate_Assembly->setEnabled(canGenerateAssembly);
        ui->actionDebug->setEnabled(canDebug);
        mProblem_RunAllCases->setEnabled(canRun && mOJProblemModel.count()>0);
    }
    if (!mDebugger->executing()) {
        disableDebugActions();
    }
    ui->actionStop_Execution->setEnabled(mCompilerManager->running() || mDebugger->executing());


}

void MainWindow::updateEditorColorSchemes()
{
    if (!mStatementColors)
        return;
    mStatementColors->clear();

    mEditorList->applyColorSchemes(pSettings->editor().colorScheme());
    QString schemeName = pSettings->editor().colorScheme();
    pColorManager->updateStatementColors(mStatementColors,schemeName);
    //color for code completion popup
    PColorSchemeItem item;
    QColor localHeaderColor=palette().color(QPalette::Text);
    QColor systemHeaderColor=palette().color(QPalette::Text);
    QColor projectHeaderColor=palette().color(QPalette::Text);
    QColor headerFolderColor=palette().color(QPalette::Text);
    QColor baseColor = palette().color(QPalette::Base);
    item = pColorManager->getItem(schemeName, SYNS_AttrPreprocessor);
    if (item) {
        localHeaderColor = item->foreground();
    }
    item = pColorManager->getItem(schemeName, SYNS_AttrPreprocessor);
    if (item) {
        systemHeaderColor = item->foreground();
    }
    item = pColorManager->getItem(schemeName, SYNS_AttrString);
    if (item) {
        projectHeaderColor = item->foreground();
    }
    item = pColorManager->getItem(schemeName, SYNS_AttrStringEscapeSequences);
    if (item) {
        headerFolderColor = item->foreground();
    }
    item = pColorManager->getItem(schemeName, COLOR_SCHEME_ERROR);
    if (item && haveGoodContrast(item->foreground(), baseColor)) {
        mErrorColor = item->foreground();
    } else {
        mErrorColor = palette().color(QPalette::Text);
    }
    ui->tableIssues->setErrorColor(mErrorColor);
    item = pColorManager->getItem(schemeName, COLOR_SCHEME_WARNING);
    if (item && haveGoodContrast(item->foreground(), baseColor)) {
        ui->tableIssues->setWarningColor(item->foreground());
    } else {
        ui->tableIssues->setWarningColor(palette().color(QPalette::Text));
    }
    item = pColorManager->getItem(schemeName, COLOR_SCHEME_TEXT);
    if (item) {
        QPalette pal = palette();
        pal.setColor(QPalette::Base,item->background());
        pal.setColor(QPalette::Text,item->foreground());
        mCompletionPopup->setPalette(pal);
        mHeaderCompletionPopup->setPalette(pal);
        ui->classBrowser->setPalette(pal);
        ui->txtProblemCaseInput->setPalette(pal);
        ui->txtProblemCaseExpected->setPalette(pal);
        ui->txtProblemCaseOutput->setPalette(pal);
    } else {
        QPalette pal = palette();
        mCompletionPopup->setPalette(pal);
        mHeaderCompletionPopup->setPalette(pal);
        ui->classBrowser->setPalette(pal);
        ui->txtProblemCaseInput->setPalette(pal);
        ui->txtProblemCaseExpected->setPalette(pal);
        ui->txtProblemCaseOutput->setPalette(pal);
    }
    item = pColorManager->getItem(schemeName, COLOR_SCHEME_GUTTER);
    if (item) {
        ui->txtProblemCaseInput->setLineNumberAreaForeground(item->foreground());
        ui->txtProblemCaseInput->setLineNumberAreaBackground(item->background());
        ui->txtProblemCaseOutput->setLineNumberAreaForeground(item->foreground());
        ui->txtProblemCaseOutput->setLineNumberAreaBackground(item->background());
        ui->txtProblemCaseExpected->setLineNumberAreaForeground(item->foreground());
        ui->txtProblemCaseExpected->setLineNumberAreaBackground(item->background());
    } else {
        QPalette pal = palette();
        ui->txtProblemCaseInput->setLineNumberAreaForeground(pal.color(QPalette::ButtonText));
        ui->txtProblemCaseInput->setLineNumberAreaBackground(pal.color(QPalette::Button));
        ui->txtProblemCaseOutput->setLineNumberAreaForeground(pal.color(QPalette::ButtonText));
        ui->txtProblemCaseOutput->setLineNumberAreaBackground(pal.color(QPalette::Button));
        ui->txtProblemCaseExpected->setLineNumberAreaForeground(pal.color(QPalette::ButtonText));
        ui->txtProblemCaseExpected->setLineNumberAreaBackground(pal.color(QPalette::Button));
    }
    item = pColorManager->getItem(schemeName, COLOR_SCHEME_GUTTER_ACTIVE_LINE);
    if (item) {
        ui->txtProblemCaseInput->setLineNumberAreaCurrentLine(item->foreground());
        ui->txtProblemCaseOutput->setLineNumberAreaCurrentLine(item->foreground());
        ui->txtProblemCaseExpected->setLineNumberAreaCurrentLine(item->foreground());
    } else {
        QPalette pal = palette();
        ui->txtProblemCaseInput->setLineNumberAreaCurrentLine(pal.color(QPalette::ButtonText));
        ui->txtProblemCaseOutput->setLineNumberAreaCurrentLine(pal.color(QPalette::ButtonText));
        ui->txtProblemCaseExpected->setLineNumberAreaCurrentLine(pal.color(QPalette::ButtonText));
    }
    mHeaderCompletionPopup->setSuggestionColor(localHeaderColor,
                                               projectHeaderColor,
                                               systemHeaderColor,
                                               headerFolderColor);
}

void MainWindow::applySettings()
{
    ThemeManager themeManager;
    PAppTheme appTheme;
    try {
        appTheme = themeManager.theme(pSettings->environment().theme());
    } catch (FileError e) {
        QMessageBox::critical(this,
                              tr("Load Theme Error"),
                              e.reason());
        appTheme = AppTheme::fallbackTheme();
    }
#ifdef ENABLE_LUA_ADDON
    catch (AddOn::LuaError e) {
        QMessageBox::critical(this,
                              tr("Load Theme Error"),
                              e.reason());
        appTheme = AppTheme::fallbackTheme();
    }
#endif
    const QString& style = appTheme->style();
    if (style == "RedPandaDarkFusion")
        QApplication::setStyle(new DarkFusionStyle());//app takes the onwership
    else if (style == "RedPandaLightFusion")
        QApplication::setStyle(new LightFusionStyle());//app takes the onwership
    else
        QApplication::setStyle(style);
    qApp->setPalette(appTheme->palette());
    //fix for qstatusbar bug
    mFileEncodingStatus->setPalette(appTheme->palette());
    mFileModeStatus->setPalette(appTheme->palette());
    mFileInfoStatus->setPalette(appTheme->palette());

    updateEditorColorSchemes();

    QFont font(pSettings->environment().interfaceFont());
    font.setPixelSize(pointToPixel(pSettings->environment().interfaceFontSize()));
    font.setStyleStrategy(QFont::PreferAntialias);
    qApp->setFont(font);
    this->setFont(font);
    for (QWidget* p:findChildren<QWidget*>()) {
        if (mCPUDialog && (p==mCPUDialog || mCPUDialog->isAncestorOf(p)))
            continue;
        p->setFont(font);
    }
    if (pSettings->environment().useCustomIconSet()) {
        QString customIconSetFolder = pSettings->dirs().config(Settings::Dirs::DataType::IconSet);
        pIconsManager->prepareCustomIconSet(customIconSetFolder);
        pIconsManager->setIconSetsFolder(customIconSetFolder);
    }
    pIconsManager->updateParserIcons(
                pSettings->environment().iconSet(),
                calIconSize(
                    pSettings->environment().interfaceFont(),
                    pSettings->environment().interfaceFontSize()));

    QFont caseEditorFont(pSettings->executor().caseEditorFontName());
    caseEditorFont.setPixelSize(pointToPixel(pSettings->executor().caseEditorFontSize()));
    font.setStyleStrategy(QFont::PreferAntialias);
    ui->txtProblemCaseInput->setFont(caseEditorFont);
    ui->lblProblemCaseInput->setFont(caseEditorFont);
    ui->txtProblemCaseOutput->setFont(caseEditorFont);
    ui->lblProblemCaseOutput->setFont(caseEditorFont);
    ui->txtProblemCaseExpected->setFont(caseEditorFont);
    ui->lblProblemCaseExpected->setFont(caseEditorFont);

    mCCHandler.stop();
    mCCHandler.start();

    showHideInfosTab(ui->tabProblemSet,pSettings->ui().showProblemSet()
                     && pSettings->executor().enableProblemSet());
    showHideMessagesTab(ui->tabProblem, pSettings->ui().showProblem()
                        && pSettings->executor().enableProblemSet());

    ui->cbProblemCaseValidateType->setCurrentIndex((int)(pSettings->executor().problemCaseValidateType()));
    ui->actionInterrupt->setVisible(mDebugger && mDebugger->useDebugServer());
    //icon sets for editors
    updateEditorSettings();
    updateDebuggerSettings();
    updateActionIcons();

    //icon sets for files view
    pIconsManager->updateFileSystemIcons(
                pSettings->environment().iconSet(),
                calIconSize(
                    pSettings->environment().interfaceFont(),
                    pSettings->environment().interfaceFontSize()));
    if (!mFileSystemModel.rootPath().isEmpty() && mFileSystemModel.rootPath()!=".")
        setFilesViewRoot(pSettings->environment().currentFolder());
//    for (int i=0;i<ui->cbFilesPath->count();i++) {
//        ui->cbFilesPath->setItemIcon(i,pIconsManager->getIcon(IconsManager::FILESYSTEM_GIT));
//    }
    stretchExplorerPanel(!ui->tabExplorer->isShrinked());
    stretchMessagesPanel(!ui->tabMessages->isShrinked());
}

void MainWindow::applyUISettings()
{
    const Settings::UI& settings = pSettings->ui();
    ui->chkOpenFileInEditors->setChecked(settings.openEditorsWhenReplace());
    restoreGeometry(settings.mainWindowGeometry());
    restoreState(settings.mainWindowState());
    ui->actionTool_Window_Bars->setChecked(settings.showToolWindowBars());
    ui->tabExplorer->setVisible(settings.showToolWindowBars());
    ui->tabMessages->setVisible(settings.showToolWindowBars());
    ui->actionStatus_Bar->setChecked(settings.showStatusBar());
    ui->statusbar->setVisible(settings.showStatusBar());

    ui->actionProject->setChecked(settings.showProject());
    showHideInfosTab(ui->tabProject,settings.showProject());
    ui->actionWatch->setChecked(settings.showWatch());
    showHideInfosTab(ui->tabWatch,settings.showWatch());
    ui->actionStructure->setChecked(settings.showStructure());
    showHideInfosTab(ui->tabStructure,settings.showStructure());
    ui->actionFiles->setChecked(settings.showFiles());
    showHideInfosTab(ui->tabFiles,settings.showFiles());
    ui->actionProblem_Set->setChecked(settings.showProblemSet());
    showHideInfosTab(ui->tabProblemSet,settings.showProblemSet()
                     && pSettings->executor().enableProblemSet());

    ui->actionIssues->setChecked(settings.showIssues());
    showHideMessagesTab(ui->tabIssues,settings.showIssues());
    ui->actionTools_Output->setChecked(settings.showCompileLog());
    showHideMessagesTab(ui->tabToolsOutput,settings.showCompileLog());
    ui->actionDebug_Window->setChecked(settings.showDebug());
    showHideMessagesTab(ui->tabDebug,settings.showDebug());
    ui->actionSearch->setChecked(settings.showSearch());
    showHideMessagesTab(ui->tabSearch,settings.showSearch());
    ui->actionTODO->setChecked(settings.showTODO());
    showHideMessagesTab(ui->tabTODO,settings.showTODO());
    ui->actionBookmark->setChecked(settings.showBookmark());
    showHideMessagesTab(ui->tabBookmark,settings.showBookmark());
    ui->actionProblem->setChecked(settings.showProblem());
    showHideMessagesTab(ui->tabProblem,settings.showProblem()
                        && pSettings->executor().enableProblemSet());

    ui->tabMessages->setBeforeShrinkSize(settings.messagesTabsSize());
    ui->tabExplorer->setBeforeShrinkSize(settings.explorerTabsSize());
    if (settings.shrinkMessagesTabs())
        ui->tabMessages->setShrinkedFlag(true);
    if (settings.shrinkExplorerTabs())
        ui->tabExplorer->setShrinkedFlag(true);
}

QFileSystemWatcher *MainWindow::fileSystemWatcher()
{
    return &mFileSystemWatcher;
}

void MainWindow::initDocks()
{
    ui->dockExplorer->setMinimumSize(0,0);
    ui->dockMessages->setMinimumSize(0,0);
    setDockExplorerToArea(dockWidgetArea(ui->dockExplorer));
    setDockMessagesToArea(dockWidgetArea(ui->dockMessages));
}

void MainWindow::removeActiveBreakpoints()
{
    for (int i=0;i<mEditorList->pageCount();i++) {
        Editor* e= (*mEditorList)[i];
        e->removeBreakpointFocus();
    }
}

void MainWindow::setActiveBreakpoint(QString fileName, int Line, bool setFocus)
{
    removeActiveBreakpoints();
    // Then active the current line in the current file
    Editor *e = openFile(fileName, false);
    if (e!=nullptr) {
        e->setActiveBreakpointFocus(Line,setFocus);
        e->activate(false);
        if (setFocus) {
            activateWindow();
        }
    } else {
    //   showCPUInfoDialog();
    }
    return;
}

void MainWindow::updateDPI(int oldDPI, int /*newDPI*/)
{
    //applySettings();
    if (oldDPI<1)
        oldDPI = 1;
}

void MainWindow::onFileSaved(const QString &path, bool inProject)
{
#ifdef ENABLE_VCS
    if (pSettings->vcs().gitOk()) {
        QString branch;
        if (inProject && mProject && mProject->model()->iconProvider()->VCSRepository()->hasRepository(branch)) {
            mProject->model()->refreshIcon(path);
        }
        QModelIndex index =  mFileSystemModel.index(path);
        if (index.isValid()) {
            if (!inProject) {
                if ( (isCFile(path) || isHFile(path))
                        &&  !mFileSystemModelIconProvider.VCSRepository()->isFileInRepository(path)) {
                    QString output;
                    mFileSystemModelIconProvider.VCSRepository()->add(extractRelativePath(mFileSystemModelIconProvider.VCSRepository()->folder(),path),output);
                }
            }
//            qDebug()<<"update icon provider";
            mFileSystemModelIconProvider.update();
            mFileSystemModel.setIconProvider(&mFileSystemModelIconProvider);
            ui->treeFiles->update(index);
        }
    }
#else
    Q_UNUSED(path);
    Q_UNUSED(inProject);
#endif
    //updateForEncodingInfo();
}

void MainWindow::executeTool(PToolItem item)
{
    QMap<QString, QString> macros = devCppMacroVariables();
    QString program = parseMacros(item->program, macros);
    QString workDir = parseMacros(item->workingDirectory, macros);
    QStringList params = parseArguments(item->parameters, macros, true);
    Editor *e;
    QByteArray inputContent;
    QByteArray output;
    QString errorMessage;
    clearToolsOutput();
    switch(item->inputOrigin) {
    case ToolItemInputOrigin::None:
        break;
    case ToolItemInputOrigin::CurrentSelection:
        e=mEditorList->getEditor();
        if (e)
            inputContent=stringToByteArray(e->selText(), item->isUTF8);
        break;
    case ToolItemInputOrigin::WholeDocument:
        e=mEditorList->getEditor();
        if (e)
            inputContent=stringToByteArray(e->text(), item->isUTF8);
        break;
    }
    QString command;
#ifdef Q_OS_WIN
    if (!fileExists(program)) {
        QTemporaryFile file(QDir::tempPath()+QDir::separator()+"XXXXXX.bat");
        file.setAutoRemove(false);
        if (file.open()) {
            QString localizedDir=localizePath(workDir);
            if (!localizedDir.isEmpty()) {
                file.write(escapeCommandForPlatformShell(
                    "cd", {"/d", localizedDir}
                    ).toLocal8Bit() + LINE_BREAKER);
            }
            file.write(escapeCommandForPlatformShell(program, params).toLocal8Bit()
                       + LINE_BREAKER);
            file.close();
            QString cmd="cmd";
            QStringList args{"/C",file.fileName()};
            command = escapeCommandForPlatformShell(cmd, args);
            auto [o, _, em] = runAndGetOutput(cmd, workDir, args, inputContent);
            output = o;
            errorMessage = em;
        }
    } else {
#endif
        command = escapeCommandForPlatformShell(program, params);
        auto [o, _, em] = runAndGetOutput(program, workDir, params, inputContent);
        output = o;
        errorMessage = em;
#ifdef Q_OS_WIN
    }
#endif
    switch(item->outputTarget) {
    case ToolItemOutputTarget::RedirectToToolsOutputPanel:
        logToolsOutput(tr(" - Command: %1").arg(command));
        if (!errorMessage.isEmpty())
            logToolsOutput(errorMessage);
        logToolsOutput("");
        logToolsOutput(byteArrayToString(output, item->isUTF8));
        stretchMessagesPanel(true);
        ui->tabMessages->setCurrentWidget(ui->tabToolsOutput);
        break;
    case ToolItemOutputTarget::RedirectToNull:
        break;
    case ToolItemOutputTarget::RepalceWholeDocument:
        e=mEditorList->getEditor();
        if (e)
            e->replaceContent(errorMessage + byteArrayToString(output, item->isUTF8));
        break;
    case ToolItemOutputTarget::ReplaceCurrentSelection:
        e=mEditorList->getEditor();
        if (e)
            e->setSelText(errorMessage + byteArrayToString(output, item->isUTF8));
        break;
    }
}

int MainWindow::calIconSize(const QString &fontName, int fontPointSize)
{
    QFont font(fontName,fontPointSize);
    QFontMetrics metrics(font);
    return metrics.ascent();
}

void MainWindow::hideAllSearchDialogs()
{
    if (mSearchDialog)
        mSearchDialog->hide();
    if (mSearchInFilesDialog)
        mSearchInFilesDialog->hide();
}

void MainWindow::prepareSearchDialog()
{
    if (!mSearchDialog)
        mSearchDialog = new SearchDialog(this);
}

void MainWindow::prepareSearchInFilesDialog()
{
    if (mSearchInFilesDialog==nullptr) {
        mSearchInFilesDialog = new SearchInFileDialog(this);
    }
}

void MainWindow::updateAppTitle()
{
    Editor *e = mEditorList->getEditor();
    updateAppTitle(e);
}

void MainWindow::updateAppTitle(const Editor *e)
{
    QString appName=tr("Red Panda C++");
#ifdef APP_VERSION_SUFFIX
    appName += tr(" %1 Version").arg(APP_VERSION_SUFFIX);
#endif
    QCoreApplication *app = QApplication::instance();
    if (e && !e->inProject()) {
        QString str;
        if (e->modified())
          str = e->filename() + " [*]";
        else
          str = e->filename();
        if (mDebugger->executing()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,tr("Debugging"),appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Debugging"),appName));
        } else if (mCompilerManager->running()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,tr("Running"),appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Running"),appName));
        } else if (mCompilerManager->compiling()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,tr("Compiling"),appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Compiling"),appName));
        } else {
            this->setWindowTitle(QString("%1 - %2 %3")
                                 .arg(str,appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - %2")
                                    .arg(str,appName));
        }
    } else if (e && e->inProject() && mProject) {
        QString str,str2;
        if (mProject->modified())
            str = mProject->name() + " [*]";
        else
            str = mProject->name();
        if (e->modified())
          str2 = extractFileName(e->filename()) + " [*]";
        else
          str2 = extractFileName(e->filename());
        if (mDebugger->executing()) {
            setWindowTitle(QString("%1 - %2 [%3] - %4 %5")
                           .arg(str,str2,
                                tr("Debugging"),appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Debugging"),appName));
        } else if (mCompilerManager->running()) {
            setWindowTitle(QString("%1 - %2 [%3] - %4 %5")
                           .arg(str,str2,
                                tr("Running"),appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Running"),appName));
        } else if (mCompilerManager->compiling()) {
            setWindowTitle(QString("%1 - %2 [%3] - %4 %5")
                           .arg(str,str2,
                                tr("Compiling"),appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Compiling"),appName));
        } else {
            setWindowTitle(QString("%1 - %2 %3")
                                 .arg(str,appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - %2")
                                    .arg(str,appName));
        }
    } else if (mProject) {
        QString str,str2;
        if (mProject->modified())
            str = mProject->name() + " [*]";
        else
            str = mProject->name();
        if (mDebugger->executing()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,tr("Debugging"),appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Debugging"),appName));
        } else if (mCompilerManager->running()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,tr("Running"),appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Running"),appName));
        } else if (mCompilerManager->compiling()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,tr("Compiling"),appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Compiling"),appName));
        } else {
            this->setWindowTitle(QString("%1 - %2 %3")
                                 .arg(str,appName,REDPANDA_CPP_VERSION));
            app->setApplicationName(QString("%1 - %2")
                                    .arg(str,appName));
        }
    } else {
        setWindowTitle(QString("%1 %2").arg(appName,REDPANDA_CPP_VERSION));
        app->setApplicationName(QString("%1").arg(appName));
    }
}

void MainWindow::addDebugOutput(const QString &text)
{
    if (!pSettings->debugger().enableDebugConsole())
        return;
    if (text.isEmpty()) {
        ui->debugConsole->addLine("");
    } else {
        ui->debugConsole->addText(text);
    }
}

void MainWindow::changeDebugOutputLastline(const QString &test)
{
    ui->debugConsole->changeLastLine(test);
}

void MainWindow::updateDebugEval(const QString &value)
{
    ui->txtEvalOutput->clear();
    ui->txtEvalOutput->appendPlainText(value);
    ui->txtEvalOutput->moveCursor(QTextCursor::Start);
}

void MainWindow::rebuildOpenedFileHisotryMenu()
{
    mMenuRecentFiles->clear();
    mMenuRecentProjects->clear();
    if (mVisitHistoryManager->files().count()==0) {
        mMenuRecentFiles->setEnabled(false);
    } else {
        mMenuRecentFiles->setEnabled(true);
        foreach (const PVisitRecord& record, mVisitHistoryManager->files()) {
            QString filename = record->filename;
            //menu takes the ownership
            QAction* action = new QAction(filename,mMenuRecentFiles);
            connect(action, &QAction::triggered, [filename,this](bool){
                openFile(filename);
            });
            mMenuRecentFiles->addAction(action);
        }
        mMenuRecentFiles->addSeparator();
        //menu takes the ownership
        QAction *action = new QAction(tr("Clear History"),mMenuRecentFiles);
        connect(action, &QAction::triggered, [this](bool){
            mVisitHistoryManager->clearFiles();
        });
        mMenuRecentFiles->addAction(action);
    }

    if (mVisitHistoryManager->projects().count()==0) {
        mMenuRecentProjects->setEnabled(false);
    } else {
        mMenuRecentProjects->setEnabled(true);
        foreach (const PVisitRecord& record, mVisitHistoryManager->projects()) {
            QString filename = record->filename;
            //menu takes the ownership
            QAction* action = new QAction(filename,mMenuRecentProjects);
            connect(action, &QAction::triggered, [filename,this](bool){
                openProject(filename);
            });
            mMenuRecentProjects->addAction(action);
        }
        mMenuRecentProjects->addSeparator();
        //menu takes the ownership
        QAction *action = new QAction(tr("Clear History"),mMenuRecentProjects);
        connect(action, &QAction::triggered, [this](bool){
            mVisitHistoryManager->clearProjects();
        });
        mMenuRecentProjects->addAction(action);
    }

}

void MainWindow::updateClassBrowserForEditor(Editor *editor)
{

    if (mQuitting) {
        mClassBrowserModel.beginUpdate();
        mClassBrowserModel.setParser(nullptr);
        mClassBrowserModel.setCurrentFile("");
        mClassBrowserModel.endUpdate();
        return;
    }

    if (editor) {
        if ((mClassBrowserModel.currentFile() == editor->filename())
             && (mClassBrowserModel.parser() == editor->parser()))
                return;

        if (mClassBrowserModel.parser() == editor->parser() && mClassBrowserModel.classBrowserType()==ProjectClassBrowserType::WholeProject) {
            mClassBrowserModel.setCurrentFile(editor->filename());
            return;
        }

        if (editor->inProject() && !mProject) {
            //project is in creation
            mClassBrowserModel.setCurrentFile(editor->filename());
            return;
        }


        mClassBrowserModel.beginUpdate();
        mClassBrowserModel.setParser(editor->parser());
        if (editor->inProject()) {
            mClassBrowserModel.setClassBrowserType(mProject->options().classBrowserType);
        } else {
            mClassBrowserModel.setClassBrowserType(ProjectClassBrowserType::CurrentFile);
        }
        mClassBrowserModel.setCurrentFile(editor->filename());
        mClassBrowserModel.endUpdate();
    } else if (mProject) {
        if (mClassBrowserModel.parser() == mProject->cppParser()) {
            mClassBrowserModel.beginUpdate();
            mClassBrowserModel.setCurrentFile("");
            mClassBrowserModel.endUpdate();
            return;
        }

        mClassBrowserModel.beginUpdate();
        mClassBrowserModel.setParser(mProject->cppParser());
        mClassBrowserModel.setClassBrowserType(mProject->options().classBrowserType);
        mClassBrowserModel.setCurrentFile("");
        mClassBrowserModel.endUpdate();
    } else {
        mClassBrowserModel.beginUpdate();
        mClassBrowserModel.setParser(nullptr);
        mClassBrowserModel.setCurrentFile("");
        mClassBrowserModel.endUpdate();
        return;
    }
}

void MainWindow::resetAutoSaveTimer()
{
    if (pSettings->editor().enableAutoSave()) {
        mAutoSaveTimer.stop();
        //minute to milliseconds
        mAutoSaveTimer.start(pSettings->editor().autoSaveInterval()*60*1000);
        onAutoSaveTimeout();
    } else {
        mAutoSaveTimer.stop();
    }
}

void MainWindow::updateShortcuts()
{
    ShortcutManager manager;
    manager.load();
    // foreach(QAction* action, listShortCutableActions())
    //     qDebug()<<action->text()<<action->objectName();
    manager.applyTo(listShortCutableActions());
}

QPlainTextEdit *MainWindow::txtLocals()
{
    return ui->txtLocals;
}

QMenuBar *MainWindow::menuBar() const
{
    return ui->menubar;
}

void MainWindow::updateStatusbarForLineCol(const Editor* e, bool clear)
{
    if (!clear && e!=nullptr) {
        QString msg;
        if (pSettings->editor().forceFixedFontWidth()){
            int col = e->charToGlyphLeft(e->caretY(),e->caretX())/e->charWidth()+1;
            if (e->selAvail()) {
                msg = tr("Line: %1/%2 Col: %3 Sel: %4")
                        .arg(e->caretY())
                        .arg(e->lineCount())
                        .arg(col)
                        .arg(e->selCount());
            } else {
                msg = tr("Line: %1/%2 Col: %3")
                        .arg(e->caretY())
                        .arg(e->lineCount())
                        .arg(col);
            }
        } else {
            if (e->selAvail()) {
                msg = tr("Line: %1/%2 Char: %3/%4 Sel: %5")
                        .arg(e->caretY())
                        .arg(e->lineCount())
                        .arg(e->caretX())
                        .arg(e->lineText().length())
                        .arg(e->selCount());
            } else {
                msg = tr("Line: %1/%2 Char: %3/%4")
                        .arg(e->caretY())
                        .arg(e->lineCount())
                        .arg(e->caretX())
                        .arg(e->lineText().length());
            }
        }
        mFileInfoStatus->setText(msg);
    } else {
        mFileInfoStatus->setText("");
    }
}

void MainWindow::updateForStatusbarModeInfo(bool clear)
{
    Editor* e = mEditorList->getEditor();
    updateForStatusbarModeInfo(e,clear);
}

void MainWindow::updateForStatusbarModeInfo(const Editor* e, bool clear)
{
    if (!clear && e!=nullptr) {
        QString msg;
        if (e->readOnly()) {
            msg = tr("Read Only");
        } else if (e->insertMode()) {
            msg = tr("Insert");
        } else {
            msg = tr("Overwrite");
        }
        mFileModeStatus->setText(msg);
    } else {
        mFileModeStatus->setText("");
    }
}

void MainWindow::updateStatusbarMessage(const QString &s)
{
    ui->statusbar->showMessage(s);
}

void MainWindow::setProjectCurrentFile(const QString &filename)
{
    if (!mProject)
        return;
    PProjectUnit unit = mProject->findUnit(filename);
    if (!unit)
        return;
    QModelIndex index = mProject->model()->getNodeIndex(unit->node().get());
    if (index.isValid()) {
        index = mProjectProxyModel->mapFromSource(index);
        ui->projectView->expand(index);
        ui->projectView->setCurrentIndex(index);
    }
}

void MainWindow::openFiles(const QStringList &files)
{
    mOpeningFiles=true;
    mEditorList->beginUpdate();
    auto end = finally([this] {
        this->mEditorList->endUpdate();
        mOpeningFiles=false;
        Editor* e=mEditorList->getEditor();
        if (e) {
            e->reparse(false);
            e->checkSyntaxInBack();
            e->reparseTodo();
            e->activate();
        }
    });
    //Check if there is a project file in the list and open it
    for (const QString& file:files) {
        if (getFileType(file)==FileType::Project) {
            openProject(file);
            return;
        }
    }
    //Didn't find a project? Open all files
    for (int i=0;i<files.length()-1;i++) {
        openFile(files[i],false);
    }
    if (files.length()>0) {
        openFile(files.last(),true);
    }
}

Editor* MainWindow::openFile(QString filename, bool activate, QTabWidget* page)
{
    if (!fileExists(filename))
        return nullptr;

    QFileInfo info=QFileInfo(filename);
    if (info.isAbsolute())
        filename = info.absoluteFilePath();

    Editor* editor = mEditorList->getOpenedEditorByFilename(filename);
    if (editor!=nullptr) {
        if (activate) {
            editor->activate();
        }
        return editor;
    }
    try {
        Editor* oldEditor=nullptr;
        if (mEditorList->pageCount()==1) {
            oldEditor = mEditorList->getEditor(0);
            if (!oldEditor->isNew() || oldEditor->modified()) {
                oldEditor = nullptr;
            }
        }
        //mVisitHistoryManager->removeFile(filename);
        PProjectUnit unit;
        if (mProject) {
            unit = mProject->findUnit(filename);
        }
        bool inProject = (mProject && unit);
        QByteArray encoding = unit ? unit->encoding() :
                                     (pSettings->editor().autoDetectFileEncoding() ? QByteArray(ENCODING_AUTO_DETECT) : pSettings->editor().defaultEncoding());
        Project * pProject = (inProject?mProject.get():nullptr);
        if (pProject && encoding==ENCODING_PROJECT)
            encoding=pProject->options().encoding;
        editor = mEditorList->newEditor(filename,encoding,
                                    pProject, false, page);
//        if (mProject) {
//            mProject->associateEditorToUnit(editor,unit);
//        }
        if (activate) {
            editor->activate();
        }
        if (mEditorList->pageCount()>1 && oldEditor)
            mEditorList->closeEditor(oldEditor);
//        editor->activate();
        return editor;
    } catch (FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
    return nullptr;
}

void MainWindow::openProject(QString filename, bool openFiles)
{
    mOpeningProject=true;
    auto action=finally([this]{
        mOpeningProject=false;
    });
    if (!fileExists(filename)) {
        return;
    }
    QFileInfo info=QFileInfo(filename);
    if (info.isAbsolute())
        filename = info.absoluteFilePath();
    Editor* oldEditor=nullptr;
    if (mProject) {
        if (mProject->filename() == filename)
            return;
        ProjectAlreadyOpenDialog dlg;
        if (dlg.exec()!=QDialog::Accepted)
            return;
        if (dlg.openType()==ProjectAlreadyOpenDialog::OpenType::ThisWindow) {
            closeProject(false);
        } else {
            QProcess process;
            process.setProgram(QApplication::instance()->applicationFilePath());
            process.setWorkingDirectory(QDir::currentPath());
            QStringList args;
            args.append("-ns");
            args.append(filename);
            process.setArguments(args);
            process.startDetached();
            return;
        }

    } else {
        if (mEditorList->pageCount()==1) {
            oldEditor = mEditorList->getEditor(0);
            if (!oldEditor->isNew() || oldEditor->modified()) {
                oldEditor = nullptr;
            }
        }
    }
    //ui->tabProject->setVisible(true);
    //stretchExplorerPanel(true);
    if (openFiles)
        ui->tabExplorer->setCurrentWidget(ui->tabProject);

    // Only update class browser once
    mClassBrowserModel.beginUpdate();
    mProject = Project::load(filename,mEditorList,&mFileSystemWatcher);
    updateProjectView();
    ui->projectView->expand(
                mProjectProxyModel->mapFromSource(
                    mProject->model()->rootIndex()));
    //mVisitHistoryManager->removeProject(filename);

//  // if project manager isn't open then open it
//  if not devData.ShowLeftPages then
//    actProjectManager.Execute;
        //checkForDllProfiling();

    //parse the project
    //  UpdateClassBrowsing;

    scanActiveProject(true);

    mBookmarkModel->setIsForProject(true);
    mBookmarkModel->loadProjectBookmarks(
                changeFileExt(mProject->filename(), PROJECT_BOOKMARKS_EXT),
                mProject->directory());
    mDebugger->setIsForProject(true);
    mDebugger->loadForProject(
                changeFileExt(mProject->filename(), PROJECT_DEBUG_EXT),
                mProject->directory());
    mTodoModel.setIsForProject(true);
    if (pSettings->editor().parseTodos())
        mTodoParser->parseFiles(mProject->unitFiles());

    if (openFiles) {
        PProjectUnit unit = mProject->doAutoOpen();
        setProjectViewCurrentUnit(unit);
    }

    //update editor's inproject flag
    foreach (PProjectUnit unit, mProject->unitList()) {
        Editor* e = mEditorList->getOpenedEditorByFilename(unit->fileName());
        mProject->associateEditorToUnit(e,unit);
        if (e)
            e->resetBookmarks();
    }

    Editor * e = mEditorList->getEditor();
    if (e) {
        checkSyntaxInBack(e);
    }
    updateAppTitle();
    updateCompilerSet();
    updateClassBrowserForEditor(e);
    mClassBrowserModel.endUpdate();
    if (oldEditor)
        mEditorList->closeEditor(oldEditor);
    setupSlotsForProject();
    //updateForEncodingInfo();
}

void MainWindow::changeOptions(const QString &widgetName, const QString &groupName)
{
    PSettingsDialog settingsDialog = SettingsDialog::optionDialog();
    if (!groupName.isEmpty()) {
        settingsDialog->setCurrentWidget(widgetName, groupName);
    }
    settingsDialog->exec();
    if (settingsDialog->appShouldQuit()) {
        mShouldRemoveAllSettings = true;
        close();
        return;
    }

    if (mProject) {
        scanActiveProject(true);
    }
    reparseNonProjectEditors();

//    Editor *e = mEditorList->getEditor();
//    if (mProject && !e) {
//        scanActiveProject(true);
//    } else if (mProject && e && e->inProject()) {
//        scanActiveProject(true);
//    } else if (e) {
//        reparseNonProjectEditors();
//    }

}

void MainWindow::changeProjectOptions(const QString &widgetName, const QString &groupName)
{
    if (!mProject)
        return;
//    int oldCompilerSet = mProject->options().compilerSet;
    QString oldName = mProject->name();
    PSettingsDialog dialog = SettingsDialog::projectOptionDialog();
    if (!groupName.isEmpty()) {
        dialog->setCurrentWidget(widgetName, groupName);
    }
    dialog->exec();
    updateCompilerSet();
}

void MainWindow::updateCompilerSet()
{
    updateCompilerSet(mEditorList->getEditor());
}

void MainWindow::updateCompilerSet(const Editor *e)
{
    mCompilerSet->blockSignals(true);
    mCompilerSet->clear();
    for (size_t i=0;i<pSettings->compilerSets().size();i++) {
        mCompilerSet->addItem(pSettings->compilerSets().getSet(i)->name());
    }
    int index=pSettings->compilerSets().defaultIndex();
    if (mProject) {
        if ( !e || e->inProject()) {
            index = mProject->options().compilerSet;
        } else if (e->syntaxer()->language()==QSynedit::ProgrammingLanguage::Makefile
                   && mProject->directory() == extractFileDir(e->filename())) {
            index = mProject->options().compilerSet;
        }

        if (index < 0 || index>=mCompilerSet->count()) {
            index = pSettings->compilerSets().defaultIndex();
        }
    }
    mCompilerSet->setCurrentIndex(index);
    mCompilerSet->blockSignals(false);
    mCompilerSet->update();
}

void MainWindow::updateDebuggerSettings()
{
    QFont font(pSettings->debugger().fontName());
    font.setPixelSize(pointToPixel(pSettings->debugger().fontSize()));
    ui->debugConsole->setFont(font);
    ui->tblMemoryView->setFont(font);
    //ui->txtMemoryView->setFont(font);
    ui->txtLocals->setFont(font);

    int idx = findTabIndex(ui->debugViews,ui->tabDebugConsole);
    if (idx>=0) {
        if (!pSettings->debugger().enableDebugConsole()) {
            ui->debugViews->removeTab(idx);
        }
    } else {
        if (pSettings->debugger().enableDebugConsole()) {
            ui->debugViews->insertTab(0, ui->tabDebugConsole, tr("Debug Console"));
        }
    }

}

void MainWindow::updateActionIcons()
{
    int size = calIconSize(
                pSettings->environment().interfaceFont(),
                pSettings->environment().interfaceFontSize())*pSettings->environment().iconZoomFactor();
    pIconsManager->updateActionIcons(pSettings->environment().iconSet(), size);
    QSize iconSize(size,size);
    ui->toolbarMain->setIconSize(iconSize);
    ui->toolbarCode->setIconSize(iconSize);
    ui->toolbarCompile->setIconSize(iconSize);
    ui->toolbarDebug->setIconSize(iconSize);
    ui->toolbarCompilerSet->setIconSize(iconSize);
    for (QToolButton* btn: mClassBrowserToolbar->findChildren<QToolButton *>()) {
        btn->setIconSize(iconSize);
    }
    for (QToolButton* btn: ui->panelFiles->findChildren<QToolButton *>()) {
        btn->setIconSize(iconSize);
    }
    for (QToolButton* btn: ui->tabProblemSet->findChildren<QToolButton *>()) {
        btn->setIconSize(iconSize);
    }
    for (QToolButton* btn: ui->panelProblemCaseInfo->findChildren<QToolButton *>()) {
        btn->setIconSize(iconSize);
    }

    ui->tabExplorer->setIconSize(iconSize);
    ui->tabMessages->setIconSize(iconSize);
    ui->EditorTabsLeft->setIconSize(iconSize);
    ui->EditorTabsRight->setIconSize(iconSize);

    ui->actionNew->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_NEW));
    ui->actionNew_Project->setIcon(pIconsManager->getIcon(IconsManager::ACTION_PROJECT_NEW));
    ui->actionOpen->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_OPEN));
    ui->actionOpen_Folder->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_OPEN_FOLDER));
    ui->actionSave->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_SAVE));
    ui->actionSaveAs->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_SAVE_AS));
    ui->actionSaveAll->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_SAVE_ALL));
    ui->actionClose->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_CLOSE));
    ui->actionClose_Project->setIcon(pIconsManager->getIcon(IconsManager::ACTION_PROJECT_CLOSE));
    ui->actionClose_All->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_CLOSE_ALL));
    ui->actionClose_Others->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_CLOSE_ALL));
    ui->actionPrint->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_PRINT));

    ui->actionUndo->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_UNDO));
    ui->actionRedo->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_REDO));
    ui->actionCut->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_CUT));
    ui->actionCopy->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_COPY));
    ui->actionPaste->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_PASTE));
    ui->actionIndent->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_INDENT));
    ui->actionUnIndent->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_UNINDENT));

    ui->actionFind->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_SEARCH));
    ui->actionReplace->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_REPLACE));
    ui->actionFind_in_files->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_SEARCH_IN_FILES));

    ui->actionBack->setIcon(pIconsManager->getIcon(IconsManager::ACTION_CODE_BACK));
    ui->actionForward->setIcon(pIconsManager->getIcon(IconsManager::ACTION_CODE_FORWARD));
    ui->actionToggle_Bookmark->setIcon(pIconsManager->getIcon(IconsManager::ACTION_CODE_ADD_BOOKMARK));
    ui->actionReformat_Code->setIcon(pIconsManager->getIcon(IconsManager::ACTION_CODE_REFORMAT));

    ui->actionProject_New_File->setIcon(pIconsManager->getIcon(IconsManager::ACTION_PROJECT_NEW_FILE));
    ui->actionAdd_to_project->setIcon(pIconsManager->getIcon(IconsManager::ACTION_PROJECT_ADD_FILE));
    ui->actionRemove_from_project->setIcon(pIconsManager->getIcon(IconsManager::ACTION_PROJECT_REMOVE_FILE));
    ui->actionProject_Open_Folder_In_Explorer->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_FOLDER));
    ui->actionProject_Open_In_Terminal->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_TERM));
    ui->actionMakeClean->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_CLEAN));
    ui->actionProject_options->setIcon(pIconsManager->getIcon(IconsManager::ACTION_PROJECT_PROPERTIES));


    ui->actionCompile->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_COMPILE));
    ui->actionRun->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_RUN));
    ui->actionRebuild->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_REBUILD));
    ui->actionRun_Parameters->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_OPTIONS));
    ui->actionDebug->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_DEBUG));
    ui->actionInterrupt->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_INTERRUPT));
    ui->actionStep_Over->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_STEP_OVER));
    ui->actionStep_Into->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_STEP_INTO));
    ui->actionStep_Out->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_STEP_OUT));
    ui->actionRun_To_Cursor->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_RUN_TO_CURSOR));
    ui->actionContinue->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_CONTINUE));
    ui->actionStop_Execution->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_STOP));
    ui->actionAdd_Watch->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_ADD_WATCH));
    ui->actionRemove_Watch->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_REMOVE_WATCH));
    ui->actionRemove_All_Watches->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_CLEAN));
    ui->actionCompiler_Options->setIcon(pIconsManager->getIcon(IconsManager::ACTION_RUN_COMPILE_OPTIONS));

    ui->actionOptions->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_GEAR));

    ui->actionMaximize_Editor->setIcon(pIconsManager->getIcon(IconsManager::ACTION_VIEW_MAXIMUM));
    ui->actionNext_Editor->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_FORWARD));
    ui->actionPrevious_Editor->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_BACK));

    ui->actionAbout->setIcon(pIconsManager->getIcon(IconsManager::ACTION_HELP_ABOUT));

    //editor context menu
    ui->actionOpen_Containing_Folder->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_FOLDER));
    ui->actionOpen_Terminal->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_TERM));
    ui->actionFile_Properties->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_PROPERTIES));
    ui->actionLocate_in_Files_View->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_LOCATE));

    //bookmark context menu
    mBookmark_Remove->setIcon(pIconsManager->getIcon(IconsManager::ACTION_CODE_REMOVE_BOOKMARK));
    mBookmark_RemoveAll->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_CLEAN));

    //issues context menu
    mTableIssuesCopyAction->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_COPY));
    mTableIssuesClearAction->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_CLEAN));

    //search context menu
    mSearchViewClearAction->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_CROSS));
    mSearchViewClearAllAction->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_CLEAN));

    //breakpoint context menu
    //mBreakpointViewPropertyAction
    mBreakpointViewRemoveAllAction->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_CLEAN));
    mBreakpointViewRemoveAction->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_CROSS));

    //Tools Output

    //classbrowser
    mClassBrowser_Sort_By_Name->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_SORT_BY_NAME));
    mClassBrowser_Sort_By_Type->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_SORT_BY_TYPE));
    mClassBrowser_Show_Inherited->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_SHOW_INHERITED));

    //debug console
    mDebugConsole_Copy->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_COPY));
    mDebugConsole_Paste->setIcon(pIconsManager->getIcon(IconsManager::ACTION_EDIT_PASTE));
    mDebugConsole_Clear->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_CLEAN));

    //file view
    mFilesView_Open->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_OPEN));
    mFilesView_OpenInTerminal->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_TERM));
    mFilesView_OpenInExplorer->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_FOLDER));
    ui->actionFilesView_Hide_Non_Support_Files->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_FILTER));

    //problem view
    mProblemSet_New->setIcon(pIconsManager->getIcon(IconsManager::ACTION_PROBLEM_SET));
    mProblemSet_AddProblem->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_ADD));
    mProblemSet_RemoveProblem->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_CROSS));
    mProblemSet_Save->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_SAVE_AS));
    mProblemSet_Load->setIcon(pIconsManager->getIcon(IconsManager::ACTION_FILE_OPEN_FOLDER));
    mProblemSet_ImportFPS->setIcon(pIconsManager->getIcon(IconsManager::ACTION_CODE_BACK));
    mProblemSet_ExportFPS->setIcon(pIconsManager->getIcon(IconsManager::ACTION_CODE_FORWARD));

    mProblem_AddCase->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_ADD));
    mProblem_RemoveCases->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_REMOVE));
    mProblem_OpenAnswer->setIcon(pIconsManager->getIcon(IconsManager::ACTION_PROBLEM_EDIT_SOURCE));
    mProblem_RunAllCases->setIcon(pIconsManager->getIcon(IconsManager::ACTION_PROBLEM_RUN_CASES));
    mProblem_CaseValidationOptions->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_GEAR));

    pIconsManager->setIcon(ui->btnProblemCaseClearInputFileName, IconsManager::ACTION_MISC_CLEAN);
    pIconsManager->setIcon(ui->btnProblemCaseInputFileName, IconsManager::ACTION_MISC_FOLDER);
    pIconsManager->setIcon(ui->btnProblemCaseClearExpectedOutputFileName, IconsManager::ACTION_MISC_CLEAN);
    pIconsManager->setIcon(ui->btnProblemCaseExpectedOutputFileName, IconsManager::ACTION_MISC_FOLDER);

    mProblem_Properties->setIcon(pIconsManager->getIcon(IconsManager::ACTION_PROBLEM_PROPERTIES));

    int idx = ui->tabExplorer->indexOf(ui->tabWatch);
    if (idx>=0)
        ui->tabExplorer->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_RUN_ADD_WATCH));
    idx = ui->tabExplorer->indexOf(ui->tabProject);
    if (idx>=0)
        ui->tabExplorer->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_PROJECT_NEW));
    idx = ui->tabExplorer->indexOf(ui->tabFiles);
    if (idx>=0)
        ui->tabExplorer->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_VIEW_FILES));
    idx = ui->tabExplorer->indexOf(ui->tabStructure);
    if (idx>=0)
        ui->tabExplorer->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_VIEW_CLASSBROWSER));
    idx = ui->tabExplorer->indexOf(ui->tabProblemSet);
    if (idx>=0)
        ui->tabExplorer->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_PROBLEM_SET));

    idx = ui->tabMessages->indexOf(ui->tabIssues);
    if (idx>=0)
        ui->tabMessages->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_RUN_COMPILE));
    idx = ui->tabMessages->indexOf(ui->tabDebug);
    if (idx>=0)
        ui->tabMessages->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_RUN_DEBUG));
    idx = ui->tabMessages->indexOf(ui->tabSearch);
    if (idx>=0)
        ui->tabMessages->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_EDIT_SEARCH));
    idx = ui->tabMessages->indexOf(ui->tabToolsOutput);
    if (idx>=0)
        ui->tabMessages->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_VIEW_COMPILELOG));
    idx = ui->tabMessages->indexOf(ui->tabTODO);
    if (idx>=0)
        ui->tabMessages->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_VIEW_TODO));
    idx = ui->tabMessages->indexOf(ui->tabBookmark);
    if (idx>=0)
        ui->tabMessages->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_VIEW_BOOKMARK));

    idx = ui->tabMessages->indexOf(ui->tabProblem);
    if (idx>=0)
        ui->tabMessages->setTabIcon(idx,pIconsManager->getIcon(IconsManager::ACTION_PROBLEM_PROBLEM));
}

void MainWindow::checkSyntaxInBack(Editor *e)
{
    if (e==nullptr)
        return;

    if (!pSettings->editor().syntaxCheck()) {
        return;
    }

    //not c or cpp file
    FileType fileType = getFileType(e->filename());
    if (fileType != FileType::CSource
            && fileType != FileType::CppSource
            && fileType != FileType::CHeader
            && fileType != FileType::CppHeader
            && fileType != FileType::GAS
            )
        return;
    if (mCompilerManager->backgroundSyntaxChecking())
        return;
    if (mCompilerManager->compiling())
        return;
    if (mCheckSyntaxInBack)
        return;

    if (mCompileIssuesState==CompileIssuesState::ProjectCompilationResultFilled
            || mCompileIssuesState==CompileIssuesState::ProjectCompiling) {
        if (e->inProject() && mProject) {
            if (!e->modified())
                return;
        }
    }

    mCheckSyntaxInBack=true;
    clearIssues();
    CompileTarget target =getCompileTarget();
    if (target ==CompileTarget::Project) {
        int index = mProject->options().compilerSet;
        Settings::PCompilerSet set = pSettings->compilerSets().getSet(index);
        if (!set || !CompilerInfoManager::supportSyntaxCheck(set->compilerType()))
            return;
        mCompilerManager->checkSyntax(e->filename(), e->fileEncoding(), e->text(), mProject);
    } else {
        Settings::PCompilerSet set = pSettings->compilerSets().defaultSet();
        if (!set || !CompilerInfoManager::supportSyntaxCheck(set->compilerType()))
            return;
        mCompilerManager->checkSyntax(e->filename(),e->fileEncoding(),e->text(), nullptr);
    }
}

bool MainWindow::parsing()
{
    if (mProject && mProject->cppParser() && mProject->cppParser()->parsing())
        return true;
    for(int i=0;i<mEditorList->pageCount();i++) {
        Editor * editor = (*mEditorList)[i];
        if (editor->parser() && editor->parser()->parsing())
            return true;
    }
    return false;
}


bool MainWindow::compile(bool rebuild, CppCompileType compileType)
{
    mCompilerManager->stopPausing();
    CompileTarget target =getCompileTarget();
    if (target == CompileTarget::Project && compileType == CppCompileType::Normal) {
        QStringList missedUnits;
        foreach(const PProjectUnit &unit, mProject->unitList()) {
            if (!fileExists(unit->fileName())) {
                missedUnits.append(
                            extractRelativePath(
                                mProject->directory(),
                            unit->fileName()));
            }
        }
        if (!missedUnits.empty()) {
            ui->actionProject->setChecked(true);
            showHideInfosTab(ui->tabProject,true);
            ui->tabExplorer->setCurrentWidget(ui->tabProject);
            QString s=missedUnits.join("<br/>");
            QMessageBox::critical(this,
                                  tr("Missing Project Files"),
                                  tr("The following files is missing, can't build the project:")
                                  +"<br/><br/>"
                                  +s);
            return false;
        }
        if (mProject->modified()) {
            mProject->saveAll();
        }
        mEditorList->saveAll();
        clearIssues();

        // Increment build number automagically
        if (mProject->options().versionInfo.autoIncBuildNr) {
            mProject->incrementBuildNumber();
        }
        mProject->buildPrivateResource();
        if (mCompileSuccessionTask) {
            mCompileSuccessionTask->execName = mProject->outputFilename();
            mCompileSuccessionTask->isExecutable = true;
        }
        stretchMessagesPanel(true);
        ui->tabMessages->setCurrentWidget(ui->tabToolsOutput);
        mCompilerManager->compileProject(mProject,rebuild);
        updateCompileActions();
        updateAppTitle();
    } else {
        Editor * editor = mEditorList->getEditor();
        if (editor) {
            clearIssues();
            if (editor->modified() || editor->isNew()) {
                if (!editor->save(false,false))
                    return false;
            }
            if (mCompileSuccessionTask) {
                Settings::PCompilerSet compilerSet=pSettings->compilerSets().defaultSet();
                if (editor->inProject())
                    compilerSet = pSettings->compilerSets().getSet(mProject->options().compilerSet);
                if (compilerSet)  {
                    Settings::CompilerSet::CompilationStage stage;
                    switch(compileType) {
                    case CppCompileType::GenerateAssemblyOnly:
                        stage = Settings::CompilerSet::CompilationStage::CompilationProperOnly;
                        break;
                    case CppCompileType::PreprocessOnly:
                        stage = Settings::CompilerSet::CompilationStage::PreprocessingOnly;
                        break;
                    default:
                        stage = compilerSet->compilationStage();
                        break;
                    }
                    mCompileSuccessionTask->execName = compilerSet->getOutputFilename(editor->filename(),stage);
                    mCompileSuccessionTask->isExecutable = compilerSet->isOutputExecutable(stage);
                } else {
                    mCompileSuccessionTask->execName = changeFileExt(editor->filename(),DEFAULT_EXECUTABLE_SUFFIX);
                    mCompileSuccessionTask->isExecutable = true;
                }
                if (!mCompileSuccessionTask->isExecutable) {
                    QString targetFileName = QFileInfo(mCompileSuccessionTask->execName).absoluteFilePath();
                    Editor *editor = mEditorList->getOpenedEditorByFilename(targetFileName);
                    if (editor) {
                        mEditorList->closeEditor(editor,false,true);
                    }
                }
            }
            stretchMessagesPanel(true);
            ui->tabMessages->setCurrentWidget(ui->tabToolsOutput);
            mCompilerManager->compile(editor->filename(),editor->fileEncoding(),rebuild,compileType);
            updateCompileActions();
            updateAppTitle();
        }
    }
    return true;
}

void MainWindow::runExecutable(
        const QString &exeName,
        const QString &filename,
        RunType runType,
        const QStringList& binDirs)
{
    mCompilerManager->stopPausing();
    // Check if it exists
    if (!fileExists(exeName)) {
        if (ui->actionCompile->isEnabled()) {
            doCompileRun(runType);
            return;
        } else {
            QMessageBox::critical(this,"Error",
                                  tr("Source file is not compiled."));
            return;
        }
    } else {
        if (!filename.isEmpty() &&
                ( compareFileModifiedTime(filename,exeName)>=0
                  || compareFileModifiedTime(exeName, pSettings->compilerSets().defaultIndexTimestamp())<=0 )) {
            doCompileRun(runType);
            return;
        }
    }

    QString params;
    if (pSettings->executor().useParams()) {
        params = pSettings->executor().params();
    }
    if (runType==RunType::Normal) {
        if (pSettings->executor().minimizeOnRun()) {
            showMinimized();
        }
        mCompilerManager->run(exeName,params,QFileInfo(exeName).absolutePath(),binDirs);
    } else if (runType == RunType::ProblemCases) {
        POJProblem problem = mOJProblemModel.problem();
        if (problem) {
            mCompilerManager->runProblem(exeName,params,QFileInfo(exeName).absolutePath(),
                                         problem->cases,
                                         problem);
            stretchMessagesPanel(true);
            ui->tabMessages->setCurrentWidget(ui->tabProblem);
        }
    } else if (runType == RunType::CurrentProblemCase) {
        QModelIndex index = ui->tblProblemCases->currentIndex();
        if (index.isValid()) {
            POJProblemCase problemCase =mOJProblemModel.getCase(index.row());
            POJProblem problem = mOJProblemModel.problem();
            mCompilerManager->runProblem(exeName,params,QFileInfo(exeName).absolutePath(),
                                         problemCase,
                                         problem);
            stretchMessagesPanel(true);
            ui->tabMessages->setCurrentWidget(ui->tabProblem);
        }
    }
    updateCompileActions();
    updateAppTitle();
}

void MainWindow::runExecutable(RunType runType)
{
    CompileTarget target =getCompileTarget();
    if (target == CompileTarget::Project) {
        QStringList binDirs = mProject->binDirs();
        QFileInfo execInfo(mProject->outputFilename());
        QDateTime execModTime = execInfo.lastModified();
        if (execInfo.exists() && mProject->modifiedSince(execModTime)) {
            //if project options changed, or units added/removed
            //mProject->saveAll();
            mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
            mCompileSuccessionTask->type = runTypeToCompileSuccessionTaskType(runType);
            mCompileSuccessionTask->execName=mProject->outputFilename();
            mCompileSuccessionTask->isExecutable=true;
            mCompileSuccessionTask->binDirs=binDirs;
            compile(true);
            return;
        }
        if (execInfo.exists() && mProject->unitsModifiedSince(execModTime)) {
            //if units modified;
            //mProject->saveAll();
            mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
            mCompileSuccessionTask->type = runTypeToCompileSuccessionTaskType(runType);
            mCompileSuccessionTask->execName=mProject->outputFilename();
            mCompileSuccessionTask->isExecutable=true;
            mCompileSuccessionTask->binDirs=binDirs;
            compile();
            return;
        }

        runExecutable(mProject->outputFilename(),mProject->filename(),runType,
                      binDirs);
    } else {
        Editor * editor = mEditorList->getEditor();
        if (editor) {
            if (editor->modified() || editor->isNew()) {
                if (!editor->save(false,false))
                    return;
            }
            QStringList binDirs = getDefaultCompilerSetBinDirs();
            QString exeName;
            Settings::PCompilerSet compilerSet =pSettings->compilerSets().defaultSet();
            bool isExecutable;
            if (compilerSet) {
                exeName = compilerSet->getOutputFilename(editor->filename());
                isExecutable = compilerSet->isOutputExecutable();
            } else {
                exeName = changeFileExt(editor->filename(), DEFAULT_EXECUTABLE_SUFFIX);
                isExecutable = true;
            }
            if (isExecutable)
                runExecutable(exeName,editor->filename(),runType,binDirs);
            else if (runType==RunType::Normal) {
                if (fileExists(exeName))
                    openFile(exeName);
            } else {
                QMessageBox::critical(this,tr("Wrong Compiler Settings"),
                                      tr("Compiler is set not to generate executable.")+"<BR/><BR/>"
                                      +tr("We need the executabe to run problem case."));
                return;
            }
        }
    }
}

void MainWindow::debug()
{
    if (mCompilerManager->compiling())
        return;
    mCompilerManager->stopPausing();
    Settings::PCompilerSet compilerSet = pSettings->compilerSets().defaultSet();
    if (!compilerSet) {
        QMessageBox::critical(pMainWindow,
                              tr("No compiler set"),
                              tr("No compiler set is configured.")+"<BR/>"+tr("Can't start debugging."));
        return;
    }
    bool debugEnabled;
    bool stripEnabled;
    QString filePath;
    QFileInfo debugFile;
    QStringList binDirs;
    QSet<QString> unitFiles;
    switch(getCompileTarget()) {
    case CompileTarget::Project: {
        compilerSet=pSettings->compilerSets().getSet(mProject->options().compilerSet);
        if (!compilerSet)
            compilerSet = pSettings->compilerSets().defaultSet();
        binDirs = mProject->binDirs();
        // Check if we enabled proper options
        debugEnabled = mProject->getCompileOption(CC_CMD_OPT_DEBUG_INFO) == COMPILER_OPTION_ON;
        stripEnabled = mProject->getCompileOption(LINK_CMD_OPT_STRIP_EXE) == COMPILER_OPTION_ON;
        if (stripEnabled && !debugEnabled) {
            if (QMessageBox::question(this,
                                  tr("Correct compile settings for debug"),
                                  tr("The generated executable won't have debug symbol infos, and can't be debugged.")
                                  +"<BR /><BR />"
                                  +tr("If you are using the Release compiler set, please use choose the Debug version from toolbar.")
                                  +"<BR /><BR />"
                                  +tr("Or you can manually change the following settings in the options dialog's compiler set page:")
                                  +"<BR />"
                                  +tr(" - Turned on the \"Generate debug info (-g3)\" option.")
                                  +"<BR />"
                                  +tr(" - Turned off the \"Strip executable (-s)\" option.")
                                  +"<BR />"
                                  +tr(" - Turned off the \"Optimization level (-O)\" option or set it to \"Debug (-Og)\".")
                                  +"<BR /><BR />"
                                  +tr("You should recompile after change the compiler set or it's settings.")
                                  +"<BR /><BR />"
                                  +tr("Do you want to mannually change the compiler set settings now?")
                                  )== QMessageBox::Yes) {
                changeProjectOptions(
                           SettingsDialog::tr("Compiler Set"),
                           SettingsDialog::tr("Project")
                           );
            }
            return;
        }

        // Did we compile?
        if (!fileExists(mProject->outputFilename())) {
            mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
            mCompileSuccessionTask->type = CompileSuccessionTaskType::Debug;
            mCompileSuccessionTask->execName = mProject->outputFilename();
            mCompileSuccessionTask->isExecutable = true;
            mCompileSuccessionTask->binDirs = binDirs;
            compile();
            return;
        }
        {
            QFileInfo execInfo(mProject->outputFilename());
            QDateTime execModTime = execInfo.lastModified();
            if (execInfo.exists() && mProject->modifiedSince(execModTime)) {
                //if project options changed, or units added/removed
                //mProject->saveAll();
                mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
                mCompileSuccessionTask->type = CompileSuccessionTaskType::Debug;
                mCompileSuccessionTask->execName=mProject->outputFilename();
                mCompileSuccessionTask->isExecutable=true;
                mCompileSuccessionTask->binDirs=binDirs;
                compile(true);
                return;
            }
            if (execInfo.exists() && mProject->unitsModifiedSince(execModTime)) {
                //if units modified
                //mProject->saveAll();
                mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
                mCompileSuccessionTask->type = CompileSuccessionTaskType::Debug;
                mCompileSuccessionTask->execName=mProject->outputFilename();
                mCompileSuccessionTask->isExecutable=true;
                mCompileSuccessionTask->binDirs=binDirs;
                compile();
                return;
            }
        }
        // Did we choose a host application for our DLL?
        if (mProject->options().type == ProjectType::DynamicLib) {
            if (mProject->options().hostApplication.isEmpty()) {
                QMessageBox::critical(this,
                                      tr("Host applcation missing"),
                                      tr("DLL project needs a host application to run.")
                                      +"<br />"
                                      +tr("But it's missing."),
                                      QMessageBox::Ok);
                return;
            } else if (!fileExists(mProject->options().hostApplication)) {
                QMessageBox::critical(this,
                                      tr("Host application not exists"),
                                      tr("Host application file '%1' doesn't exist.")
                                      .arg(mProject->options().hostApplication),
                                      QMessageBox::Ok);
                return;
            }
        }
        // Reset UI, remove invalid breakpoints
        prepareDebugger();
        filePath = mProject->outputFilename();

//        mDebugger->setUseUTF8(e->fileEncoding() == ENCODING_UTF8 || e->fileEncoding() == ENCODING_UTF8_BOM);

        foreach(const PProjectUnit& unit, mProject->unitList()) {
            if (fileExists(unit->fileName()))
                unitFiles.insert(unit->fileName());
        }
        mDebugger->deleteInvalidProjectBreakpoints(unitFiles);
        bool inferiorHasSymbols { true };
        QString inferior { filePath };
        if (mProject->options().type == ProjectType::DynamicLib) {
            inferior=mProject->options().hostApplication;
            inferiorHasSymbols = false;
        }
        QDir::fromNativeSeparators(inferior);
        if (!mDebugger->startClient(
                    mProject->options().compilerSet,
                    inferior,
                    inferiorHasSymbols,
                    debugInferiorhasBreakpoint(),
                    binDirs
                    ))
            return;

        mDebugger->includeOrSkipDirsInSymbolSearch(
                    mProject->options().includeDirs,
                    pSettings->debugger().skipProjectLibraries());
        mDebugger->includeOrSkipDirsInSymbolSearch(
                    mProject->options().libDirs,
                    pSettings->debugger().skipProjectLibraries());
    }
        break;
    case CompileTarget::File: {
            binDirs = compilerSet->binDirs();

            // Check if we enabled proper options
            debugEnabled = compilerSet->getCompileOptionValue(CC_CMD_OPT_DEBUG_INFO) == COMPILER_OPTION_ON;
            stripEnabled = compilerSet->getCompileOptionValue(LINK_CMD_OPT_STRIP_EXE) == COMPILER_OPTION_ON;
            if (stripEnabled && !debugEnabled) {
                if (QMessageBox::question(this,
                                      tr("Correct compile settings for debug"),
                                          tr("The generated executable won't have debug symbol infos, and can't be debugged.")
                                          +"<BR /><BR />"
                                          +tr("If you are using the Release compiler set, please use choose the Debug version from toolbar.")
                                          +"<BR /><BR />"
                                          +tr("Or you can manually change the following settings in the options dialog's compiler set page:")
                                          +"<BR />"
                                          +tr(" - Turned on the \"Generate debug info (-g3)\" option.")
                                          +"<BR />"
                                          +tr(" - Turned off the \"Strip executable (-s)\" option.")
                                          +"<BR />"
                                          +tr(" - Turned off the \"Optimization level (-O)\" option or set it to \"Debug (-Og)\".")
                                          +"<BR /><BR />"
                                          +tr("You should recompile after change the compiler set or it's settings.")
                                          +"<BR /><BR />"
                                          +tr("Do you want to mannually change the compiler set settings now?")
                                      )== QMessageBox::Yes) {
                    changeOptions(
                               SettingsDialog::tr("Compiler Set"),
                               SettingsDialog::tr("Compiler")
                               );
                }
                return;
            }
            Editor* e = mEditorList->getEditor();
            if (e!=nullptr) {
                // Did we saved?
                if (e->modified() || e->isNew()) {
                    // if file is modified,save it first
                    if (!e->save(false,false))
                            return;
                }

                // Did we compiled?
                Settings::PCompilerSet compilerSet =pSettings->compilerSets().defaultSet();
                bool isExecutable;
                if (compilerSet) {
                    filePath = compilerSet->getOutputFilename(e->filename());
                    isExecutable = compilerSet->isOutputExecutable();
                } else {
                    filePath = changeFileExt(e->filename(), DEFAULT_EXECUTABLE_SUFFIX);
                    isExecutable = true;
                }
                if (!isExecutable) {
                    QMessageBox::warning(
                                this,
                                tr("Wrong Compiler Settings"),
                                tr("Compiler is set not to generate executable.")+"<BR /><BR />"
                                +tr("Please correct this before start debugging"));
                    compile();
                    return;
                }

                debugFile.setFile(filePath);
                if (!debugFile.exists()) {
                    mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
                    mCompileSuccessionTask->type = CompileSuccessionTaskType::Debug;
                    mCompileSuccessionTask->binDirs = binDirs;
                    compile();
                    return;
                } else if (
                           compareFileModifiedTime(e->filename(),filePath)>=0
                           || compareFileModifiedTime(filePath, pSettings->compilerSets().defaultIndexTimestamp())<=0 ) {
                    mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
                    mCompileSuccessionTask->type = CompileSuccessionTaskType::Debug;
                    mCompileSuccessionTask->binDirs = binDirs;
                    compile();
                    return;
                }

                prepareDebugger();
                QString newFilePath =QDir::fromNativeSeparators(debugFile.filePath());
                if (!mDebugger->startClient(
                            pSettings->compilerSets().defaultIndex(),
                            newFilePath,
                            true,
                            debugInferiorhasBreakpoint(),
                            binDirs,
                            e->filename()))
                    return;
            }
        }
        break;
    default:
        //don't compile
        updateEditorActions();
        return;
    }

    updateEditorActions();

    mDebugger->runInferior();
}

void MainWindow::showSearchPanel(bool showReplace)
{
    stretchMessagesPanel(true);
    showSearchReplacePanel(showReplace);
    ui->tabMessages->setCurrentWidget(ui->tabSearch);
}

void MainWindow::showCPUInfoDialog()
{
    if (mCPUDialog==nullptr) {
        //main window takes the owner
        mCPUDialog = new CPUDialog(this);
        connect(mCPUDialog, &CPUDialog::closed, this, &MainWindow::cleanUpCPUDialog);
        updateCompileActions();
    }
    mCPUDialog->show();
}

static void setDockMovable(QDockWidget* dock, bool movable) {
    if (movable) {
        dock->setFeatures(dock->features() | QDockWidget::DockWidgetMovable);
    } else {
        dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetMovable);
    }
}

void MainWindow::stretchMessagesPanel(bool open)
{
    //ui->dockMessages->setVisible(open);
    ui->tabMessages->setShrinked(!open);
    if (open) {
        resizeDocks({ui->dockMessages},
                    {ui->tabMessages->beforeShrinkWidthOrHeight()},
                    ui->tabMessages->shrinkOrientation());
    }
    setDockMovable(ui->dockMessages,open);
}


void MainWindow::stretchExplorerPanel(bool open)
{
    ui->tabExplorer->setShrinked(!open);
    if (open) {
        resizeDocks({ui->dockExplorer},
                    {ui->tabExplorer->beforeShrinkWidthOrHeight()},
                    ui->tabExplorer->shrinkOrientation());
    }
    setDockMovable(ui->dockExplorer,open);
}

void MainWindow::prepareDebugger()
{
    mDebugger->stop();

    // Clear logs
    ui->debugConsole->clear();
    if (pSettings->debugger().enableDebugConsole()) {
        ui->debugConsole->addLine("(gdb)");
    }
    ui->txtEvalOutput->clear();

    // Restore when no watch vars are shown
    mDebugger->setLeftPageIndexBackup(ui->tabExplorer->currentIndex());

    // Focus on the debugging buttons
    ui->tabExplorer->setCurrentWidget(ui->tabWatch);
    ui->tabMessages->setCurrentWidget(ui->tabDebug);
    ui->debugViews->setCurrentWidget(ui->tabLocals);
    stretchMessagesPanel(true);
    stretchExplorerPanel(true);

    // Reset watch vars
    //    mDebugger->deleteWatchVars(false);
}

void MainWindow::doAutoSave(Editor *e)
{
    if (!e || !e->canAutoSave())
        return;
    QString filename = e->filename();
    try {
        QFileInfo fileInfo(filename);
        QDir parent = fileInfo.absoluteDir();
        QString baseName = fileInfo.completeBaseName();
        QString suffix = fileInfo.suffix();
        switch(pSettings->editor().autoSaveStrategy()) {
        case assOverwrite:
            e->save();
            return;
        case assAppendUnixTimestamp:
            filename = parent.filePath(
                        QString("%1.%2.%3")
                        .arg(baseName)
                        .arg(QDateTime::currentSecsSinceEpoch())
                        .arg(suffix));
            break;
        case assAppendFormatedTimeStamp: {
            QDateTime time = QDateTime::currentDateTime();
            filename = parent.filePath(
                        QString("%1.%2.%3")
                        .arg(baseName)
                        .arg(time.toString("yyyy.MM.dd.hh.mm.ss"))
                        .arg(suffix));
        }
        }
        if (e->isNew()) {
            e->saveAs();
        } else {
            e->saveFile(filename);
            e->setCanAutoSave(false);
        }
    } catch (FileError& error) {
        QMessageBox::critical(e,
                              tr("Auto Save Error"),
                              tr("Auto save \"%1\" to \"%2\" failed:%3")
                              .arg(e->filename(), filename, error.reason()));
    }
}

void MainWindow::createCustomActions()
{
    // action for problem set
    mProblemSet_New = createAction(
                tr("New Problem Set"),
                ui->tabProblemSet);
    connect(mProblemSet_New,&QAction::triggered,
            this, &MainWindow::onNewProblemSet);

    mProblemSet_Rename = createAction(
                tr("Rename Problem Set"),
                ui->tabProblemSet);
    connect(mProblemSet_Rename, &QAction::triggered,
            this, &MainWindow::onRenameProblemSet);

    mProblemSet_Save = createAction(
                tr("Save Problem Set"),
                ui->tabProblemSet);
    connect(mProblemSet_Save,&QAction::triggered,
            this, &MainWindow::onSaveProblemSet);

    mProblemSet_Load = createAction(
                tr("Load Problem Set"),
                ui->tabProblemSet);
    connect(mProblemSet_Load,&QAction::triggered,
            this, &MainWindow::onLoadProblemSet);

    mProblemSet_ImportFPS = createAction(
                tr("Import FPS Problem Set"),
                ui->tabProblemSet);
    connect(mProblemSet_ImportFPS,&QAction::triggered,
            this, &MainWindow::onImportFPSProblemSet);

    mProblemSet_ExportFPS = createAction(
                tr("Export FPS Problem Set"),
                ui->tabProblemSet);
    connect(mProblemSet_ExportFPS,&QAction::triggered,
            this, &MainWindow::onExportFPSProblemSet);

    mProblemSet_AddProblem = createAction(
                tr("Add Problem"),
                ui->tabProblemSet);
    connect(mProblemSet_AddProblem,&QAction::triggered,
            this, &MainWindow::onAddProblem);

    mProblemSet_RemoveProblem = createAction(
                tr("Remove Problem"),
                ui->tabProblemSet);
    connect(mProblemSet_RemoveProblem,&QAction::triggered,
            this, &MainWindow::onRemoveProblem);

    //problem
    mProblem_OpenSource=createAction(
                tr("Open Source File"),
                ui->tabProblemSet);
    connect(mProblem_OpenSource, &QAction::triggered, this,
            &MainWindow::onProblemOpenSource);

    mProblem_Rename=createAction(
                tr("Rename Problem"),
                ui->tabProblemSet);
    connect(mProblem_Rename, &QAction::triggered, this,
            &MainWindow::onProblemRename);

    mProblem_GotoUrl=createAction(
                tr("Goto Url"),
                ui->tabProblemSet);
    connect(mProblem_GotoUrl, &QAction::triggered, this,
            &MainWindow::onProblemGotoUrl);

    mProblem_Properties = createAction(
                tr("Properties..."),
                ui->tabProblemSet
                );
    connect(mProblem_Properties, &QAction::triggered, this,
            &MainWindow::onProblemProperties);

    //problem cases (buttons)
    mProblem_AddCase = createAction(
                tr("Add Problem Case"),
                ui->tabProblem);
    connect(mProblem_AddCase, &QAction::triggered, this,
            &MainWindow::onAddProblemCase);

    mProblem_RemoveCases = createAction(
                tr("Remove Problem Case"),
                ui->tabProblem);
    connect(mProblem_RemoveCases, &QAction::triggered, this,
            &MainWindow::onRemoveProblemCases);

    mProblem_OpenAnswer = createAction(
                tr("Open Anwser Source File"),
                ui->tabProblem);
    connect(mProblem_OpenAnswer, &QAction::triggered, this,
            &MainWindow::onOpenProblemAnswerFile);

    mProblem_CaseValidationOptions = createAction(
                tr("Problem Cases Validation Options"),
                ui->tabProblem);
    connect(mProblem_CaseValidationOptions, &QAction::triggered, this,
            &MainWindow::onOpenCaseValidationOptions);

    // problem case run
    mProblem_RunAllCases = createGlobalAction(
                tr("Run All Cases"),
                "Problem_RunAllCases",
                tr("Problem"));
    connect(mProblem_RunAllCases, &QAction::triggered, this,
            &MainWindow::onProblemRunAllCases);

    mProblem_RunCurrentCase = createGlobalAction(
                tr("Run Current Case"),
                "Problem_RunCurrentCases",
                tr("Problem"));
    connect(mProblem_RunCurrentCase, &QAction::triggered, this,
            &MainWindow::onProblemRunCurrentCase);

    mProblem_batchSetCases = createGlobalAction(
                tr("Batch Set Cases"),
                "Problem_BatchSetCases",
                tr("Problem"));
    connect(mProblem_batchSetCases, &QAction::triggered, this,
            &MainWindow::onProblemBatchSetCases);

    //Bookmark
    ui->tableBookmark->setContextMenuPolicy(Qt::CustomContextMenu);
    mBookmark_Remove=createAction(
                tr("Remove"),
                ui->tableBookmark);
    connect(mBookmark_Remove, &QAction::triggered,
            this, &MainWindow::onBookmarkRemove);

    mBookmark_RemoveAll=createAction(
                tr("Remove All Bookmarks"),
                ui->tableBookmark);
    connect(mBookmark_RemoveAll, &QAction::triggered,
            this, &MainWindow::onBookmarkRemoveAll);
    mBookmark_Modify=createAction(
                tr("Modify Description"),
                ui->tableBookmark);
    connect(mBookmark_Modify, &QAction::triggered,
            this, &MainWindow::onBookmarkModify);

    //watch view
    mDebugConsole_ShowDetailLog = createAction(
                tr("Show detail debug logs"),
                ui->debugConsole);
    mDebugConsole_ShowDetailLog->setCheckable(true);
    connect(mDebugConsole_ShowDetailLog, &QAction::toggled,
            this, &MainWindow::onDebugConsoleShowDetailLog);

    mDebugConsole_Copy=createAction(
                tr("Copy"),
                ui->debugConsole,
                QKeySequence("Ctrl+C"));
    connect(mDebugConsole_Copy, &QAction::triggered,
            this, &MainWindow::onDebugConsoleCopy);

    mDebugConsole_Paste=createAction(
                tr("Paste"),
                ui->debugConsole,
                QKeySequence("Ctrl+V"));
    connect(mDebugConsole_Paste, &QAction::triggered,
            this, &MainWindow::onDebugConsolePaste);

    mDebugConsole_SelectAll=createAction(
                tr("Select All"),
                ui->debugConsole,
                QKeySequence("Ctrl+A"));
    connect(mDebugConsole_SelectAll, &QAction::triggered,
            this, &MainWindow::onDebugConsoleSelectAll);

    mDebugConsole_Clear=createAction(
                tr("Clear"),
                ui->debugConsole);
    connect(mDebugConsole_Clear, &QAction::triggered,
            this, &MainWindow::onDebugConsoleClear);

    //compile issues
    mTableIssuesCopyAction = createAction(
                tr("Copy"),
                ui->tableIssues,
                QKeySequence("Ctrl+C"));
    connect(mTableIssuesCopyAction,&QAction::triggered,
            this, &MainWindow::onTableIssuesCopy);

    mTableIssuesCopyAllAction = createAction(
                tr("Copy all"),
                ui->tableIssues,
                QKeySequence("Ctrl+Shift+C"));
    connect(mTableIssuesCopyAllAction,&QAction::triggered,
            this, &MainWindow::onTableIssuesCopyAll);

    mTableIssuesClearAction = createAction(
                tr("Clear"),
                ui->tableIssues);
    connect(mTableIssuesClearAction,&QAction::triggered,
            this, &MainWindow::onTableIssuesClear);

    //search
    mSearchViewClearAction = createAction(
                tr("Remove this search"),
                ui->searchHistoryPanel);
    connect(mSearchViewClearAction, &QAction::triggered,
            this, &MainWindow::onSearchViewClear);

    mSearchViewClearAllAction = createAction(
                tr("Clear all searches"),
                ui->searchHistoryPanel);
    connect(mSearchViewClearAllAction,&QAction::triggered,
            this, &MainWindow::onSearchViewClearAll);

    //breakpoints
    mBreakpointViewPropertyAction = createAction(
                tr("Breakpoint condition..."),
                ui->tblBreakpoints);
    connect(mBreakpointViewPropertyAction,&QAction::triggered,
            this, &MainWindow::onModifyBreakpointCondition);

    mBreakpointViewRemoveAllAction = createAction(
                tr("Remove All Breakpoints"),
                ui->tblBreakpoints);
    connect(mBreakpointViewRemoveAllAction,&QAction::triggered,
            this, &MainWindow::onBreakpointViewRemoveAll);
    mBreakpointViewRemoveAction = createAction(
                tr("Remove Breakpoint"),
                ui->tblBreakpoints);
    connect(mBreakpointViewRemoveAction,&QAction::triggered,
            this, &MainWindow::onBreakpointRemove);

    //project
    mProject_Rename_Unit = createAction(
                tr("Rename File"),
                ui->projectView);
    connect(mProject_Rename_Unit, &QAction::triggered,
            this, &MainWindow::onProjectRenameUnit);
    mProject_Add_Folder = createAction(
                tr("Add Folder"),
                ui->projectView);
    connect(mProject_Add_Folder, &QAction::triggered,
            this, &MainWindow::onProjectAddFolder);

    mProject_Rename_Folder = createAction(
                tr("Rename Folder"),
                ui->projectView);
    connect(mProject_Rename_Folder, &QAction::triggered,
            this, &MainWindow::onProjectRenameFolder);

    mProject_Remove_Folder = createAction(
                tr("Remove Folder"),
                ui->projectView);
    connect(mProject_Remove_Folder, &QAction::triggered,
            this, &MainWindow::onProjectRemoveFolder);
    mProject_SwitchFileSystemViewMode = createAction(
                tr("Switch to normal view"),
                ui->projectView);
    connect(mProject_SwitchFileSystemViewMode, &QAction::triggered,
            this, &MainWindow::onProjectSwitchFileSystemViewMode);

    mProject_SwitchCustomViewMode = createAction(
                tr("Switch to custom view"),
                ui->projectView);
    connect(mProject_SwitchCustomViewMode, &QAction::triggered,
            this, &MainWindow::onProjectSwitchCustomViewMode);

    //browser
    mClassBrowser_Sort_By_Type = createAction(
                tr("Sort By Type"),
                ui->tabStructure);
    mClassBrowser_Sort_By_Type->setCheckable(true);
    mClassBrowser_Sort_By_Name = createAction(
                tr("Sort alphabetically"),
                ui->tabStructure);
    mClassBrowser_Sort_By_Name->setCheckable(true);
    mClassBrowser_Show_Inherited = createAction(
                tr("Show inherited members"),
                ui->tabStructure);
    mClassBrowser_Show_Inherited->setCheckable(true);
    mClassBrowser_goto_declaration = createAction(
                tr("Goto declaration"),
                ui->tabStructure);
    mClassBrowser_goto_definition = createAction(
                tr("Goto definition"),
                ui->tabStructure);
    mClassBrowser_Show_CurrentFile = createAction(
                tr("In current file"),
                ui->tabStructure);
    mClassBrowser_Show_CurrentFile->setCheckable(true);
    mClassBrowser_Show_WholeProject = createAction(
                tr("In current project"),
                ui->tabStructure);
    mClassBrowser_Show_WholeProject->setCheckable(true);

    mClassBrowser_Sort_By_Name->setChecked(pSettings->ui().classBrowserSortAlpha());
    mClassBrowser_Sort_By_Type->setChecked(pSettings->ui().classBrowserSortType());
    mClassBrowser_Show_Inherited->setChecked(pSettings->ui().classBrowserShowInherited());
    connect(mClassBrowser_Sort_By_Name, &QAction::toggled,
            this, &MainWindow::onClassBrowserSortByName);
    connect(mClassBrowser_Sort_By_Type, &QAction::toggled,
            this, &MainWindow::onClassBrowserSortByType);
    connect(mClassBrowser_Show_Inherited, &QAction::toggled,
            this, &MainWindow::onClassBrowserShowInherited);

    connect(mClassBrowser_goto_definition,&QAction::triggered,
            this, &MainWindow::onClassBrowserGotoDefinition);

    connect(mClassBrowser_goto_declaration,&QAction::triggered,
            this, &MainWindow::onClassBrowserGotoDeclaration);

    connect(mClassBrowser_Show_CurrentFile,&QAction::triggered,
            this, &MainWindow::onClassBrowserChangeScope);

    connect(mClassBrowser_Show_WholeProject,&QAction::triggered,
            this, &MainWindow::onClassBrowserChangeScope);


    //Files view
    mFilesView_CreateFolder = createAction(
                tr("New Folder"),
                ui->treeFiles);
    connect(mFilesView_CreateFolder, &QAction::triggered,
            this, &MainWindow::onFilesViewCreateFolder);

    mFilesView_CreateFile = createAction(
                tr("New File"),
                ui->treeFiles);
    connect(mFilesView_CreateFile, &QAction::triggered,
            this, &MainWindow::onFilesViewCreateFile);

    mFilesView_Rename = createAction(
                tr("Rename"),
                ui->treeFiles);
    connect(mFilesView_Rename, &QAction::triggered,
            this, &MainWindow::onFilesViewRename);

    mFilesView_RemoveFile = createAction(
                tr("Delete"),
                ui->treeFiles,
                Qt::Key_Delete,
                Qt::WidgetShortcut);
    connect(mFilesView_RemoveFile, &QAction::triggered,
            this, &MainWindow::onFilesViewRemoveFiles);

    mFilesView_Open = createAction(
                tr("Open in Editor"),
                ui->treeFiles,
                Qt::Key_Return,
                Qt::WidgetShortcut);
    mFilesView_Open->setShortcuts(
                {Qt::Key_Return, Qt::Key_Enter});
    connect(mFilesView_Open, &QAction::triggered,
            this, &MainWindow::onFilesViewOpen);

    mFilesView_OpenWithExternal = createAction(
                tr("Open in External Program"),
                ui->treeFiles);
    connect(mFilesView_OpenWithExternal, &QAction::triggered,
            this, &MainWindow::onFilesViewOpenWithExternal);

    mFilesView_OpenInTerminal = createAction(
                tr("Open in Terminal"),
                ui->treeFiles);
    mFilesView_OpenInTerminal->setIcon(ui->actionOpen_Terminal->icon());
    connect(mFilesView_OpenInTerminal, &QAction::triggered,
            this, &MainWindow::onFilesViewOpenInTerminal);

    mFilesView_OpenInExplorer = createAction(
                tr("Open in Windows Explorer"),
                ui->treeFiles);
    mFilesView_OpenInExplorer->setIcon(ui->actionOpen_Containing_Folder->icon());
    connect(mFilesView_OpenInExplorer, &QAction::triggered,
            this, &MainWindow::onFilesViewOpenInExplorer);

    //Tools output
    mToolsOutput_Clear = createAction(
                tr("Clear"),
                ui->txtToolsOutput);
    connect(mToolsOutput_Clear, &QAction::triggered,
            this, &MainWindow::onToolsOutputClear);
    mToolsOutput_Copy = createAction(
                tr("Copy"),
                ui->txtToolsOutput,
                QKeySequence("Ctrl+C"));
    connect(mToolsOutput_Copy, &QAction::triggered,
            this, &MainWindow::onToolsOutputCopy);
    mToolsOutput_SelectAll = createAction(
                tr("Select All"),
                ui->txtToolsOutput,
                QKeySequence("Ctrl+A"));
    connect(mToolsOutput_SelectAll, &QAction::triggered,
            this, &MainWindow::onToolsOutputSelectAll);
}

void MainWindow::initToolButtons()
{
    //problem set toolbuttons
    ui->btnNewProblemSet->setDefaultAction(mProblemSet_New);
    ui->btnSaveProblemSet->setDefaultAction(mProblemSet_Save);
    ui->btnLoadProblemSet->setDefaultAction(mProblemSet_Load);
    ui->btnImportFPS->setDefaultAction(mProblemSet_ImportFPS);
    ui->btnExportFPS->setDefaultAction(mProblemSet_ExportFPS);
    ui->btnAddProblem->setDefaultAction(mProblemSet_AddProblem);
    ui->btnRemoveProblem->setDefaultAction(mProblemSet_RemoveProblem);

    //problem toolbuttons
    ui->btnAddProblemCase->setDefaultAction(mProblem_AddCase);
    ui->btnRemoveProblemCase->setDefaultAction(mProblem_RemoveCases);
    ui->btnOpenProblemAnswer->setDefaultAction(mProblem_OpenAnswer);
    ui->btnCaseValidateOptions->setDefaultAction(mProblem_CaseValidationOptions);


    ui->btnRunAllProblemCases->setDefaultAction(mProblem_RunAllCases);


    //toolbar for class browser
    mClassBrowserToolbar = new QWidget();
    {
        QVBoxLayout* layout = dynamic_cast<QVBoxLayout*>( ui->tabStructure->layout());
        layout->insertWidget(0,mClassBrowserToolbar);
        QHBoxLayout* hlayout =  new QHBoxLayout();
        hlayout->setContentsMargins(2,2,2,2);
        mClassBrowserToolbar->setLayout(hlayout);
        QToolButton * toolButton;
        int size = calIconSize(
                    pSettings->environment().interfaceFont(),
                    pSettings->environment().interfaceFontSize())*pSettings->environment().iconZoomFactor();
        QSize iconSize(size,size);
        toolButton = new QToolButton;
        toolButton->setIconSize(iconSize);
        toolButton->setDefaultAction(mClassBrowser_Sort_By_Type);
        hlayout->addWidget(toolButton);
        toolButton = new QToolButton;
        toolButton->setIconSize(iconSize);
        toolButton->setDefaultAction(mClassBrowser_Sort_By_Name);
        hlayout->addWidget(toolButton);
        QFrame * vLine = new QFrame();
        vLine->setFrameShape(QFrame::VLine);
        vLine->setFrameShadow(QFrame::Sunken);
        hlayout->addWidget(vLine);
        toolButton = new QToolButton;
        toolButton->setIconSize(iconSize);
        toolButton->setDefaultAction(mClassBrowser_Show_Inherited);
        hlayout->addWidget(toolButton);
        hlayout->addStretch();
    }

    //toolbar for files view
    {
        QHBoxLayout* hlayout =  dynamic_cast<QHBoxLayout*>(ui->panelFiles->layout());
        QToolButton * toolButton;
        int size = calIconSize(
                    pSettings->environment().interfaceFont(),
                    pSettings->environment().interfaceFontSize())*pSettings->environment().iconZoomFactor();
        QSize iconSize(size,size);
        toolButton = new QToolButton;
        toolButton->setIconSize(iconSize);
        toolButton->setDefaultAction(ui->actionOpen_Folder);
        hlayout->addWidget(toolButton);
        toolButton = new QToolButton;
        toolButton->setIconSize(iconSize);
        toolButton->setDefaultAction(ui->actionLocate_in_Files_View);
        hlayout->addWidget(toolButton);
        QFrame * vLine = new QFrame();
        vLine->setFrameShape(QFrame::VLine);
        vLine->setFrameShadow(QFrame::Sunken);
        hlayout->addWidget(vLine);
        toolButton = new QToolButton;
        toolButton->setIconSize(iconSize);
        toolButton->setDefaultAction(ui->actionFilesView_Hide_Non_Support_Files);
        ui->actionFilesView_Hide_Non_Support_Files->setChecked(pSettings->environment().hideNonSupportFilesInFileView());
        hlayout->addWidget(toolButton);
    }

}

//static void limitActionShortCutScope(QAction* action,QWidget* scopeWidget) {
//    action->setParent(scopeWidget);
//    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
//}

QAction* MainWindow::createAction(
        const QString& text,
        QWidget* parent,
        QKeySequence shortcut,
        Qt::ShortcutContext shortcutContext) {
    QAction* action= new QAction(text,parent);
    if (!shortcut.isEmpty())
        action->setShortcut(shortcut);
    action->setPriority(QAction::HighPriority);
    action->setShortcutContext(shortcutContext);
    parent->addAction(action);
    return action;
}

QAction *MainWindow::createGlobalAction(const QString &text, const QString &objectName, const QString &groupName, QKeySequence shortcut)
{
    QAction* action = createAction(text,this,shortcut);
    action->setObjectName(objectName);
    action->setShortcutContext(Qt::WindowShortcut);
    action->setData(groupName);
    return action;
}

void MainWindow::scanActiveProject(bool parse)
{
    if (!mProject)
        return;
    if (!pSettings->codeCompletion().enabled())
        return;
    if (!mProject->cppParser()->enabled())
        return;

    //UpdateClassBrowsing;
    if (parse) {
        resetCppParser(mProject->cppParser(), mProject->options().compilerSet);
        mProject->resetParserProjectFiles();
        parseFileList(mProject->cppParser());
    } else {
        mProject->resetParserProjectFiles();
    };
}

void MainWindow::onBookmarkContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(mBookmark_Remove);
    menu.addAction(mBookmark_RemoveAll);
    menu.addAction(mBookmark_Modify);
    menu.exec(ui->tableBookmark->mapToGlobal(pos));
}

bool MainWindow::saveLastOpens()
{
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + DEV_LASTOPENS_FILE;
    QJsonObject rootObj;
    if (mProject) {
        rootObj["lastProject"]=mProject->filename();
    }
    QJsonArray filesArray;
    for (int i=0;i<mEditorList->pageCount();i++) {
      Editor * editor = (*mEditorList)[i];
      QJsonObject fileObj;
      if (editor->isNew()) {
          if (!editor->modified())
              continue;
          QMessageBox::StandardButton reply;
          reply = QMessageBox::question(editor,QObject::tr("Save"),
                                        QString(QObject::tr("Save changes to %1?")).arg(editor->filename()),
                                        QMessageBox::Yes|QMessageBox::No|QMessageBox::Yes|QMessageBox::Cancel);
          if (reply == QMessageBox::No) {
              editor->setModified(false);
              continue;
          } else if (reply == QMessageBox::Yes) {
              if (!editor->save(false,false)) {
                  return false;
              }
          } else {
              return false;
          }
      }

      fileObj["filename"] = editor->filename();
      fileObj["onLeft"] = (editor->pageControl() != mEditorList->rightPageWidget());
      fileObj["focused"] = editor->hasFocus();
      fileObj["caretX"] = editor->caretX();
      fileObj["caretY"] = editor->caretY();
      fileObj["top"] = editor->topPos();
      fileObj["left"] = editor->leftPos();
      filesArray.append(fileObj);
    }
    rootObj["files"]=filesArray;
    QJsonDocument doc;
    doc.setObject(rootObj);
    QByteArray json = doc.toJson();
    QFile file(filename);

    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        QMessageBox::critical(this,
                              tr("Save last open info error"),
                              tr("Can't open last open information file '%1' for write!")
                              .arg(filename),
                              QMessageBox::Ok);
        return true;
    }
    if (file.write(doc.toJson())!=json.count()) {
        QMessageBox::critical(this,
                              tr("Save last open info error"),
                              tr("Can't save last open info file '%1'")
                              .arg(filename),
                              QMessageBox::Ok);
        return true;
    }
    file.close();
    return true;
}

void MainWindow::loadLastOpens()
{
    mOpeningFiles=true;
    auto action=finally([this]{
        mOpeningFiles=false;
    });
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + DEV_LASTOPENS_FILE;
    if (!fileExists(filename))
        return;
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(this,
                              tr("Load last open info error"),
                              tr("Can't load last open info file '%1'")
                              .arg(filename),
                              QMessageBox::Ok);
        return;
    }
    QJsonParseError error;
    QJsonDocument doc=QJsonDocument::fromJson(file.readAll(),&error);
    if (error.error != QJsonParseError::NoError) {
        QMessageBox::critical(this,
                              tr("Load last open info error"),
                              tr("Can't load last open info file '%1'")
                              .arg(filename)+" : <BR/>"
                              +QString("%1").arg(error.errorString()),
                              QMessageBox::Ok);
        return;

    }
    QJsonObject rootObj = doc.object();
    QString projectFilename = rootObj["lastProject"].toString();
    if (!projectFilename.isEmpty()) {
        openProject(projectFilename, false);
    }
    QJsonArray filesArray = rootObj["files"].toArray();
    Editor *  focusedEditor = nullptr;
    for (int i=0;i<filesArray.count();i++) {
        QJsonObject fileObj = filesArray[i].toObject();
        QString editorFilename = fileObj["filename"].toString("");
        if (!fileExists(editorFilename))
            continue;
        bool onLeft = fileObj["onLeft"].toBool();
        QTabWidget* page;
        if (onLeft)
            page = mEditorList->leftPageWidget();
        else
            page = mEditorList->rightPageWidget();
        PProjectUnit unit;
        if (mProject) {
            unit = mProject->findUnit(editorFilename);
        }
        bool inProject = (mProject && unit);
        QByteArray encoding = unit ? unit->encoding() :
                                     (pSettings->editor().autoDetectFileEncoding()? QByteArray(ENCODING_AUTO_DETECT) : pSettings->editor().defaultEncoding());
        Project* pProject = (inProject?mProject.get():nullptr);
        if (pProject && encoding==ENCODING_PROJECT)
            encoding=pProject->options().encoding;
        Editor * editor = mEditorList->newEditor(editorFilename, encoding, pProject,false,page);

        if (inProject && editor) {
            mProject->loadUnitLayout(editor);
        }
//        if (mProject) {
//            mProject->associateEditorToUnit(editor,unit);
//        }
        if (!editor)
            continue;
        QSynedit::BufferCoord pos;
        pos.ch = fileObj["caretX"].toInt(1);
        pos.line = fileObj["caretY"].toInt(1);
        editor->setCaretXY(pos);
        editor->setTopPos(
                    fileObj["top"].toInt(1)
                    );
        editor->setLeftPos(
                    fileObj["left"].toInt(1)
                    );
        if (fileObj["focused"].toBool(false))
            focusedEditor = editor;
        //mVisitHistoryManager->removeFile(editorFilename);
    }
    if (mProject && mEditorList->pageCount()==0) {
        mProject->doAutoOpen();
        updateEditorBookmarks();
        updateEditorBreakpoints();
    }

    if (mEditorList->pageCount()>0) {
        updateEditorActions();
        //updateForEncodingInfo();
    }
    if (!focusedEditor) {
        focusedEditor = mEditorList->getEditor();
    }
    if (focusedEditor) {
        focusedEditor->reparse(false);
        focusedEditor->checkSyntaxInBack();
        focusedEditor->reparseTodo();
        focusedEditor->activate();
    }
}

void MainWindow::updateTools()
{
    QList<QAction*> oldToolActions;
    //save old custom tools actions.
    foreach(QAction* action, ui->menuTools->actions()) {
        if (action->objectName().startsWith("tool-"))
            oldToolActions.append(action);
    }
    ui->menuTools->clear();
    //delete old custom tools actions;
    for(int i=0;i<oldToolActions.length();i++) {
        delete oldToolActions[i];
    }
    ui->menuTools->addAction(ui->actionOptions);
    if (!mToolsManager->tools().isEmpty()) {
        ui->menuTools->addSeparator();
        QList<QAction*> actions;
        foreach (const PToolItem& item, mToolsManager->tools()) {
            QAction* action = createGlobalAction(item->title,"tool-"+item->id, tr("Tools"));
            connect(action, &QAction::triggered,
                    [item,this] (){
                executeTool(item);
            });
            ui->menuTools->addAction(action);
            actions.append(action);
        }
        ShortcutManager shortcutManager;
        shortcutManager.load();
        shortcutManager.applyTo(actions);
    }
}

void MainWindow::newEditor(const QString& suffix)
{
    try {
        QString filename;
        do {
            filename = QString("untitled%1").arg(getNewFileNumber());
            if (suffix.isEmpty()) {
                if (pSettings->editor().defaultFileCpp()) {
                    Settings::PCompilerSet compilerSet = pSettings->compilerSets().defaultSet();
                    if (compilerSet && !compilerSet->canCompileCPP()) {
                        filename+=".c";
                    } else {
                        filename+=".cpp";
                    }
                } else
                    filename+=".c";
            } else
                filename+= "." + suffix;

        } while(mEditorList->hasFilename(filename));
        Editor * editor=mEditorList->newEditor(filename,
                                               pSettings->editor().defaultEncoding(),
                                               nullptr,true);
        editor->activate();
        //updateForEncodingInfo();
    }  catch (FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }

}

void MainWindow::buildContextMenus()
{

    //context menu signal for the problem list view
    ui->lstProblemSet->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lstProblemSet, &QWidget::customContextMenuRequested,
            this, &MainWindow::onLstProblemSetContextMenu);

    //context menu signal for the problem cases list view
    ui->tblProblemCases->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tblProblemCases, &QWidget::customContextMenuRequested,
            this, &MainWindow::onTableProblemCasesContextMenu);

    //context menu signal for the watch view
    ui->watchView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->watchView,&QWidget::customContextMenuRequested,
            this, &MainWindow::onWatchViewContextMenu);

    //context menu signal for the bookmark view
    ui->tableBookmark->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableBookmark,&QWidget::customContextMenuRequested,
            this, &MainWindow::onBookmarkContextMenu);

    //context menu signal for the debug console
    ui->debugConsole->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->debugConsole,&QWidget::customContextMenuRequested,
            this, &MainWindow::onDebugConsoleContextMenu);

    //context menu signal for Editor's tabbar
    ui->EditorTabsLeft->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->EditorTabsLeft->tabBar(),
            &QWidget::customContextMenuRequested,
            this,
            &MainWindow::onEditorLeftTabContextMenu
            );

    ui->EditorTabsRight->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->EditorTabsRight->tabBar(),
            &QWidget::customContextMenuRequested,
            this,
            &MainWindow::onEditorRightTabContextMenu
            );

    //context menu signal for Compile Issue view
    ui->tableIssues->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableIssues,&QWidget::customContextMenuRequested,
            this, &MainWindow::onTableIssuesContextMenu);

    //context menu signal for search view
    ui->tabSearch->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tabSearch, &QWidget::customContextMenuRequested,
            this, &MainWindow::onSearchViewContextMenu);

    //context menu signal for breakpoints view
    ui->tblBreakpoints->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tblBreakpoints,&QWidget::customContextMenuRequested,
             this, &MainWindow::onBreakpointsViewContextMenu);

    //context menu signal for project view
    ui->projectView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->projectView,&QWidget::customContextMenuRequested,
             this, &MainWindow::onProjectViewContextMenu);

    //context menu signal for class browser
    ui->tabStructure->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tabStructure,&QWidget::customContextMenuRequested,
             this, &MainWindow::onClassBrowserContextMenu);

    //menu for statusbar
    mFileEncodingStatus->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mFileEncodingStatus,&QWidget::customContextMenuRequested,
             this, &MainWindow::onFileEncodingContextMenu);

    //menu for files view
    ui->treeFiles->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeFiles,&QWidget::customContextMenuRequested,
             this, &MainWindow::onFilesViewContextMenu);

    //context menu signal for class browser
    ui->txtToolsOutput->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->txtToolsOutput,&QWidget::customContextMenuRequested,
             this, &MainWindow::onToolsOutputContextMenu);
}

void MainWindow::buildEncodingMenu()
{
    QMenu* menuCharsets = new QMenu();
    menuCharsets->setTitle(tr("Character sets"));
    QStringList languages = pCharsetInfoManager->languageNames();
    foreach (const QString& langName, languages) {
        QMenu* menuLang = new QMenu();
        menuLang->setTitle(langName);
        menuCharsets->addMenu(menuLang);
        QList<PCharsetInfo> charInfos = pCharsetInfoManager->findCharsetsByLanguageName(langName);
        connect(menuLang,&QMenu::aboutToShow,
                [langName,menuLang,this]() {
            menuLang->clear();
            Editor* editor = mEditorList->getEditor();
            QList<PCharsetInfo> charInfos = pCharsetInfoManager->findCharsetsByLanguageName(langName);
            foreach (const PCharsetInfo& info, charInfos) {
                QAction * action = new QAction(info->name);
                action->setCheckable(true);
                if (editor)
                    action->setChecked(info->name == editor->encodingOption());
                connect(action, &QAction::triggered,
                        [info,this](){
                    Editor * editor = mEditorList->getEditor();
                    if (editor == nullptr)
                        return;
                    try {
                        editor->setEncodingOption(info->name);
                    } catch(FileError e) {
                        QMessageBox::critical(this,tr("Error"),e.reason());
                    }
                });
                menuLang->addAction(action);
            }
        });
    }

    mMenuEncoding = new QMenu(this);
    mMenuEncoding->setTitle(tr("File Encoding"));
    mMenuEncoding->addAction(ui->actionAuto_Detect);
    mMenuEncoding->addAction(ui->actionEncode_in_ANSI);
    mMenuEncoding->addAction(ui->actionEncode_in_UTF_8);
    mMenuEncoding->addAction(ui->actionEncode_in_UTF_8_BOM);

    mMenuEncoding->addMenu(menuCharsets);
    mMenuEncoding->addSeparator();
    mMenuEncoding->addAction(ui->actionConvert_to_ANSI);
    mMenuEncoding->addAction(ui->actionConvert_to_UTF_8);
    mMenuEncoding->addAction(ui->actionConvert_to_UTF_8_BOM);

    QList<PCharsetInfo> charsetsForLocale = pCharsetInfoManager->findCharsetByLocale(pCharsetInfoManager->localeName());

    foreach(const PCharsetInfo& charset, charsetsForLocale) {
        QAction * action = new QAction(
                    tr("Convert to %1").arg(QString(charset->name)));
        connect(action, &QAction::triggered,
                [charset,this](){
            Editor * editor = mEditorList->getEditor();
            if (editor == nullptr)
                return;
            if (QMessageBox::warning(this,tr("Confirm Convertion"),
                           tr("The editing file will be saved using %1 encoding. <br />This operation can't be reverted. <br />Are you sure to continue?")
                           .arg(QString(charset->name)),
                           QMessageBox::Yes, QMessageBox::No)!=QMessageBox::Yes)
                return;
            editor->convertToEncoding(charset->name);
        });
        mMenuEncoding->addAction(action);
    }

    ui->menuEdit->insertMenu(ui->actionFoldAll,mMenuEncoding);
    ui->menuEdit->insertSeparator(ui->actionFoldAll);
    //ui->actionAuto_Detect->setCheckable(true);
    ui->actionEncode_in_ANSI->setCheckable(true);
    ui->actionEncode_in_UTF_8->setCheckable(true);
    ui->actionEncode_in_UTF_8_BOM->setCheckable(true);
}

void MainWindow::buildNewlineMenu()
{
    mMenuNewline = new QMenu(this);
    mMenuNewline->setTitle(tr("Newline"));
}

void MainWindow::maximizeEditor()
{
    if (ui->tabExplorer->isShrinked() && ui->tabExplorer->isShrinked()) {
        stretchMessagesPanel(true);
        stretchExplorerPanel(true);
    } else {
        stretchMessagesPanel(false);
        stretchExplorerPanel(false);
    }
}

QStringList MainWindow::getBinDirsForCurrentEditor()
{
    Editor * e=mEditorList->getEditor();
    if (e) {
        if (e->inProject() && mProject) {
            return mProject->binDirs();
        } else {
            return getDefaultCompilerSetBinDirs();
        }
    } else if (mProject) {
        return mProject->binDirs();
    }
    return QStringList();
}

QStringList MainWindow::getDefaultCompilerSetBinDirs()
{
    if (pSettings->compilerSets().defaultSet())
        return pSettings->compilerSets().defaultSet()->binDirs();
    return QStringList();
}

void MainWindow::openShell(const QString &folder, const QString &shellCommand, const QStringList& binDirs)
{
    QProcess process;
    process.setWorkingDirectory(folder);
    process.setProgram(shellCommand);
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments * args){
        args->flags |= CREATE_NEW_CONSOLE;
        args->startupInfo->dwFlags &=  ~STARTF_USESTDHANDLES; //
    });
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList pathAdded;
    pathAdded.append(binDirs);
    pathAdded.append(pSettings->dirs().appDir());
    if (!path.isEmpty()) {
        path= pathAdded.join(PATH_SEPARATOR) + PATH_SEPARATOR + path;
    } else {
        path = pathAdded.join(PATH_SEPARATOR);
    }
    env.insert("PATH",path);
    process.setProcessEnvironment(env);
#endif
    process.startDetached();
}

void MainWindow::onAutoSaveTimeout()
{
    if (mQuitting)
        return;
    if (!pSettings->editor().enableAutoSave())
        return;
    mAutoSaveTimer.stop();
    {
        auto action=finally([this]{
            mAutoSaveTimer.start(pSettings->editor().autoSaveInterval()*60*1000);
        });
        int updateCount = 0;
        switch (pSettings->editor().autoSaveTarget()) {
        case astCurrentFile: {
            Editor *e = mEditorList->getEditor();
            doAutoSave(e);
            updateCount++;
        }
            break;
        case astAllOpennedFiles:
            for (int i=0;i<mEditorList->pageCount();i++) {
                Editor *e = (*mEditorList)[i];
                doAutoSave(e);
                updateCount++;
            }
            break;
        case astAllProjectFiles:
            if (!mProject)
                return;
            for (int i=0;i<mEditorList->pageCount();i++) {
                Editor *e = (*mEditorList)[i];
                if (!e->inProject())
                    return;
                doAutoSave(e);
                updateCount++;
            }
            //todo: auto save project files
            break;
        }
        updateStatusbarMessage(tr("%1 files autosaved").arg(updateCount));
    }
}

void MainWindow::onWatchViewContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(ui->actionAdd_Watch);
    menu.addAction(ui->actionRemove_Watch);
    menu.addAction(ui->actionRemove_All_Watches);
    menu.addAction(ui->actionModify_Watch);
    menu.exec(ui->watchView->mapToGlobal(pos));
}

void MainWindow::onTableIssuesContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(mTableIssuesCopyAction);
    menu.addAction(mTableIssuesCopyAllAction);
    menu.addSeparator();
    menu.addAction(mTableIssuesClearAction);
    menu.exec(ui->tableIssues->mapToGlobal(pos));
}

void MainWindow::onSearchViewContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(mSearchViewClearAction);
    menu.addAction(mSearchViewClearAllAction);
    mSearchViewClearAction->setEnabled(ui->cbSearchHistory->currentIndex()>0);
    mSearchViewClearAction->setEnabled(mSearchResultListModel->rowCount(QModelIndex())>0);
    menu.exec(ui->searchHistoryPanel->mapToGlobal(pos));
}

void MainWindow::onBreakpointsViewContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(mBreakpointViewPropertyAction);
    menu.addAction(mBreakpointViewRemoveAllAction);
    menu.addAction(mBreakpointViewRemoveAction);
    mBreakpointViewPropertyAction->setEnabled(ui->tblBreakpoints->currentIndex().isValid());
    mBreakpointViewRemoveAction->setEnabled(ui->tblBreakpoints->currentIndex().isValid());
    menu.exec(ui->tblBreakpoints->mapToGlobal(pos));
}

void MainWindow::onProjectViewContextMenu(const QPoint &pos)
{
    if (!mProject)
        return;
    bool onFolder = false;
    bool onUnit = false;
    bool onRoot = false;
    bool folderEmpty = false;
    bool multiSelection = ui->projectView->selectionModel()->selectedRows().count()>1;
    QModelIndex current = mProjectProxyModel->mapToSource(ui->projectView->selectionModel()->currentIndex());
    if (current.isValid() && mProject) {
        ProjectModelNode * node = static_cast<ProjectModelNode*>(current.internalPointer());
        PProjectModelNode pNode = mProject->pointerToNode(node);
        if (pNode) {
            onFolder = (!pNode->isUnit);
            onUnit = (pNode->isUnit);
            onRoot = false;
            if (onFolder && !onRoot) {
                folderEmpty = pNode->children.isEmpty();
            }
        } else {
            onFolder = true;
            onRoot = true;
        }
    }

    QMenu menu(this);
#ifdef ENABLE_VCS
    QMenu vcsMenu(this);
    QString branch;
    GitManager vcsManager;
    bool hasRepository = vcsManager.hasRepository(mProject->folder(),branch);
#endif
    updateProjectActions();
    menu.addAction(ui->actionProject_New_File);
    menu.addAction(ui->actionNew_Class);
    menu.addAction(ui->actionNew_Header);
    menu.addAction(ui->actionAdd_to_project);
    if (!onFolder) {
        menu.addAction(ui->actionRemove_from_project);
    }
    if (onUnit && !multiSelection) {
        menu.addAction(mProject_Rename_Unit);
    }
    menu.addSeparator();
#ifdef ENABLE_VCS
    if (pSettings->vcs().gitOk()) {
        if (hasRepository) {
            menu.addMenu(&vcsMenu);
        } else {
            ui->actionGit_Create_Repository->setEnabled(true);
            menu.addAction(ui->actionGit_Create_Repository);
        }
        menu.addSeparator();
    }
#endif
    if (onFolder && mProject->modelType()==ProjectModelType::Custom) {
        menu.addAction(mProject_Add_Folder);
        if (!onRoot) {
            menu.addAction(mProject_Rename_Folder);
            if (folderEmpty) {
                menu.addAction(mProject_Remove_Folder);
            }
        }
        menu.addSeparator();
    }
    menu.addAction(ui->actionProject_Open_Folder_In_Explorer);
    menu.addAction(ui->actionProject_Open_In_Terminal);
    menu.addSeparator();
    if (mProject->modelType() == ProjectModelType::Custom) {
        menu.addAction(mProject_SwitchFileSystemViewMode);
    } else {
        menu.addAction(mProject_SwitchCustomViewMode);
    }
    menu.addAction(ui->actionProject_options);
    menu.addSeparator();
    menu.addAction(ui->actionClose_Project);

#ifdef ENABLE_VCS
    if (pSettings->vcs().gitOk() && hasRepository) {
        mProject->model()->iconProvider()->update();
        vcsMenu.setTitle(tr("Version Control"));
        if (ui->projectView->selectionModel()->hasSelection()) {
            bool shouldAdd = true;
            foreach (const QModelIndex& index, ui->projectView->selectionModel()->selectedRows()) {
                if (!index.isValid()) {
                    shouldAdd=false;
                    break;
                }
                QModelIndex realIndex = mProjectProxyModel->mapToSource(index);
                ProjectModelNode * node = static_cast<ProjectModelNode*>(realIndex.internalPointer());
                if (!node || !node->isUnit) {
                    shouldAdd=false;
                    break;
                }
                PProjectUnit pUnit=node->pUnit.lock();
                if (mProject->model()->iconProvider()->VCSRepository()->isFileInRepository(
                            pUnit->fileName()
                            )
                        &&
                        !mProject->model()->iconProvider()->VCSRepository()->isFileConflicting(
                                                    pUnit->fileName()
                                                    )) {
                    shouldAdd=false;
                    break;
                }
            }
            if (shouldAdd)
                vcsMenu.addAction(ui->actionGit_Add_Files);
        }
        vcsMenu.addAction(ui->actionGit_Remotes);
        vcsMenu.addAction(ui->actionGit_Log);
        vcsMenu.addAction(ui->actionGit_Branch);
        vcsMenu.addAction(ui->actionGit_Merge);
        vcsMenu.addAction(ui->actionGit_Commit);
        vcsMenu.addAction(ui->actionGit_Restore);

        bool canBranch = false;
        canBranch = !mProject->model()->iconProvider()->VCSRepository()->hasChangedFiles()
                && !mProject->model()->iconProvider()->VCSRepository()->hasStagedFiles();
        ui->actionGit_Branch->setEnabled(canBranch);
        ui->actionGit_Merge->setEnabled(canBranch);
        ui->actionGit_Commit->setEnabled(true);
        ui->actionGit_Remotes->setEnabled(true);
        ui->actionGit_Log->setEnabled(true);
        ui->actionGit_Restore->setEnabled(true);

//        vcsMenu.addAction(ui->actionGit_Reset);
//        vcsMenu.addAction(ui->actionGit_Revert);
//        ui->actionGit_Reset->setEnabled(true);
//        ui->actionGit_Revert->setEnabled(true);
    }
#endif
    menu.exec(ui->projectView->mapToGlobal(pos));
}

void MainWindow::onClassBrowserContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    bool canGoto = false;
    QModelIndex index = ui->classBrowser->currentIndex();
    if (index.isValid()) {
        ClassBrowserNode * node = static_cast<ClassBrowserNode*>(index.internalPointer());
        if (node) {
            PStatement statement = node->statement;
            if (statement) {
                canGoto = true;
            }
        }
    }
    mClassBrowser_goto_declaration->setEnabled(canGoto);
    mClassBrowser_goto_definition->setEnabled(canGoto);
    menu.addAction(mClassBrowser_goto_declaration);
    menu.addAction(mClassBrowser_goto_definition);
    menu.addSeparator();
    menu.addAction(mClassBrowser_Sort_By_Name);
    menu.addAction(mClassBrowser_Sort_By_Type);
    menu.addAction(mClassBrowser_Show_Inherited);
    Editor * editor = mEditorList->getEditor();
    if ((editor &&editor->inProject()) || mProject) {
        menu.addSeparator();
        menu.addAction(mClassBrowser_Show_CurrentFile);
        menu.addAction(mClassBrowser_Show_WholeProject);

        if (mProject) {
            mClassBrowser_Show_CurrentFile->setChecked(mProject->options().classBrowserType==ProjectClassBrowserType::CurrentFile);
            mClassBrowser_Show_WholeProject->setChecked(mProject->options().classBrowserType==ProjectClassBrowserType::WholeProject);
        }
    }

    menu.exec(ui->projectView->mapToGlobal(pos));
}

void MainWindow::onDebugConsoleContextMenu(const QPoint &pos)
{
    QMenu menu(this);

    bool oldBlock = mDebugConsole_ShowDetailLog->blockSignals(true);
    mDebugConsole_ShowDetailLog->setChecked(pSettings->debugger().showDetailLog());
    mDebugConsole_ShowDetailLog->blockSignals(oldBlock);

    menu.addAction(mDebugConsole_Copy);
    menu.addAction(mDebugConsole_Paste);
    menu.addAction(mDebugConsole_SelectAll);
    menu.addAction(mDebugConsole_Clear);
    menu.addSeparator();
    menu.addAction(mDebugConsole_ShowDetailLog);
    menu.exec(ui->debugConsole->mapToGlobal(pos));
}

void MainWindow::onFileEncodingContextMenu(const QPoint &pos)
{
    if (mMenuEncoding->isEnabled())
        mMenuEncoding->exec(mFileEncodingStatus->mapToGlobal(pos));
}

void MainWindow::onFilesViewContextMenu(const QPoint &pos)
{
    QMenu menu(this);
#ifdef ENABLE_VCS
    GitManager vcsManager;
    QString branch;
    bool hasRepository = vcsManager.hasRepository(pSettings->environment().currentFolder(),branch);
    QMenu vcsMenu(this);
#endif
    menu.addAction(ui->actionOpen_Folder);
    menu.addSeparator();
    menu.addAction(mFilesView_CreateFolder);
    menu.addAction(mFilesView_CreateFile);
    menu.addAction(mFilesView_RemoveFile);
    menu.addAction(mFilesView_Rename);
    menu.addSeparator();
#ifdef ENABLE_VCS
    if (pSettings->vcs().gitOk()) {
        if (hasRepository) {
            menu.addMenu(&vcsMenu);
        } else {
            ui->actionGit_Create_Repository->setEnabled(true);
            menu.addAction(ui->actionGit_Create_Repository);
        }
        menu.addSeparator();
    }
#endif
    menu.addAction(mFilesView_Open);
    menu.addAction(mFilesView_OpenWithExternal);
    menu.addSeparator();
    menu.addAction(mFilesView_OpenInTerminal);
    menu.addAction(mFilesView_OpenInExplorer);
    menu.addSeparator();
    menu.addAction(ui->actionFilesView_Hide_Non_Support_Files);
    QString path = mFileSystemModel.filePath(ui->treeFiles->currentIndex());
    QFileInfo info(path);
    mFilesView_Open->setEnabled(info.isFile());
    mFilesView_OpenWithExternal->setEnabled(info.isFile());
    mFilesView_OpenInTerminal->setEnabled(!path.isEmpty());
    mFilesView_OpenInExplorer->setEnabled(!path.isEmpty());
    mFilesView_Rename->setEnabled(!path.isEmpty());
    mFilesView_RemoveFile->setEnabled(!path.isEmpty() || !ui->treeFiles->selectionModel()->selectedRows().isEmpty());

#ifdef ENABLE_VCS
    if (pSettings->vcs().gitOk() && hasRepository) {
        mFileSystemModelIconProvider.update();
        vcsMenu.setTitle(tr("Version Control"));
        if (ui->treeFiles->selectionModel()->hasSelection()) {
            bool shouldAdd = true;
            foreach (const QModelIndex& index, ui->treeFiles->selectionModel()->selectedRows()) {
                if (mFileSystemModelIconProvider.VCSRepository()->isFileInRepository(
                            mFileSystemModel.fileInfo(index)
                            ) &&
                        ! mFileSystemModelIconProvider.VCSRepository()->isFileConflicting(
                            mFileSystemModel.fileInfo(index)
                            )
                        ) {
                    shouldAdd=false;
                    break;
                }
            }
            if (shouldAdd)
                vcsMenu.addAction(ui->actionGit_Add_Files);
        }
        vcsMenu.addAction(ui->actionGit_Remotes);
        vcsMenu.addAction(ui->actionGit_Log);
        vcsMenu.addAction(ui->actionGit_Branch);
        vcsMenu.addAction(ui->actionGit_Merge);
        vcsMenu.addAction(ui->actionGit_Commit);
        vcsMenu.addAction(ui->actionGit_Restore);

        bool canBranch = !mFileSystemModelIconProvider.VCSRepository()->hasChangedFiles()
                && !mFileSystemModelIconProvider.VCSRepository()->hasStagedFiles();
        ui->actionGit_Branch->setEnabled(canBranch);
        ui->actionGit_Merge->setEnabled(canBranch);
        ui->actionGit_Log->setEnabled(true);
        ui->actionGit_Remotes->setEnabled(true);
        ui->actionGit_Commit->setEnabled(true);
        ui->actionGit_Restore->setEnabled(true);

//        vcsMenu.addAction(ui->actionGit_Reset);
//        vcsMenu.addAction(ui->actionGit_Revert);
//        ui->actionGit_Reset->setEnabled(true);
//        ui->actionGit_Revert->setEnabled(true);
    }
#endif
    menu.exec(ui->treeFiles->mapToGlobal(pos));
}

void MainWindow::onLstProblemSetContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QModelIndex idx = ui->lstProblemSet->currentIndex();
    mProblem_Properties->setEnabled(idx.isValid());
    mProblem_Rename->setEnabled(idx.isValid());
    menu.addAction(mProblemSet_Rename);
    menu.addAction(mProblemSet_New);
    menu.addAction(mProblemSet_Load);
    menu.addAction(mProblemSet_Save);
    menu.addAction(mProblemSet_ImportFPS);
    menu.addAction(mProblemSet_ExportFPS);
    menu.addAction(mProblemSet_AddProblem);
    menu.addAction(mProblemSet_RemoveProblem);
    menu.addSeparator();
    menu.addAction(mProblem_Rename);
    menu.addAction(mProblem_GotoUrl);
    if (idx.isValid()) {
        POJProblem problem = mOJProblemSetModel.problem(idx.row());
        QMenu * menuSetAnswer = new QMenu(&menu);
        QActionGroup *actionGroup = new QActionGroup(menuSetAnswer);
        bool answerFound=false;
        menuSetAnswer->setTitle(tr("Set answer to..."));
        for (int i=0;i<mEditorList->pageCount();i++) {
            Editor *e = (*mEditorList)[i];
            QString filename = e->filename();
            QAction* action = new QAction(filename,menuSetAnswer);
            action->setCheckable(true);
            action->setActionGroup(actionGroup);

            if (filename.compare(problem->answerProgram, PATH_SENSITIVITY)==0) {
                action->setChecked(true);
                answerFound = true;
            }
            menuSetAnswer->addAction(action);
        }
        if (!answerFound && !problem->answerProgram.isEmpty()) {
            QAction* action = new QAction(problem->answerProgram,menuSetAnswer);
            action->setCheckable(true);
            action->setChecked(true);
            action->setActionGroup(actionGroup);
            menuSetAnswer->addAction(action);
        }
        connect(actionGroup, &QActionGroup::triggered,
                [problem,this](QAction* action) {
            if (action->text().compare(problem->answerProgram, PATH_SENSITIVITY)
                    !=0)
                problem->answerProgram = action->text();
            else
                problem->answerProgram = "";
            if (problem == mOJProblemModel.problem()) {
                ui->btnOpenProblemAnswer->setEnabled(!problem->answerProgram.isEmpty());
            }
        });
        QAction * action = new QAction(tr("select other file..."),menuSetAnswer);
        connect(action, &QAction::triggered,
                [problem,this](){
            QString filename = QFileDialog::getOpenFileName(
                        this,
                        tr("Select Answer Source File"),
                        QString(),
                        tr("C/C++ Source Files (*.c *.cpp *.cc *.cxx)"),
                        nullptr);
            if (!filename.isEmpty()) {
                QDir::setCurrent(extractFileDir(filename));
                problem->answerProgram = filename;
                if (problem == mOJProblemModel.problem()) {
                    ui->btnOpenProblemAnswer->setEnabled(!problem->answerProgram.isEmpty());
                }
            }
        });
        menuSetAnswer->addAction(action);
        menu.addMenu(menuSetAnswer);
        mProblem_GotoUrl->setEnabled(!problem->url.isEmpty());
        mProblem_OpenSource->setEnabled(!problem->answerProgram.isEmpty());
    } else {
        mProblem_GotoUrl->setEnabled(false);
        mProblem_OpenSource->setEnabled(false);
    }
    menu.addAction(mProblem_OpenSource);
    menu.addAction(mProblem_Properties);
    menu.exec(ui->lstProblemSet->mapToGlobal(pos));
}

void MainWindow::onTableProblemCasesContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(mProblem_AddCase);
    menu.addAction(mProblem_RemoveCases);
    menu.addAction(mProblem_batchSetCases);
    menu.addSeparator();
    QModelIndex idx = ui->tblProblemCases->currentIndex();
    menu.addAction(mProblem_RunAllCases);
    menu.addAction(mProblem_RunCurrentCase);
    menu.addAction(mProblem_CaseValidationOptions);
    mProblem_RunAllCases->setEnabled(mOJProblemModel.count()>0 && ui->actionRun->isEnabled());
    mProblem_RunCurrentCase->setEnabled(idx.isValid() && ui->actionRun->isEnabled());
    menu.exec(ui->tblProblemCases->mapToGlobal(pos));
}

void MainWindow::onToolsOutputContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(mToolsOutput_Copy);
    menu.addAction(mToolsOutput_SelectAll);
    menu.addSeparator();
    menu.addAction(mToolsOutput_Clear);
    menu.exec(ui->txtToolsOutput->mapToGlobal(pos));
}

void MainWindow::onProblemSetIndexChanged(const QModelIndex &current, const QModelIndex &/* previous */)
{
    QModelIndex idx = current;
    if (!idx.isValid()) {
        mProblemSet_RemoveProblem->setEnabled(false);
        mOJProblemModel.setProblem(nullptr);
        ui->txtProblemCaseExpected->clearAll();
        ui->txtProblemCaseInput->clearAll();
        ui->txtProblemCaseOutput->clearAll();
        ui->tabProblem->setEnabled(false);
        ui->lblProblem->clear();
        ui->lblProblem->setToolTip("");
        ui->tabProblem->setEnabled(false);
    } else {
        mProblemSet_RemoveProblem->setEnabled(true);
        POJProblem problem = mOJProblemSetModel.problem(idx.row());
        if (mFullInitialized) {
            if (problem && !problem->answerProgram.isEmpty()) {
                openFile(problem->answerProgram);
            }
        }
        mOJProblemModel.setProblem(problem);
        updateProblemTitle();
        if (mOJProblemModel.count()>0) {
            ui->tblProblemCases->setCurrentIndex(mOJProblemModel.index(0,0));
        } else {
            onProblemCaseIndexChanged(QModelIndex(),QModelIndex());
        }
        stretchMessagesPanel(true);
        ui->tabMessages->setCurrentWidget(ui->tabProblem);
        ui->tabProblem->setEnabled(true);
        ui->btnOpenProblemAnswer->setEnabled(!problem->answerProgram.isEmpty());
    }
}

void MainWindow::onProblemCaseIndexChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QModelIndex idx = current;
    if (previous.isValid()) {
        POJProblemCase problemCase = mOJProblemModel.getCase(previous.row());
        problemCase->input = ui->txtProblemCaseInput->toPlainText();
        problemCase->expected = ui->txtProblemCaseExpected->toPlainText();
    }
    if (idx.isValid()) {
        POJProblemCase problemCase = mOJProblemModel.getCase(idx.row());
        if (problemCase) {
            mProblem_RemoveCases->setEnabled(true);
            mProblem_RunAllCases->setEnabled(ui->actionRun->isEnabled());
            fillProblemCaseInputAndExpected(problemCase);
            ui->txtProblemCaseOutput->clearAll();
            ui->txtProblemCaseOutput->setPlainText(problemCase->output);
            updateProblemCaseOutput(problemCase);
            return;
        }
    }
    ui->btnProblemCaseClearInputFileName->setVisible(false);
    ui->btnProblemCaseInputFileName->setEnabled(false);
    ui->txtProblemCaseInputFileName->clear();
    ui->txtProblemCaseInputFileName->setToolTip("");

    ui->btnProblemCaseClearExpectedOutputFileName->setVisible(false);
    ui->btnProblemCaseExpectedOutputFileName->setEnabled(false);
    ui->txtProblemCaseExpectedOutputFileName->clear();
    ui->txtProblemCaseExpectedOutputFileName->setToolTip("");

    mProblem_RemoveCases->setEnabled(false);
    mProblem_RunAllCases->setEnabled(false);
    ui->txtProblemCaseInputFileName->clear();
    ui->btnProblemCaseInputFileName->setEnabled(false);
    ui->txtProblemCaseInput->clearAll();
    ui->txtProblemCaseInput->setReadOnly(true);
    ui->txtProblemCaseExpected->clearAll();
    ui->txtProblemCaseExpected->setReadOnly(true);
    ui->txtProblemCaseOutput->clearAll();

    ui->lblProblemCaseExpected->clear();
    ui->lblProblemCaseOutput->clear();
    ui->lblProblemCaseInput->clear();
}

void MainWindow::onProblemNameChanged(int index)
{
    QModelIndex idx = ui->lstProblemSet->currentIndex();
    if (idx.isValid() && index == idx.row()) {
        updateProblemTitle();
    }
}

void MainWindow::onProblemRunCurrentCase()
{
    if (!ui->tblProblemCases->currentIndex().isValid())
        return;
    showHideMessagesTab(ui->tabProblem,ui->actionProblem);
    applyCurrentProblemCaseChanges();

    runExecutable(RunType::CurrentProblemCase);
}

void MainWindow::onProblemBatchSetCases()
{
    showHideMessagesTab(ui->tabProblem,ui->actionProblem);
    if (mOJProblemModel.count()>0 && QMessageBox::question(this,tr("Batch Set Cases"),
                              tr("This operation will remove all cases for the current problem.")
                              +"<br />"
                              +tr("Do you really want to do that?"),
                              QMessageBox::Yes| QMessageBox::No,
                              QMessageBox::No)!=QMessageBox::Yes)
        return;
    QString folder = QDir::currentPath();
    if (!mOJProblemSetModel.exportFilename().isEmpty())
        folder = extractFileDir(mOJProblemSetModel.exportFilename());
    QStringList files = QFileDialog::getOpenFileNames(
                this,
                tr("Choose input files"),
                folder,
                tr("Input data files (*.in)"));
    if (files.isEmpty())
        return;
    mOJProblemModel.removeCases();
    foreach (const QString& filename, files) {
        POJProblemCase problemCase = std::make_shared<OJProblemCase>();
        problemCase->name = QFileInfo(filename).baseName();
        problemCase->testState = ProblemCaseTestState::NotTested;
        problemCase->inputFileName = filename;
        QString expectedFileName;
        expectedFileName = filename.mid(0,filename.length()-2)+"ans";
        if (fileExists(expectedFileName)) {
            problemCase->expectedOutputFileName = expectedFileName;
        } else {
            expectedFileName = filename.mid(0,filename.length()-2)+"out";
            if (fileExists(expectedFileName))
                problemCase->expectedOutputFileName = expectedFileName;
        }
        mOJProblemModel.addCase(problemCase);
    }
}

void MainWindow::onNewProblemReceived(int num, int total, POJProblem newProblem)
{
    if (mOJProblemSetModel.problemNameUsed(newProblem->name))
        return;
    updateStatusbarMessage(tr("Problem '%1' received (%2/%3).")
                              .arg(newProblem->name).arg(num).arg(total));
    mOJProblemSetModel.addProblem(newProblem);
    ui->tabExplorer->setCurrentWidget(ui->tabProblemSet);
    ui->lstProblemSet->setCurrentIndex(mOJProblemSetModel.index(
                                           mOJProblemSetModel.count()-1
                                           ,0));
    if (isMinimized())
        showNormal();
    raise(); // for mac OS?
    activateWindow();
}

void MainWindow::updateProblemTitle()
{
    ui->lblProblem->setText(mOJProblemModel.getTitle());
    ui->lblProblem->setToolTip(mOJProblemModel.getTooltip());
}

void MainWindow::onEditorClosed()
{
    if (mQuitting)
        return;
    updateEditorActions();
    updateAppTitle();
}

void MainWindow::onToolsOutputClear()
{
    ui->txtToolsOutput->clear();
}

void MainWindow::onToolsOutputCopy()
{
    ui->txtToolsOutput->copy();
}

void MainWindow::onToolsOutputSelectAll()
{
    ui->txtToolsOutput->selectAll();
}

void MainWindow::onShowInsertCodeSnippetMenu()
{
    mMenuInsertCodeSnippet->clear();
    QList<PCodeSnippet> snippets;
    foreach (const PCodeSnippet& snippet, mCodeSnippetManager->snippets()) {
        if (snippet->section>=0 && !snippet->caption.isEmpty())
            snippets.append(snippet);
    }
    if (snippets.isEmpty())
        return;
    std::sort(snippets.begin(),snippets.end(),[](const PCodeSnippet& s1, const PCodeSnippet& s2){
        return s1->section<s2->section;
    });
    int section = 0;
    int sectionCount = 0;
    int count = 0;
    bool sectionNotEmpty = false;
    foreach (const PCodeSnippet& snippet, snippets) {
        if (snippet->section>section && sectionCount<6) {
            section = snippet->section;
            sectionCount++;
            if (sectionNotEmpty)
                mMenuInsertCodeSnippet->addSeparator();
        }
        QAction * action = mMenuInsertCodeSnippet->addAction(snippet->caption);
        connect(action, &QAction::triggered,
                [snippet,this](){
            Editor * editor = mEditorList->getEditor();
            if (editor) {
                editor->insertCodeSnippet(snippet->code);
            }
        });
        sectionNotEmpty = true;
        count++;
        if (count>15)
            break;
    }

}

void MainWindow::onFilesViewCreateFolderFolderLoaded(const QString& path)
{

    if (mFilesViewNewCreatedFolder.isEmpty() && mFilesViewNewCreatedFile.isEmpty())
        return;

    if (path!=extractFilePath(mFilesViewNewCreatedFolder) && path!=extractFilePath(mFilesViewNewCreatedFile))
        return;

    disconnect(&mFileSystemModel,&QFileSystemModel::directoryLoaded,
            this,&MainWindow::onFilesViewCreateFolderFolderLoaded);

    QModelIndex newIndex = mFileSystemModel.index(mFilesViewNewCreatedFolder.isEmpty() ? mFilesViewNewCreatedFile : mFilesViewNewCreatedFolder);

    if (newIndex.isValid()) {
        ui->treeFiles->setCurrentIndex(newIndex);
        ui->treeFiles->edit(newIndex);
    }
    mFilesViewNewCreatedFolder="";
}

void MainWindow::onFilesViewCreateFolder()
{
    QModelIndex index = ui->treeFiles->currentIndex();
    QModelIndex parentIndex;
    QDir dir;
    if (index.isValid()
            && ui->treeFiles->selectionModel()->isSelected(index)) {
        if (mFileSystemModel.isDir(index)) {
            dir = QDir(mFileSystemModel.fileInfo(index).absoluteFilePath());
            parentIndex = index;
        } else {
            dir = mFileSystemModel.fileInfo(index).absoluteDir();
            parentIndex = mFileSystemModel.index(dir.absolutePath());
        }
        //ui->treeFiles->expand(index);
    } else {
        dir = mFileSystemModel.rootDirectory();
        parentIndex=mFileSystemModel.index(dir.absolutePath());
    }
    QString folderName = tr("New Folder");
    int count = 0;
    while (dir.exists(folderName)) {
        count++;
        folderName = tr("New Folder %1").arg(count);
    }
    QModelIndex newIndex = mFileSystemModel.mkdir(parentIndex,folderName);
    if (newIndex.isValid()) {
        if (ui->treeFiles->isExpanded(parentIndex)) {
            ui->treeFiles->setCurrentIndex(newIndex);
            ui->treeFiles->edit(newIndex);
        } else {
            connect(&mFileSystemModel,&QFileSystemModel::directoryLoaded,
                    this,&MainWindow::onFilesViewCreateFolderFolderLoaded);
            ui->treeFiles->expand(parentIndex);
            mFilesViewNewCreatedFolder=mFileSystemModel.filePath(newIndex);
        }

    }
}

void MainWindow::onFilesViewCreateFile()
{
    QModelIndex index = ui->treeFiles->currentIndex();
    QDir dir;
    if (index.isValid()
            && ui->treeFiles->selectionModel()->isSelected(index)) {
        if (mFileSystemModel.isDir(index))
            dir = QDir(mFileSystemModel.fileInfo(index).absoluteFilePath());
        else
            dir = mFileSystemModel.fileInfo(index).absoluteDir();
    } else {
        dir = mFileSystemModel.rootDirectory();
    }
    QString suffix;
    if (pSettings->editor().defaultFileCpp())
        suffix=".cpp";
    else
        suffix=".c";
    QString fileName = QString("untitled")+suffix;
    int count = 0;
    while (dir.exists(fileName)) {
        count++;
        fileName = QString("untitled%1").arg(count)+suffix;
    }
    QFile file(dir.filePath(fileName));
    file.open(QFile::NewOnly);
    file.close();
    QModelIndex newIndex = mFileSystemModel.index(dir.filePath(fileName));
    connect(&mFileSystemModel,&QFileSystemModel::directoryLoaded,
            this,&MainWindow::onFilesViewCreateFolderFolderLoaded);
    ui->treeFiles->expand(index);
    mFilesViewNewCreatedFile=mFileSystemModel.filePath(newIndex);
}


void MainWindow::onFilesViewRemoveFiles()
{
    QModelIndexList indexList = ui->treeFiles->selectionModel()->selectedRows();
    if (indexList.isEmpty()) {
        QModelIndex index = ui->treeFiles->currentIndex();
        if (QMessageBox::question(ui->treeFiles,tr("Delete")
                                  ,tr("Do you really want to delete %1?").arg(mFileSystemModel.fileName(index)),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No)!=QMessageBox::Yes)
            return;
        doFilesViewRemoveFile(index);
    } else if (indexList.count()==1) {
        QModelIndex index = indexList[0];
        if (QMessageBox::question(ui->treeFiles,tr("Delete")
                                  ,tr("Do you really want to delete %1?").arg(mFileSystemModel.fileName(index)),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No)!=QMessageBox::Yes)
            return;
        doFilesViewRemoveFile(index);
    } else {
        if (QMessageBox::question(ui->treeFiles,tr("Delete")
                                  ,tr("Do you really want to delete %1 files?").arg(indexList.count()),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No)!=QMessageBox::Yes)
            return;
        foreach (const QModelIndex& index, indexList) {
            doFilesViewRemoveFile(index);
        }
    }
}

void MainWindow::onFilesViewRename() {
    QModelIndex index = ui->treeFiles->currentIndex();
    if (!index.isValid())
        return ;
    ui->treeFiles->edit(index);
}

void MainWindow::onProblemProperties()
{
    showHideMessagesTab(ui->tabProblem,ui->actionProblem);
    QModelIndex idx = ui->lstProblemSet->currentIndex();
    if (!idx.isValid())
        return;
    POJProblem problem=mOJProblemSetModel.problem(idx.row());
    if (!problem)
        return;
    OJProblemPropertyWidget dialog;
    dialog.loadFromProblem(problem);
    if (dialog.exec() == QDialog::Accepted) {
        dialog.saveToProblem(problem);
    }
}

void MainWindow::onProblemOpenSource()
{
    QModelIndex idx = ui->lstProblemSet->currentIndex();
    if (!idx.isValid())
        return;
    POJProblem problem=mOJProblemSetModel.problem(idx.row());
    if (!problem)
        return;
    if (!problem->answerProgram.isEmpty()) {
        openFile(problem->answerProgram);
    }
}

void MainWindow::onProblemRename()
{
    QModelIndex idx = ui->lstProblemSet->currentIndex();
    if (!idx.isValid())
        return;
    ui->lstProblemSet->edit(idx);
}

void MainWindow::onProblemGotoUrl()
{
    QModelIndex idx = ui->lstProblemSet->currentIndex();
    if (!idx.isValid())
        return;
    POJProblem problem=mOJProblemSetModel.problem(idx.row());
    if (!problem)
        return;
    if (!problem->url.isEmpty()) {
        QDesktopServices::openUrl(problem->url);
    }
}

void MainWindow::onRenameProblemSet()
{
    QString newName = QInputDialog::getText(
                ui->lblProblemSet,
                tr("Set Problem Set Name"),
                tr("Problem Set Name:"),
                QLineEdit::Normal,
                ui->lblProblemSet->text());
    newName = newName.trimmed();
    if (!newName.isEmpty()){
        mOJProblemSetModel.rename(newName);
        ui->lblProblemSet->setText(mOJProblemSetModel.name());
    }
}

void MainWindow::onBookmarkRemove()
{
    QModelIndex index = ui->tableBookmark->currentIndex();
    if (index.isValid()) {
        PBookmark bookmark = mBookmarkModel->bookmark(index.row());
        if (bookmark) {
            Editor * editor = mEditorList->getOpenedEditorByFilename(bookmark->filename);
            if (editor && editor->inProject() == mBookmarkModel->isForProject()) {
                editor->removeBookmark(bookmark->line);
            }
            mBookmarkModel->removeBookmarkAt(index.row());
        }
    }
}

void MainWindow::onBookmarkRemoveAll()
{
    mBookmarkModel->clear(mBookmarkModel->isForProject());
    for (int i=0;i<mEditorList->pageCount();i++) {
        Editor * editor = (*mEditorList)[i];
        if (editor->inProject() == mBookmarkModel->isForProject())
            editor->clearBookmarks();
    }
}

void MainWindow::onBookmarkModify()
{
    QModelIndex index = ui->tableBookmark->currentIndex();
    if (index.isValid()) {
        PBookmark bookmark = mBookmarkModel->bookmark(index.row());
        if (bookmark) {
            QString desc = QInputDialog::getText(ui->tableBookmark,tr("Bookmark Description"),
                                             tr("Description:"),QLineEdit::Normal,
                                             bookmark->description);
            desc = desc.trimmed();
            mBookmarkModel->updateDescription(bookmark->filename,bookmark->line,desc);
        }
    }
}

void MainWindow::onDebugConsoleShowDetailLog()
{
    pSettings->debugger().setShowDetailLog(mDebugConsole_ShowDetailLog->isChecked());
    pSettings->debugger().save();
}

void MainWindow::onDebugConsolePaste()
{
    ui->debugConsole->paste();
}

void MainWindow::onDebugConsoleSelectAll()
{
    ui->debugConsole->selectAll();
}

void MainWindow::onDebugConsoleCopy()
{
    ui->debugConsole->copy();
}

void MainWindow::onDebugConsoleClear()
{
    ui->debugConsole->clear();
    if (pSettings->debugger().enableDebugConsole()) {
        ui->debugConsole->addLine("(gdb)");
    }
}

void MainWindow::onBreakpointTableDoubleClicked(const QModelIndex &index)
{
    if (index.isValid() && index.column()==2) {
        modifyBreakpointCondition(index.row());
    }
}

void MainWindow::onFilesViewOpenInExplorer()
{
    QString path = mFileSystemModel.filePath(ui->treeFiles->currentIndex());
    if (!path.isEmpty()) {
        openFileFolderInExplorer(path);
    }
}

void MainWindow::onFilesViewOpenInTerminal()
{
    QString path = mFileSystemModel.filePath(ui->treeFiles->currentIndex());
    if (!path.isEmpty()) {
        QFileInfo fileInfo(path);
#ifdef Q_OS_WIN
        openShell(fileInfo.path(),"cmd.exe",getDefaultCompilerSetBinDirs());
#else
        openShell(fileInfo.path(),pSettings->environment().terminalPath(),getDefaultCompilerSetBinDirs());
#endif
    }
}

void MainWindow::onFilesViewOpenWithExternal()
{
    QString path = mFileSystemModel.filePath(ui->treeFiles->currentIndex());
    if (!path.isEmpty() && QFileInfo(path).isFile()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void MainWindow::onFilesViewOpen()
{
    QString path = mFileSystemModel.filePath(ui->treeFiles->currentIndex());
    if (!path.isEmpty() && QFileInfo(path).isFile()) {
        if (getFileType(path)==FileType::Project) {
            openProject(path);
        } else {
            openFile(path);
        }
    }
}

void MainWindow::onClassBrowserGotoDeclaration()
{
    QModelIndex index = ui->classBrowser->currentIndex();
    if (!index.isValid())
        return ;
    ClassBrowserNode * node = static_cast<ClassBrowserNode*>(index.internalPointer());
    if (!node)
        return ;
    PStatement statement = node->statement;
    if (!statement) {
        return;
    }
    QString filename;
    int line;
    filename = statement->fileName;
    line = statement->line;
    Editor* e=openFile(filename);
    if (e) {
        e->setCaretPositionAndActivate(line,1);
    }
}

void MainWindow::onClassBrowserGotoDefinition()
{
    QModelIndex index = ui->classBrowser->currentIndex();
    if (!index.isValid())
        return ;
    ClassBrowserNode * node = static_cast<ClassBrowserNode*>(index.internalPointer());
    if (!node)
        return ;
    PStatement statement = node->statement;
    if (!statement) {
        return;
    }
    QString filename;
    int line;
    filename = statement->definitionFileName;
    line = statement->definitionLine;
    Editor* e=openFile(filename);
    if (e) {
        e->setCaretPositionAndActivate(line,1);
    }
}

void MainWindow::onClassBrowserShowInherited()
{
    pSettings->ui().setClassBrowserShowInherited(mClassBrowser_Show_Inherited->isChecked());
    pSettings->ui().save();
    mClassBrowserModel.fillStatements();
}

void MainWindow::onClassBrowserSortByType()
{
    pSettings->ui().setClassBrowserSortType(mClassBrowser_Sort_By_Type->isChecked());
    pSettings->ui().save();
    mClassBrowserModel.fillStatements();
}

void MainWindow::onClassBrowserSortByName()
{
    pSettings->ui().setClassBrowserSortAlpha(mClassBrowser_Sort_By_Name->isChecked());
    pSettings->ui().save();
    mClassBrowserModel.fillStatements();
}

void MainWindow::onClassBrowserChangeScope()
{
    if (!mProject)
        return;
    ProjectClassBrowserType classBrowserType;
    if (mProject->options().classBrowserType==ProjectClassBrowserType::CurrentFile) {
        classBrowserType=ProjectClassBrowserType::WholeProject;
    } else {
        classBrowserType=ProjectClassBrowserType::CurrentFile;
    }
    mProject->options().classBrowserType=classBrowserType;
    mProject->saveOptions();
    Editor* editor = mEditorList->getEditor();
    if ((!editor || editor->inProject())  &&
            mClassBrowserModel.classBrowserType()!=classBrowserType) {
        mClassBrowserModel.setClassBrowserType(classBrowserType);
    }
}

void MainWindow::onClassBrowserRefreshStart()
{
    mClassBrowserCurrentStatement="";
    QModelIndex index = ui->classBrowser->currentIndex();
    if (!index.isValid())
        return ;
    ClassBrowserNode * node = static_cast<ClassBrowserNode*>(index.internalPointer());
    if (!node)
        return ;
    PStatement statement = node->statement;
    if (!statement) {
        return;
    }
    mClassBrowserCurrentStatement=QString("%1+%2+%3")
            .arg(statement->fullName)
            .arg(statement->noNameArgs)
            .arg((int)statement->kind);
}

void MainWindow::onClassBrowserRefreshEnd()
{
    QModelIndex index = mClassBrowserModel.modelIndexForStatement(mClassBrowserCurrentStatement);
    if (index.isValid()) {
        ui->classBrowser->expand(index);
        ui->classBrowser->setCurrentIndex(index);
    }
}

void MainWindow::onProjectSwitchCustomViewMode()
{
    mProject->setModelType(ProjectModelType::Custom);
    ui->projectView->expand(
                mProjectProxyModel->mapFromSource(
                    mProject->model()->rootIndex()));
}

void MainWindow::onProjectSwitchFileSystemViewMode()
{
    mProject->setModelType(ProjectModelType::FileSystem);
    ui->projectView->expand(
                mProjectProxyModel->mapFromSource(
                    mProject->model()->rootIndex()));
}

void MainWindow::onProjectRemoveFolder()
{
    if (!mProject)
        return;
    QModelIndex current = mProjectProxyModel->mapToSource(ui->projectView->currentIndex());
    if (!current.isValid()) {
        return;
    }
    ProjectModelNode * node = static_cast<ProjectModelNode*>(current.internalPointer());
    PProjectModelNode folderNode =  mProject->pointerToNode(node);
    if (!folderNode)
        return;
    if (folderNode->isUnit)
        return;
    mProject->removeFolder(folderNode);
    mProject->saveOptions();

}

void MainWindow::onProjectRenameFolder()
{
    if (ui->projectView->currentIndex().isValid()) {
        ui->projectView->edit(ui->projectView->currentIndex());
    }
}

void MainWindow::onProjectAddFolder()
{
    if (!mProject)
        return;
    QModelIndex current = mProjectProxyModel->mapToSource(ui->projectView->currentIndex());
    if (!current.isValid()) {
        return;
    }
    ProjectModelNode * node = static_cast<ProjectModelNode*>(current.internalPointer());
    PProjectModelNode folderNode =  mProject->pointerToNode(node);
    if (!folderNode)
        folderNode = mProject->rootNode();
    if (folderNode->isUnit)
        return;
    QString s=tr("New folder");
    int i=1;
    while (fileExists(s)) {
        s=tr("New folder")+QString("%1").arg(i);
        i++;
    }
    bool ok;
    s = QInputDialog::getText(ui->projectView,
                          tr("Add Folder"),
                          tr("Folder name:"),
                          QLineEdit::Normal, s,
                          &ok).trimmed();
    if (ok && !s.isEmpty()) {
        PProjectModelNode node = mProject->addFolder(folderNode,s);

        mProject->saveAll();
        setProjectViewCurrentNode(node);
        updateProjectView();
    }
}

void MainWindow::onProjectRenameUnit()
{
    if (ui->projectView->currentIndex().isValid()) {
        ui->projectView->edit(ui->projectView->currentIndex());
    }
}

void MainWindow::onBreakpointRemove()
{
    int index =ui->tblBreakpoints->selectionModel()->currentIndex().row();

    PBreakpoint breakpoint = debugger()->breakpointModel()->breakpoint(index, debugger()->isForProject());
    if (breakpoint) {
        Editor * e = mEditorList->getOpenedEditorByFilename(breakpoint->filename);
        if (e) {
            if (e->hasBreakpoint(breakpoint->line))
                e->toggleBreakpoint(breakpoint->line);
        } else {
            debugger()->breakpointModel()->removeBreakpoint(index,debugger()->isForProject());
        }
    }
}

void MainWindow::onBreakpointViewRemoveAll()
{
    debugger()->deleteBreakpoints(debugger()->isForProject());
    for (int i=0;i<mEditorList->pageCount();i++) {
        Editor * e = (*(mEditorList))[i];
        if (e) {
            e->resetBreakpoints();
        }
    }
}

void MainWindow::onModifyBreakpointCondition()
{
    int index =ui->tblBreakpoints->selectionModel()->currentIndex().row();

    modifyBreakpointCondition(index);
}

void MainWindow::onSearchViewClearAll()
{
    mSearchResultModel.clear();
}

void MainWindow::onSearchViewClear()
{
    int index = ui->cbSearchHistory->currentIndex();
    if (index>=0) {
        mSearchResultModel.removeSearchResults(index);
    }
}

void MainWindow::onTableIssuesClear()
{
    clearIssues();
}

void MainWindow::onTableIssuesCopyAll()
{
    QClipboard* clipboard=QGuiApplication::clipboard();
    QMimeData * mimeData = new QMimeData();
    mimeData->setText(ui->tableIssues->toTxt());
    mimeData->setHtml(ui->tableIssues->toHtml());
    clipboard->clear();
    clipboard->setMimeData(mimeData);
}

void MainWindow::onTableIssuesCopy()
{
    QModelIndex index = ui->tableIssues->selectionModel()->currentIndex();
    PCompileIssue issue = ui->tableIssues->issue(index);
    if (issue) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(issue->description);
    }
}

void MainWindow::onEditorContextMenu(const QPoint& pos)
{
    Editor * editor = mEditorList->getEditor();
    if (!editor)
        return;
    FileType fileType=getFileType(editor->filename());
    bool canDebug = (fileType==FileType::CSource
            || fileType==FileType::CHeader
            || fileType==FileType::CppSource
            || fileType==FileType::CppHeader
            || fileType==FileType::GAS
            );
    QMenu menu(this);
    QSynedit::BufferCoord p;
    int line;
    if (editor->getPositionOfMouse(p)) {
        if (!editor->getLineOfMouse(line))
            line=-1;
        if (!switchHeaderSourceTarget(editor).isEmpty()) {
            menu.addAction(ui->actionSwitchHeaderSource);
            menu.addSeparator();
        }
        //mouse on editing area
        if (canDebug) {
            menu.addAction(ui->actionRun);
            menu.addAction(ui->actionDebug);
            menu.addSeparator();
        }
        if (editor->parser() && editor->parser()->enabled()) {
            menu.addAction(ui->actionGoto_Declaration);
            menu.addAction(ui->actionGoto_Definition);
            menu.addAction(ui->actionFind_references);
            menu.addSeparator();
        }

        menu.addAction(ui->actionOpen_Containing_Folder);
        menu.addAction(ui->actionOpen_Terminal);
        menu.addAction(ui->actionLocate_in_Files_View);
        menu.addSeparator();
        menu.addAction(ui->actionTrim_trailing_spaces);
        menu.addAction(ui->actionReformat_Code);
        menu.addSeparator();
        menu.addAction(ui->actionCut);
        menu.addAction(ui->actionCopy);
        menu.addAction(ui->actionPaste);
        menu.addAction(ui->actionSelectAll);
        menu.addSeparator();
        if (canDebug) {
            menu.addAction(ui->actionAdd_Watch);
            menu.addAction(ui->actionAdd_Watchpoint);
            menu.addAction(ui->actionToggle_Breakpoint);
            menu.addAction(ui->actionBreakpoint_property);
            menu.addAction(ui->actionClear_all_breakpoints);
            menu.addSeparator();
        }
        menu.addAction(ui->actionToggle_Bookmark);
        menu.addAction(ui->actionModify_Bookmark_Description);
        menu.addSeparator();
        menu.addAction(ui->actionGo_to_Line);
        menu.addSeparator();
        menu.addAction(ui->actionFile_Properties);
    } else {
        //mouse on gutter
        if (!editor->getLineOfMouse(line))
            line=-1;
        if (canDebug) {
            menu.addAction(ui->actionToggle_Breakpoint);
            menu.addAction(ui->actionBreakpoint_property);
            menu.addAction(ui->actionClear_all_breakpoints);
            menu.addSeparator();
        }
        menu.addAction(ui->actionToggle_Bookmark);
        menu.addAction(ui->actionModify_Bookmark_Description);
        menu.addSeparator();
        menu.addAction(ui->actionGo_to_Line);
    }
    ui->actionLocate_in_Files_View->setEnabled(!editor->isNew());
    ui->actionBreakpoint_property->setEnabled(editor->hasBreakpoint(line));
    ui->actionToggle_Bookmark->setEnabled(
                line>=0 && editor->lineCount()>0
                );
    ui->actionModify_Bookmark_Description->setEnabled(editor->hasBookmark(line));
    menu.exec(editor->viewport()->mapToGlobal(pos));
}

void MainWindow::onEditorRightTabContextMenu(const QPoint& pos)
{
    onEditorTabContextMenu(ui->EditorTabsRight,pos);
}

void MainWindow::onEditorLeftTabContextMenu(const QPoint& pos)
{
    onEditorTabContextMenu(ui->EditorTabsLeft,pos);
}


void MainWindow::onEditorTabContextMenu(QTabWidget* tabWidget, const QPoint &pos)
{
    int index = tabWidget->tabBar()->tabAt(pos);
    if (index<0)
        return;
    tabWidget->setCurrentIndex(index);
    QMenu menu(this);
    QTabBar* tabBar = tabWidget->tabBar();
    Editor * editor = dynamic_cast<Editor *>(tabWidget->widget(index));
    if (!switchHeaderSourceTarget(editor).isEmpty()) {
        menu.addAction(ui->actionSwitchHeaderSource);
        menu.addSeparator();
    }
    menu.addAction(ui->actionClose);
    menu.addAction(ui->actionClose_All);
    menu.addAction(ui->actionClose_Others);
    menu.addSeparator();
    menu.addAction(ui->actionToggle_Readonly);
    menu.addSeparator();
    menu.addAction(ui->actionOpen_Containing_Folder);
    menu.addAction(ui->actionOpen_Terminal);
    menu.addAction(ui->actionLocate_in_Files_View);
    menu.addSeparator();
    menu.addAction(ui->actionMove_To_Other_View);
    menu.addSeparator();
    menu.addAction(ui->actionFile_Properties);
    if (editor ) {
        ui->actionLocate_in_Files_View->setEnabled(!editor->isNew());
    }
    menu.exec(tabBar->mapToGlobal(pos));
}

void MainWindow::disableDebugActions()
{
    ui->actionInterrupt->setVisible(mDebugger && mDebugger->useDebugServer());
    ui->actionInterrupt->setEnabled(false);
    ui->actionStep_Into->setEnabled(false);
    ui->actionStep_Over->setEnabled(false);
    ui->actionStep_Out->setEnabled(false);
    ui->actionRun_To_Cursor->setEnabled(false);
    ui->actionContinue->setEnabled(false);
    ui->actionAdd_Watchpoint->setEnabled(false);
    ui->cbEvaluate->setEnabled(false);
    ui->cbMemoryAddress->setEnabled(false);
    if (mCPUDialog) {
        mCPUDialog->updateButtonStates(false);
    }
}

void MainWindow::enableDebugActions()
{
    ui->actionInterrupt->setVisible(mDebugger->useDebugServer());
    ui->actionInterrupt->setEnabled(mDebugger->useDebugServer() && mDebugger->inferiorRunning());
    ui->actionStep_Into->setEnabled(!mDebugger->inferiorRunning());
    ui->actionStep_Over->setEnabled(!mDebugger->inferiorRunning());
    ui->actionStep_Out->setEnabled(!mDebugger->inferiorRunning());
    ui->actionRun_To_Cursor->setEnabled(!mDebugger->inferiorRunning());
    ui->actionContinue->setEnabled(!mDebugger->inferiorRunning());
    ui->actionAdd_Watchpoint->setEnabled(!mDebugger->inferiorRunning());
    ui->cbEvaluate->setEnabled(!mDebugger->inferiorRunning());
    ui->cbMemoryAddress->setEnabled(!mDebugger->inferiorRunning());
    if (mCPUDialog) {
        mCPUDialog->updateButtonStates(true);
    }
}

void MainWindow::stopDebugForNoSymbolTable()
{
    mDebugger->stop();
    if (QMessageBox::question(this,
                              tr("Correct compile settings for debug"),
                              tr("The executable doesn't have symbol table, and can't be debugged.")
                              +"<BR /><BR />"
                              +tr("If you are using the Release compiler set, please use choose the Debug version from toolbar.")
                              +"<BR /><BR />"
                              +tr("Or you can manually change the following settings in the options dialog's compiler set page:")
                              +"<BR />"
                              +tr(" - Turned on the \"Generate debug info (-g3)\" option.")
                              +"<BR />"
                              +tr(" - Turned off the \"Strip executable (-s)\" option.")
                              +"<BR />"
                              +tr(" - Turned off the \"Optimization level (-O)\" option or set it to \"Debug (-Og)\".")
                              +"<BR /><BR />"
                              +tr("You should recompile after change the compiler set or it's settings.")
                              +"<BR /><BR />"
                              +tr("You should recompile after change the compiler set or it's settings.")
                              +"<BR /><BR />"
                              +tr("Do you want to mannually change the compiler set settings now?")
                          )== QMessageBox::Yes) {
        on_actionCompiler_Options_triggered();
//        Editor * editor = mEditorList->getEditor();
//        if (!mProject || (editor && !editor->inProject())) {
//            changeOptions(
//                       SettingsDialog::tr("Compiler Set"),
//                       SettingsDialog::tr("Compiler")
//                       );
//        } else {
//            changeProjectOptions(
//                       SettingsDialog::tr("Compiler Set"),
//                       SettingsDialog::tr("Project")
//                       );
//        }
    }
}

void MainWindow::onTodoParsingFile(const QString& filename)
{
    mTodoModel.removeTodosForFile(filename);
}

void MainWindow::onTodoParseStarted()
{
    mTodoModel.clear();
}

void MainWindow::onTodoFound(const QString& filename, int lineNo, int ch, const QString& line)
{
    mTodoModel.addItem(filename,lineNo,ch,line);
}

void MainWindow::onTodoParseFinished()
{
}

void MainWindow::onWatchpointHitted(const QString &var, const QString &oldVal, const QString &newVal)
{
    QMessageBox::information(this,
                             tr("Watchpoint hitted"),
                             tr("Value of \"%1\" has changed:").arg(var)
                             +"<br />"
                             +tr("Old value: %1").arg(oldVal)
                             +"<br />"
                             +tr("New value: %1").arg(newVal));
}

void MainWindow::prepareProjectForCompile()
{
    if (!mProject)
        return;
    if (!mProject->saveUnits())
        return;
    // Check if saves have been succesful
    for (int i=0; i<mEditorList->pageCount();i++) {
        Editor * e= (*(mEditorList))[i];
        if (e->inProject() && e->modified())
            return;
    }
    mProject->buildPrivateResource();
}

void MainWindow::closeProject(bool refreshEditor)
{
    // Stop executing program
    on_actionStop_Execution_triggered();

    // Only update file monitor once (and ignore updates)
    bool oldBlock= mFileSystemWatcher.blockSignals(true);
    {
        auto action = finally([&,this]{
            mFileSystemWatcher.blockSignals(oldBlock);
        });

        //save all files

        // TODO: should we save watches?
        if (fileExists(mProject->directory())){
            if (mEditorList->projectEditorsModified()) {
                QString s;
                if (mProject->name().isEmpty()) {
                    s = mProject->filename();
                } else {
                    s = mProject->name();
                }
                if (mSystemTurnedOff) {
                    mProject->saveAll();
                    mEditorList->saveAllForProject();
                } else {
                    int answer = QMessageBox::question(
                                this,
                                tr("Save project"),
                                tr("The project '%1' has modifications.").arg(s)
                                + "<br />"
                                + tr("Do you want to save it?"),
                                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                QMessageBox::Yes);
                    switch (answer) {
                    case QMessageBox::Yes:
                        mProject->saveAll();
                        mEditorList->saveAllForProject();
                        break;
                    case QMessageBox::No:
                        mEditorList->clearProjectEditorsModified();
                        mProject->setModified(false);
                        mProject->saveLayout();
                        break;
                    case QMessageBox::Cancel:
                        mProject->saveLayout();
                        return;
                    }
                }
            } else
                mProject->saveAll(); // always save layout, but not when SaveAll has been called
        }

        mClosingProject=true;

        if (fileExists(mProject->directory())){
            mBookmarkModel->saveProjectBookmarks(
                        changeFileExt(mProject->filename(), PROJECT_BOOKMARKS_EXT),
                        mProject->directory());

            mDebugger->saveForProject(
                        changeFileExt(mProject->filename(), PROJECT_DEBUG_EXT),
                        mProject->directory());
        }

        mClassBrowserModel.beginUpdate();
        // Remember it
        mVisitHistoryManager->addProject(mProject->filename());

        mEditorList->beginUpdate();
        mProject.reset();

        if (!mQuitting && refreshEditor) {
            //reset Class browsing
            ui->tabExplorer->setCurrentWidget(ui->tabStructure);
            Editor * e = mEditorList->getEditor();
            updateClassBrowserForEditor(e);
        } else {
            mClassBrowserModel.setParser(nullptr);
            mClassBrowserModel.setCurrentFile("");
        }
        mEditorList->endUpdate();
        mClassBrowserModel.endUpdate();

        if (!mQuitting) {
            mBookmarkModel->clear(true);
            mBookmarkModel->setIsForProject(false);
            mDebugger->clearForProject();
            mDebugger->setIsForProject(false);
            mTodoModel.clear(true);
            mTodoModel.setIsForProject(false);
            // Clear error browser
            clearIssues();
        }
        updateProjectView();
        mClosingProject=false;
    }
    if (refreshEditor)
        on_cbSearchHistory_currentIndexChanged(ui->cbSearchHistory->currentIndex());
}

void MainWindow::updateProjectView()
{
    if (mProject) {
        if (mProjectProxyModel->sourceModel()!=mProject->model()) {
            mProjectProxyModel->setSourceModel(mProject->model());
            mProjectProxyModel->sort(0);
            connect(mProject.get(), &Project::nodeRenamed,
                    this, &MainWindow::onProjectViewNodeRenamed);
//                    this, &MainWindow::invalidateProjectProxyModel);
//            connect(mProject->model(), &ProjectModel::rowsInserted,
//                    this, &MainWindow::invalidateProjectProxyModel);
//            connect(mProject->model(), &QAbstractItemModel::modelReset,
//                    ui->projectView,&QTreeView::expandAll);
        } else
            mProjectProxyModel->invalidate();
        //ui->projectView->expandAll();
        stretchExplorerPanel(true);
        ui->tabProject->setVisible(true);
        //ui->tabExplorer->setCurrentWidget(ui->tabProject);
    } else {
        // Clear project browser
        mProjectProxyModel->setSourceModel(nullptr);
        ui->tabProject->setVisible(false);
    }
    updateProjectActions();
}

void MainWindow::onFileChanged(const QString &path)
{
    if (mFilesChangedNotifying.contains(path))
        return;
    mFilesChangedNotifying.insert(path);
    Editor *e = mEditorList->getOpenedEditorByFilename(path);
    if (e) {
        if (fileExists(path)) {
            e->activate();
            if (QMessageBox::question(this,tr("File Changed"),
                                      tr("File '%1' was changed.").arg(path)+"<BR /><BR />" + tr("Reload its content from disk?"),
                                      QMessageBox::Yes|QMessageBox::No,
                                      QMessageBox::No) == QMessageBox::Yes) {
                try {
                    int top = e->topPos();
                    QSynedit::BufferCoord caretPos = e->caretXY();
                    e->loadFile();
                    e->setCaretPositionAndActivate(caretPos.line,1);
                    e->setTopPos(top);
                } catch(FileError e) {
                    QMessageBox::critical(this,tr("Error"),e.reason());
                }
            } else {
                e->setModified(true);
            }
        } else {
            mFileSystemWatcher.removePath(path);
            if (QMessageBox::question(this,tr("File Changed"),
                                      tr("File '%1' was removed.").arg(path)+"<BR /><BR />" + tr("Keep it open?"),
                                      QMessageBox::Yes|QMessageBox::No,
                                      QMessageBox::Yes) == QMessageBox::No) {
                mEditorList->closeEditor(e);
            } else {
                e->setModified(true);
            }
        }
    }
    mFilesChangedNotifying.remove(path);
}

void MainWindow::onDirChanged(const QString &path)
{
    if (mFilesChangedNotifying.contains(path))
        return;
    mFilesChangedNotifying.insert(path);
    if (mProject && QString::compare(mProject->directory(),path,PATH_SENSITIVITY)==0
            && !fileExists(path)) {
        QMessageBox::information(this,tr("Project folder removed."),
                                  tr("Folder for project '%1' was removed.").arg(path)
                                 +"<BR /><BR />"
                                 + tr("It will be closed."));
        closeProject(false);
    }
    mFilesChangedNotifying.remove(path);
}

void MainWindow::onFilesViewPathChanged()
{
    QString filesPath = ui->cbFilesPath->currentText();
    QFileInfo fileInfo(filesPath);
    ui->cbFilesPath->blockSignals(true);
    if (fileInfo.exists() && fileInfo.isDir()) {
        setFilesViewRoot(filesPath);
    } else {
        ui->cbFilesPath->setCurrentText(pSettings->environment().currentFolder());
    }
    ui->cbFilesPath->blockSignals(false);
}

const std::shared_ptr<HeaderCompletionPopup> &MainWindow::headerCompletionPopup() const
{
    return mHeaderCompletionPopup;
}

const std::shared_ptr<FunctionTooltipWidget> &MainWindow::functionTip() const
{
    return mFunctionTip;
}

const std::shared_ptr<CodeCompletionPopup> &MainWindow::completionPopup() const
{
    return mCompletionPopup;
}

SearchInFileDialog *MainWindow::searchInFilesDialog() const
{
    return mSearchInFilesDialog;
}

SearchResultModel *MainWindow::searchResultModel()
{
    return &mSearchResultModel;
}

EditorList *MainWindow::editorList() const
{
    return mEditorList;
}

Debugger *MainWindow::debugger() const
{
    return mDebugger.get();
}

CPUDialog *MainWindow::cpuDialog() const
{
    return mCPUDialog;
}


void MainWindow::on_actionNew_triggered()
{
    if (mProject) {
        if (QMessageBox::question(this,
                                  tr("New Project File?"),
                                  tr("Do you want to add the new file to the project?"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes) == QMessageBox::Yes) {
            if (mProject->options().isCpp)
                newProjectUnitFile("cpp");
            else
                newProjectUnitFile("c");
            return;
        }
    }
    newEditor();
}

void MainWindow::on_EditorTabsLeft_tabCloseRequested(int index)
{
    Editor* editor = mEditorList->getEditor(index,ui->EditorTabsLeft);
    mEditorList->closeEditor(editor);
}

void MainWindow::on_EditorTabsRight_tabCloseRequested(int index)
{
    Editor* editor = mEditorList->getEditor(index,ui->EditorTabsRight);
    mEditorList->closeEditor(editor);
}

void MainWindow::onFileSystemModelLayoutChanged()
{
    ui->treeFiles->scrollTo(ui->treeFiles->currentIndex(),QTreeView::PositionAtCenter);
}

void MainWindow::onFileRenamedInFileSystemModel(const QString &path, const QString &oldName, const QString &newName)
{
    QDir folder(path);
    QString oldFile = folder.absoluteFilePath(oldName);
    QString newFile = folder.absoluteFilePath(newName);

    Editor *e = mEditorList->getOpenedEditorByFilename(oldFile);
    if (e) {
        e->setFilename(newFile);
    }
}

void MainWindow::on_actionOpen_triggered()
{
    try {
        QString selectedFileFilter;
        selectedFileFilter = pSystemConsts->defaultAllFileFilter();
        QStringList files = QFileDialog::getOpenFileNames(pMainWindow,
            tr("Open"), QString(), pSystemConsts->defaultFileFilters().join(";;"),
            &selectedFileFilter);
        if (!files.isEmpty()) {
            QDir::setCurrent(extractFileDir(files[0]));
        }
        openFiles(files);
    }  catch (FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    mQuitting = true;
    if (!mShouldRemoveAllSettings) {
        if (mCPUDialog)
            mCPUDialog->close();
        Settings::UI& settings = pSettings->ui();
        settings.setOpenEditorsWhenReplace(ui->chkOpenFileInEditors->isChecked());
        settings.setMainWindowState(saveState());
        settings.setMainWindowGeometry(saveGeometry());
        settings.setBottomPanelIndex(ui->tabMessages->currentIndex());
        settings.setLeftPanelIndex(ui->tabExplorer->currentIndex());
        settings.setDebugPanelIndex(ui->debugViews->currentIndex());

        settings.setShowStatusBar(ui->actionStatus_Bar->isChecked());
        settings.setShowToolWindowBars(ui->actionTool_Window_Bars->isChecked());

        settings.setShowProject(ui->actionProject->isChecked());
        settings.setShowWatch(ui->actionWatch->isChecked());
        settings.setShowStructure(ui->actionStructure->isChecked());
        settings.setShowFiles(ui->actionFiles->isChecked());
        settings.setShowProblemSet(ui->actionProblem_Set->isChecked());

        settings.setShowIssues(ui->actionIssues->isChecked());
        settings.setShowCompileLog(ui->actionTools_Output->isChecked());
        settings.setShowDebug(ui->actionDebug_Window->isChecked());
        settings.setShowSearch(ui->actionSearch->isChecked());
        settings.setShowTODO(ui->actionTODO->isChecked());
        settings.setShowBookmark(ui->actionBookmark->isChecked());
        settings.setShowProblem(ui->actionProblem->isChecked());

        settings.setMessagesTabsSize(ui->tabMessages->currentSize());
        settings.setExplorerTabsSize(ui->tabExplorer->currentSize());
        settings.setShrinkExplorerTabs(ui->tabExplorer->isShrinked());
        settings.setShrinkMessagesTabs(ui->tabMessages->isShrinked());
        settings.save();

        if (pSettings->sync()!=QSettings::NoError) {
            QMessageBox::warning(nullptr,
                             tr("Save Error"),
                             tr("Save settings failed!"));
        }

        //save current folder ( for files view )
        pSettings->environment().setDefaultOpenFolder(QDir::currentPath());
        pSettings->environment().save();

        try {
            mBookmarkModel->saveBookmarks(includeTrailingPathDelimiter(pSettings->dirs().config())
                             +DEV_BOOKMARK_FILE);
        } catch (FileError& e) {
            QMessageBox::warning(nullptr,
                             tr("Save Error"),
                             e.reason());
        }

        try {
            int currentIndex=-1;
            if (ui->lstProblemSet->currentIndex().isValid())
                currentIndex = ui->lstProblemSet->currentIndex().row();
            mOJProblemSetModel.save(currentIndex);
        } catch (FileError& e) {
            QMessageBox::warning(nullptr,
                             tr("Save Error"),
                             e.reason());
        }

        if (pSettings->debugger().autosave()) {
            try {
                mDebugger->saveForNonproject(includeTrailingPathDelimiter(pSettings->dirs().config())
                                               +DEV_DEBUGGER_FILE);
            } catch (FileError& e) {
                QMessageBox::warning(nullptr,
                                 tr("Save Error"),
                                 e.reason());
            }
        } else
            removeFile(includeTrailingPathDelimiter(pSettings->dirs().config())
                          +DEV_DEBUGGER_FILE);
    }

    if (!mShouldRemoveAllSettings && pSettings->editor().autoLoadLastFiles()) {
        if (!saveLastOpens()) { //canceled
            mClosingAll=false;
            mQuitting = false;
            event->ignore();
            return ;
        }
    } /*else {
        //if don't save last open files, close project before editors, to save project opened editors;

    }*/
    if (mProject) {
        closeProject(false);
    }

    mClosingAll=true;
    if (!mEditorList->closeAll(false)) {
        mClosingAll=false;
        mQuitting = false;
        event->ignore();
        return ;
    }
    mClosingAll=false;

//    if (!mShouldRemoveAllSettings && pSettings->editor().autoLoadLastFiles()) {
//        if (mProject) {
//            closeProject(false);
//        }
//    }

    mCCHandler.stop();
    mCompilerManager->stopAllRunners();
    mCompilerManager->stopCompile();
    mCompilerManager->stopRun();
    if (!mShouldRemoveAllSettings)
        mSymbolUsageManager->save();

    if (mCPUDialog!=nullptr)
        cleanUpCPUDialog();
    event->accept();
    return;
}

void MainWindow::showEvent(QShowEvent *)
{
    if (mFullInitialized)
        return;
    mFullInitialized = true;
    applySettings();
    const Settings::UI& settings = pSettings->ui();
    ui->tabMessages->setCurrentIndex(settings.bottomPanelIndex());
    ui->tabExplorer->setCurrentIndex(settings.leftPanelIndex());
    ui->debugViews->setCurrentIndex(settings.debugPanelIndex());
}

void MainWindow::hideEvent(QHideEvent *)
{
//    Settings::UI& settings = pSettings->ui();
//    settings.setBottomPanelIndex(ui->tabMessages->currentIndex());
//    settings.setLeftPanelIndex(ui->tabExplorer->currentIndex());
}

bool MainWindow::event(QEvent *event)
{
    if (event->type()==DPI_CHANGED_EVENT) {
        applySettings();
        event->accept();
        return true;
    }
    return QMainWindow::event(event);
}

//void MainWindow::dragEnterEvent(QDragEnterEvent *event)
//{
//    if (event->mimeData()->hasUrls()){
//        event->acceptProposedAction();
//    }
//}

//void MainWindow::dropEvent(QDropEvent *event)
//{
//    if (event->mimeData()->hasUrls()) {
//        foreach(const QUrl& url, event->mimeData()->urls()){
//            if (!url.isLocalFile())
//                continue;
//            QString file = url.toLocalFile();
//            if (getFileType(file)==FileType::Project) {
//                openProject(file);
//                return;
//            }
//        }
//        foreach(const QUrl& url, event->mimeData()->urls()){
//            if (!url.isLocalFile())
//                continue;
//            QString file = url.toLocalFile();
//            openFile(file);
//        }
//    }
//}

void MainWindow::on_actionSave_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        if (editor->inProject() && mCompileIssuesState == CompileIssuesState::ProjectCompilationResultFilled)
            mCompileIssuesState = CompileIssuesState::None;
        editor->save();
//            if (editor->inProject() && (mProject))
//                mProject->saveAll();
    }    
}

void MainWindow::on_actionSaveAs_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->saveAs();
    }
}

void MainWindow::on_actionOptions_triggered()
{
    changeOptions();
}

void MainWindow::onCompilerSetChanged(int index)
{
    if (index<0)
        return;
    Editor *e = mEditorList->getEditor();
    updateCompileActions();
    if ( mProject && (!e || e->inProject())
         ) {
        if (index==mProject->options().compilerSet)
            return;
        if(QMessageBox::warning(
                    e,
                    tr("Change Project Compiler Set"),
                    tr("Change the project's compiler set will lose all custom compiler set options.")
                    +"<br />"
                    + tr("Do you really want to do that?"),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No) != QMessageBox::Yes) {
            mCompilerSet->setCurrentIndex(mProject->options().compilerSet);
            //ui->actionRebuild->trigger();
            return;
        }
        mProject->setCompilerSet(index);
        mProject->saveOptions();
        scanActiveProject(true);
        return;
    }

    pSettings->compilerSets().setDefaultIndex(index);
    pSettings->compilerSets().saveDefaultIndex();

    reparseNonProjectEditors();
}

void MainWindow::logToolsOutput(const QString& msg)
{
    ui->txtToolsOutput->appendPlainText(msg);
    ui->txtToolsOutput->moveCursor(QTextCursor::End);
    ui->txtToolsOutput->moveCursor(QTextCursor::StartOfLine);
    ui->txtToolsOutput->ensureCursorVisible();
}

void MainWindow::onCompileIssue(PCompileIssue issue)
{
    if (issue->filename.isEmpty())
        return;
    if (issue->filename.contains("*"))
        return;
    ui->tableIssues->addIssue(issue);

    if (issue->type == CompileIssueType::Error || issue->type ==
            CompileIssueType::Warning) {
        Editor* e = mEditorList->getOpenedEditorByFilename(issue->filename);
        if (e!=nullptr && (issue->line>0)) {
            int line = issue->line;
            if (line > e->lineCount())
                return;
            int col = std::min(issue->column,e->lineText(line).length()+1);
            if (col < 1)
                col = e->lineText(line).length()+1;
            e->addSyntaxIssues(line,col,issue->endColumn,issue->type,issue->description);
        }
    }
}

void MainWindow::clearToolsOutput()
{
    ui->txtToolsOutput->clear();
}

void MainWindow::clearTodos()
{
    mTodoModel.clear();
}

void MainWindow::onCompileStarted()
{
    mCompileIssuesState = CompileIssuesState::Compiling;
}

void MainWindow::onProjectCompileStarted()
{
    mCompileIssuesState = CompileIssuesState::ProjectCompiling;
}

void MainWindow::onSyntaxCheckStarted()
{
    mCompileIssuesState = CompileIssuesState::SyntaxChecking;
}

void MainWindow::onCompileFinished(QString filename, bool isCheckSyntax)
{
    if (mQuitting) {
        if (isCheckSyntax)
            mCheckSyntaxInBack = false;
        else
            mCompileSuccessionTask = nullptr;
        return;
    }

    // Update tab caption
    int i = ui->tabMessages->indexOf(ui->tabIssues);
    if (i!=-1) {
        if (isCheckSyntax) {
            if (mCompilerManager->syntaxCheckIssueCount()>0) {
                ui->tabMessages->setTabText(i, tr("Issues") +
                                    QString(" (%1)").arg(mCompilerManager->syntaxCheckIssueCount()));
            } else {
                ui->tabMessages->setTabText(i, tr("Issues"));
            }
        } else {
            if (mCompilerManager->compileIssueCount()>0) {
                ui->tabMessages->setTabText(i, tr("Issues") +
                                    QString(" (%1)").arg(mCompilerManager->compileIssueCount()));
            } else {
                ui->tabMessages->setTabText(i, tr("Issues"));
            }
        }
    }

    switch(mCompileIssuesState) {
    case CompileIssuesState::Compiling:
        mCompileIssuesState = CompileIssuesState::CompilationResultFilled;
        break;
    case CompileIssuesState::ProjectCompiling:
        mCompileIssuesState = CompileIssuesState::ProjectCompilationResultFilled;
        break;
    case CompileIssuesState::SyntaxChecking:
        mCompileIssuesState = CompileIssuesState::SyntaxCheckResultFilled;
        break;
    default:
        break;
    }


    if (isCheckSyntax) {
        if (!CompilerInfoManager::supportSyntaxCheck(pSettings->compilerSets().defaultSet()->compilerType())) {
            QDir dir(extractFileDir(filename));
#ifdef Q_OS_WIN
            QFile::remove(dir.absoluteFilePath("a.exe"));
#else
            QFile::remove(dir.absoluteFilePath("a.out"));
#endif
        }

      // check syntax in back, don't change message panel
    } else if (ui->tableIssues->count() == 0) {
        // Close it if there's nothing to show
        if (ui->tabMessages->currentIndex() == i)
            stretchMessagesPanel(false);
    } else {
        if (ui->tabMessages->currentIndex() != i) {
            ui->tabMessages->setCurrentIndex(i);
        }
        stretchMessagesPanel(true);
    }

    Editor * e = mEditorList->getEditor();
    if (e!=nullptr) {
        e->invalidate();
    }

    if (!isCheckSyntax) {
        //run succession task if there aren't any errors
        if (mCompileSuccessionTask && mCompilerManager->compileErrorCount()==0) {
            if (!fileExists(mCompileSuccessionTask->execName)) {
                QMessageBox::critical(this,tr("Compile Failed"),
                                      tr("Failed to generate the executable.")+"<BR/><BR/>"
                                      +tr("Please check detail info in \"Tools Output\" panel."));

                return;
            }
            if (!mCompileSuccessionTask->isExecutable) {
                Editor * editor;
                switch (mCompileSuccessionTask->type) {
                case MainWindow::CompileSuccessionTaskType::RunNormal:
                    editor = openFile(mCompileSuccessionTask->execName);
                    if (e && editor) {
                        int line = e->caretY();
                        int startLine = 1;
                        QString s = "# "+e->filename()+":";
                        for(int i=1;i<=editor->lineCount();i++) {
                            QString t=editor->lineText(i).trimmed();
                            if (t.startsWith(s,PATH_SENSITIVITY)) {
                                t=t.mid(s.length());
                                int pos = t.indexOf(":");
                                if (pos>0) {
                                    QString numstring=t.mid(0,pos);
                                    bool isOk;
                                    int l=numstring.toInt(&isOk);
                                    if (isOk) {
                                        if (l<=line)
                                            startLine=i;
                                        if (l>=line)
                                            break;
                                    }
                                }
                            }
                        }
                        editor->setCaretPositionAndActivate(startLine,1);
                    }
                    break;
                case MainWindow::CompileSuccessionTaskType::RunProblemCases:
                case MainWindow::CompileSuccessionTaskType::RunCurrentProblemCase:
                    QMessageBox::critical(this,tr("Wrong Compiler Settings"),
                                          tr("Compiler is set not to generate executable.")+"<BR/><BR/>"
                                          +tr("We need the executabe to run problem case."));
                    break;
                case MainWindow::CompileSuccessionTaskType::Debug:
                    QMessageBox::critical(
                                this,
                                tr("Wrong Compiler Settings"),
                                tr("Compiler is set not to generate executable.")+"<BR /><BR />"
                                +tr("Please correct this before start debugging"));
                    compile();
                    return;
                    break;
                default:
                    break;
                }
            } else {
                switch (mCompileSuccessionTask->type) {
                case MainWindow::CompileSuccessionTaskType::RunNormal:
                    runExecutable(mCompileSuccessionTask->execName,QString(),RunType::Normal, mCompileSuccessionTask->binDirs);
                    break;
                case MainWindow::CompileSuccessionTaskType::RunProblemCases:
                    runExecutable(mCompileSuccessionTask->execName,QString(),RunType::ProblemCases, mCompileSuccessionTask->binDirs);
                    break;
                case MainWindow::CompileSuccessionTaskType::RunCurrentProblemCase:
                    runExecutable(mCompileSuccessionTask->execName,QString(),RunType::CurrentProblemCase, mCompileSuccessionTask->binDirs);
                    break;
                case MainWindow::CompileSuccessionTaskType::Debug:
                    debug();
                    break;
                default:
                    break;
                }
            }
            mCompileSuccessionTask.reset();
            // Jump to problem location, sorted by significance
        } else if ((mCompilerManager->compileIssueCount() > 0) && (!mCheckSyntaxInBack)) {
            bool hasError = false;
            // First try to find errors
            for (int i=0;i<ui->tableIssues->count();i++) {
                PCompileIssue issue = ui->tableIssues->issue(i);
                if (issue->type == CompileIssueType::Error) {
                    ui->tableIssues->selectRow(i);
                    hasError = true;
                    break;
                }
            }
            if (!hasError) {
                // Then try to find warnings
                for (int i=0;i<ui->tableIssues->count();i++) {
                    PCompileIssue issue = ui->tableIssues->issue(i);
                    if (issue->type == CompileIssueType::Warning) {
                        ui->tableIssues->selectRow(i);
                        break;
                    }
                }
            }
        }
    } else {
        mCheckSyntaxInBack=false;
    }
    updateCompileActions();
    updateAppTitle();
}

void MainWindow::onCompileErrorOccured(const QString& reason)
{
    QMessageBox::critical(this,tr("Compile Failed"),reason);
}

void MainWindow::onRunErrorOccured(const QString& reason)
{
    mCompilerManager->stopRun();
    QMessageBox::critical(this,tr("Run Failed"),reason);
}

void MainWindow::onRunFinished()
{
    updateCompileActions();
    if (pSettings->executor().minimizeOnRun()) {
        showNormal();
    }
    updateAppTitle();
}

void MainWindow::onRunPausingForFinish()
{
    updateCompileActions();
}

void MainWindow::onRunProblemFinished()
{
    updateProblemTitle();
    ui->pbProblemCases->setVisible(false);
    updateCompileActions();
    updateAppTitle();
}

void MainWindow::onOJProblemCaseStarted(const QString& id,int current, int total)
{
    ui->pbProblemCases->setVisible(true);
    ui->pbProblemCases->setMaximum(total);
    ui->pbProblemCases->setValue(current);
    int row = mOJProblemModel.getCaseIndexById(id);
    if (row>=0) {
        POJProblemCase problemCase = mOJProblemModel.getCase(row);
        problemCase->testState = ProblemCaseTestState::Testing;
        mOJProblemModel.update(row);
        QModelIndex idx = ui->tblProblemCases->currentIndex();
        if (!idx.isValid() || row != idx.row()) {
            ui->tblProblemCases->setCurrentIndex(mOJProblemModel.index(row,0));
        }
        ui->txtProblemCaseOutput->clearAll();
        if (ui->txtProblemCaseExpected->document()->blockCount()<=5000) {
            ui->txtProblemCaseExpected->clearFormat();
        }
    }
}

void MainWindow::onOJProblemCaseFinished(const QString& id, int current, int total)
{
    int row = mOJProblemModel.getCaseIndexById(id);
    if (row>=0) {
        POJProblemCase problemCase = mOJProblemModel.getCase(row);
        ProblemCaseValidator validator;
        problemCase->testState = validator.validate(problemCase,pSettings->executor().problemCaseValidateType())?
                    ProblemCaseTestState::Passed:
                    ProblemCaseTestState::Failed;
        mOJProblemModel.update(row);
        updateProblemCaseOutput(problemCase);
    }
    ui->pbProblemCases->setMaximum(total);
    ui->pbProblemCases->setValue(current);
    updateProblemTitle();
}

void MainWindow::onOJProblemCaseNewOutputGetted(const QString &/* id */, const QString &line)
{
    ui->txtProblemCaseOutput->appendPlainText(line);
}

void MainWindow::onOJProblemCaseResetOutput(const QString &/* id */, const QString &line)
{
    ui->txtProblemCaseOutput->clearAll();
    ui->txtProblemCaseOutput->setPlainText(line);
}

void MainWindow::cleanUpCPUDialog()
{
    disconnect(mCPUDialog,&CPUDialog::closed,
               this,&MainWindow::cleanUpCPUDialog);
    CPUDialog* ptr=mCPUDialog;
    mCPUDialog=nullptr;
    ptr->deleteLater();
}

void MainWindow::onDebugCommandInput(const QString& command)
{
    if (mDebugger->executing()) {
        mDebugger->runClientCommand(command,"", DebugCommandSource::Console);
    }
}

void MainWindow::on_actionCompile_triggered()
{
    mCompileSuccessionTask.reset();
    compile();
}

void MainWindow::on_actionRun_triggered()
{
    runExecutable();
}

void MainWindow::on_actionUndo_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->undo();
    }
}

void MainWindow::on_actionRedo_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->redo();
    }
}

void MainWindow::on_actionCut_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->cutToClipboard();
    }
}

void MainWindow::on_actionSelectAll_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->selectAll();
    }
}

void MainWindow::on_actionCopy_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->copyToClipboard();
    }
}

void MainWindow::on_actionPaste_triggered()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* data = clipboard->mimeData();
    if (!data)
        return;
    if (data->hasText()) {
        Editor * editor = mEditorList->getEditor();
        if (editor) {
            editor->pasteFromClipboard();
            editor->activate();
        }
    }
}

void MainWindow::on_actionIndent_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->tab();
    }
}

void MainWindow::on_actionUnIndent_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->shifttab();
    }
}

void MainWindow::on_actionToggleComment_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->toggleComment();
    }
}

void MainWindow::on_actionUnfoldAll_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->unCollpaseAll();
    }
}

void MainWindow::on_actionFoldAll_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->collapseAll();
    }
}

void MainWindow::on_tableIssues_doubleClicked(const QModelIndex &index)
{
    PCompileIssue issue = ui->tableIssues->issue(index);
    if (!issue)
        return;

    Editor * editor = openFile(issue->filename);
    if (editor == nullptr)
        return;

    editor->setCaretPositionAndActivate(issue->line,issue->column);
}

void MainWindow::on_actionEncode_in_ANSI_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    try {
        editor->setEncodingOption(ENCODING_SYSTEM_DEFAULT);
    } catch(FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
}

void MainWindow::on_actionEncode_in_UTF_8_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    try {
        editor->setEncodingOption(ENCODING_UTF8);
    } catch(FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
}

void MainWindow::on_actionAuto_Detect_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    editor->setEncodingOption(ENCODING_AUTO_DETECT);
}

void MainWindow::on_actionConvert_to_ANSI_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    if (QMessageBox::warning(this,tr("Confirm Convertion"),
                   tr("The editing file will be saved using %1 encoding. <br />This operation can't be reverted. <br />Are you sure to continue?")
                   .arg(QString(TextEncoder::encoderForSystem().name())),
                   QMessageBox::Yes, QMessageBox::No)!=QMessageBox::Yes)
        return;
    editor->convertToEncoding(ENCODING_SYSTEM_DEFAULT);

}

void MainWindow::on_actionConvert_to_UTF_8_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    if (QMessageBox::warning(this,tr("Confirm Convertion"),
                   tr("The editing file will be saved using %1 encoding. <br />This operation can't be reverted. <br />Are you sure to continue?")
                   .arg(ENCODING_UTF8),
                   QMessageBox::Yes, QMessageBox::No)!=QMessageBox::Yes)
        return;
    editor->convertToEncoding(ENCODING_UTF8);
}

void MainWindow::on_tabMessages_tabBarClicked(int index)
{
    if (index == ui->tabMessages->currentIndex() && !ui->tabMessages->isShrinked()) {
        stretchMessagesPanel(false);
    } else {
        stretchMessagesPanel(true);
    }
}

void MainWindow::on_actionRebuild_triggered()
{
    mCompileSuccessionTask.reset();
    compile(true);
}

void MainWindow::on_actionStop_Execution_triggered()
{
    mCompilerManager->stopRun();
    mDebugger->stop();
}

void MainWindow::on_actionDebug_triggered()
{
    debug();
}

CompileTarget MainWindow::getCompileTarget()
{
    // Check if the current file belongs to a project
    CompileTarget target = CompileTarget::None;
    Editor* e = mEditorList->getEditor();
    if (e!=nullptr) {
        // Treat makefiles as InProject files too
        if (mProject
                && (e->inProject() || (mProject->makeFileName() == e->filename()))
                ) {
            target = CompileTarget::Project;
        } else {
            target = CompileTarget::File;
        }
    // No editors have been opened. Check if a project is open
    } else if (mProject) {
        target = CompileTarget::Project;
    }
    return target;
}

bool MainWindow::debugInferiorhasBreakpoint()
{
    Editor * e = mEditorList->getEditor();
    if (e==nullptr)
        return false;
    for (const PBreakpoint& breakpoint:mDebugger->breakpointModel()->breakpoints(e->inProject())) {
        if (e->filename() == breakpoint->filename) {
            return true;
        }
    }
    return false;
}

void MainWindow::on_actionStep_Over_triggered()
{
    if (mDebugger->executing()) {
        //WatchView.Items.BeginUpdate();
        mDebugger->stepOver();
    }
}

void MainWindow::on_actionStep_Into_triggered()
{
    if (mDebugger->executing()) {
        //WatchView.Items.BeginUpdate();
        mDebugger->stepInto();
    }

}

void MainWindow::on_actionStep_Out_triggered()
{
    if (mDebugger->executing()) {
        //WatchView.Items.BeginUpdate();
        mDebugger->stepOut();
    }

}

void MainWindow::on_actionRun_To_Cursor_triggered()
{
    if (mDebugger->executing()) {
        Editor *e=mEditorList->getEditor();
        if (e!=nullptr) {
            //WatchView.Items.BeginUpdate();
            mDebugger->runTo(e->filename(), e->caretY());
        }
    }

}

void MainWindow::on_actionContinue_triggered()
{
    if (mDebugger->executing()) {
        //WatchView.Items.BeginUpdate();
        mDebugger->resume();
    }
}

void MainWindow::on_actionAdd_Watch_triggered()
{
    QString s = "";
    Editor *e = mEditorList->getEditor();
    if (e!=nullptr) {
        if (e->selAvail()) {
            s = e->selText();
        } else {
            s = e->wordAtCursor();
        }
    }
    bool isOk;
    s=QInputDialog::getText(this,
                              tr("New Watch Expression"),
                              tr("Enter Watch Expression (it is recommended to use 'this->' for class members):"),
                            QLineEdit::Normal,
                            s,&isOk);
    if (!isOk)
        return;
    s = s.trimmed();
    if (!s.isEmpty()) {
        mDebugger->addWatchVar(s);
    }
}

void MainWindow::on_actionView_CPU_Window_triggered()
{
    showCPUInfoDialog();
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::onDebugEvaluateInput()
{
    QString s=ui->cbEvaluate->currentText().trimmed();
    if (!s.isEmpty()) {
        connect(mDebugger.get(), &Debugger::evalValueReady,
                   this, &MainWindow::onEvalValueReady);
        mDebugger->evalExpression(s);
        pMainWindow->debugger()->refreshAll();
    }
}

void MainWindow::onDebugMemoryAddressInput()
{
    QString s=ui->cbMemoryAddress->currentText().trimmed();
    if (!s.isEmpty()) {
//        connect(mDebugger, &Debugger::memoryExamineReady,
//                   this, &MainWindow::onMemoryExamineReady);
        mDebugger->readMemory(s,
                              pSettings->debugger().memoryViewRows(),
                              pSettings->debugger().memoryViewColumns()
                               );
    }
}

void MainWindow::onParserProgress(const QString &fileName, int total, int current)
{
    // Mention every 5% progress
    int showStep = total / 20;

    // For Total = 1, avoid division by zero
    if (showStep == 0)
        showStep = 1;

    // Only show if needed (it's a very slow operation)
    if (current ==1 || current % showStep==0) {
        updateStatusbarMessage(tr("Parsing file %1 of %2: \"%3\"")
                                  .arg(current).arg(total).arg(fileName));
    }
}

void MainWindow::onStartParsing()
{
    mParserTimer.restart();
}

void MainWindow::onEndParsing(int total, int)
{
    double parseTime = mParserTimer.elapsed() / 1000.0;
    double parsingFrequency;


    if (total > 1) {
        if (parseTime>0) {
            parsingFrequency = total / parseTime;
        } else {
            parsingFrequency = 999;
        }
        updateStatusbarMessage(tr("Done parsing %1 files in %2 seconds")
                                  .arg(total).arg(parseTime)
                                  + " "
                                  + tr("(%1 files per second)")
                                  .arg(parsingFrequency));
    } else {
        updateStatusbarMessage(tr("Done parsing %1 files in %2 seconds")
                                  .arg(total).arg(parseTime));
    }
}

void MainWindow::onEvalValueReady(const QString& value)
{
    updateDebugEval(value);
    disconnect(mDebugger.get(), &Debugger::evalValueReady,
               this, &MainWindow::onEvalValueReady);
}

void MainWindow::onLocalsReady(const QStringList& value)
{
    ui->txtLocals->clear();
    foreach (QString s, value) {
        ui->txtLocals->appendPlainText(s);
    }
    ui->txtLocals->moveCursor(QTextCursor::Start);
}

void MainWindow::on_actionFind_triggered()
{
    Editor *e = mEditorList->getEditor();
    if (!e)
        return;
    hideAllSearchDialogs();
    prepareSearchDialog();
    if (e->selAvail())
        mSearchDialog->find(e->selText());
    else
        mSearchDialog->find(e->wordAtCursor());
}

void MainWindow::on_actionFind_in_files_triggered()
{
    hideAllSearchDialogs();
    prepareSearchInFilesDialog();
    Editor *e = mEditorList->getEditor();
    if (e) {
        if (e->selAvail())
            mSearchInFilesDialog->findInFiles(e->selText());
        else
            mSearchInFilesDialog->findInFiles(e->wordAtCursor());
    } else {
        mSearchInFilesDialog->findInFiles("");
    }
}

void MainWindow::on_actionReplace_triggered()
{
    Editor *e = mEditorList->getEditor();
    if (!e)
        return;

    hideAllSearchDialogs();
    prepareSearchDialog();
    if (e->selAvail())
        mSearchDialog->replace(e->selText());
    else
        mSearchDialog->replace(e->wordAtCursor());
}

void MainWindow::on_actionFind_Next_triggered()
{
    Editor *e = mEditorList->getEditor();
    if (e==nullptr)
        return;

    if (mSearchDialog==nullptr)
        return;

    mSearchDialog->findNext();
}

void MainWindow::on_actionFind_Previous_triggered()
{
    Editor *e = mEditorList->getEditor();
    if (e==nullptr)
        return;

    if (mSearchDialog==nullptr)
        return;

    mSearchDialog->findPrevious();
}

void MainWindow::on_cbSearchHistory_currentIndexChanged(int index)
{
    mSearchResultModel.setCurrentIndex(index);
    PSearchResults results = mSearchResultModel.results(index);
    if (results) {
        if (results->searchType==SearchType::Search
                && results->scope==SearchFileScope::wholeProject
                && pMainWindow->project()==nullptr)
            ui->btnSearchAgain->setEnabled(false);
        else
            ui->btnSearchAgain->setEnabled(true);
    } else {
        ui->btnSearchAgain->setEnabled(false);
    }
}

void MainWindow::on_btnSearchAgain_clicked()
{
    hideAllSearchDialogs();
    PSearchResults results=mSearchResultModel.currentResults();
    if (!results)
        return;
    if (results->searchType == SearchType::Search){
        if (results->scope==SearchFileScope::wholeProject
                && pMainWindow->project()==nullptr)
            return;
        mSearchInFilesDialog->findInFiles(
                    results->keyword,
                    results->scope,
                    results->options,
                    results->folder,
                    results->filters,
                    results->searchSubfolders);
    } else if (results->searchType == SearchType::FindOccurences) {
        CppRefacter refactor;
        refactor.findOccurence(results->statementFullname,results->scope);
    }
}

void MainWindow::on_actionRemove_Watch_triggered()
{
    QModelIndexList lst=ui->watchView->selectionModel()->selectedRows();
    if (lst.count()<=1) {
        QModelIndex index =ui->watchView->currentIndex();
        QModelIndex parent;
        while (true) {
            parent = ui->watchView->model()->parent(index);
            if (parent.isValid()) {
                index=parent;
            } else {
                break;
            }
        }
        mDebugger->removeWatchVar(index);
    } else {
        QModelIndexList filteredList;
        foreach(const QModelIndex& index,lst) {
            if (!index.parent().isValid())
                filteredList.append(index);
        };
        std::sort(filteredList.begin(),filteredList.end(), [](const QModelIndex& index1,
                  const QModelIndex& index2) {
            return index1.row()>index2.row();
        });
        foreach(const QModelIndex& index,filteredList) {
            mDebugger->removeWatchVar(index);
        };
    }
}


void MainWindow::on_actionRemove_All_Watches_triggered()
{
    mDebugger->removeWatchVars(true);
}


void MainWindow::on_actionModify_Watch_triggered()
{
    QModelIndexList lst=ui->watchView->selectionModel()->selectedRows();
    if (lst.count()<=1) {
        QModelIndex index =ui->watchView->currentIndex();
        QModelIndex parent;
        parent = ui->watchView->model()->parent(index);
        if (parent.isValid())
            return;
        PWatchVar var = mDebugger->watchVarAt(index);
        if (!var)
            return;
        bool isOk;
        QString newExpr = QInputDialog::getText(
                    this,tr("Modify Watch"),
                    tr("Watch Expression"),
                    QLineEdit::Normal,
                    var->expression,
                    &isOk);
        if (isOk) {
            mDebugger->modifyWatchVarExpression(var->expression,newExpr);
        }
    }
}


void MainWindow::on_actionReformat_Code_triggered()
{
    Editor* e = mEditorList->getEditor();
    if (e) {
        e->reformat();
        e->activate();
    }
}

CaretList &MainWindow::caretList()
{
    return mCaretList;
}

void MainWindow::updateCaretActions()
{
    ui->actionBack->setEnabled(mCaretList.hasPrevious());
    ui->actionForward->setEnabled(mCaretList.hasNext());
}


void MainWindow::on_actionBack_triggered()
{
    PEditorCaret caret = mCaretList.gotoAndGetPrevious();
    mCaretList.pause();
    if (caret) {
        caret->editor->setCaretPositionAndActivate(caret->line,caret->aChar);
    }
    mCaretList.unPause();
    updateCaretActions();
}


void MainWindow::on_actionForward_triggered()
{
    PEditorCaret caret = mCaretList.gotoAndGetNext();
    mCaretList.pause();
    if (caret) {
        caret->editor->setCaretPositionAndActivate(caret->line,caret->aChar);
    }
    mCaretList.unPause();
    updateCaretActions();
}


void MainWindow::on_tabExplorer_tabBarClicked(int index)
{
    if (index == ui->tabExplorer->currentIndex() && !ui->tabExplorer->isShrinked()) {
        stretchExplorerPanel(false);
    } else {
        stretchExplorerPanel(true);
    }
}


void MainWindow::on_EditorTabsLeft_tabBarDoubleClicked(int)
{
    maximizeEditor();
}

void MainWindow::on_EditorTabsRight_tabBarDoubleClicked(int)
{
    maximizeEditor();
}


void MainWindow::on_actionClose_triggered()
{
    mClosing = true;
    Editor* e = mEditorList->getEditor();
    if (e) {
        mEditorList->closeEditor(e);
    }
    mClosing = false;
}


void MainWindow::on_actionClose_All_triggered()
{
    mClosingAll=true;
    mClosing = true;
    mEditorList->closeAll(mSystemTurnedOff);
    mClosing = false;
    mClosingAll=false;
}


void MainWindow::on_actionMaximize_Editor_triggered()
{
    maximizeEditor();
}


void MainWindow::on_actionNext_Editor_triggered()
{
    mEditorList->selectNextPage();
}


void MainWindow::on_actionPrevious_Editor_triggered()
{
    mEditorList->selectPreviousPage();
}


void MainWindow::on_actionToggle_Breakpoint_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor)
        editor->toggleBreakpoint(editor->caretY());
}


void MainWindow::on_actionClear_all_breakpoints_triggered()
{
    Editor *e=mEditorList->getEditor();
    if (!e)
        return;
    if (QMessageBox::question(this,
                              tr("Clear all breakpoints"),
                              tr("Do you really want to clear all breakpoints in this file?"),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::Yes) {
        e->clearBreakpoints();
    }
}


void MainWindow::on_actionBreakpoint_property_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        int line = editor->caretY();
        if (editor->hasBreakpoint(line))
            editor->modifyBreakpointProperty(line);
    }
}


void MainWindow::on_actionGoto_Declaration_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->gotoDeclaration(editor->caretXY());
    }
}

void MainWindow::on_actionGoto_Definition_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->gotoDefinition(editor->caretXY());
    }
}


void MainWindow::on_actionFind_references_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        CppRefacter refactor;
        refactor.findOccurence(editor, editor->caretXY());
        showSearchPanel(false);
    }
}


void MainWindow::on_actionOpen_Containing_Folder_triggered()
{
    Editor* editor = mEditorList->getEditor();
    if (editor) {
        openFileFolderInExplorer(editor->filename());
//        QFileInfo info(editor->filename());
//        if (!info.path().isEmpty()) {
//            QDesktopServices::openUrl(
//                        QUrl("file:///"+
//                             includeTrailingPathDelimiter(info.path()),QUrl::TolerantMode));
//        }
    }
}


void MainWindow::on_actionOpen_Terminal_triggered()
{
    Editor* editor = mEditorList->getEditor();
    if (editor) {
        QFileInfo info(editor->filename());
        if (!info.path().isEmpty()) {
#ifdef Q_OS_WIN
            openShell(info.path(),"cmd.exe",getBinDirsForCurrentEditor());
#else
            openShell(info.path(),pSettings->environment().terminalPath(),getBinDirsForCurrentEditor());
#endif
        }
    }

}


void MainWindow::on_actionFile_Properties_triggered()
{
    Editor* editor = mEditorList->getEditor();
    if (editor) {
        FilePropertiesDialog dialog(editor,this);
        dialog.exec();
        dialog.setParent(nullptr);
    }
}

void MainWindow::on_searchView_doubleClicked(const QModelIndex &index)
{
    QString filename;
    int line;
    int start;
    if (mSearchResultTreeModel->getItemFileAndLineChar(
                index,filename,line,start)) {
        Editor *e = openFile(filename);
        if (e) {
            e->setCaretPositionAndActivate(line,start);
        }
    }
}


void MainWindow::on_tblStackTrace_doubleClicked(const QModelIndex &index)
{
    switchCurrentStackTrace(index.row());
}


void MainWindow::on_tblBreakpoints_doubleClicked(const QModelIndex &index)
{
    PBreakpoint breakpoint = mDebugger->breakpointModel()->breakpoint(
                index.row(),
                mDebugger->isForProject());
    if (breakpoint) {
        Editor * e = openFile(breakpoint->filename);
        if (e) {
            e->setCaretPositionAndActivate(breakpoint->line,1);
        }
    }
}

std::shared_ptr<Project> MainWindow::project()
{
    return mProject;
}


void MainWindow::on_projectView_doubleClicked(const QModelIndex &index)
{
    QModelIndex sourceIndex = mProjectProxyModel->mapToSource(index);
    if (!sourceIndex.isValid())
        return;
    ProjectModelNode * node = static_cast<ProjectModelNode*>(sourceIndex.internalPointer());
    if (!node)
        return;
    if (node->isUnit) {
        PProjectUnit unit = node->pUnit.lock();
        mProject->openUnit(unit);
    }
}


void MainWindow::on_actionClose_Project_triggered()
{
    mClosing = true;
    closeProject(true);
    mClosing = false;
}


void MainWindow::on_actionProject_options_triggered()
{
    if (!mProject)
        return;
//    int oldCompilerSet = mProject->options().compilerSet;
    QString oldName = mProject->name();
    PSettingsDialog dialog = SettingsDialog::projectOptionDialog();
    dialog->exec();
    updateCompilerSet();
//    if (oldCompilerSet != mProject->options().compilerSet)
//        ui->actionRebuild->trigger();
}


void MainWindow::on_actionNew_Project_triggered()
{
    NewProjectDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        if (dialog.makeDefaultLanguage()) {
            pSettings->editor().setDefaultFileCpp(dialog.isCppProject());
            pSettings->editor().save();
        }
        if (dialog.useAsDefaultProjectDir()) {
            pSettings->dirs().setProjectDir(dialog.getLocation());
            pSettings->dirs().save();
        }
        // Take care of the currently opened project
        QString s;
        if (mProject) {
            if (mProject->name().isEmpty())
                s = mProject->filename();
            else
                s = mProject->name();

            // Ask if the user wants to close the current one. If not, abort
            if (QMessageBox::question(this,
                                     tr("New project"),
                                     tr("Close %1 and start new project?").arg(s),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::Yes)==QMessageBox::Yes) {
                closeProject(false);
            } else
                return;
        }

        //Create the project folder
        QString location = includeTrailingPathDelimiter(dialog.getLocation())+dialog.getProjectName();
        QDir dir(location);
        if (!dir.exists()) {
            if (QMessageBox::question(this,
                                      tr("Folder not exist"),
                                      tr("Folder '%1' doesn't exist. Create it now?").arg(location),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::Yes) != QMessageBox::Yes) {
                return;
            }
            if (!dir.mkpath(location)) {
                QMessageBox::critical(this,
                                      tr("Can't create folder"),
                                      tr("Failed to create folder '%1'.").arg(location),
                                      QMessageBox::Yes);
                return;
            }
        }

        QDir projectDir = QDir(location);
        if (!projectDir.isEmpty()) {
            if (QMessageBox::question(
                        nullptr,
                        tr("Folder Not Empty"),
                        tr("The project folder is not empty, existing files may be overwritten.")
                        + "<br/><br/>"
                        +tr("Do you want to proceed?"),
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::No
                        ) == QMessageBox::No) {
                return;
            }
        }
        s = includeTrailingPathDelimiter(location)
                + dialog.getProjectName() + "." + DEV_PROJECT_EXT;

        if (fileExists(s)) {
            QString saveName = QFileDialog::getSaveFileName(
                        this,
                        tr("Save new project as"),
                        location,
                        tr("Red Panda C++ project file (*.dev)"));
            if (!saveName.isEmpty()) {
                s = saveName;
            }
        }

        mProject = Project::create(s,dialog.getProjectName(),
                                             mEditorList,
                                             &mFileSystemWatcher,
                                   dialog.getTemplate(),dialog.isCppProject());
        if (!mProject) {
            QMessageBox::critical(this,
                                  tr("New project fail"),
                                  tr("Can't assign project template"),
                                  QMessageBox::Ok);
            return;
        }
        mProject->saveAll();
        updateProjectView();
        Editor* editor = mEditorList->getEditor();
        updateClassBrowserForEditor(editor);
        if (editor) {
            PProjectUnit unit=mProject->findUnit(editor);
            if (unit) {
                QModelIndex index=mProject->model()->getNodeIndex(unit->node().get());
                index = mProjectProxyModel->mapFromSource(index);
                ui->projectView->expand(index);
                ui->projectView->setCurrentIndex(index);
            }
        }
        scanActiveProject(true);
        if (pSettings->editor().parseTodos())
            mTodoParser->parseFiles(mProject->unitFiles());
        if (pSettings->ui().showProject())
            ui->tabExplorer->setCurrentWidget(ui->tabProject);
        setupSlotsForProject();
    }
    pSettings->ui().setNewProjectDialogWidth(dialog.width());
    pSettings->ui().setNewProjectDialogHeight(dialog.height());
}


void MainWindow::on_actionSaveAll_triggered()
{
    // Pause the change notifier
    bool oldBlock = mFileSystemWatcher.blockSignals(true);
    auto action = finally([oldBlock,this] {
        mFileSystemWatcher.blockSignals(oldBlock);
    });
    if (mProject) {
        if (mCompileIssuesState == CompileIssuesState::ProjectCompilationResultFilled)
            mCompileIssuesState = CompileIssuesState::None;
        mProject->saveAll();
    }

    // Make changes to files
    mEditorList->saveAll();
    updateAppTitle();
}


void MainWindow::on_actionProject_New_File_triggered()
{
    newProjectUnitFile();
}


void MainWindow::on_actionAdd_to_project_triggered()
{
    if (!mProject)
        return;
    QFileDialog dialog(this,tr("Add to project"),
                       mProject->directory(),
                       pSystemConsts->defaultFileFilters().join(";;"));
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    if (dialog.exec()) {
        QModelIndex current = mProjectProxyModel->mapToSource(ui->projectView->currentIndex());
        ProjectModelNode * node = nullptr;
        if (current.isValid()) {
            node = static_cast<ProjectModelNode*>(current.internalPointer());
        }
        PProjectModelNode folderNode =  mProject->pointerToNode(node);
        foreach (const QString& filename, dialog.selectedFiles()) {
            PProjectUnit newUnit = mProject->addUnit(filename,folderNode);
            mProject->cppParser()->addProjectFile(filename,true);
#ifdef ENABLE_VCS
            QString branch;
            if (pSettings->vcs().gitOk() && mProject->model()->iconProvider()->VCSRepository()->hasRepository(branch)) {
                QString output;
                mProject->model()->iconProvider()->VCSRepository()->add(
                            extractRelativePath(mProject->folder(),filename),
                            output
                            );
            }
#endif
            if (newUnit) {
                QModelIndex index = mProject->model()->getNodeIndex(newUnit->node().get());
                index = mProjectProxyModel->mapFromSource(index);
                if (index.isValid()) {
                    ui->projectView->expand(index);
                    ui->projectView->setCurrentIndex(index);
                }
            }
        }
        mProject->saveAll();
        updateProjectView();
        parseFileList(mProject->cppParser());
    }
}


void MainWindow::on_actionRemove_from_project_triggered()
{
    if (!mProject)
        return;
    if (!ui->projectView->selectionModel()->hasSelection())
        return;

    bool removeFile = (QMessageBox::question(this,tr("Remove file"),
                              tr("Remove the file from disk?"),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes);

    QList<PProjectUnit> units;
    foreach (const QModelIndex& index, ui->projectView->selectionModel()->selectedIndexes()){
        if (!index.isValid())
            continue;
        QModelIndex realIndex = mProjectProxyModel->mapToSource(index);
        ProjectModelNode * node = static_cast<ProjectModelNode*>(realIndex.internalPointer());
        PProjectModelNode folderNode =  mProject->pointerToNode(node);
        if (!folderNode)
            continue;
        PProjectUnit unit = folderNode->pUnit.lock();
        units.append(unit);
    }
    for(PProjectUnit& unit: units) {
        mProject->removeUnit(unit, true, removeFile);
    }
    mClassBrowserModel.beginUpdate();
    mClassBrowserModel.endUpdate();
    ui->projectView->selectionModel()->clearSelection();
    mProject->saveAll();
    updateProjectView();
}


void MainWindow::on_actionView_Makefile_triggered()
{
    if (!mProject)
        return;
    Editor *editor = mEditorList->getOpenedEditorByFilename(mProject->makeFileName());
    if (editor) {
        mEditorList->closeEditor(editor, false, true);
    }
    prepareProjectForCompile();
    mCompilerManager->buildProjectMakefile(mProject);
    openFile(mProject->makeFileName());
}


void MainWindow::on_actionMakeClean_triggered()
{
    if (!mProject)
        return;
    prepareProjectForCompile();
    mCompilerManager->cleanProject(mProject);
}


void MainWindow::on_actionProject_Open_Folder_In_Explorer_triggered()
{
    if (!mProject)
        return;
    QDesktopServices::openUrl(
                QUrl("file:///"+includeTrailingPathDelimiter(mProject->directory()),QUrl::TolerantMode));
}


void MainWindow::on_actionProject_Open_In_Terminal_triggered()
{
    if (!mProject)
        return;
#ifdef Q_OS_WIN
    openShell(mProject->directory(),"cmd.exe",mProject->binDirs());
#else
    openShell(mProject->directory(),pSettings->environment().terminalPath(),mProject->binDirs());
#endif
}

const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &MainWindow::statementColors() const
{
    return mStatementColors;
}


void MainWindow::on_classBrowser_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return ;
    ClassBrowserNode * node = static_cast<ClassBrowserNode*>(index.internalPointer());
    if (!node)
        return ;
    PStatement statement = node->statement;
    if (!statement) {
        return;
    }
    QString filename;
    int line;
    Editor* currentEditor = mEditorList->getEditor();
    if (currentEditor) {
        if (statement->fileName == currentEditor->filename()
                && statement->definitionFileName!=currentEditor->filename()) {
            filename = statement->definitionFileName;
            line = statement->definitionLine;
        } else if (statement->fileName != currentEditor->filename()
                && statement->definitionFileName==currentEditor->filename()) {
            filename = statement->fileName;
            line = statement->line;
        } else if (currentEditor->caretY()==statement->line) {
            filename = statement->definitionFileName;
            line = statement->definitionLine;
        } else {
            filename = statement->fileName;
            line = statement->line;
        }
    } else {
        filename = statement->fileName;
        line = statement->line;
    }
    Editor* e = openFile(filename);
    if (e) {
        e->setCaretPositionAndActivate(line,1);
    }
}

const PTodoParser &MainWindow::todoParser() const
{
    return mTodoParser;
}

PCodeSnippetManager &MainWindow::codeSnippetManager()
{
    return mCodeSnippetManager;
}

PSymbolUsageManager &MainWindow::symbolUsageManager()
{
    return mSymbolUsageManager;
}

void MainWindow::showHideInfosTab(QWidget *widget, bool show)
{
    int idx = findTabIndex(ui->tabExplorer,widget);
    if (idx>=0) {
        if (!show) {
            if (mTabInfosData.contains(widget)) {
                PTabWidgetInfo info = mTabInfosData[widget];
                info->icon = ui->tabExplorer->tabIcon(idx);
                info->text = ui->tabExplorer->tabText(idx);
            }

            ui->tabExplorer->removeTab(idx);
        }
    } else {
        if (show && mTabInfosData.contains(widget)) {
            PTabWidgetInfo info = mTabInfosData[widget];
            int insert = -1;
            for (int i=0;i<ui->tabExplorer->count();i++) {
                QWidget * w=ui->tabExplorer->widget(i);
                PTabWidgetInfo infoW = mTabInfosData[w];
                if (infoW->order>info->order) {
                    insert = i;
                    break;
                }
            }
            if (insert>=0) {
                ui->tabExplorer->insertTab(insert, widget, info->icon, info->text);
            } else {
                ui->tabExplorer->addTab(widget, info->icon, info->text);
            }
        }
    }
}

void MainWindow::showHideMessagesTab(QWidget *widget, bool show)
{
    int idx = findTabIndex(ui->tabMessages,widget);
    if (idx>=0) {
        if (!show) {
            if (mTabMessagesData.contains(widget)) {
                PTabWidgetInfo info = mTabMessagesData[widget];
                info->icon = ui->tabMessages->tabIcon(idx);
                info->text = ui->tabMessages->tabText(idx);
            }
            ui->tabMessages->removeTab(idx);
        }
    } else {
        if (show && mTabMessagesData.contains(widget)) {
            PTabWidgetInfo info = mTabMessagesData[widget];
            int insert = -1;
            for (int i=0;i<ui->tabMessages->count();i++) {
                QWidget * w=ui->tabMessages->widget(i);
                PTabWidgetInfo infoW = mTabMessagesData[w];
                if (infoW->order>info->order) {
                    insert = i;
                    break;
                }
            }
            if (insert>=0) {
                ui->tabMessages->insertTab(insert, widget, info->icon, info->text);
            } else {
                ui->tabMessages->addTab(widget, info->icon, info->text);
            }
        }
    }
}


void MainWindow::prepareTabInfosData()
{
//    QHash<int,QWidget*> tabOrders;
//    tabOrders.insert(pSettings->ui().projectOrder(), ui->tabProject);
//    tabOrders.insert(pSettings->ui().watchOrder(), ui->tabWatch);
//    tabOrders.insert(pSettings->ui().structureOrder(), ui->tabStructure);
//    tabOrders.insert(pSettings->ui().filesOrder(), ui->tabFiles);
//    tabOrders.insert(pSettings->ui().problemSetOrder(), ui->tabProblemSet);

//    for (int i=1;i<tabOrders.count();i++) {

//    }
    for (int i=0;i<ui->tabExplorer->count();i++) {
        QWidget* widget = ui->tabExplorer->widget(i);
        PTabWidgetInfo info = std::make_shared<TabWidgetInfo>();
        info->order =i;
        info->text = ui->tabExplorer->tabText(i);
        info->icon = ui->tabExplorer->tabIcon(i);
        mTabInfosData[widget]=info;
    }
}

void MainWindow::prepareTabMessagesData()
{
//    QHash<int,QWidget*> tabOrders;
//    tabOrders.insert(pSettings->ui().issuesOrder(), ui->tabIssues);
//    tabOrders.insert(pSettings->ui().compileLogOrder(), ui->tabToolsOutput);
//    tabOrders.insert(pSettings->ui().debugOrder(), ui->tabDebug);
//    tabOrders.insert(pSettings->ui().searchOrder(), ui->tabSearch);
//    tabOrders.insert(pSettings->ui().TODOOrder(), ui->tabTODO);
//    tabOrders.insert(pSettings->ui().bookmarkOrder(), ui->tabBookmark);

    for (int i=0;i<ui->tabMessages->count();i++) {
        QWidget* widget = ui->tabMessages->widget(i);
        PTabWidgetInfo info = std::make_shared<TabWidgetInfo>();
        info->order =i;
        info->text = ui->tabMessages->tabText(i);
        info->icon = ui->tabMessages->tabIcon(i);
        mTabMessagesData[widget]=info;
    }
}

void MainWindow::newProjectUnitFile(const QString& suffix)
{
    if (!mProject)
        return;
    QModelIndex current = mProjectProxyModel->mapToSource(ui->projectView->currentIndex());
    ProjectModelNode * node = nullptr;
    if (current.isValid()) {
        node = static_cast<ProjectModelNode*>(current.internalPointer());
    }
    PProjectModelNode pNode = mProject->pointerToNode(node);

    while (pNode && pNode->isUnit) {
        pNode = pNode->parent.lock();
    }

    if (!pNode) {
        pNode = mProject->rootNode();
    }

    QString newFileName;
    PProjectUnit newUnit;
    if (mProject->modelType() == ProjectModelType::FileSystem) {
        PProjectModelNode modelTypeNode = pNode;
        while (modelTypeNode && modelTypeNode->folderNodeType==ProjectModelNodeType::Folder) {
            modelTypeNode=modelTypeNode->parent.lock();
        }
        if (!modelTypeNode) {
             modelTypeNode = mProject->rootNode();
        }
        NewProjectUnitDialog newProjectUnitDialog;
        if (!suffix.isEmpty()) {
            newProjectUnitDialog.setSuffix(suffix);
        } else {
            if (modelTypeNode == mProject->rootNode()) {
                if (mProject->options().isCpp)
                    newProjectUnitDialog.setSuffix("cpp");
                else
                    newProjectUnitDialog.setSuffix("c");
            } else {
                switch (modelTypeNode->folderNodeType) {
                case ProjectModelNodeType::DUMMY_HEADERS_FOLDER:
                    newProjectUnitDialog.setSuffix("h");
                    break;
                case ProjectModelNodeType::DUMMY_SOURCES_FOLDER:
                    if (mProject->options().isCpp)
                        newProjectUnitDialog.setSuffix("cpp");
                    else
                        newProjectUnitDialog.setSuffix("c");
                    break;
                default:
                    newProjectUnitDialog.setSuffix("txt");
                }
            }
        }
        QString folder = mProject->fileSystemNodeFolderPath(pNode);
//        qDebug()<<folder;
        newProjectUnitDialog.setFolder(folder);
        if (newProjectUnitDialog.exec()!=QDialog::Accepted) {
            return;
        }
        newFileName= generateAbsolutePath(newProjectUnitDialog.folder(),newProjectUnitDialog.filename());
        if (newFileName.isEmpty())
            return;
    } else {
        do {
            newFileName = QString("untitled")+QString("%1").arg(getNewFileNumber());
            if (!suffix.isEmpty()) {
                newFileName += "." + suffix;
            } else {
                if (mProject->options().isCpp)
                    newFileName += ".cpp";
                else
                    newFileName += ".c";
            }
        } while (QDir(mProject->directory()).exists(newFileName));
        newFileName = QInputDialog::getText(
                    this,
                    tr("New Project File Name"),
                    tr("File Name:"),
                    QLineEdit::Normal,
                    newFileName);
        if (newFileName.isEmpty())
            return;
        newFileName = generateAbsolutePath(mProject->directory(),newFileName);
    }
    if (fileExists(newFileName)) {
        QMessageBox::critical(this,tr("File Already Exists!"),
                              tr("File '%1' already exists!").arg(newFileName));
        return;
    } else {
        //create an empty file
        createFile(newFileName);
    }
    newUnit = mProject->newUnit(
                    pNode,newFileName);

    setProjectViewCurrentUnit(newUnit);

    mProject->saveAll();

    parseFileList(mProject->cppParser());
    Editor * editor = mProject->openUnit(newUnit, false);
    if (editor)
        editor->activate();
    QString branch;
#ifdef ENABLE_VCS
    if (pSettings->vcs().gitOk() && mProject->model()->iconProvider()->VCSRepository()->hasRepository(branch)) {
        QString output;
        mProject->model()->iconProvider()->VCSRepository()->add(newFileName,output);
        mProject->model()->refreshIcon(newFileName);
    }
#endif
    updateProjectView();
}

void MainWindow::fillProblemCaseInputAndExpected(const POJProblemCase &problemCase)
{
    ui->btnProblemCaseInputFileName->setEnabled(true);
    if (fileExists(problemCase->inputFileName)) {
        ui->txtProblemCaseInput->setReadOnly(true);
        ui->txtProblemCaseInput->setPlainText(readFileToByteArray(problemCase->inputFileName));
        ui->btnProblemCaseClearInputFileName->setVisible(true);
        ui->txtProblemCaseInputFileName->setText(extractFileName(problemCase->inputFileName));
        ui->txtProblemCaseInputFileName->setToolTip(problemCase->inputFileName);
    } else {
        ui->txtProblemCaseInput->setReadOnly(false);
        ui->txtProblemCaseInput->setPlainText(problemCase->input);
        ui->btnProblemCaseClearInputFileName->setVisible(false);
        ui->txtProblemCaseInputFileName->clear();
        ui->txtProblemCaseInputFileName->setToolTip("");
    }
    ui->btnProblemCaseExpectedOutputFileName->setEnabled(true);
    if (fileExists(problemCase->expectedOutputFileName)) {
        ui->txtProblemCaseExpected->setReadOnly(true);
        ui->txtProblemCaseExpected->clearAll();
        ui->txtProblemCaseExpected->setPlainText(readFileToByteArray(problemCase->expectedOutputFileName));
        ui->btnProblemCaseClearExpectedOutputFileName->setVisible(true);
        ui->txtProblemCaseExpectedOutputFileName->setText(extractFileName(problemCase->expectedOutputFileName));
        ui->txtProblemCaseExpectedOutputFileName->setToolTip(problemCase->inputFileName);
    } else {
        ui->txtProblemCaseExpected->setReadOnly(false);
        ui->txtProblemCaseExpected->clearAll();
        ui->txtProblemCaseExpected->setPlainText(problemCase->expected);
        ui->btnProblemCaseClearExpectedOutputFileName->setVisible(false);
        ui->txtProblemCaseExpectedOutputFileName->clear();
        ui->txtProblemCaseExpectedOutputFileName->setToolTip("");
    }
}

void MainWindow::doFilesViewRemoveFile(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    if (mFileSystemModel.isDir(index)) {
        QDir dir(mFileSystemModel.fileInfo(index).absoluteFilePath());
        if (!dir.isEmpty() &&
                QMessageBox::question(ui->treeFiles
                                      ,tr("Delete")
                                      ,tr("Folder %1 is not empty.").arg(mFileSystemModel.fileName(index))
                                      + tr("Do you really want to delete it?"),
                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No)!=QMessageBox::Yes)
            return;
        if (!QFile::moveToTrash(dir.absolutePath()))
            dir.removeRecursively();
    } else {
        if (!QFile::moveToTrash(mFileSystemModel.filePath(index)))
            QFile::remove(mFileSystemModel.filePath(index));
    }
}

void MainWindow::setProjectViewCurrentUnit(std::shared_ptr<ProjectUnit> unit) {
    if (unit) {
        setProjectViewCurrentNode(unit->node());
    }
}

void MainWindow::reparseNonProjectEditors()
{
    if (pSettings->codeCompletion().shareParser()) {
        bool hasC=false;
        bool hasCpp=false;
        for(int i=0;i<mEditorList->pageCount();i++) {
            Editor* e=(*mEditorList)[i];
            if (!e->inProject() && e->parser()) {
                if (e->parser()->language()==ParserLanguage::C) {
                    hasC=true;
                } else if (e->parser()->language()==ParserLanguage::CPlusPlus) {
                    hasCpp=true;
                }
            }
        }
        if (hasC) {
            PCppParser parser{Editor::sharedParser(ParserLanguage::C)};
            if (parser)
                resetCppParser(parser);
        }
        if (hasCpp) {
            PCppParser parser{Editor::sharedParser(ParserLanguage::CPlusPlus)};
            if (parser)
                resetCppParser(parser);
        }
    }
    for (int i=0;i<mEditorList->pageCount();i++) {
        Editor* e=(*mEditorList)[i];
        if (!e->inProject()) {
//            if (!pSettings->codeCompletion().clearWhenEditorHidden() || e->isVisible()) {
            if (e->isVisible()) {
                e->reparse(true);
                e->checkSyntaxInBack();
            }
        }
    }
}

QString MainWindow::switchHeaderSourceTarget(Editor *editor)
{
    QString filename=editor->filename();
    if (getFileType(filename)==FileType::CHeader
            || getFileType(filename)==FileType::CppHeader) {
        QStringList lst;
        lst.push_back("c");
        lst.push_back("cc");
        lst.push_back("cpp");
        lst.push_back("cxx");
        lst.push_back("C");
        lst.push_back("CC");
        foreach(const QString& suffix,lst) {
            QString newFile=changeFileExt(filename,suffix);
            if (fileExists(newFile)) {
                return newFile;
            }
        }
    } else if (getFileType(filename)==FileType::CSource) {
        QStringList lst;
        lst.push_back("h");
        foreach(const QString& suffix,lst) {
            QString newFile=changeFileExt(filename,suffix);
            if (fileExists(newFile)) {
                return newFile;
            }
        }
    } else if (getFileType(filename)==FileType::CppSource) {
        QStringList lst;
        lst.push_back("h");
        lst.push_back("hpp");
        lst.push_back("hxx");
        lst.push_back("HH");
        lst.push_back("H");

        foreach(const QString& suffix,lst) {
            QString newFile=changeFileExt(filename,suffix);
            if (fileExists(newFile)) {
                return newFile;
            }
        }
    }
    return QString();
}

void MainWindow::modifyBreakpointCondition(int index)
{
    PBreakpoint breakpoint = debugger()->breakpointModel()->breakpoint(
                index,
                debugger()->isForProject()
                );
    if (breakpoint) {
        bool isOk;
        QString s=QInputDialog::getText(this,
                                  tr("Break point condition"),
                                  tr("Enter the condition of the breakpoint:"),
                                QLineEdit::Normal,
                                breakpoint->condition,&isOk);
        if (isOk) {
            debugger()->setBreakPointCondition(index,s,debugger()->isForProject());
        }
    }

}

void MainWindow::initEditorActions()
{
    ui->menuCode->menuAction()->setVisible(false);
    ui->menuEdit->menuAction()->setVisible(false);
    ui->menuSelection->menuAction()->setVisible(false);
    ui->menuRefactor->menuAction()->setVisible(false);

    backupMenuForEditor(ui->menuEdit, mMenuEditBackup);
    backupMenuForEditor(ui->menuSelection, mMenuSelectionBackup);
    backupMenuForEditor(ui->menuCode, mMenuCodeBackup);
    backupMenuForEditor(ui->menuRefactor, mMenuRefactorBackup);
    backupMenuForEditor(ui->menuMove_Caret, mMenuMoveCaretBackup);
    backupMenuForEditor(mMenuEncoding, mMenuEncodingBackup);
    backupMenuForEditor(mMenuExport, mMenuExportBackup);

    // changeEditorActionParent(ui->actionPrint, tr("File"));
    // changeEditorActionParent(ui->actionSave, tr("File"));
    // changeEditorActionParent(ui->actionSaveAs, tr("File"));
    // changeEditorActionParent(ui->actionClose, tr("File"));

    // changeEditorActionParent(ui->actionFind, tr("Search"));
    // changeEditorActionParent(ui->actionReplace, tr("Search"));
    changeEditorActionParent(ui->actionFind_references, tr("Search"));
    // changeEditorActionParent(ui->actionFind_Next, tr("Search"));
    // changeEditorActionParent(ui->actionFind_Previous, tr("Search"));
    changeEditorActionParent(ui->actionToggle_Breakpoint, tr("Debug"));
    changeEditorActionParent(ui->actionGoto_Declaration, tr("Code"));
    changeEditorActionParent(ui->actionGoto_Definition, tr("Code"));
    changeEditorActionParent(ui->actionFile_Properties, tr("Code"));
    changeEditorActionParent(ui->actionLocate_in_Files_View, tr("Code"));
    changeEditorActionParent(ui->actionSwitchHeaderSource, tr("Code"));
}

void MainWindow::changeEditorActionParent(QAction *action, const QString& groupName)
{
    removeAction(action);
    action->setParent(ui->EditorPanel);
    ui->EditorPanel->addAction(action);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    action->setData(groupName);
}

void MainWindow::backupMenuForEditor(QMenu *menu, QList<QAction *> &backup)
{
    foreach (QAction* action, menu->actions()) {
        if (!action->objectName().isEmpty())
            changeEditorActionParent(action, menu->title());
        if (action->isSeparator()) {
            action = new QAction();
            action->setSeparator(true);
        }
        backup.append(action);
    }
    menu->clear();
    connect(menu,&QMenu::aboutToShow,
            [menu, backup] {
        foreach (QAction* action, backup) {
            if (action->isSeparator()) {
                menu->addSeparator();
            } else {
                menu->addAction(action);
            }
        }
    });
    connect(menu,&QMenu::aboutToHide,
            [menu] {
        menu->clear();
    });
}

void MainWindow::setupSlotsForProject()
{
    connect(mProject.get(), &Project::unitAdded,
            this, &MainWindow::onProjectUnitAdded);
    connect(mProject.get(), &Project::unitRemoved,
            this, &MainWindow::onProjectUnitRemoved);
    connect(mProject.get(), &Project::unitRenamed,
            this, &MainWindow::onProjectUnitRenamed);
}

void MainWindow::onProjectUnitAdded(const QString &filename)
{
    mProject->cppParser()->addProjectFile(filename,true);
    if (pSettings->editor().parseTodos()) {
        mTodoParser->parseFile(filename,true);
    }
}

void MainWindow::onProjectUnitRemoved(const QString &filename)
{
    mProject->cppParser()->invalidateFile(filename);
    mProject->cppParser()->removeProjectFile(filename);
    if (pSettings->editor().parseTodos()) {
        mTodoModel.removeTodosForFile(filename);
    }
    mDebugger->breakpointModel()->removeBreakpointsInFile(filename,true);
    mBookmarkModel->removeBookmarks(filename,true);
}

void MainWindow::onProjectUnitRenamed(const QString &oldFilename, const QString &newFilename)
{
    mProject->cppParser()->invalidateFile(oldFilename);
    mProject->cppParser()->removeProjectFile(oldFilename);
    mProject->cppParser()->addProjectFile(newFilename,true);
    parseFileList(mProject->cppParser());
    if (pSettings->editor().parseTodos()) {
        mTodoModel.removeTodosForFile(oldFilename);
        mTodoParser->parseFile(newFilename,true);
    }
    mBookmarkModel->renameBookmarkFile(oldFilename,newFilename,true);
    mDebugger->breakpointModel()->renameBreakpointFilenames(oldFilename,newFilename,true);
}

void MainWindow::onProjectViewNodeRenamed()
{
    updateProjectView();
}

void MainWindow::setProjectViewCurrentNode(PProjectModelNode node)
{
    if (node) {
        QModelIndex index = mProject->model()->getNodeIndex(node.get());
        index = mProjectProxyModel->mapFromSource(index);
        if (index.isValid()) {
            ui->projectView->expand(index);
            ui->projectView->setCurrentIndex(index);
        }
    }
}

static void setDockTitlebarLocation(QDockWidget* dock, const Qt::DockWidgetArea &area) {
    switch(area) {
    case Qt::DockWidgetArea::BottomDockWidgetArea:
    case Qt::DockWidgetArea::TopDockWidgetArea:
        dock->setFeatures(dock->features() | QDockWidget::DockWidgetVerticalTitleBar);
        break;
    default:
        dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetVerticalTitleBar);
    }
}

static void setTabsInDockLocation(QTabWidget* tabs, const Qt::DockWidgetArea &area) {
    switch(area) {
    case Qt::DockWidgetArea::BottomDockWidgetArea:
    case Qt::DockWidgetArea::TopDockWidgetArea:
        tabs->setTabPosition(QTabWidget::TabPosition::South);
        break;
    case Qt::DockWidgetArea::LeftDockWidgetArea:
        tabs->setTabPosition(QTabWidget::TabPosition::West);
        break;
    case Qt::DockWidgetArea::RightDockWidgetArea:
        tabs->setTabPosition(QTabWidget::TabPosition::East);
        break;
    default:
        break;
    }
}

static void setSplitterInDockLocation(QSplitter* splitter, const Qt::DockWidgetArea& area) {
    switch(area) {
    case Qt::DockWidgetArea::BottomDockWidgetArea:
    case Qt::DockWidgetArea::TopDockWidgetArea:
        splitter->setOrientation(Qt::Orientation::Horizontal);
        break;
    default:
        splitter->setOrientation(Qt::Orientation::Vertical);
    }

}
void MainWindow::setDockExplorerToArea(const Qt::DockWidgetArea &area)
{
    ui->dockMessages->setAllowedAreas(
                (Qt::DockWidgetArea::LeftDockWidgetArea |
                 Qt::DockWidgetArea::BottomDockWidgetArea |
                 Qt::DockWidgetArea::RightDockWidgetArea)
                & ~area);
    if (area==Qt::DockWidgetArea::NoDockWidgetArea)
        return;
    setDockTitlebarLocation(ui->dockExplorer,area);
    setTabsInDockLocation(ui->tabExplorer,area);
}

void MainWindow::setDockMessagesToArea(const Qt::DockWidgetArea &area)
{
    ui->dockExplorer->setAllowedAreas(
                (Qt::DockWidgetArea::LeftDockWidgetArea |
                 Qt::DockWidgetArea::BottomDockWidgetArea |
                 Qt::DockWidgetArea::RightDockWidgetArea)
                & ~area);
    Qt::DockWidgetArea effectiveArea;
    if (area==Qt::DockWidgetArea::NoDockWidgetArea) {
        switch (mMessagesDockLocation) {
        case Qt::DockWidgetArea::BottomDockWidgetArea:
        case Qt::DockWidgetArea::TopDockWidgetArea:
            effectiveArea = Qt::DockWidgetArea::RightDockWidgetArea;
            break;
        default:
            if (dockWidgetArea(ui->dockExplorer)!=Qt::DockWidgetArea::BottomDockWidgetArea)
                effectiveArea = Qt::DockWidgetArea::BottomDockWidgetArea;
            else
                effectiveArea = Qt::DockWidgetArea::LeftDockWidgetArea;
        }
    } else {
        effectiveArea = area;
        mMessagesDockLocation = area;
        setDockTitlebarLocation(ui->dockMessages,effectiveArea);
    }
    setTabsInDockLocation(ui->tabMessages,effectiveArea);
    setSplitterInDockLocation(ui->splitterDebug,effectiveArea);
    setSplitterInDockLocation(ui->splitterProblem,effectiveArea);
    QGridLayout* layout=(QGridLayout*)ui->panelProblemCase->layout();
    layout->removeWidget(ui->widgetProblemCaseInputCaption);
    layout->removeWidget(ui->widgetProblemCaseOutputCaption);
    layout->removeWidget(ui->widgetProblemCaseExpectedCaption);
    layout->removeWidget(ui->txtProblemCaseInput);
    layout->removeWidget(ui->txtProblemCaseOutput);
    layout->removeWidget(ui->txtProblemCaseExpected);
    layout->removeWidget(ui->lblProblemCaseInput);
    layout->removeWidget(ui->lblProblemCaseOutput);
    layout->removeWidget(ui->lblProblemCaseExpected);
    switch(effectiveArea) {
    case Qt::DockWidgetArea::BottomDockWidgetArea:
    case Qt::DockWidgetArea::TopDockWidgetArea:
        layout->addWidget(ui->widgetProblemCaseInputCaption, 0, 0, 1, 1);
        layout->addWidget(ui->txtProblemCaseInput, 1, 0, 1, 1);
//        layout->addWidget(ui->lblProblemCaseInput, 2, 0, 1, 1);

        layout->addWidget(ui->widgetProblemCaseOutputCaption, 0, 1, 1, 1);
        layout->addWidget(ui->txtProblemCaseOutput, 1, 1, 1, 1);
//        layout->addWidget(ui->lblProblemCaseOutput, 2, 1, 1, 1);

        layout->addWidget(ui->widgetProblemCaseExpectedCaption, 0, 2, 1, 1);
        layout->addWidget(ui->txtProblemCaseExpected, 1, 2, 1, 1);
//        layout->addWidget(ui->lblProblemCaseExpected, 2, 2, 1, 1);



        break;
    default:
        layout->addWidget(ui->widgetProblemCaseInputCaption, 0, 0, 1, 1);
        layout->addWidget(ui->txtProblemCaseInput, 1, 0, 1, 1);
        //layout->addWidget(ui->lblProblemCaseInput, 2, 0, 1, 1);


        layout->addWidget(ui->widgetProblemCaseOutputCaption, 3, 0, 1, 1);
        layout->addWidget(ui->txtProblemCaseOutput, 4, 0, 1, 1);
        //layout->addWidget(ui->lblProblemCaseOutput, 5, 0, 1, 1);

        layout->addWidget(ui->widgetProblemCaseExpectedCaption, 6, 0, 1, 1);
        layout->addWidget(ui->txtProblemCaseExpected, 7, 0, 1, 1);
        //layout->addWidget(ui->lblProblemCaseExpected, 8, 0, 1, 1);
    }
}

#ifdef ENABLE_VCS
void MainWindow::updateVCSActions()
{
    bool hasRepository = false;
    bool shouldEnable = false;
    bool canBranch = false;
    if (ui->projectView->isVisible() && mProject) {
        mProject->model()->iconProvider()->update();
        QString branch;
        hasRepository = mProject->model()->iconProvider()->VCSRepository()->hasRepository(branch);
        shouldEnable = true;
        canBranch = !mProject->model()->iconProvider()->VCSRepository()->hasChangedFiles()
                && !mProject->model()->iconProvider()->VCSRepository()->hasStagedFiles();
    } else if (ui->treeFiles->isVisible()) {
        mFileSystemModelIconProvider.update();
        QString branch;
        hasRepository = mFileSystemModelIconProvider.VCSRepository()->hasRepository(branch);
        shouldEnable = true;
        canBranch =!mFileSystemModelIconProvider.VCSRepository()->hasChangedFiles()
                && !mFileSystemModelIconProvider.VCSRepository()->hasStagedFiles();
    }

    ui->actionGit_Remotes->setEnabled(hasRepository && shouldEnable);
    ui->actionGit_Create_Repository->setEnabled(!hasRepository && shouldEnable);
    ui->actionGit_Push->setEnabled(hasRepository && shouldEnable);
    ui->actionGit_Pull->setEnabled(hasRepository && shouldEnable);
    ui->actionGit_Fetch->setEnabled(hasRepository && shouldEnable);
    ui->actionGit_Log->setEnabled(hasRepository && shouldEnable);
    ui->actionGit_Commit->setEnabled(hasRepository && shouldEnable);
    ui->actionGit_Branch->setEnabled(hasRepository && shouldEnable && canBranch);
    ui->actionGit_Merge->setEnabled(hasRepository && shouldEnable && canBranch);
    ui->actionGit_Reset->setEnabled(hasRepository && shouldEnable);
    ui->actionGit_Restore->setEnabled(hasRepository && shouldEnable);
    ui->actionGit_Revert->setEnabled(hasRepository && shouldEnable);
}
#endif

void MainWindow::invalidateProjectProxyModel()
{
    mProjectProxyModel->invalidate();
}

void MainWindow::onEditorRenamed(const QString &oldFilename, const QString &newFilename, bool firstSave)
{
    if (firstSave)
        mOJProblemSetModel.updateProblemAnswerFilename(oldFilename, newFilename);
    Editor * editor=mEditorList->getOpenedEditorByFilename(newFilename);
    if (editor && !editor->inProject()) {
        mBookmarkModel->renameBookmarkFile(oldFilename,newFilename,false);
        mDebugger->breakpointModel()->renameBreakpointFilenames(oldFilename,newFilename,false);
    }
}

void MainWindow::on_EditorTabsLeft_currentChanged(int)
{
}


void MainWindow::on_EditorTabsRight_currentChanged(int)
{
}

void MainWindow::on_tableTODO_doubleClicked(const QModelIndex &index)
{
    PTodoItem item = mTodoModel.getItem(index);
    if (item) {
        Editor * editor = mEditorList->getOpenedEditorByFilename(item->filename);
        if (editor) {
            editor->setCaretPositionAndActivate(item->lineNo,item->ch+1);
        }
    }
}


void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dialog;
    dialog.exec();
}


void MainWindow::on_actionRename_Symbol_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (!editor)
        return;
    if (!editor->parser())
        return;
    editor->beginEditing();
    QSynedit::BufferCoord oldCaretXY = editor->caretXY();
    //    mClassBrowserModel.beginUpdate();
    QCursor oldCursor = editor->cursor();
    editor->setCursor(Qt::CursorShape::WaitCursor);
    auto action = finally([oldCursor,editor]{
        editor->endEditing();
//        mClassBrowserModel.EndTreeUpdate;
        editor->setCursor(oldCursor);
    });

    QStringList expression = editor->getExpressionAtPosition(oldCaretXY);
    if (expression.isEmpty() && oldCaretXY.ch>1) {
        QSynedit::BufferCoord coord=oldCaretXY;
        coord.ch--;
        expression = editor->getExpressionAtPosition(coord);
    }

    if (editor->inProject() && mProject) {
        for (int i=0;i<mEditorList->pageCount();i++) {
            Editor * e=(*mEditorList)[i];
            if (e->modified())  {
                mProject->cppParser()->parseFile(editor->filename(), editor->inProject(), false, false);
            }
        }

        // Find it's definition
        PStatement oldStatement = editor->parser()->findStatementOf(
                        editor->filename(),
                        expression,
                        oldCaretXY.line);
        // definition of the symbol not found
        if (!oldStatement)
            return;
        // found but not in this file
        if (editor->filename() != oldStatement->fileName
            || editor->filename() != oldStatement->definitionFileName) {
            // it's defined in system header, dont rename
            if (mProject->cppParser()->isSystemHeaderFile(oldStatement->fileName)) {
                QMessageBox::critical(editor,
                            tr("Rename Error"),
                            tr("Symbol '%1' is defined in system header.")
                                      .arg(oldStatement->fullName));
                return;
            }
            CppRefacter refactor;
            refactor.findOccurence(editor,oldCaretXY);
            showSearchPanel(true);
            return;
        }
    }
    //not in project
    PStatement oldStatement = editor->parser()->findStatementOf(
                    editor->filename(),
                    expression,
                    oldCaretXY.line);
    if (!oldStatement)
        return;
    QString word = oldStatement->command;
    if (word.isEmpty())
        return;
    if (isCppKeyword(word)) {
        return;
    }

    bool ok;
    QString newWord = QInputDialog::getText(editor,
                                            tr("Rename Symbol"),
                                            tr("New Name"),
                                            QLineEdit::Normal,word, &ok);
    if (!ok)
        return;

    if (word == newWord)
        return;

    if (!editor->inProject() && editor->modified() ) {
        PCppParser parser = editor->parser();
        //here we must reparse the file in sync, or rename may fail
        parser->parseFile(editor->filename(), editor->inProject(), false, false);
    }
    CppRefacter refactor;

    refactor.renameSymbol(editor,oldCaretXY,newWord);
    editor->reparse(true);
    editor->checkSyntaxInBack();
    editor->reparseTodo();
}


void MainWindow::showSearchReplacePanel(bool show)
{
    ui->replacePanel->setVisible(show);
    ui->cbSearchHistory->setDisabled(show);
    if (show && mSearchResultModel.currentResults()) {
        ui->cbReplaceInHistory->setCurrentText(
                    mSearchResultModel.currentResults()->keyword);
    }
    mSearchResultTreeModel->setSelectable(show);
}

void MainWindow::setFilesViewRoot(const QString &path, bool setOpenFolder)
{
    mFileSystemModelIconProvider.setRootFolder(path);
    mFileSystemModel.setIconProvider(&mFileSystemModelIconProvider);
    mFileSystemModel.setRootPath(path);
    ui->treeFiles->setRootIndex(mFileSystemModel.index(path));
    pSettings->environment().setCurrentFolder(path);
    if (setOpenFolder)
        QDir::setCurrent(path);
    int pos = ui->cbFilesPath->findText(path);
    if (pos<0) {
        ui->cbFilesPath->addItem(mFileSystemModel.iconProvider()->icon(QFileIconProvider::Folder),path);
        pos =  ui->cbFilesPath->findText(path);
    } else if (ui->cbFilesPath->itemIcon(pos).isNull()) {
        ui->cbFilesPath->setItemIcon(pos,mFileSystemModel.iconProvider()->icon(QFileIconProvider::Folder));
    }
    ui->cbFilesPath->setCurrentIndex(pos);
    ui->cbFilesPath->lineEdit()->setCursorPosition(1);
}

void MainWindow::clearIssues()
{
    int i = ui->tabMessages->indexOf(ui->tabIssues);
    if (i!=-1) {
        ui->tabMessages->setTabText(i, tr("Issues"));
    }
    ui->tableIssues->clearIssues();
    mCompileIssuesState = CompileIssuesState::None;
}

void MainWindow::doCompileRun(RunType runType)
{
    CompileTarget target =getCompileTarget();
    QStringList binDirs;
    QString execName;
    if (target == CompileTarget::Project) {
        binDirs = mProject->binDirs();
        execName = mProject->outputFilename();
    } else {
        binDirs = getDefaultCompilerSetBinDirs();
    }
    mCompileSuccessionTask = std::make_shared<CompileSuccessionTask>();
    mCompileSuccessionTask->binDirs=binDirs;
    mCompileSuccessionTask->type = runTypeToCompileSuccessionTaskType(runType);
    compile();
}

void MainWindow::doGenerateAssembly()
{
    CompileTarget target =getCompileTarget();
    QString execName;
    if (target!= CompileTarget::File
            && target != CompileTarget::Project) {
        return;
    }
    mCompileSuccessionTask = std::make_shared<CompileSuccessionTask>();
    //mCompileSuccessionTask->binDirs="";
    mCompileSuccessionTask->type = CompileSuccessionTaskType::RunNormal;
    compile(false,CppCompileType::GenerateAssemblyOnly);
}

void MainWindow::updateProblemCaseOutput(POJProblemCase problemCase)
{
    if (problemCase->testState == ProblemCaseTestState::Failed) {
        int diffLine=-1;
        if (problemCase->firstDiffLine!=-1) {
            diffLine = problemCase->firstDiffLine;
        } else
            return;
        if (diffLine < problemCase->outputLineCounts) {
            ui->txtProblemCaseOutput->highlightLine(diffLine, mErrorColor);
        } else {
            ui->txtProblemCaseOutput->moveCursor(QTextCursor::MoveOperation::End);
            ui->txtProblemCaseOutput->moveCursor(QTextCursor::MoveOperation::StartOfLine);
        }
        if (diffLine < problemCase->expectedLineCounts) {
            if (ui->txtProblemCaseExpected->document()->blockCount()<=5000) {
                ui->txtProblemCaseExpected->highlightLine(diffLine, mErrorColor);
            } else {
                ui->txtProblemCaseExpected->locateLine(diffLine);
            }
        } else {
            ui->txtProblemCaseExpected->moveCursor(QTextCursor::MoveOperation::End);
            ui->txtProblemCaseExpected->moveCursor(QTextCursor::MoveOperation::StartOfLine);
        }
    } else {
        ui->txtProblemCaseOutput->moveCursor(QTextCursor::MoveOperation::Start);
        ui->txtProblemCaseExpected->moveCursor(QTextCursor::MoveOperation::Start);
    }
}

void MainWindow::applyCurrentProblemCaseChanges()
{
    QModelIndex idx = ui->tblProblemCases->currentIndex();
    if (idx.isValid()) {
        POJProblemCase problemCase = mOJProblemModel.getCase(idx.row());
        if (problemCase) {
            if (!fileExists(problemCase->inputFileName))
                problemCase->input = ui->txtProblemCaseInput->toPlainText();
            problemCase->expected = ui->txtProblemCaseExpected->toPlainText();
        }
    }
}

void MainWindow::on_btnReplace_clicked()
{
    //select all items by default
    PSearchResults results = mSearchResultModel.currentResults();
    if (!results) {
        return;
    }
    QString newWord = ui->cbReplaceInHistory->currentText();
    foreach (const PSearchResultTreeItem& file, results->results) {
        QVector<PSearchResultTreeItem> selections;
        foreach(const PSearchResultTreeItem& item,file->results) {
            if (item->selected) {
                selections.push_back(item);
            }
        }
        if (selections.isEmpty())
            continue;
        Editor* editor = nullptr;
        if (ui->chkOpenFileInEditors->isChecked()) {
            editor = openFile(file->filename);
            if (!editor) {
                QMessageBox::critical(this,
                                      tr("Replace Error"),
                                      tr("Can't open file '%1' for replace!").arg(file->filename));
                return;
            }
        } else {
            editor = mEditorList->getOpenedEditorByFilename(file->filename);
        }
        bool needSave=false;
        std::shared_ptr<Editor> pEditor;
        if (editor) {
            editor->clearSelection();
            editor->addGroupBreak();
            editor->beginEditing();
        } else {
            needSave=true;
            pEditor = std::make_shared<Editor>(nullptr);
            editor = pEditor.get();
            QByteArray encoding;
            editor->setSyntaxer(syntaxerManager.getSyntaxer(QSynedit::ProgrammingLanguage::CPP));
            try {
                editor->document()->loadFromFile(file->filename,ENCODING_AUTO_DETECT,encoding);
            } catch(FileError e) {
                QMessageBox::critical(this,
                                      tr("Replace Error"),
                                      e.reason());
                return;
            }
        }
        while (!selections.isEmpty()) {
            const PSearchResultTreeItem& item = selections.back();
            selections.pop_back();
            QString line = editor->lineText(item->line);
            if (line.mid(item->start-1,results->keyword.length())!=results->keyword) {
                QMessageBox::critical(editor,
                            tr("Replace Error"),
                            tr("Contents has changed since last search!"));
                if (!needSave)
                    editor->endEditing();
                return;
            }
            line.remove(item->start-1,results->keyword.length());
            line.insert(item->start-1, newWord);
            editor->replaceLine(item->line,line);
        }
        if (!needSave) {
            editor->endEditing();
        } else {
            QByteArray realEncoding;
            QFile toFile(file->filename);
            try {
                editor->document()->saveToFile(toFile,ENCODING_AUTO_DETECT,
                                       pSettings->editor().defaultEncoding(),
                                       realEncoding);
            } catch(FileError e) {
                QMessageBox::critical(this,
                                      tr("Replace Error"),
                                      e.reason());
                return;
            }
        }
    }
    showSearchReplacePanel(false);
    stretchMessagesPanel(false);
}

void MainWindow::on_btnCancelReplace_clicked()
{
    showSearchReplacePanel(false);
}

void MainWindow::on_actionPrint_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (!editor)
        return;
    editor->print();
}

bool MainWindow::shouldRemoveAllSettings() const
{
    return mShouldRemoveAllSettings;
}

const PToolsManager &MainWindow::toolsManager() const
{
    return mToolsManager;
}


void MainWindow::on_actionExport_As_RTF_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (!editor)
        return;
    QString rtfFile = QFileDialog::getSaveFileName(editor,
                                 tr("Export As RTF"),
                                 extractFilePath(editor->filename()),
                                 tr("Rich Text Format Files (*.rtf)")
                                 );
    if (rtfFile.isEmpty())
        return;
    try {
        editor->exportAsRTF(rtfFile);
    } catch (FileError e) {
        QMessageBox::critical(editor,
                              "Error",
                              e.reason());
    }
}


void MainWindow::on_actionExport_As_HTML_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (!editor)
        return;
    QString htmlFile = QFileDialog::getSaveFileName(editor,
                                 tr("Export As HTML"),
                                 extractFilePath(editor->filename()),
                                 tr("HTML Files (*.html)")
                                 );
    if (htmlFile.isEmpty())
        return;
    try {
        editor->exportAsHTML(htmlFile);
    } catch (FileError e) {
        QMessageBox::critical(editor,
                              "Error",
                              e.reason());
    }
}


void MainWindow::on_actionMove_To_Other_View_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        mEditorList->swapEditor(editor);
    }
}


void MainWindow::on_actionC_C_Reference_triggered()
{
    QFileInfo fileInfo(includeTrailingPathDelimiter(pSettings->dirs().appDir())+
                       QString("cppreference-%1.chm").arg(pSettings->environment().language()));
    if (fileInfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
        return;
    }
    fileInfo=QFileInfo(includeTrailingPathDelimiter(pSettings->dirs().appDir())+
                       QString("cppreference.chm"));
    if (fileInfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
        return;
    }
    if (pSettings->environment().language()=="zh_CN") {
        QDesktopServices::openUrl(QUrl("https://zh.cppreference.com/w/cpp"));
    } else {
        QDesktopServices::openUrl(QUrl("https://en.cppreference.com/w/cpp"));
    }
}


void MainWindow::on_actionEGE_Manual_triggered()
{
    QDesktopServices::openUrl(QUrl("https://xege.org/ege-open-source"));
}

const PBookmarkModel &MainWindow::bookmarkModel() const
{
    return mBookmarkModel;
}

TodoModel *MainWindow::todoModel()
{
    return &mTodoModel;
}

void MainWindow::on_tableBookmark_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    PBookmark bookmark = mBookmarkModel->bookmark(index.row());
    if (bookmark) {
        Editor *editor= openFile(bookmark->filename);
        if (editor) {
            editor->setCaretPositionAndActivate(bookmark->line,1);
        }
    }
}


void MainWindow::on_actionModify_Bookmark_Description_triggered()
{
    Editor* editor = mEditorList->getEditor();
    if (editor) {
        int line = editor->caretY();
        PBookmark bookmark = mBookmarkModel->bookmark(editor->filename(),line);
        if (bookmark) {
            QString desc = QInputDialog::getText(editor,tr("Bookmark Description"),
                                                 tr("Description:"),QLineEdit::Normal,
                                                 bookmark->description);
            desc = desc.trimmed();
            mBookmarkModel->updateDescription(editor->filename(),line,desc);
        }
    }
}


void MainWindow::on_actionLocate_in_Files_View_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        QFileInfo fileInfo(editor->filename());
        //qDebug()<<fileInfo.absoluteFilePath();
        //qDebug()<<includeTrailingPathDelimiter(mFileSystemModel.rootDirectory().absolutePath());
        if (!fileInfo.absoluteFilePath().startsWith(
                    includeTrailingPathDelimiter(mFileSystemModel.rootDirectory().absolutePath()),
                    PATH_SENSITIVITY
                    )) {
            QString fileDir = extractFileDir(editor->filename());
            if (QMessageBox::question(this,
                                      tr("Change working folder"),
                                      tr("File '%1' is not in the current working folder.")
                                      .arg(extractFileName(editor->filename()))
                                      +"<br />"
                                      +tr("Do you want to change working folder to '%1'?")
                                      .arg(fileDir),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::Yes
                                      )!=QMessageBox::Yes) {
                return;
            }
            if (!fileDir.isEmpty())
                setFilesViewRoot(fileDir,true);
            else
                return;
        }
        QModelIndex index = mFileSystemModel.index(editor->filename());
        ui->treeFiles->setCurrentIndex(index);
        ui->treeFiles->scrollTo(index, QAbstractItemView::PositionAtCenter);
        ui->tabExplorer->setCurrentWidget(ui->tabFiles);
        stretchExplorerPanel(true);
    }
}


void MainWindow::on_treeFiles_doubleClicked(const QModelIndex &index)
{
    if (index!=ui->treeFiles->currentIndex())
        return;
    QString filepath = mFileSystemModel.filePath(index);
    QFileInfo file(filepath);
    if (file.isFile()) {
        switch (getFileType(filepath)) {
        case FileType::Project:
            openProject(filepath);
            break;
        case FileType::Other:
            {
            QMimeDatabase db;
            QMimeType mimeType=db.mimeTypeForFile(file);
            if (mimeType.isValid() && mimeType.name().startsWith("text/")) {
                openFile(filepath);
            } else {
                QDesktopServices::openUrl(QUrl::fromLocalFile(file.absoluteFilePath()));
            }
        }
            break;
        default:
            openFile(filepath);
        }
    }
}


void MainWindow::on_actionOpen_Folder_triggered()
{
    QString folder = QFileDialog::getExistingDirectory(this,tr("Choose Working Folder"),
                                                       pSettings->environment().currentFolder());
    if (!folder.isEmpty()) {
        setFilesViewRoot(folder,true);
    }
}


void MainWindow::on_actionRun_Parameters_triggered()
{
    changeOptions(
                SettingsDialog::tr("General"),
                SettingsDialog::tr("Program Runner")
                );
}


void MainWindow::onNewProblemSet()
{
    if (mOJProblemSetModel.count()>0) {
        if (QMessageBox::warning(this,
                             tr("New Problem Set"),
                             tr("The current problem set is not empty.")
                             +"<br />"
                             +tr("Do you want to save it?"),
                             QMessageBox::Yes | QMessageBox::No)==QMessageBox::Yes) {
            onSaveProblemSet();
        }
    }
    mOJProblemSetNameCounter++;
    mOJProblemSetModel.create(tr("Problem Set %1").arg(mOJProblemSetNameCounter));
    ui->lblProblemSet->setText(mOJProblemSetModel.name());
    onProblemSetIndexChanged(QModelIndex(),QModelIndex());
}


void MainWindow::onAddProblem()
{
    int startCount = mOJProblemSetModel.count();
    QString name;
    while (true) {
        name = tr("Problem %1").arg(startCount+1);
        if (!mOJProblemSetModel.problemNameUsed(name))
            break;
    }
    POJProblem problem = std::make_shared<OJProblem>();
    problem->name = name;
    mOJProblemSetModel.addProblem(problem);
    ui->lstProblemSet->setCurrentIndex(mOJProblemSetModel.index(mOJProblemSetModel.count()-1));
    mProblem_Properties->trigger();
}


void MainWindow::onRemoveProblem()
{
    if (ui->lstProblemSet->selectionModel()->selectedIndexes().isEmpty()) {
        QModelIndex idx=ui->lstProblemSet->currentIndex();
        if (idx.isValid())
            mOJProblemSetModel.removeProblem(idx.row());
    } else {
        QList<int> idxList;
        foreach (const QModelIndex idx,ui->lstProblemSet->selectionModel()->selectedIndexes()) {
            idxList.append(idx.row());
        }
        if (idxList.isEmpty())
            return;
        std::sort(idxList.begin(),idxList.end(),[](int i1, int i2){
           return i1>i2;
        });
        bool oldBlock = ui->lstProblemSet->selectionModel()->blockSignals(true);
        for (int i=0;i<idxList.count();i++) {
            mOJProblemSetModel.removeProblem(idxList[i]);
        }
        ui->lstProblemSet->selectionModel()->blockSignals(oldBlock);
        onProblemSetIndexChanged(ui->lstProblemSet->currentIndex(),QModelIndex());
    }
}


void MainWindow::onSaveProblemSet()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Save Problem Set"));
    if (!mOJProblemSetModel.exportFilename().isEmpty()) {
        dialog.setDirectory(mOJProblemSetModel.exportFilename());
        dialog.selectFile(mOJProblemSetModel.exportFilename());
    } else {
        dialog.setDirectory(QDir().absolutePath());
    }
    dialog.setNameFilter(tr("Problem Set Files (*.pbs)"));
    dialog.setDefaultSuffix("pbs");
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec() == QDialog::Accepted) {
        QString fileName=dialog.selectedFiles()[0];
        QFileInfo fileInfo(fileName);
        if (fileInfo.suffix().isEmpty()) {
            fileName.append(".pbs");
        }
        QDir::setCurrent(extractFileDir(fileName));
        try {
            applyCurrentProblemCaseChanges();
            int currentIndex=-1;
            if (ui->lstProblemSet->currentIndex().isValid())
                currentIndex = ui->lstProblemSet->currentIndex().row();
            mOJProblemSetModel.saveToFile(fileName,currentIndex);
        } catch (FileError& error) {
            QMessageBox::critical(this,tr("Save Error"),
                                  error.reason());
        }
    }
}


void MainWindow::onLoadProblemSet()
{
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Load Problem Set"),
                QString(),
                tr("Problem Set Files (*.pbs)"));
    if (!fileName.isEmpty()) {
        QDir::setCurrent(extractFileDir(fileName));
        try {
            int currentIndex;
            mOJProblemSetModel.loadFromFile(fileName,currentIndex);
            if (currentIndex>=0) {
                if (currentIndex>=0) {
                    QModelIndex index = mOJProblemSetModel.index(currentIndex,0);
                    ui->lstProblemSet->setCurrentIndex(index);
                    ui->lstProblemSet->scrollTo(index);
                }
            }
        } catch (FileError& error) {
            QMessageBox::critical(this,tr("Load Error"),
                                  error.reason());
        }
    }
    ui->lblProblemSet->setText(mOJProblemSetModel.name());
    ui->lstProblemSet->setCurrentIndex(mOJProblemSetModel.index(0,0));
}


void MainWindow::onAddProblemCase()
{
    int startCount = mOJProblemModel.count();
    QString name;
    while (true) {
        name = tr("Problem Case %1").arg(startCount+1);
        if (!mOJProblemSetModel.problemNameUsed(name))
            break;
    }
    POJProblemCase problemCase = std::make_shared<OJProblemCase>();
    problemCase->name = name;
    problemCase->testState = ProblemCaseTestState::NotTested;
    mOJProblemModel.addCase(problemCase);
    ui->tblProblemCases->setCurrentIndex(mOJProblemModel.index(mOJProblemModel.count()-1,0));
}

void MainWindow::onProblemRunAllCases()
{    
    if (mOJProblemModel.count()<=0)
        return;
    showHideMessagesTab(ui->tabProblem,ui->actionProblem);
    applyCurrentProblemCaseChanges();
    runExecutable(RunType::ProblemCases);
}


void MainWindow::on_actionC_Reference_triggered()
{
    QFileInfo fileInfo(includeTrailingPathDelimiter(pSettings->dirs().appDir())+
                       QString("cppreference-%1.chm").arg(pSettings->environment().language()));
    if (fileInfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
        return;
    }
    fileInfo=QFileInfo(includeTrailingPathDelimiter(pSettings->dirs().appDir())+
                       QString("cppreference.chm"));
    if (fileInfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
        return;
    }
    if (pSettings->environment().language()=="zh_CN") {
        QDesktopServices::openUrl(QUrl("https://zh.cppreference.com/w/c"));
    } else {
        QDesktopServices::openUrl(QUrl("https://en.cppreference.com/w/c"));
    }
}


void MainWindow::onRemoveProblemCases()
{
    QModelIndex idx = ui->tblProblemCases->currentIndex();
    if (idx.isValid()) {
        mOJProblemModel.removeCase(idx.row());
    }
}


void MainWindow::onOpenProblemAnswerFile()
{
    POJProblem problem = mOJProblemModel.problem();
    if (!problem || problem->answerProgram.isEmpty())
        return;
    Editor *e = openFile(problem->answerProgram);
    if (e) {
        e->activate();
    }
}

bool MainWindow::openningFiles() const
{
    return mOpenningFiles;
}

QList<QAction *> MainWindow::listShortCutableActions()
{
    QList<QAction*> actions = findChildren<QAction *>(QString(), Qt::FindDirectChildrenOnly);
    actions += ui->EditorPanel->actions();
    return actions;
}

void MainWindow::switchCurrentStackTrace(int idx)
{
    PTrace trace = mDebugger->backtraceModel()->backtrace(idx);
    if (trace) {
        Editor *e = openFile(trace->filename);
        if (e) {
            e->setCaretPositionAndActivate(trace->line,1);
        }
        mDebugger->selectFrame(trace);
        mDebugger->refreshStackVariables();
        mDebugger->refreshWatchVars();
        if (this->mCPUDialog) {
            this->mCPUDialog->updateInfo();
        }
        if (idx!=ui->tblStackTrace->currentIndex().row()) {
            ui->tblStackTrace->setCurrentIndex(ui->tblStackTrace->model()->index(idx,0));
        }
    }

}

MainWindow::CompileSuccessionTaskType MainWindow::runTypeToCompileSuccessionTaskType(RunType runType)
{
    switch (runType) {
    case RunType::CurrentProblemCase:
        return CompileSuccessionTaskType::RunCurrentProblemCase;
    case RunType::ProblemCases:
        return CompileSuccessionTaskType::RunProblemCases;
    default:
        return CompileSuccessionTaskType::RunNormal;
    }
}


void MainWindow::on_actionTool_Window_Bars_triggered()
{
    bool state = ui->tabExplorer->isVisible();
    state = !state;
    ui->tabExplorer->setVisible(state);
    ui->tabMessages->setVisible(state);
    ui->actionTool_Window_Bars->setChecked(state);
}

void MainWindow::on_actionStatus_Bar_triggered()
{
    bool state = ui->statusbar->isVisible();
    state = !state;
    ui->statusbar->setVisible(state);
    ui->actionStatus_Bar->setChecked(state);
}

void MainWindow::on_actionProject_triggered()
{
    bool state = ui->actionProject->isChecked();
    ui->actionProject->setChecked(state);
    showHideInfosTab(ui->tabProject,state);
}


void MainWindow::on_actionWatch_triggered()
{
    bool state = ui->actionWatch->isChecked();
    ui->actionWatch->setChecked(state);
    showHideInfosTab(ui->tabWatch,state);
}


void MainWindow::on_actionStructure_triggered()
{
    bool state = ui->actionStructure->isChecked();
    ui->actionStructure->setChecked(state);
    showHideInfosTab(ui->tabStructure,state);
}


void MainWindow::on_actionFiles_triggered()
{
    bool state = ui->actionFiles->isChecked();
    ui->actionFiles->setChecked(state);
    showHideInfosTab(ui->tabFiles,state);
}


void MainWindow::on_actionProblem_Set_triggered()
{
    bool state = ui->actionProblem_Set->isChecked();
    ui->actionProblem_Set->setChecked(state);
    showHideInfosTab(ui->tabProblemSet,state);
}


void MainWindow::on_actionIssues_triggered()
{
    bool state = ui->actionIssues->isChecked();
    ui->actionIssues->setChecked(state);
    showHideMessagesTab(ui->tabIssues,state);
}


void MainWindow::on_actionTools_Output_triggered()
{
    bool state = ui->actionTools_Output->isChecked();
    ui->actionTools_Output->setChecked(state);
    showHideMessagesTab(ui->tabToolsOutput,state);
}


void MainWindow::on_actionDebug_Window_triggered()
{
    bool state = ui->actionDebug_Window->isChecked();
    ui->actionDebug_Window->setChecked(state);
    showHideMessagesTab(ui->tabDebug, state);
}


void MainWindow::on_actionSearch_triggered()
{
    bool state = ui->actionSearch->isChecked();
    ui->actionSearch->setChecked(state);
    showHideMessagesTab(ui->tabSearch,state);
}



void MainWindow::on_actionTODO_triggered()
{
    bool state = ui->actionTODO->isChecked();
    ui->actionTODO->setChecked(state);
    showHideMessagesTab(ui->tabTODO,state);
}


void MainWindow::on_actionBookmark_triggered()
{
    bool state = ui->actionBookmark->isChecked();
    ui->actionBookmark->setChecked(state);
    showHideMessagesTab(ui->tabBookmark,state);
}


void MainWindow::on_actionProblem_triggered()
{
    bool state = ui->actionProblem->isChecked();
    ui->actionProblem->setChecked(state);
    showHideMessagesTab(ui->tabProblem,state);
}

void MainWindow::on_actionDelete_Line_triggered()
{
    Editor *e=mEditorList->getEditor();
    if (e) {
        e->deleteLine();
    }
}

void MainWindow::on_actionDuplicate_Line_triggered()
{
    Editor *e=mEditorList->getEditor();
    if (e) {
        e->duplicateLine();
    }
}


void MainWindow::on_actionDelete_Word_triggered()
{
    Editor *e=mEditorList->getEditor();
    if (e) {
        e->deleteWord();
    }
}


void MainWindow::on_actionDelete_to_EOL_triggered()
{
    Editor *e=mEditorList->getEditor();
    if (e) {
        e->deleteToEOL();
    }
}

void MainWindow::on_actionDelete_to_BOL_triggered()
{
    Editor *e=mEditorList->getEditor();
    if (e) {
        e->deleteToBOL();
    }
}


void MainWindow::onOpenCaseValidationOptions()
{
    changeOptions(
                SettingsDialog::tr("Problem Set"),
                SettingsDialog::tr("Program Runner")
                );
}


void MainWindow::on_actionInterrupt_triggered()
{
    if (mDebugger->executing()) {
        //WatchView.Items.BeginUpdate();
        mDebugger->interrupt();
    }
}

void MainWindow::on_actionDelete_Last_Word_triggered()
{
    Editor *e=mEditorList->getEditor();
    if (e) {
        e->deleteToWordStart();
    }
}


void MainWindow::on_actionDelete_to_Word_End_triggered()
{
    Editor *e=mEditorList->getEditor();
    if (e) {
        e->deleteToWordEnd();
    }
}


void MainWindow::on_actionNew_Header_triggered()
{
    if (!mProject)
        return;
    NewHeaderDialog dialog;
    dialog.setPath(mProject->folder());
    QString newFileName;
    int i=1;
    do {
        newFileName = QString("untitled%1").arg(i);
        newFileName += ".h";
        i++;
    } while (QDir(mProject->directory()).exists(newFileName));
    dialog.setHeaderName(newFileName);
    if (dialog.exec()==QDialog::Accepted) {
        QDir dir(dialog.path());
        if (dialog.headerName().isEmpty()
                || !dir.exists())
            return;
        QString headerFilename = includeTrailingPathDelimiter(dialog.path())+dialog.headerName();
        if (fileExists(headerFilename)){
            QMessageBox::critical(this,
                                  tr("Header Exists"),
                                  tr("Header file \"%1\" already exists!").arg(headerFilename));
            return;
        }
        QString header_macro = QFileInfo(dialog.headerName()).baseName().toUpper()+"_H";
        QStringList header;
        QString indents;
        if (pSettings->editor().tabToSpaces()) {
            indents = QString(pSettings->editor().tabWidth(),' ');
        } else {
            indents = "\t";
        }
        header.append(QString("#ifndef %1").arg(header_macro));
        header.append(QString("#define %1").arg(header_macro));
        header.append("");
        header.append("#endif");
        stringsToFile(header, headerFilename);

        PProjectUnit newUnit=mProject->addUnit(headerFilename,mProject->rootNode());
        mProject->saveAll();

        parseFileList(mProject->cppParser());
        setProjectViewCurrentUnit(newUnit);
        updateProjectView();

        openFile(headerFilename);
    }
    pSettings->ui().setNewClassDialogWidth(dialog.width());
    pSettings->ui().setNewClassDialogHeight(dialog.height());
}


void MainWindow::on_actionNew_Class_triggered()
{
    if (!mProject)
        return;
    NewClassDialog dialog(mProject->cppParser());
    dialog.setPath(mProject->folder());
    if (dialog.exec()==QDialog::Accepted) {
        QDir dir(dialog.path());
        if (dialog.className().isEmpty()
                || dialog.sourceName().isEmpty()
                || dialog.headerName().isEmpty()
                || !dir.exists())
            return;
        QString headerFilename = includeTrailingPathDelimiter(dialog.path())+dialog.headerName();
        QString sourceFilename = includeTrailingPathDelimiter(dialog.path())+dialog.sourceName();
        if (fileExists(headerFilename)){
            QMessageBox::critical(this,
                                  tr("Header Exists"),
                                  tr("Header file \"%1\" already exists!").arg(headerFilename));
            return;
        }
        if (fileExists(sourceFilename)){
            QMessageBox::critical(this,
                                  tr("Source Exists"),
                                  tr("Source file \"%1\" already exists!").arg(sourceFilename));
            return;
        }
        QString header_macro = dialog.className().toUpper()+"_H";
        QStringList header;
        QString indents;
        if (pSettings->editor().tabToSpaces()) {
            indents = QString(pSettings->editor().tabWidth(),' ');
        } else {
            indents = "\t";
        }
        header.append(QString("#ifndef %1").arg(header_macro));
        header.append(QString("#define %1").arg(header_macro));
        header.append("");
        if (dialog.baseClass()) {
            header.append(QString("#include \"%1\"").arg(extractRelativePath(mProject->directory(),
                                                                             dialog.baseClass()->fileName)));
            header.append("");
            header.append(QString("class %1 : public %2 {").arg(dialog.className(),
                                                                dialog.baseClass()->fullName));
        } else
            header.append(QString("class %1 {").arg(dialog.className()));
        header.append("public:");
        header.append("");
        header.append("private:");
        header.append("");
        header.append("};");
        header.append("");
        header.append("#endif");
        stringsToFile(header, headerFilename);
        QStringList source;
        source.append(QString("#include \"%1\"").arg(dialog.headerName()));
        source.append("");
        source.append("");
        stringsToFile(source, sourceFilename);

        PProjectUnit newUnit=mProject->addUnit(headerFilename,mProject->rootNode());

        setProjectViewCurrentUnit(newUnit);
        newUnit=mProject->addUnit(sourceFilename,mProject->rootNode());
        setProjectViewCurrentUnit(newUnit);
        mProject->saveAll();
        parseFileList(mProject->cppParser());
        updateProjectView();

        openFile(headerFilename);
        openFile(sourceFilename,false);
    }
    pSettings->ui().setNewHeaderDialogWidth(dialog.width());
    pSettings->ui().setNewHeaderDialogHeight(dialog.height());
}

#ifdef ENABLE_VCS
void MainWindow::on_actionGit_Create_Repository_triggered()
{
    if (ui->treeFiles->isVisible()) {
        GitManager vcsManager;
        vcsManager.createRepository(pSettings->environment().currentFolder());
        //update files view;
        int pos = ui->cbFilesPath->findText(pSettings->environment().currentFolder());
        if (pos>=0) {
            ui->cbFilesPath->setItemIcon(pos, pIconsManager->getIcon(IconsManager::FILESYSTEM_GIT));
        }
        mFileSystemModelIconProvider.setRootFolder(pSettings->environment().currentFolder());
        mFileSystemModel.setIconProvider(&mFileSystemModelIconProvider);
        //update project view
        if (mProject && mProject->folder() == mFileSystemModel.rootPath()) {
            mProject->addUnit(includeTrailingPathDelimiter(mProject->folder())+".gitignore", mProject->rootNode());
        } else if (mProject && mFileSystemModel.index(mProject->folder()).isValid()) {
            mProject->model()->refreshIcons();
        }
    } else if (ui->projectView->isVisible() && mProject) {
        GitManager vcsManager;
        vcsManager.createRepository(mProject->folder());
        QString output;
        vcsManager.add(mProject->folder(), extractFileName(mProject->filename()), output);
        vcsManager.add(mProject->folder(), extractFileName(mProject->options().icon), output);
        foreach (PProjectUnit pUnit, mProject->unitList()) {
            vcsManager.add(mProject->folder(),extractRelativePath(mProject->folder(),pUnit->fileName()),output);
        }
        //update project view
        QString ignoreFile=includeTrailingPathDelimiter(mProject->folder())+".gitignore";
        mProject->addUnit(ignoreFile, mProject->rootNode());
        createFile(ignoreFile);
        mProject->saveAll();
        if (mProject->folder() == mFileSystemModel.rootPath()
                || mFileSystemModel.rootPath().startsWith(includeTrailingPathDelimiter(mProject->folder()), PATH_SENSITIVITY)) {

            //update files view;
            int pos = ui->cbFilesPath->findText(pSettings->environment().currentFolder());
            if (pos>=0) {
                ui->cbFilesPath->setItemIcon(pos, pIconsManager->getIcon(IconsManager::FILESYSTEM_GIT));
            }
            mFileSystemModelIconProvider.update();
            mFileSystemModel.setIconProvider(&mFileSystemModelIconProvider);
        }
    }
}


void MainWindow::on_actionGit_Add_Files_triggered()
{
    if (ui->treeFiles->isVisible()) {
        GitManager vcsManager;
        QModelIndexList indices = ui->treeFiles->selectionModel()->selectedRows();
        QString output;
        foreach (const QModelIndex index,indices) {
            QFileInfo info = mFileSystemModel.fileInfo(index);
            vcsManager.add(info.absolutePath(),info.fileName(),output);
        }
        //update icons in files view
        mFileSystemModelIconProvider.update();
        mFileSystemModel.setIconProvider(&mFileSystemModelIconProvider);
    } else if (ui->projectView->isVisible() && mProject) {
        GitManager vcsManager;
        QModelIndexList indices = ui->projectView->selectionModel()->selectedRows();
        foreach (const QModelIndex index,indices) {
            QModelIndex realIndex = mProjectProxyModel->mapToSource(index);
            ProjectModelNode * node = static_cast<ProjectModelNode*>(realIndex.internalPointer());
            PProjectModelNode folderNode =  mProject->pointerToNode(node);
            if (!folderNode)
                continue;
            if (folderNode->isUnit) {
                PProjectUnit unit = folderNode->pUnit.lock();
                QFileInfo info(unit->fileName());
                QString output;
                vcsManager.add(info.absolutePath(),info.fileName(),output);
            }
            mProject->model()->refreshIcon(index);
        }
    }

    //update icons in files view too
    mFileSystemModelIconProvider.update();
    mFileSystemModel.setIconProvider(&mFileSystemModelIconProvider);
}


void MainWindow::on_actionGit_Commit_triggered()
{
    QString folder;
    if (ui->treeFiles->isVisible()) {
        folder = pSettings->environment().currentFolder();
    } else if (ui->projectView->isVisible() && mProject) {
        folder = mProject->folder();
    }
    if (folder.isEmpty())
        return;

    GitManager vcsManager;
    QStringList conflicts = vcsManager.listConflicts(folder);
    if (!conflicts.isEmpty()) {
        InfoMessageBox infoBox;
        infoBox.setMessage(
                    tr("Can't commit!") + "<br />"
                    +tr("The following files are in conflicting:")+"<br />"
                           + linesToText(conflicts));
        infoBox.exec();
        return;
    }
    QString message = QInputDialog::getText(this,tr("Commit Message"),tr("Commit Message:"));
    if (message.isEmpty()) {
        QMessageBox::critical(this,
                              tr("Commit Failed"),
                              tr("Commit message shouldn't be empty!")
                              );
        return;
    }
    QString output;
    QString userName = vcsManager.getUserName(folder);
    QString userEmail = vcsManager.getUserEmail(folder);
    if (userName.isEmpty() || userEmail.isEmpty()) {
        GitUserConfigDialog dialog(folder);
        if (dialog.exec()!=QDialog::Accepted) {
            QMessageBox::critical(this,
                                  tr("Can't Commit"),
                                  tr("Git needs user info to commit."));
            return;
        }
    }
    if (vcsManager.commit(folder,message,true,output)) {
        //update project view
        if (mProject) {
            mProject->model()->refreshIcons();
        }
        //update files view
        mFileSystemModelIconProvider.update();
        mFileSystemModel.setIconProvider(&mFileSystemModelIconProvider);
    }
    if (!output.isEmpty()) {
        InfoMessageBox infoBox;
        infoBox.showMessage(output);
    }
}


void MainWindow::on_actionGit_Restore_triggered()
{
    QString folder;
    if (ui->treeFiles->isVisible()) {
        folder = pSettings->environment().currentFolder();
    } else if (ui->projectView->isVisible() && mProject) {
        folder = mProject->folder();
    }
    if (folder.isEmpty())
        return;
    GitManager vcsManager;
    QString output;
    if (vcsManager.restore(folder,"",output)) {

        //update project view
        if (mProject) {
            mProject->model()->refreshIcons();
        }
        //update files view
        mFileSystemModelIconProvider.update();
        mFileSystemModel.setIconProvider(&mFileSystemModelIconProvider);
    }
    if (!output.isEmpty()) {
        InfoMessageBox infoBox;
        infoBox.showMessage(output);
    }
}



void MainWindow::on_actionGit_Branch_triggered()
{
    QString folder;
    if (ui->treeFiles->isVisible()) {
        folder = pSettings->environment().currentFolder();
    } else if (ui->projectView->isVisible() && mProject) {
        folder = mProject->folder();
    }
    if (folder.isEmpty())
        return;
    GitBranchDialog dialog(folder);
    if (dialog.exec()==QDialog::Accepted) {
        //update project view
        if (mProject) {
            mProject->model()->refreshIcons();
        }
        //update files view
        setFilesViewRoot(pSettings->environment().currentFolder());
    }
}

void MainWindow::on_actionGit_Merge_triggered()
{
    QString folder;
    if (ui->treeFiles->isVisible()) {
        folder = pSettings->environment().currentFolder();
    } else if (ui->projectView->isVisible() && mProject) {
        folder = mProject->folder();
    }
    if (folder.isEmpty())
        return;
    GitMergeDialog dialog(folder);
    if (dialog.exec()==QDialog::Accepted) {
        //update project view
        if (mProject) {
            mProject->model()->refreshIcons();
        }
        //update files view
        setFilesViewRoot(pSettings->environment().currentFolder());
    }
}


void MainWindow::on_actionGit_Log_triggered()
{
    QString folder;
    if (ui->treeFiles->isVisible()) {
        folder = pSettings->environment().currentFolder();
    } else if (ui->projectView->isVisible() && mProject) {
        folder = mProject->folder();
    }
    if (folder.isEmpty())
        return;
    GitLogDialog dialog(folder);
    if (dialog.exec()==QDialog::Accepted) {
        //update project view
        if (mProject) {
            mProject->model()->refreshIcons();
        }
        //update files view
        setFilesViewRoot(pSettings->environment().currentFolder());
    }
    return ;
}


void MainWindow::on_actionGit_Remotes_triggered()
{
    QString folder;
    if (ui->treeFiles->isVisible()) {
        folder = pSettings->environment().currentFolder();
    } else if (ui->projectView->isVisible() && mProject) {
        folder = mProject->folder();
    }
    if (folder.isEmpty())
        return;
    GitRemoteDialog dialog(folder);
    dialog.exec();
}


void MainWindow::on_actionGit_Fetch_triggered()
{
    QString folder;
    if (ui->treeFiles->isVisible()) {
        folder = pSettings->environment().currentFolder();
    } else if (ui->projectView->isVisible() && mProject) {
        folder = mProject->folder();
    }
    if (folder.isEmpty())
        return;
    GitManager manager;
    QString output;
    if (!manager.fetch(folder,output)) {
        InfoMessageBox infoBox;
        infoBox.showMessage(output);
    }
}


void MainWindow::on_actionGit_Pull_triggered()
{
    QString folder;
    if (ui->treeFiles->isVisible()) {
        folder = pSettings->environment().currentFolder();
    } else if (ui->projectView->isVisible() && mProject) {
        folder = mProject->folder();
    }
    if (folder.isEmpty())
        return;
    GitManager manager;
    QString branch;
    if (!manager.hasRepository(folder,branch))
        return;
    QString remote = manager.getBranchRemote(folder,branch);
    QString output;
    if (remote.isEmpty()) {
        GitRemoteDialog dialog(folder);
        QString remote = dialog.chooseRemote();
        if (remote.trimmed().isEmpty())
            return;
        if (!manager.setBranchUpstream(folder,branch,remote,output)) {
            InfoMessageBox infoBox;
            infoBox.showMessage(output);
            return;
        }
    }
    manager.pull(folder,output);
    if (!output.isEmpty()) {
        InfoMessageBox infoBox;
        infoBox.showMessage(output);
    }
}


void MainWindow::on_actionGit_Push_triggered()
{
    QString folder;
    if (ui->treeFiles->isVisible()) {
        folder = pSettings->environment().currentFolder();
    } else if (ui->projectView->isVisible() && mProject) {
        folder = mProject->folder();
    }
    if (folder.isEmpty())
        return;
    GitManager manager;
    QString branch;
    if (!manager.hasRepository(folder,branch))
        return;
    QString remote = manager.getBranchRemote(folder,branch);
    QString output;
    if (remote.isEmpty()) {
        GitRemoteDialog dialog(folder);
        QString remote = dialog.chooseRemote();
        if (remote.trimmed().isEmpty())
            return;
        manager.push(folder,remote,branch,output);
        if (!output.isEmpty()) {
            InfoMessageBox infoBox;
            infoBox.showMessage(output);
        }
    } else {
        if (!output.isEmpty()) {
            InfoMessageBox infoBox;
            infoBox.showMessage(output);
        }
    }
}

#endif

void MainWindow::on_actionWebsite_triggered()
{
    if (pSettings->environment().language()=="zh_CN") {
        QDesktopServices::openUrl(QUrl("http://royqh.net/redpandacpp/"));
    } else {
        QDesktopServices::openUrl(QUrl("https://sourceforge.net/projects/redpanda-cpp/"));
    }
}

void MainWindow::on_actionFilesView_Hide_Non_Support_Files_toggled(bool /* arg1 */)
{
    mFileSystemModel.setNameFilterDisables(!ui->actionFilesView_Hide_Non_Support_Files->isChecked());
    if (!mFileSystemModel.nameFilterDisables()) {
        mFileSystemModel.setNameFilters(pSystemConsts->defaultFileNameFilters());
    } else {
        mFileSystemModel.setNameFilters(QStringList());
    }
    if (pSettings->environment().hideNonSupportFilesInFileView()
            != ui->actionFilesView_Hide_Non_Support_Files->isChecked()) {
        pSettings->environment().setHideNonSupportFilesInFileView(ui->actionFilesView_Hide_Non_Support_Files->isChecked());
        pSettings->environment().save();
    }
}


void MainWindow::on_actionToggle_Block_Comment_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->toggleBlockComment();
    }
}


void MainWindow::on_actionMatch_Bracket_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->matchBracket();
    }
}


void MainWindow::on_btnProblemCaseInputFileName_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Choose Input Data File"),
                QString(),
                tr("All files (*.*)"));
    if (!fileName.isEmpty()) {
        QModelIndex idx = ui->tblProblemCases->currentIndex();
        POJProblemCase problemCase = mOJProblemModel.getCase(idx.row());
        if (!problemCase)
            return;
        if (problemCase->inputFileName == fileName)
            return;
        problemCase->inputFileName = fileName;
        if (problemCase->expectedOutputFileName.isEmpty()
                && problemCase->expected.isEmpty()
                && QFileInfo(fileName).suffix()=="in") {
            QString expectedFileName;
            expectedFileName = fileName.mid(0,fileName.length()-2)+"ans";
            if (fileExists(expectedFileName)) {
                problemCase->expectedOutputFileName = expectedFileName;
            } else {
                expectedFileName = fileName.mid(0,fileName.length()-2)+"out";
                if (fileExists(expectedFileName))
                    problemCase->expectedOutputFileName = expectedFileName;
            }
        }
        fillProblemCaseInputAndExpected(problemCase);
    }
}


void MainWindow::on_btnProblemCaseClearExpectedOutputFileName_clicked()
{
    QModelIndex idx = ui->tblProblemCases->currentIndex();
    POJProblemCase problemCase = mOJProblemModel.getCase(idx.row());
    if (!problemCase)
        return;
    problemCase->expectedOutputFileName = "";
    fillProblemCaseInputAndExpected(problemCase);
}


void MainWindow::on_btnProblemCaseClearInputFileName_clicked()
{
    QModelIndex idx = ui->tblProblemCases->currentIndex();
    POJProblemCase problemCase = mOJProblemModel.getCase(idx.row());
    if (!problemCase)
        return;
    problemCase->inputFileName = "";
    fillProblemCaseInputAndExpected(problemCase);
}


void MainWindow::on_btnProblemCaseExpectedOutputFileName_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Choose Expected Output Data File"),
                QString(),
                tr("All files (*.*)"));
    if (!fileName.isEmpty()) {
        QModelIndex idx = ui->tblProblemCases->currentIndex();
        POJProblemCase problemCase = mOJProblemModel.getCase(idx.row());
        if (!problemCase)
            return;
        if (problemCase->expectedOutputFileName == fileName)
            return;
        problemCase->expectedOutputFileName = fileName;
        fillProblemCaseInputAndExpected(problemCase);
    }
}


void MainWindow::on_txtProblemCaseOutput_cursorPositionChanged()
{
    QTextCursor cursor = ui->txtProblemCaseOutput->textCursor();
    int val = ui->txtProblemCaseOutput->verticalScrollBar()->value();
    int line = cursor.block().firstLineNumber();
    //ui->lblProblemCaseOutput->setText(tr("Line %1").arg(cursor.block().firstLineNumber()+1));

    QTextBlock block = ui->txtProblemCaseExpected->document()->findBlockByLineNumber(line);
    if (!block.isValid())
        return;
    cursor = QTextCursor(block);
    ui->txtProblemCaseExpected->setTextCursor(cursor);
    ui->txtProblemCaseExpected->verticalScrollBar()->setValue(val);
}


void MainWindow::on_txtProblemCaseExpected_cursorPositionChanged()
{
    QTextCursor cursor = ui->txtProblemCaseExpected->textCursor();
    //ui->lblProblemCaseExpected->setText(tr("Line %1").arg(cursor.block().firstLineNumber()+1));
}


void MainWindow::on_txtProblemCaseInput_cursorPositionChanged()
{
    QTextCursor cursor = ui->txtProblemCaseInput->textCursor();
    //ui->lblProblemCaseInput->setText(tr("Line %1").arg(cursor.block().firstLineNumber()+1));
}


void MainWindow::on_actionMove_Selection_Up_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->moveSelUp();
    }
}


void MainWindow::on_actionMove_Selection_Down_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->moveSelDown();
    }
}

void MainWindow::on_actionConvert_to_UTF_8_BOM_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    if (QMessageBox::warning(this,tr("Confirm Convertion"),
                   tr("The editing file will be saved using %1 encoding. <br />This operation can't be reverted. <br />Are you sure to continue?")
                   .arg(ENCODING_UTF8_BOM),
                   QMessageBox::Yes, QMessageBox::No)!=QMessageBox::Yes)
        return;
    editor->convertToEncoding(ENCODING_UTF8_BOM);
}


void MainWindow::on_actionEncode_in_UTF_8_BOM_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    try {
        editor->setEncodingOption(ENCODING_UTF8_BOM);
    } catch(FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
}


void MainWindow::on_actionCompiler_Options_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (!mProject || (editor && !editor->inProject())) {
        changeOptions(
                   SettingsDialog::tr("Compiler Set"),
                   SettingsDialog::tr("Compiler")
                   );
    } else {
        changeProjectOptions(
                   SettingsDialog::tr("Compiler Set"),
                   SettingsDialog::tr("Project")
                   );
    }
}

void MainWindow::on_dockExplorer_dockLocationChanged(const Qt::DockWidgetArea &area)
{
    setDockExplorerToArea(area);
}


void MainWindow::on_dockMessages_dockLocationChanged(const Qt::DockWidgetArea &area)
{
    setDockMessagesToArea(area);
}


void MainWindow::on_actionToggle_Explorer_Panel_triggered()
{
    stretchExplorerPanel(ui->tabExplorer->isShrinked());
}


void MainWindow::on_actionToggle_Messages_Panel_triggered()
{
    stretchMessagesPanel(ui->tabMessages->isShrinked());
}

void MainWindow::on_actionRaylib_Manual_triggered()
{
    if (pSettings->environment().language()=="zh_CN") {
        QDesktopServices::openUrl(QUrl("https://zhuanlan.zhihu.com/p/458335134"));
    } else {
        QDesktopServices::openUrl(QUrl("https://www.raylib.com/"));
    }
}


void MainWindow::on_actionSelect_Word_triggered()
{
    Editor* e=mEditorList->getEditor();
    if (e) {
        e->selectWord();
    }
}


void MainWindow::on_actionGo_to_Line_triggered()
{
    Editor* e=mEditorList->getEditor();
    if (!e)
        return;
    bool ok;
    int lineNo=QInputDialog::getInt(e,tr("Go to Line"),tr("Line"),
                                    e->caretY(),1,e->lineCount(),
                                    1,&ok);
    if (ok && lineNo!=e->caretY()) {
        e->setCaretPosition(lineNo,1);
        e->setFocus();
    }
}

void MainWindow::on_actionNew_Template_triggered()
{
    if (!mProject)
        return;
    NewTemplateDialog dialog(this);
    if (dialog.exec()==QDialog::Accepted) {
        QDir folder(
                    includeTrailingPathDelimiter(
                        pSettings->dirs().config(Settings::Dirs::DataType::Template))
                    +dialog.getName());
        if (folder.exists()) {
            if (QMessageBox::warning(this,
                                     tr("Template Exists"),
                                     tr("Template %1 already exists. Do you want to overwrite?").arg(dialog.getName()),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No
                                     )!=QMessageBox::Yes)
                return;
        }

        mProject->saveAsTemplate(
                    folder.absolutePath(),
                    dialog.getName(),
                    dialog.getDescription(),
                    dialog.getCategory()
                    );
    }
}

bool MainWindow::closingProject() const
{
    return mClosingProject;
}

const std::shared_ptr<VisitHistoryManager> &MainWindow::visitHistoryManager() const
{
    return mVisitHistoryManager;
}

bool MainWindow::isQuitting() const
{
    return mQuitting;
}

bool MainWindow::isClosingAll() const
{
    return mClosingAll;
}


void MainWindow::on_actionGoto_block_start_triggered()
{
    Editor *editor=mEditorList->getEditor();
    if (editor)
        editor->gotoBlockStart();
}


void MainWindow::on_actionGoto_block_end_triggered()
{
    Editor *editor=mEditorList->getEditor();
    if (editor)
        editor->gotoBlockEnd();
}


void MainWindow::on_actionSwitchHeaderSource_triggered()
{
    Editor *editor=mEditorList->getEditor();
    QString file=switchHeaderSourceTarget(editor);
    if (!file.isEmpty()) {
        openFile(file);
    }
}

SearchDialog *MainWindow::searchDialog() const
{
    return mSearchDialog;
}


void MainWindow::on_actionGenerate_Assembly_triggered()
{
    doGenerateAssembly();
}


void MainWindow::onImportFPSProblemSet()
{
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Import FPS Problem Set"),
                QString(),
                tr("FPS Problem Set Files (*.fps;*.xml)"));
    if (!fileName.isEmpty()) {
        try {
            QList<POJProblem> problems = importFreeProblemSet(fileName);
            mOJProblemSetModel.addProblems(problems);
            ui->lblProblemSet->setText(mOJProblemSetModel.name());
            ui->lstProblemSet->setCurrentIndex(mOJProblemSetModel.index(0,0));
        } catch (FileError& error) {
            QMessageBox::critical(this,tr("Load Error"),
                                  error.reason());
        }
    }
}



void MainWindow::on_actionTrim_trailing_spaces_triggered()
{
    Editor * e = mEditorList->getEditor();
    if (e) {
        e->trimTrailingSpaces();
    }
}


void MainWindow::onExportFPSProblemSet()
{
    QString fileName = QFileDialog::getSaveFileName(
                this,
                tr("Export FPS Problem Set"),
                QString(),
                tr("FPS Problem Set Files (*.fps)"));
    if (!fileName.isEmpty()) {
        try {
            exportFreeProblemSet(mOJProblemSetModel.problems(),fileName);
        } catch (FileError& error) {
            QMessageBox::critical(this,tr("Export Error"),
                                  error.reason());
        }
    }
}


void MainWindow::on_actionToggle_Readonly_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && !editor->modified()) {
        editor->setReadOnly(!editor->readOnly());
        editor->updateCaption();
        updateEditorActions(editor);
        updateForStatusbarModeInfo(editor);
    }
}


void MainWindow::on_actionSubmit_Issues_triggered()
{
    if (pSettings->environment().language()=="zh_CN") {
        QDesktopServices::openUrl(QUrl("https://gitee.com/royqh1979/RedPanda-CPP/issues"));
    } else {
        QDesktopServices::openUrl(QUrl("https://github.com/royqh1979/RedPanda-CPP/issues"));
    }
}


void MainWindow::on_actionDocument_triggered()
{
    QDesktopServices::openUrl(QUrl("http://royqh.net/redpandacpp//docsy/docs/usage"));
}


void MainWindow::on_actionNew_GAS_File_triggered()
{
    if (mProject) {
        if (QMessageBox::question(this,
                                  tr("New Project File?"),
                                  tr("Do you want to add the new file to the project?"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes) == QMessageBox::Yes) {
            newProjectUnitFile("s");
            return;
        }
    }
    newEditor("s");
}

void MainWindow::on_actionGNU_Assembler_Manual_triggered()
{
    QFileInfo fileInfo{includeTrailingPathDelimiter(pSettings->dirs().appDir())+
                       QString{"Using GNU Assembler.pdf"}};
    if (fileInfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
        return;
    }
    QDesktopServices::openUrl(QUrl("https://sourceware.org/binutils/docs/as/index.html"));
}

#ifdef ARCH_X86_64
void MainWindow::on_actionx86_Assembly_Language_Reference_Manual_triggered()
{
    QFileInfo fileInfo{includeTrailingPathDelimiter(pSettings->dirs().appDir())+
                       QString{"x86 Assembly Language Reference Manual.pdf"}};
    if (fileInfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
        return;
    }
    QDesktopServices::openUrl(QUrl("https://docs.oracle.com/cd/E53394_01/html/E54851/index.html"));
}
#endif

#ifdef ARCH_X86
void MainWindow::on_actionIA_32_Assembly_Language_Reference_Manual_triggered()
{
    QFileInfo fileInfo{includeTrailingPathDelimiter(pSettings->dirs().appDir())+
                       QString{"IA-32 Assembly Language Reference Manual.pdf"}};
    if (fileInfo.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
        return;
    }
    QDesktopServices::openUrl(QUrl("https://docs.oracle.com/cd/E19455-01/806-3773/index.html"));
}
#endif

void MainWindow::on_actionAdd_Watchpoint_triggered()
{
    QString s = "";
    bool isOk;
    s=QInputDialog::getText(this,
                              tr("Watchpoint variable name"),
                              tr("Stop execution when the following variable is modified (it must be visible from the currect scope):"),
                            QLineEdit::Normal,
                            s,&isOk);
    if (!isOk)
        return;
    s = s.trimmed();
    mDebugger->addWatchpoint(s);
}


void MainWindow::on_actionNew_Text_File_triggered()
{
    if (mProject) {
        if (QMessageBox::question(this,
                                  tr("New Project File?"),
                                  tr("Do you want to add the new file to the project?"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes) == QMessageBox::Yes) {
            newProjectUnitFile("txt");
            return;
        }
    }
    newEditor("txt");
}


void MainWindow::on_actionPage_Up_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->pageUp();
    }
}


void MainWindow::on_actionPage_Down_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->pageDown();
    }
}


void MainWindow::on_actionGoto_Line_Start_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->gotoLineStart();
    }
}


void MainWindow::on_actionGoto_Line_End_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->gotoLineEnd();
    }
}


void MainWindow::on_actionGoto_File_Start_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->gotoFileStart();
    }
}


void MainWindow::on_actionGoto_File_End_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->gotoFileEnd();
    }
}


void MainWindow::on_actionPage_Up_and_Select_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->pageUpAndSelect();;
    }
}


void MainWindow::on_actionPage_Down_and_Select_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->pageDownAndSelect();
    }
}


void MainWindow::on_actionGoto_Page_Start_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->gotoPageStart();
    }
}


void MainWindow::on_actionGoto_Page_End_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->gotoPageEnd();
    }
}


void MainWindow::on_actionGoto_Page_Start_and_Select_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->selectToPageStart();
    }
}


void MainWindow::on_actionGoto_Page_End_and_Select_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->selectToPageEnd();
    }
}


void MainWindow::on_actionGoto_Line_Start_and_Select_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->selectToLineStart();
    }
}


void MainWindow::on_actionGoto_Line_End_and_Select_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->selectToLineEnd();
    }
}


void MainWindow::on_actionGoto_File_Start_and_Select_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->selectToFileStart();
    }
}


void MainWindow::on_actionGoto_File_End_and_Select_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor && editor->hasFocus()) {
        editor->selectToFileEnd();
    }
}


void MainWindow::on_actionClose_Others_triggered()
{
    mClosing = true;
    Editor* e = mEditorList->getEditor();
    if (e) {
        mEditorList->closeOthers(e);
    }
    mClosing = false;
}

void MainWindow::on_actionOI_Wiki_triggered()
{
    QDesktopServices::openUrl(QUrl("https://oi-wiki.org/"));
}


void MainWindow::on_actionTurtle_Graphics_Manual_triggered()
{
    QDesktopServices::openUrl(QUrl("https://zhuanlan.zhihu.com/p/538666844"));
}

bool MainWindow::openingProject() const
{
    return mOpeningProject;
}

bool MainWindow::openingFiles() const
{
    return mOpeningFiles;
}


void MainWindow::on_cbProblemCaseValidateType_currentIndexChanged(int index)
{
    pSettings->executor().setProblemCaseValidateType((ProblemCaseValidateType)index);
    pSettings->executor().save();
}


void MainWindow::on_actionToggle_Bookmark_triggered()
{
    Editor* editor = mEditorList->getEditor();
    if (editor) {
        if (editor->lineCount()<=0)
            return;
        int line = editor->caretY();
        editor->toggleBookmark(line);
        if (editor->hasBookmark(line)) {
            QString desc = QInputDialog::getText(editor,tr("Bookmark Description"),
                                                 tr("Description:"),QLineEdit::Normal,
                                                 editor->lineText(line).trimmed());
            desc = desc.trimmed();
            mBookmarkModel->addBookmark(editor->filename(),line,desc,editor->inProject());
        } else
            mBookmarkModel->removeBookmark(editor->filename(),line,editor->inProject());
    }
}


void MainWindow::on_actionCode_Completion_triggered()
{
    Editor* editor = mEditorList->getEditor();
    if (editor) {
        editor->showCodeCompletion();
    }
}

