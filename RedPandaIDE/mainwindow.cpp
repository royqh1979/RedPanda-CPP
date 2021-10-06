#include <windows.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editorlist.h"
#include "editor.h"
#include "systemconsts.h"
#include "settings.h"
#include "qsynedit/Constants.h"
#include "debugger.h"
#include "widgets/cpudialog.h"
#include "widgets/filepropertiesdialog.h"
#include "project.h"
#include "projecttemplate.h"
#include "widgets/newprojectdialog.h"
#include "platform.h"
#include "widgets/aboutdialog.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QTranslator>

#include "settingsdialog/settingsdialog.h"
#include "compiler/compilermanager.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QTextCodec>
#include <QDebug>
#include "cpprefacter.h"

#include <widgets/searchdialog.h>

MainWindow* pMainWindow;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      mSearchDialog(nullptr),
      mQuitting(false),
      mOpenClosingBottomPanel(false),
      mOpenClosingLeftPanel(false),
      mCheckSyntaxInBack(false),
      mClosing(false),
      mSystemTurnedOff(false)
{
    ui->setupUi(this);
    // status bar
    mFileInfoStatus=new QLabel();
    mFileEncodingStatus = new QLabel();
    mFileModeStatus = new QLabel();
    mFileInfoStatus->setStyleSheet("margin-left:10px; margin-right:10px");
    mFileEncodingStatus->setStyleSheet("margin-left:10px; margin-right:10px");
    mFileModeStatus->setStyleSheet("margin-left:10px; margin-right:10px");
    ui->statusbar->insertPermanentWidget(0,mFileModeStatus);
    ui->statusbar->insertPermanentWidget(0,mFileEncodingStatus);
    ui->statusbar->insertPermanentWidget(0,mFileInfoStatus);
    mEditorList = new EditorList(ui->EditorTabsLeft,
                                 ui->EditorTabsRight,
                                 ui->splitterEditorPanel,
                                 ui->EditorPanel);
    mProject = nullptr;
    setAcceptDrops(true);
    setupActions();
    ui->EditorTabsRight->setVisible(false);

    mCompilerSet = new QComboBox();
    mCompilerSet->setMinimumWidth(200);
    ui->toolbarCompilerSet->addWidget(mCompilerSet);
    connect(mCompilerSet,QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCompilerSetChanged);
    updateCompilerSet();

    mCompilerManager = new CompilerManager(this);
    mDebugger = new Debugger(this);

    ui->tblBreakpoints->setModel(mDebugger->breakpointModel());
    ui->tblStackTrace->setModel(mDebugger->backtraceModel());
    ui->watchView->setModel(mDebugger->watchModel());

//    ui->actionIndent->setShortcut(Qt::Key_Tab);
//    ui->actionUnIndent->setShortcut(Qt::Key_Tab | Qt::ShiftModifier);

    ui->tableIssues->setErrorColor(QColor("Red"));
    ui->tableIssues->setWarningColor(QColor("Orange"));


    mMenuNew = new QMenu();
    mMenuNew->setTitle(tr("New"));
    mMenuNew->addAction(ui->actionNew);
    mMenuNew->addAction(ui->actionNew_Project);
    ui->menuFile->insertMenu(ui->actionOpen,mMenuNew);

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
    mMenuInsertCodeSnippet->setTitle("Insert Snippet");
    ui->menuCode->insertMenu(ui->actionReformat_Code,mMenuInsertCodeSnippet);
    ui->menuCode->insertSeparator(ui->actionReformat_Code);
    connect(mMenuInsertCodeSnippet,&QMenu::aboutToShow,
            this, &MainWindow::onShowInsertCodeSnippetMenu);

    mCPUDialog = nullptr;

    updateProjectView();
    updateEditorActions();
    updateCaretActions();    
    applySettings();
    applyUISettings();


    connect(ui->debugConsole,&QConsole::commandInput,this,&MainWindow::onDebugCommandInput);
    connect(ui->cbEvaluate->lineEdit(), &QLineEdit::returnPressed,
            this, &MainWindow::onDebugEvaluateInput);
    connect(ui->cbMemoryAddress->lineEdit(), &QLineEdit::returnPressed,
            this, &MainWindow::onDebugMemoryAddressInput);

    mTodoParser = std::make_shared<TodoParser>();
    mSymbolUsageManager = std::make_shared<SymbolUsageManager>();
    mSymbolUsageManager->load();
    mCodeSnippetManager = std::make_shared<CodeSnippetsManager>();
    mCodeSnippetManager->load();
    mSearchResultTreeModel = std::make_shared<SearchResultTreeModel>(&mSearchResultModel);
    mSearchResultListModel = std::make_shared<SearchResultListModel>(&mSearchResultModel);
    mSearchViewDelegate = std::make_shared<SearchResultTreeViewDelegate>(mSearchResultTreeModel);
    ui->cbSearchHistory->setModel(mSearchResultListModel.get());
    ui->searchView->setModel(mSearchResultTreeModel.get());
    ui->searchView->setItemDelegate(mSearchViewDelegate.get());
    ui->tableTODO->setModel(&mTodoModel);
    connect(mSearchResultTreeModel.get() , &QAbstractItemModel::modelReset,
            ui->searchView,&QTreeView::expandAll);
    ui->replacePanel->setVisible(false);

    //class browser
    ui->classBrowser->setModel(&mClassBrowserModel);

    connect(&mFileSystemWatcher,&QFileSystemWatcher::fileChanged,
            this, &MainWindow::onFileChanged);

    mStatementColors = std::make_shared<QHash<StatementKind, QColor> >();
    mCompletionPopup = std::make_shared<CodeCompletionPopup>();
    mCompletionPopup->setColors(mStatementColors);
    mHeaderCompletionPopup = std::make_shared<HeaderCompletionPopup>();
    mFunctionTip = std::make_shared<FunctionTooltipWidget>();

    mClassBrowserModel.setColors(mStatementColors);
    updateAppTitle();

    connect(&mAutoSaveTimer, &QTimer::timeout,
            this, &MainWindow::onAutoSaveTimeout);
    resetAutoSaveTimer();

    connect(ui->menuFile, &QMenu::aboutToShow,
            this,&MainWindow::rebuildOpenedFileHisotryMenu);

    connect(ui->menuProject, &QMenu::aboutToShow,
            this, &MainWindow::updateProjectActions);


    buildContextMenus();

    updateEditorColorSchemes();

    updateShortcuts();
}

MainWindow::~MainWindow()
{
    delete mEditorList;
    delete ui;
}

void MainWindow::updateForEncodingInfo() {
    Editor * editor = mEditorList->getEditor();
    if (editor!=NULL) {
        if (editor->encodingOption() != editor->fileEncoding()) {
            mFileEncodingStatus->setText(
                        QString("%1(%2)")
                        .arg(QString(editor->encodingOption())
                             ,QString(editor->fileEncoding())));
        } else {
            mFileEncodingStatus->setText(
                        QString("%1")
                        .arg(QString(editor->encodingOption()))
                        );
        }
        ui->actionAuto_Detect->setChecked(editor->encodingOption() == ENCODING_AUTO_DETECT);
        ui->actionEncode_in_ANSI->setChecked(editor->encodingOption() == ENCODING_SYSTEM_DEFAULT);
        ui->actionEncode_in_UTF_8->setChecked(editor->encodingOption() == ENCODING_UTF8);
    } else {
        mFileEncodingStatus->setText("");
        ui->actionAuto_Detect->setChecked(false);
        ui->actionEncode_in_ANSI->setChecked(false);
        ui->actionEncode_in_UTF_8->setChecked(false);
    }
}

void MainWindow::updateEditorSettings()
{
    mEditorList->applySettings();
}

void MainWindow::updateEditorActions()
{
    Editor* e = mEditorList->getEditor();
    if (e==nullptr) {
        ui->actionAuto_Detect->setEnabled(false);
        ui->actionEncode_in_ANSI->setEnabled(false);
        ui->actionEncode_in_UTF_8->setEnabled(false);
        ui->actionConvert_to_ANSI->setEnabled(false);
        ui->actionConvert_to_UTF_8->setEnabled(false);
        ui->actionCopy->setEnabled(false);
        ui->actionCut->setEnabled(false);
        ui->actionFoldAll->setEnabled(false);
        ui->actionIndent->setEnabled(false);
        ui->actionPaste->setEnabled(false);
        ui->actionRedo->setEnabled(false);
        ui->actionSave->setEnabled(false);
        ui->actionSaveAs->setEnabled(false);
        ui->actionPrint->setEnabled(false);
        ui->actionSelectAll->setEnabled(false);
        ui->actionToggleComment->setEnabled(false);
        ui->actionUnIndent->setEnabled(false);
        ui->actionUndo->setEnabled(false);
        ui->actionUnfoldAll->setEnabled(false);
        ui->actionFind->setEnabled(false);
        ui->actionReplace->setEnabled(false);
        ui->actionFind_Next->setEnabled(false);
        ui->actionFind_Previous->setEnabled(false);

        //code
        ui->actionReformat_Code->setEnabled(false);

        ui->actionClose->setEnabled(false);
        ui->actionClose_All->setEnabled(false);
    } else {
        ui->actionAuto_Detect->setEnabled(true);
        ui->actionEncode_in_ANSI->setEnabled(true);
        ui->actionEncode_in_UTF_8->setEnabled(true);
        ui->actionConvert_to_ANSI->setEnabled(e->encodingOption()!=ENCODING_SYSTEM_DEFAULT && e->fileEncoding()!=ENCODING_SYSTEM_DEFAULT);
        ui->actionConvert_to_UTF_8->setEnabled(e->encodingOption()!=ENCODING_UTF8 && e->fileEncoding()!=ENCODING_UTF8);

        ui->actionCopy->setEnabled(e->selAvail());
        ui->actionCut->setEnabled(e->selAvail());
        ui->actionFoldAll->setEnabled(e->lines()->count()>0);
        ui->actionIndent->setEnabled(!e->readOnly());

        ui->actionPaste->setEnabled(!e->readOnly() && !QGuiApplication::clipboard()->text().isEmpty());
        ui->actionRedo->setEnabled(e->canRedo());
        ui->actionUndo->setEnabled(e->canUndo());
        ui->actionSave->setEnabled(e->modified());
        ui->actionSaveAs->setEnabled(true);
        ui->actionPrint->setEnabled(true);
        ui->actionSelectAll->setEnabled(e->lines()->count()>0);
        ui->actionToggleComment->setEnabled(!e->readOnly() && e->lines()->count()>0);
        ui->actionUnIndent->setEnabled(!e->readOnly() && e->lines()->count()>0);
        ui->actionUnfoldAll->setEnabled(e->lines()->count()>0);

        ui->actionFind->setEnabled(true);
        ui->actionReplace->setEnabled(true);
        ui->actionFind_Next->setEnabled(true);
        ui->actionFind_Previous->setEnabled(true);

        //code
        ui->actionReformat_Code->setEnabled(true);

        ui->actionClose->setEnabled(true);
        ui->actionClose_All->setEnabled(true);
    }    

    updateCompileActions();

}

void MainWindow::updateProjectActions()
{
    bool hasProject = (mProject != nullptr);
    ui->actionView_Makefile->setEnabled(hasProject);
    ui->actionProject_New_File->setEnabled(hasProject);
    ui->actionAdd_to_project->setEnabled(hasProject);
    ui->actionRemove_from_project->setEnabled(hasProject && ui->projectView->selectionModel()->selectedIndexes().count()>0);
    ui->actionMakeClean->setEnabled(hasProject);
    ui->actionProject_options->setEnabled(hasProject);
    ui->actionClose_Project->setEnabled(hasProject);
    ui->actionProject_Open_Folder_In_Explorer->setEnabled(hasProject);
    ui->actionProject_Open_In_Terminal->setEnabled(hasProject);
    updateCompileActions();
}

void MainWindow::updateCompileActions()
{
    bool hasProject = (mProject!=nullptr);
    bool editorCanCompile = false;
    Editor * e = mEditorList->getEditor();
    if (e) {
        FileType fileType = getFileType(e->filename());
        if (fileType == FileType::CSource
                || fileType == FileType::CppSource || e->isNew())
        editorCanCompile = true;
    }
    if (mCompilerManager->compiling() || mCompilerManager->running() || mDebugger->executing()
         || (!hasProject && !editorCanCompile)   ) {
        ui->actionCompile->setEnabled(false);
        ui->actionCompile_Run->setEnabled(false);
        ui->actionRun->setEnabled(false);
        ui->actionRebuild->setEnabled(false);
        ui->actionDebug->setEnabled(false);
    } else {
        ui->actionCompile->setEnabled(true);
        ui->actionCompile_Run->setEnabled(true);
        ui->actionRun->setEnabled(true);
        ui->actionRebuild->setEnabled(true);

        ui->actionDebug->setEnabled(true);
    }
    if (!mDebugger->executing()) {
        disableDebugActions();
    }
    ui->actionStop_Execution->setEnabled(mCompilerManager->running() || mDebugger->executing());

    //it's not a compile action, but put here for convinience
    ui->actionSaveAll->setEnabled(mProject!=nullptr
            || mEditorList->pageCount()>0);

}

void MainWindow::updateEditorColorSchemes()
{
    mEditorList->applyColorSchemes(pSettings->editor().colorScheme());
    QString schemeName = pSettings->editor().colorScheme();
    //color for code completion popup
    PColorSchemeItem item;
    item = pColorManager->getItem(schemeName, SYNS_AttrFunction);
    if (item) {
        mStatementColors->insert(StatementKind::skFunction,item->foreground());
        mStatementColors->insert(StatementKind::skConstructor,item->foreground());
        mStatementColors->insert(StatementKind::skDestructor,item->foreground());
    }
    item = pColorManager->getItem(schemeName, SYNS_AttrClass);
    if (item) {
        mStatementColors->insert(StatementKind::skClass,item->foreground());
        mStatementColors->insert(StatementKind::skTypedef,item->foreground());
        mStatementColors->insert(StatementKind::skAlias,item->foreground());
    }
    item = pColorManager->getItem(schemeName, SYNS_AttrIdentifier);
    if (item) {
        mStatementColors->insert(StatementKind::skEnumType,item->foreground());
        mStatementColors->insert(StatementKind::skEnumClassType,item->foreground());
    }
    item = pColorManager->getItem(schemeName, SYNS_AttrVariable);
    if (item) {
        mStatementColors->insert(StatementKind::skVariable,item->foreground());
    }
    item = pColorManager->getItem(schemeName, SYNS_AttrLocalVariable);
    if (item) {
        mStatementColors->insert(StatementKind::skLocalVariable,item->foreground());
        mStatementColors->insert(StatementKind::skParameter,item->foreground());
    }
    item = pColorManager->getItem(schemeName, SYNS_AttrGlobalVariable);
    if (item) {
        mStatementColors->insert(StatementKind::skGlobalVariable,item->foreground());
    }
    item = pColorManager->getItem(schemeName, SYNS_AttrPreprocessor);
    if (item) {
        mStatementColors->insert(StatementKind::skPreprocessor,item->foreground());
        mStatementColors->insert(StatementKind::skEnum,item->foreground());
        mHeaderCompletionPopup->setSuggestionColor(item->foreground());
    }
    item = pColorManager->getItem(schemeName, SYNS_AttrReservedWord);
    if (item) {
        mStatementColors->insert(StatementKind::skKeyword,item->foreground());
        mStatementColors->insert(StatementKind::skUserCodeSnippet,item->foreground());
    }
    item = pColorManager->getItem(schemeName, SYNS_AttrString);
    if (item) {
        mStatementColors->insert(StatementKind::skNamespace,item->foreground());
        mStatementColors->insert(StatementKind::skNamespaceAlias,item->foreground());
    }
}

void MainWindow::applySettings()
{
    changeTheme(pSettings->environment().theme());
    QFont font(pSettings->environment().interfaceFont(),
               pSettings->environment().interfaceFontSize());
    font.setStyleStrategy(QFont::PreferAntialias);
    QApplication * app = dynamic_cast<QApplication*>(QApplication::instance());
    app->setFont(font);
    this->setFont(font);
    updateDebuggerSettings();
}

void MainWindow::applyUISettings()
{
    const Settings::UI& settings = pSettings->ui();
    restoreGeometry(settings.mainWindowGeometry());
    restoreState(settings.mainWindowState());
    //we can show/hide left/bottom panels here, cause mainwindow layout is not calculated
//    ui->tabMessages->setCurrentIndex(settings.bottomPanelIndex());
//    if (settings.bottomPanelOpenned()) {
//        mBottomPanelHeight = settings.bottomPanelHeight();
//        openCloseBottomPanel(true);
//    } else {
//        openCloseBottomPanel(false);
//        mBottomPanelHeight = settings.bottomPanelHeight();
//    }
//    ui->tabInfos->setCurrentIndex(settings.leftPanelIndex());
//    if (settings.leftPanelOpenned()) {
//        mLeftPanelWidth = settings.leftPanelWidth();
//        openCloseLeftPanel(true);
//    } else {
//        openCloseLeftPanel(false);
//        mLeftPanelWidth = settings.leftPanelWidth();
//    }
}

QFileSystemWatcher *MainWindow::fileSystemWatcher()
{
    return &mFileSystemWatcher;
}

void MainWindow::removeActiveBreakpoints()
{
    for (int i=0;i<mEditorList->pageCount();i++) {
        Editor* e= (*mEditorList)[i];
        e->removeBreakpointFocus();
    }
}

void MainWindow::setActiveBreakpoint(QString FileName, int Line, bool setFocus)
{
    removeActiveBreakpoints();


    // Then active the current line in the current file
    FileName.replace('/',QDir::separator());
    Editor *e = mEditorList->getEditorByFilename(FileName);
    if (e!=nullptr) {
        e->setActiveBreakpointFocus(Line,setFocus);
    }
    if (setFocus) {
        this->activateWindow();
    }
}

void MainWindow::updateAppTitle()
{
    QString appName("Red Panda Dev-C++");
    Editor *e = mEditorList->getEditor();
    QCoreApplication *app = QApplication::instance();
    if (e && !e->inProject()) {
        QString str;
        if (e->modified())
          str = e->filename() + " [*]";
        else
          str = e->filename();
        if (mDebugger->executing()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,tr("Debugging"),appName,DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Debugging"),appName));
        } else if (mCompilerManager->running()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,tr("Running"),appName,DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Running"),appName));
        } else if (mCompilerManager->compiling()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,tr("Compiling"),appName,DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Compiling"),appName));
        } else {
            this->setWindowTitle(QString("%1 - %2 %3")
                                 .arg(str,appName,DEVCPP_VERSION));
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
                                tr("Debugging"),appName,DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Debugging"),appName));
        } else if (mCompilerManager->running()) {
            setWindowTitle(QString("%1 - %2 [%3] - %4 %5")
                           .arg(str,str2,
                                tr("Running"),appName,DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Running"),appName));
        } else if (mCompilerManager->compiling()) {
            setWindowTitle(QString("%1 - %2 [%3] - %4 %5")
                           .arg(str,str2,
                                tr("Compiling"),appName,DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Compiling"),appName));
        } else {
            setWindowTitle(QString("%1 - %2 %3")
                                 .arg(str,appName,DEVCPP_VERSION));
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
                           .arg(str,tr("Debugging"),appName,DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Debugging"),appName));
        } else if (mCompilerManager->running()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,tr("Running"),appName,DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Running"),appName));
        } else if (mCompilerManager->compiling()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,tr("Compiling"),appName,DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,tr("Compiling"),appName));
        } else {
            this->setWindowTitle(QString("%1 - %2 %3")
                                 .arg(str,appName,DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - %2")
                                    .arg(str,appName));
        }
    } else {
        setWindowTitle(QString("%1 %2").arg(appName,DEVCPP_VERSION));
        app->setApplicationName(QString("%1").arg(appName));
    }
}

void MainWindow::addDebugOutput(const QString &text)
{
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

    foreach (QAction* action,mRecentFileActions) {
        action->setParent(nullptr);
        action->deleteLater();
    }
    mRecentFileActions.clear();

    foreach (QAction* action,mRecentProjectActions) {
        action->setParent(nullptr);
        action->deleteLater();
    }
    mRecentProjectActions.clear();

    if (pSettings->history().openedFiles().size()==0) {
        mMenuRecentFiles->setEnabled(false);
    } else {
        mMenuRecentFiles->setEnabled(true);
        for (const QString& filename: pSettings->history().openedFiles()) {
            QAction* action = new QAction();
            action->setText(filename);
            connect(action, &QAction::triggered, [&filename,this](bool){
                this->openFile(filename);
            });
            mRecentFileActions.append(action);
        }
        mMenuRecentFiles->addActions(mRecentFileActions);
    }

    if (pSettings->history().openedProjects().size()==0) {
        mMenuRecentProjects->setEnabled(false);
    } else {
        mMenuRecentProjects->setEnabled(true);
        for (const QString& filename: pSettings->history().openedProjects()) {
            QAction* action = new QAction();
            action->setText(filename);
            connect(action, &QAction::triggered, [&filename,this](bool){
                this->openProject(filename);
            });
            mRecentProjectActions.append(action);
        }
        mMenuRecentProjects->addActions(mRecentProjectActions);
    }

}

void MainWindow::updateClassBrowserForEditor(Editor *editor)
{
    if (!editor) {
        mClassBrowserModel.setParser(nullptr);
        mClassBrowserModel.setCurrentFile("");
        mClassBrowserModel.clear();
        return;
    }
    if (mQuitting)
        return;
//    if not devCodeCompletion.Enabled then
//      Exit;
    if ((mClassBrowserModel.currentFile() == editor->filename())
         && (mClassBrowserModel.parser() == editor->parser()))
            return;

    mClassBrowserModel.beginUpdate();
    {
        auto action = finally([this] {
            mClassBrowserModel.endUpdate();
        });
        mClassBrowserModel.setParser(editor->parser());
//              if e.InProject then begin
//                ClassBrowser.StatementsType := devClassBrowsing.StatementsType;
//              end else
//                ClassBrowser.StatementsType := cbstFile;
        mClassBrowserModel.setCurrentFile(editor->filename());
    }
}

void MainWindow::resetAutoSaveTimer()
{
    if (pSettings->editor().enableAutoSave()) {
        //minute to milliseconds
        mAutoSaveTimer.start(pSettings->editor().autoSaveInterval()*60*1000);
    } else {
        mAutoSaveTimer.stop();
    }
}

void MainWindow::updateShortcuts()
{
    ShortcutManager manager;
    manager.load();
    manager.applyTo(findChildren<QAction*>());
}

QPlainTextEdit *MainWindow::txtLocals()
{
    return ui->txtLocals;
}

void MainWindow::updateStatusbarForLineCol()
{
    Editor* e = mEditorList->getEditor();
    if (e!=nullptr) {
        int col = e->charToColumn(e->caretY(),e->caretX());
        QString msg = tr("Line:%1 Col:%2 Selected:%3 Lines:%4 Length:%5")
                .arg(e->caretY(),4)
                .arg(col,3)
                .arg(e->selText().length(),6)
                .arg(e->lines()->count(),4)
                .arg(e->lines()->getTextLength(),6);
        mFileInfoStatus->setText(msg);
    } else {
        mFileInfoStatus->setText("");
    }
}

void MainWindow::updateForStatusbarModeInfo()
{
    Editor* e = mEditorList->getEditor();
    if (e!=nullptr) {
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

void MainWindow::openFiles(const QStringList &files)
{
    mEditorList->beginUpdate();
    auto end = finally([this] {
        this->mEditorList->endUpdate();
    });
    //Check if there is a project file in the list and open it
    for (const QString& file:files) {
        if (getFileType(file)==FileType::Project) {
            openProject(file);
            return;
        }
    }
    //Didn't find a project? Open all files
    for (const QString& file:files) {
        openFile(file);
    }
    mEditorList->endUpdate();
}

void MainWindow::openFile(const QString &filename)
{
    Editor* editor = mEditorList->getOpenedEditorByFilename(filename);
    if (editor!=nullptr) {
        editor->activate();
        return;
    }
    try {
        pSettings->history().removeFile(filename);
        editor = mEditorList->newEditor(filename,ENCODING_AUTO_DETECT,
                                        false,false);
        editor->activate();
        this->updateForEncodingInfo();
    } catch (FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
}

void MainWindow::openProject(const QString &filename)
{
    if (!fileExists(filename)) {
        return;
    }
    if (mProject) {
        QString s;
        if (mProject->name().isEmpty())
            s = mProject->filename();
        else
            s = mProject->name();
        if (QMessageBox::question(this,
                                  tr("Close project"),
                                  tr("Are you sure you want to close %1?")
                                  .arg(s),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes) == QMessageBox::Yes) {
            closeProject(false);
        } else {
            return;
        }
    }
    ui->tabProject->setVisible(true);
    ui->tabInfos->setCurrentWidget(ui->tabProject);
    openCloseLeftPanel(true);
//    {
//    LeftPageControl.ActivePage := LeftProjectSheet;
//    fLeftPageControlChanged := False;
//    ClassBrowser.TabVisible:= False;
//    }

    // Only update class browser once
    mClassBrowserModel.beginUpdate();
    {
        auto action = finally([this]{
            mClassBrowserModel.endUpdate();
        });
        mProject = std::make_shared<Project>(filename,DEV_INTERNAL_OPEN);
        updateProjectView();
        pSettings->history().removeProject(filename);

    //  // if project manager isn't open then open it
    //  if not devData.ShowLeftPages then
    //    actProjectManager.Execute;
            //checkForDllProfiling();
        updateAppTitle();
        updateCompilerSet();

        //parse the project
        //  UpdateClassBrowsing;
        scanActiveProject(true);
        mProject->doAutoOpen();

        //update editor's inproject flag
        for (int i=0;i<mProject->units().count();i++) {
            PProjectUnit unit = mProject->units()[i];
            Editor* e = mEditorList->getOpenedEditorByFilename(unit->fileName());
            if (e) {
                unit->setEditor(e);
                unit->setEncoding(e->encodingOption());
                e->setInProject(true);
            } else {
                unit->setEditor(nullptr);
            }
        }

        Editor * e = mEditorList->getEditor();
        if (e) {
            checkSyntaxInBack(e);
        }
        updateClassBrowserForEditor(e);
    }
    updateForEncodingInfo();
}

void MainWindow::setupActions() {

}

void MainWindow::updateCompilerSet()
{
    mCompilerSet->blockSignals(true);
    mCompilerSet->clear();
    for (size_t i=0;i<pSettings->compilerSets().list().size();i++) {
        mCompilerSet->addItem(pSettings->compilerSets().list()[i]->name());
    }
    int index=pSettings->compilerSets().defaultIndex();
    if (mProject) {
        Editor *e = mEditorList->getEditor();
        if ( !e || e->inProject()) {
            index = mProject->options().compilerSet;
        }
    }
    if (index < 0 || index>=mCompilerSet->count()) {
        index = pSettings->compilerSets().defaultIndex();
    }
    mCompilerSet->setCurrentIndex(index);
    mCompilerSet->blockSignals(false);
    mCompilerSet->update();
}

void MainWindow::updateDebuggerSettings()
{
    ui->debugConsole->setFont(QFont(
                                  pSettings->debugger().fontName(),
                                  pSettings->debugger().fontSize()));
}

void MainWindow::checkSyntaxInBack(Editor *e)
{
    if (e==nullptr)
        return;

    if (!pSettings->editor().syntaxCheck()) {
        return;
    }
//    if not devEditor.AutoCheckSyntax then
//      Exit;
    //not c or cpp file
    if (!e->highlighter() || e->highlighter()->getName()!=SYN_HIGHLIGHTER_CPP)
        return;
    if (mCompilerManager->backgroundSyntaxChecking())
        return;
    if (mCompilerManager->compiling())
        return;
    if (!pSettings->compilerSets().defaultSet())
        return;
    if (mCheckSyntaxInBack)
        return;

    mCheckSyntaxInBack=true;
    ui->tableIssues->clearIssues();
    CompileTarget target =getCompileTarget();
    if (target ==CompileTarget::Project) {
        mCompilerManager->checkSyntax(e->filename(),e->text(),
                                          e->fileEncoding() == ENCODING_ASCII, mProject);
    } else {
        mCompilerManager->checkSyntax(e->filename(),e->text(),
                                          e->fileEncoding() == ENCODING_ASCII, nullptr);
    }
//    if not PrepareForCompile(cttStdin,True) then begin
//      fCheckSyntaxInBack:=False;
//      Exit;
//    end;
//    if e.InProject then begin
//      if not assigned(MainForm.fProject) then
//        Exit;
//      fSyntaxChecker.Project := MainForm.fProject;
//    end;
    //    fSyntaxChecker.CheckSyntax(True);
}

bool MainWindow::compile(bool rebuild)
{
    CompileTarget target =getCompileTarget();
    if (target == CompileTarget::Project) {
        if (!mProject->saveUnits())
            return false;
        // Check if saves have been succesful
        for (int i=0; i<mEditorList->pageCount();i++) {
            Editor * e= (*(mEditorList))[i];
            if (e->inProject() && e->modified())
                return false;
        }
        ui->tableIssues->clearIssues();

        // Increment build number automagically
        if (mProject->options().versionInfo.autoIncBuildNr) {
            mProject->incrementBuildNumber();
        }
        mProject->buildPrivateResource();
        if (mCompileSuccessionTask) {
            mCompileSuccessionTask->filename = mProject->executable();
        }
        updateCompileActions();
        openCloseBottomPanel(true);
        ui->tabMessages->setCurrentWidget(ui->tabCompilerOutput);
        mCompilerManager->compileProject(mProject,rebuild);
        updateAppTitle();
    } else {
        Editor * editor = mEditorList->getEditor();
        if (editor != NULL ) {
            ui->tableIssues->clearIssues();
            if (editor->modified()) {
                if (!editor->save(false,false))
                    return false;
            }
            if (mCompileSuccessionTask) {
                mCompileSuccessionTask->filename = getCompiledExecutableName(editor->filename());
            }
            updateCompileActions();
            openCloseBottomPanel(true);
            ui->tabMessages->setCurrentWidget(ui->tabCompilerOutput);
            mCompilerManager->compile(editor->filename(),editor->fileEncoding(),rebuild);
            updateAppTitle();
            return true;
        }
    }
    return false;
}

void MainWindow::runExecutable(const QString &exeName,const QString &filename)
{
    // Check if it exists
    if (!fileExists(exeName)) {
        if (ui->actionCompile_Run->isEnabled()) {
            if (QMessageBox::question(this,tr("Confirm"),
                                     tr("Source file is not compiled.")
                                     +"<br /><br />"+tr("Compile now?"),
                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                ui->actionCompile_Run->trigger();
                return;
            }
        } else {
            QMessageBox::critical(this,"Error",
                                  tr("Source file is not compiled."));
            return;
        }
    } else {
        if (!filename.isEmpty() && compareFileModifiedTime(filename,exeName)>=0) {
            if (ui->actionCompile_Run->isEnabled()) {
                if (QMessageBox::warning(this,tr("Confirm"),
                                         tr("Source file is more recent than executable.")
                                         +"<br /><br />"+tr("Recompile now?"),
                        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    ui->actionCompile_Run->trigger();
                    return;
                }
            }
        }
    }
      // Pause programs if they contain a console
//          if devData.ConsolePause and ProgramHasConsole(FileToRun) then begin
//            if fUseRunParams then
//              Parameters := '"' + FileToRun + '" ' + fRunParams
//            else
//              Parameters := '"' + FileToRun + '"';

//            FileToRun := devDirs.Exec + 'ConsolePauser.exe';
//          end else begin
//            if fUseRunParams then
//              Parameters := fRunParams
//            else
//              Parameters := '';
//            FileToRun := FileToRun;
//          end;

    updateCompileActions();
    if (pSettings->executor().minimizeOnRun()) {
        showMinimized();
    }
    updateAppTitle();
    if (pSettings->executor().useParams()) {
        mCompilerManager->run(exeName,pSettings->executor().params(),QFileInfo(exeName).absolutePath());
    } else {
        mCompilerManager->run(exeName,"",QFileInfo(exeName).absolutePath());
    }
}

void MainWindow::runExecutable()
{
    CompileTarget target =getCompileTarget();
    if (target == CompileTarget::Project) {
        runExecutable(mProject->executable(),mProject->filename());
    } else {
        Editor * editor = mEditorList->getEditor();
        if (editor != NULL ) {
            if (editor->modified()) {
                if (!editor->save(false,false))
                    return;
            }
            QString exeName= getCompiledExecutableName(editor->filename());
            runExecutable(exeName,editor->filename());
        }
    }
}

void MainWindow::debug()
{
    if (mCompilerManager->compiling())
        return;
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
    switch(getCompileTarget()) {
    case CompileTarget::Project:
        // Check if we enabled proper options
        debugEnabled = mProject->getCompilerOption("-g3")!='0';
        stripEnabled = mProject->getCompilerOption("-s")!='0';
        // Ask the user if he wants to enable debugging...
        if (((!debugEnabled) || stripEnabled) &&
                (QMessageBox::question(this,
                                      tr("Enable debugging"),
                                      tr("You have not enabled debugging info (-g3) and/or stripped it from the executable (-s) in Compiler Options.<BR /><BR />Do you want to correct this now?")
                                      ) == QMessageBox::Yes)) {
            // Enable debugging, disable stripping
            mProject->setCompilerOption("-g3",'1');
            mProject->setCompilerOption("-s",'0');

            // Save changes to compiler set
            mProject->saveOptions();

            mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
            mCompileSuccessionTask->type = CompileSuccessionTaskType::Debug;

            compile();
            return;
        }
        // Did we compile?
        if (!fileExists(mProject->executable())) {
            if (QMessageBox::question(
                        this,
                        tr("Project not built"),
                        tr("Project hasn't been built. Build it now?"),
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::Yes) == QMessageBox::Yes) {
                mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
                mCompileSuccessionTask->type = CompileSuccessionTaskType::Debug;

                compile();
            }
            return;
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
        filePath = mProject->executable();

//        mDebugger->setUseUTF8(e->fileEncoding() == ENCODING_UTF8 || e->fileEncoding() == ENCODING_UTF8_BOM);

        if (!mDebugger->start())
            return;
        filePath.replace('\\','/');
        mDebugger->sendCommand("file", '"' + filePath + '"');

        if (mProject->options().type == ProjectType::DynamicLib) {
            QString host =mProject->options().hostApplication;
            host.replace('\\','/');
            mDebugger->sendCommand("exec-file", '"' + host + '"');
        }
        for (int i=0;i<mProject->units().count();i++) {
            QString file = mProject->units()[i]->fileName();
            file.replace('\\','/');
            mDebugger->sendCommand("file", '"'+file+ '"');
        }
        includeOrSkipDirs(mProject->options().includes,
                          pSettings->debugger().skipProjectLibraries());
        includeOrSkipDirs(mProject->options().libs,
                          pSettings->debugger().skipProjectLibraries());
        break;
    case CompileTarget::File:
        // Check if we enabled proper options
        debugEnabled = compilerSet->getOptionValue("-g3")!='0';
        stripEnabled = compilerSet->getOptionValue("-s")!='0';
        // Ask the user if he wants to enable debugging...
        if (((!debugEnabled) || stripEnabled) &&
                (QMessageBox::question(this,
                                      tr("Enable debugging"),
                                      tr("You have not enabled debugging info (-g3) and/or stripped it from the executable (-s) in Compiler Options.<BR /><BR />Do you want to correct this now?")
                                      ) == QMessageBox::Yes)) {
            // Enable debugging, disable stripping
            compilerSet->setOption("-g3",'1');
            compilerSet->setOption("-s",'0');

            // Save changes to compiler set
            pSettings->compilerSets().saveSet(pSettings->compilerSets().defaultIndex());

            mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
            mCompileSuccessionTask->type = CompileSuccessionTaskType::Debug;

            compile();
            return;
        }

        Editor* e = mEditorList->getEditor();
        if (e!=nullptr) {
            // Did we saved?
            if (e->modified()) {
                // if file is modified,save it first
                if (!e->save(false,false))
                        return;
            }


            // Did we compiled?
            filePath = getCompiledExecutableName(e->filename());
            debugFile.setFile(filePath);
            if (!debugFile.exists()) {
                if (QMessageBox::question(this,tr("Compile"),
                                          tr("Source file is not compiled.")+"<BR /><BR />" + tr("Compile now?"),
                                          QMessageBox::Yes|QMessageBox::No,
                                          QMessageBox::Yes) == QMessageBox::Yes) {
                    mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
                    mCompileSuccessionTask->type = CompileSuccessionTaskType::Debug;
                    compile();
                    return;
                }
            } else {
                if (compareFileModifiedTime(e->filename(),filePath)>=0) {
                    if (QMessageBox::question(this,tr("Compile"),
                                              tr("Source file is more recent than executable.")+"<BR /><BR />" + tr("Recompile?"),
                                              QMessageBox::Yes|QMessageBox::No,
                                              QMessageBox::Yes) == QMessageBox::Yes) {
                        mCompileSuccessionTask=std::make_shared<CompileSuccessionTask>();
                        mCompileSuccessionTask->type = CompileSuccessionTaskType::Debug;
                        compile();
                        return;
                    }
                }
            }

            prepareDebugger();

            mDebugger->setUseUTF8(e->fileEncoding() == ENCODING_UTF8 || e->fileEncoding() == ENCODING_UTF8_BOM);
            if (!mDebugger->start())
                return;
            mDebugger->sendCommand("file", QString("\"%1\"").arg(debugFile.filePath().replace('\\','/')));
        }
        break;
    }

    updateEditorActions();

    // Add library folders
    includeOrSkipDirs(compilerSet->libDirs(), pSettings->debugger().skipCustomLibraries());
    includeOrSkipDirs(compilerSet->CIncludeDirs(), pSettings->debugger().skipCustomLibraries());
    includeOrSkipDirs(compilerSet->CppIncludeDirs(), pSettings->debugger().skipCustomLibraries());

    //gcc system libraries is auto loaded by gdb
    if (pSettings->debugger().skipSystemLibraries()) {
        includeOrSkipDirs(compilerSet->defaultCIncludeDirs(),true);
        includeOrSkipDirs(compilerSet->defaultCIncludeDirs(),true);
        includeOrSkipDirs(compilerSet->defaultCppIncludeDirs(),true);
    }

    // Add breakpoints and watch vars
//    for i := 0 to fDebugger.WatchVarList.Count - 1 do
//      fDebugger.AddWatchVar(i);
    mDebugger->sendAllWatchvarsToDebugger();
    mDebugger->sendAllBreakpointsToDebugger();

    // Run the debugger
    mDebugger->sendCommand("set", "width 0"); // don't wrap output, very annoying
    mDebugger->sendCommand("set", "new-console on");
    mDebugger->sendCommand("set", "confirm off");
    mDebugger->sendCommand("set", "print repeats 0"); // don't repeat elements
    mDebugger->sendCommand("set", "print elements 0"); // don't limit elements
    mDebugger->sendCommand("cd", excludeTrailingPathDelimiter(debugFile.path())); // restore working directory
    if (!debugInferiorhasBreakpoint()) {
        QString params;
        switch(getCompileTarget()) {
        case CompileTarget::None:
            return;
        case CompileTarget::File:
//            if (mCompiler->useRunParams) {

//            }
            mDebugger->sendCommand("start",params);
            mDebugger->updateDebugInfo();
            break;
        case CompileTarget::Project:
            params = "";
//if fCompiler.UseRunParams then
//  params := params + ' ' + fProject.Options.CmdLineArgs;
//if fCompiler.UseInputFile then
//  params := params + ' < "' + fCompiler.InputFile + '"';

            mDebugger->sendCommand("start",params);
            mDebugger->updateDebugInfo();
            break;
        }
    } else {
        QString params;
        switch(getCompileTarget()) {
        case CompileTarget::None:
            return;
        case CompileTarget::File:
//            if (mCompiler->useRunParams) {

//            }
            mDebugger->sendCommand("run",params);
            mDebugger->updateDebugInfo();
            break;
        case CompileTarget::Project:
//params := '';
//if fCompiler.UseRunParams then
//  params := params + ' ' + fProject.Options.CmdLineArgs;
//if fCompiler.UseInputFile then
//  params := params + ' < "' + fCompiler.InputFile + '"';

            mDebugger->sendCommand("run",params);
            mDebugger->updateDebugInfo();
            break;
        }
    }
}

void MainWindow::showSearchPanel(bool showReplace)
{
    openCloseBottomPanel(true);
    showSearchReplacePanel(showReplace);
    ui->tabMessages->setCurrentWidget(ui->tabSearch);
}

void MainWindow::openCloseBottomPanel(bool open)
{
//    if Assigned(fReportToolWindow) then
//      Exit;
    if (mOpenClosingBottomPanel)
        return;
    mOpenClosingBottomPanel = true;
    auto action = finally([this]{
        mOpenClosingBottomPanel = false;
    });
    // Switch between open and close
    if (open) {
        QList<int> sizes = ui->splitterMessages->sizes();
        int tabHeight = ui->tabMessages->tabBar()->height();
        ui->tabMessages->setMinimumHeight(tabHeight+5);
        int totalSize = sizes[0] + sizes[1];
        sizes[1] = mBottomPanelHeight;
        sizes[0] = std::max(1,totalSize - sizes[1]);
        ui->splitterMessages->setSizes(sizes);
    } else {
        QList<int> sizes = ui->splitterMessages->sizes();
        mBottomPanelHeight = sizes[1];
        int totalSize = sizes[0] + sizes[1];
        int tabHeight = ui->tabMessages->tabBar()->height();
        ui->tabMessages->setMinimumHeight(tabHeight);
        sizes[1] = tabHeight;
        sizes[0] = std::max(1,totalSize - sizes[1]);
        ui->splitterMessages->setSizes(sizes);
    }
    mBottomPanelOpenned = open;
    QSplitterHandle* handle = ui->splitterMessages->handle(1);
    handle->setEnabled(open);
}

void MainWindow::openCloseLeftPanel(bool open)
{
    if (mOpenClosingLeftPanel)
        return;
    mOpenClosingLeftPanel = true;
    auto action = finally([this]{
        mOpenClosingLeftPanel = false;
    });
    // Switch between open and close
    if (open) {
        QList<int> sizes = ui->splitterInfos->sizes();
        int tabWidth = ui->tabInfos->tabBar()->width();
        ui->tabInfos->setMinimumWidth(tabWidth+5);
        int totalSize = sizes[0] + sizes[1];
        sizes[0] = mLeftPanelWidth;
        sizes[1] = std::max(1,totalSize - sizes[0]);
        ui->splitterInfos->setSizes(sizes);
    } else {
        QList<int> sizes = ui->splitterInfos->sizes();
        mLeftPanelWidth = sizes[0];
        int totalSize = sizes[0] + sizes[1];
        int tabWidth = ui->tabInfos->tabBar()->width();
        ui->tabInfos->setMinimumWidth(tabWidth);
        sizes[0] = tabWidth;
        sizes[1] = std::max(1,totalSize - sizes[0]);
        ui->splitterInfos->setSizes(sizes);
    }
    mLeftPanelOpenned = open;
    QSplitterHandle* handle = ui->splitterInfos->handle(1);
    handle->setEnabled(open);
}

void MainWindow::prepareDebugger()
{
    mDebugger->stop();

    // Clear logs
    ui->debugConsole->clear();
    if (!pSettings->debugger().showCommandLog()) {
        ui->debugConsole->addLine("(gdb) ");
    }
    ui->txtEvalOutput->clear();

    // Restore when no watch vars are shown
    mDebugger->setLeftPageIndexBackup(ui->tabInfos->currentIndex());

    // Focus on the debugging buttons
    ui->tabInfos->setCurrentWidget(ui->tabWatch);
    ui->tabMessages->setCurrentWidget(ui->tabDebug);
    ui->debugViews->setCurrentWidget(ui->tabDebugConsole);
    openCloseBottomPanel(true);
    openCloseLeftPanel(true);

    // Reset watch vars
    //    mDebugger->deleteWatchVars(false);
}

void MainWindow::doAutoSave(Editor *e)
{
    if (!e)
        return;
    if (!e->modified())
        return;
    QString filename = e->filename();
    QFileInfo fileInfo(filename);
    QDir parent = fileInfo.absoluteDir();
    QString baseName = fileInfo.completeBaseName();
    QString suffix = fileInfo.suffix();
    switch(pSettings->editor().autoSaveStrategy()) {
    case assOverwrite:
        break;
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
    e->saveFile(filename);
}

//static void limitActionShortCutScope(QAction* action,QWidget* scopeWidget) {
//    action->setParent(scopeWidget);
//    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
//}

QAction* MainWindow::createActionFor(
        const QString& text,
        QWidget* parent,
        QKeySequence shortcut) {
    QAction* action= new QAction(text,parent);
    if (!shortcut.isEmpty())
        action->setShortcut(shortcut);
    action->setPriority(QAction::HighPriority);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    parent->addAction(action);
    return action;
}

void MainWindow::scanActiveProject(bool parse)
{
    if (!mProject)
        return;
    //UpdateClassBrowsing;
    if (parse) {
        resetCppParser(mProject->cppParser());
        mProject->resetParserProjectFiles();
        parseFileList(mProject->cppParser());
    } else {
        mProject->resetParserProjectFiles();
    };
}

void MainWindow::includeOrSkipDirs(const QStringList &dirs, bool skip)
{
    Q_ASSERT(mDebugger);
    foreach (QString dir,dirs) {
        QString dirName = dir.replace('\\','/');
        if (skip) {
            mDebugger->sendCommand(
                        "skip",
                        QString("-gfi \"%1/%2\"")
                        .arg(dirName,"*.*"));
        } else {
            mDebugger->sendCommand(
                        "dir",
                        QString("\"%1\"").arg(dirName));
        }
    }
}

void MainWindow::saveLastOpens()
{
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + DEV_LASTOPENS_FILE;
    if (fileExists(filename)) {
        if (!QFile::remove(filename)) {
            QMessageBox::critical(this,
                                  tr("Save last open info error"),
                                  tr("Can't remove old last open information file '%1'")
                                  .arg(filename),
                                  QMessageBox::Ok);
            return;
        }

    }
    SimpleIni lastOpenIni;
    lastOpenIni.SetLongValue("LastOpens","Count", mEditorList->pageCount());
    if (mProject) {
        lastOpenIni.SetValue("LastOpens","Project",mProject->filename().toLocal8Bit());
    }
    for (int i=0;i<mEditorList->pageCount();i++) {
      Editor * editor = (*mEditorList)[i];
      QByteArray sectionName = QString("Editor_%1").arg(i).toLocal8Bit();
      lastOpenIni.SetValue(sectionName,"FileName", editor->filename().toLocal8Bit());
      lastOpenIni.SetBoolValue(sectionName, "OnLeft",editor->pageControl() != ui->EditorTabsRight);
      lastOpenIni.SetBoolValue(sectionName, "Focused",editor->hasFocus());
      lastOpenIni.SetLongValue(sectionName, "CursorCol", editor->caretX());
      lastOpenIni.SetLongValue(sectionName, "CursorRow", editor->caretY());
      lastOpenIni.SetLongValue(sectionName, "TopLine", editor->topLine());
      lastOpenIni.SetLongValue(sectionName, "LeftChar", editor->leftChar());
    }
    if (lastOpenIni.SaveFile(filename.toLocal8Bit())!=SI_Error::SI_OK) {
        QMessageBox::critical(this,
                              tr("Save last open info error"),
                              tr("Can't save last open info file '%1'")
                              .arg(filename),
                              QMessageBox::Ok);
        return;
    }
}

void MainWindow::loadLastOpens()
{
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + DEV_LASTOPENS_FILE;
    if (!fileExists(filename))
        return;
    SimpleIni lastOpenIni;
    if (lastOpenIni.LoadFile(filename.toLocal8Bit())!=SI_Error::SI_OK) {
        QMessageBox::critical(this,
                              tr("Load last open info error"),
                              tr("Can't load last open info file '%1'")
                              .arg(filename),
                              QMessageBox::Ok);
        return;
    }
    Editor *  focusedEditor = nullptr;
    int count = lastOpenIni.GetLongValue("LastOpens","Count",0);
    for (int i=0;i<count;i++) {
        QByteArray sectionName = QString("Editor_%1").arg(i).toLocal8Bit();
        QString editorFilename = QString::fromLocal8Bit(lastOpenIni.GetValue(sectionName,"FileName",""));
        if (!fileExists(editorFilename))
            continue;
        bool onLeft = lastOpenIni.GetBoolValue(sectionName,"OnLeft",true);
//        if onLeft then
//      page := EditorList.LeftPageControl
//    else
//      page := EditorList.RightPageControl;
        Editor * editor = mEditorList->newEditor(editorFilename,ENCODING_AUTO_DETECT,false,false);
        if (!editor)
            continue;
        BufferCoord pos;
        pos.Char = lastOpenIni.GetLongValue(sectionName,"CursorCol", 1);
        pos.Line = lastOpenIni.GetLongValue(sectionName,"CursorRow", 1);
        editor->setCaretXY(pos);
        editor->setTopLine(
                    lastOpenIni.GetLongValue(sectionName,"TopLine", 1)
                    );
        editor->setLeftChar(
                    lastOpenIni.GetLongValue(sectionName,"LeftChar", 1)
                    );
        if (lastOpenIni.GetBoolValue(sectionName,"Focused",false))
            focusedEditor = editor;
        pSettings->history().removeFile(editorFilename);
    }
    QString projectFilename = QString::fromLocal8Bit((lastOpenIni.GetValue("LastOpens", "Project","")));
    if (fileExists(projectFilename)) {
        openProject(projectFilename);
    } else {
        updateEditorActions();
        updateForEncodingInfo();
    }
    if (focusedEditor)
        focusedEditor->activate();
}

void MainWindow::newEditor()
{
    try {
        Editor * editor=mEditorList->newEditor("",ENCODING_AUTO_DETECT,false,true);
        editor->activate();
        updateForEncodingInfo();
    }  catch (FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }

}

void MainWindow::buildContextMenus()
{

    //context menu signal for the watch view
    ui->watchView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->watchView,&QWidget::customContextMenuRequested,
            this, &MainWindow::onWatchViewContextMenu);

    //context menu signal for the watch view
    ui->debugConsole->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->debugConsole,&QWidget::customContextMenuRequested,
            this, &MainWindow::onDebugConsoleContextMenu);
    mDebugConsole_ShowCommandLog = createActionFor(
                tr("Show debug logs in the debug console"),
                ui->debugConsole);
    mDebugConsole_ShowCommandLog->setCheckable(true);
    connect(mDebugConsole_ShowCommandLog, &QAction::toggled,
            [this]() {
        pSettings->debugger().setShowCommandLog(mDebugConsole_ShowCommandLog->isChecked());
        pSettings->debugger().save();
    });
    mDebugConsole_Copy=createActionFor(
                tr("Copy"),
                ui->debugConsole,
                QKeySequence("Ctrl+C"));
    connect(mDebugConsole_Copy, &QAction::triggered,
            [this]() {
        ui->debugConsole->copy();
    });
    mDebugConsole_Paste=createActionFor(
                tr("Paste"),
                ui->debugConsole,
                QKeySequence("Ctrl+V"));
    connect(mDebugConsole_Paste, &QAction::triggered,
            [this]() {
        ui->debugConsole->paste();
    });
    mDebugConsole_SelectAll=createActionFor(
                tr("Select All"),
                ui->debugConsole);
    connect(mDebugConsole_SelectAll, &QAction::triggered,
            [this]() {
        ui->debugConsole->selectAll();
    });
    mDebugConsole_Clear=createActionFor(
                tr("Clear"),
                ui->debugConsole);
    connect(mDebugConsole_Clear, &QAction::triggered,
            [this]() {
        ui->debugConsole->clear();
    });

    //context menu signal for Editor's tabbar
    ui->EditorTabsLeft->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->EditorTabsLeft->tabBar(),
            &QWidget::customContextMenuRequested,
            this,
            &MainWindow::onEditorTabContextMenu
            );

    ui->EditorTabsRight->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->EditorTabsRight->tabBar(),
            &QWidget::customContextMenuRequested,
            this,
            &MainWindow::onEditorTabContextMenu
            );

    //context menu signal for Compile Issue view
    ui->tableIssues->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableIssues,&QWidget::customContextMenuRequested,
            this, &MainWindow::onTableIssuesContextMenu);
    mTableIssuesCopyAction = createActionFor(
                tr("Copy"),
                ui->tableIssues,
                QKeySequence("Ctrl+C"));
    connect(mTableIssuesCopyAction,&QAction::triggered,
            [this](){
        QModelIndex index = ui->tableIssues->selectionModel()->currentIndex();
        PCompileIssue issue = ui->tableIssues->issue(index);
        if (issue) {
            QClipboard* clipboard = QApplication::clipboard();
            clipboard->setText(issue->description);
        }
    });
    mTableIssuesCopyAllAction = createActionFor(
                tr("Copy all"),
                ui->tableIssues,
                QKeySequence("Ctrl+Shift+C"));
    connect(mTableIssuesCopyAllAction,&QAction::triggered,
            [this](){
        QClipboard* clipboard=QGuiApplication::clipboard();
        QMimeData * mimeData = new QMimeData();
        mimeData->setText(ui->tableIssues->toTxt());
        mimeData->setHtml(ui->tableIssues->toHtml());
        clipboard->clear();
        clipboard->setMimeData(mimeData);
    });
    mTableIssuesClearAction = createActionFor(
                tr("Clear"),
                ui->tableIssues);
    connect(mTableIssuesClearAction,&QAction::triggered,
            [this](){
        ui->tableIssues->clearIssues();
    });

    //context menu signal for search view
    ui->searchHistoryPanel->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->searchHistoryPanel, &QWidget::customContextMenuRequested,
            this, &MainWindow::onSearchViewContextMenu);
    mSearchViewClearAction = createActionFor(
                tr("Remove this search"),
                ui->searchHistoryPanel);
    connect(mSearchViewClearAction, &QAction::triggered,
            [this](){
       int index = ui->cbSearchHistory->currentIndex();
       if (index>=0) {
           mSearchResultModel.removeSearchResults(index);
       }
    });
    mSearchViewClearAllAction = createActionFor(
                tr("Clear all searches"),
                ui->searchHistoryPanel);
    connect(mSearchViewClearAllAction,&QAction::triggered,
            [this]() {
       mSearchResultModel.clear();
    });

    //context menu signal for breakpoints view
    ui->tblBreakpoints->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tblBreakpoints,&QWidget::customContextMenuRequested,
             this, &MainWindow::onBreakpointsViewContextMenu);
    mBreakpointViewPropertyAction = createActionFor(
                tr("Breakpoint condition..."),
                ui->tblBreakpoints);
    connect(mBreakpointViewPropertyAction,&QAction::triggered,
            [this](){
        int index =ui->tblBreakpoints->selectionModel()->currentIndex().row();

        PBreakpoint breakpoint = debugger()->breakpointModel()->breakpoint(
                    index
                    );
        if (breakpoint) {
            bool isOk;
            QString s=QInputDialog::getText(this,
                                      tr("Break point condition"),
                                      tr("Enter the condition of the breakpoint:"),
                                    QLineEdit::Normal,
                                    breakpoint->condition,&isOk);
            if (isOk) {
                pMainWindow->debugger()->setBreakPointCondition(index,s);
            }
        }
    });
    mBreakpointViewRemoveAllAction = createActionFor(
                tr("Remove all breakpoints"),
                ui->tblBreakpoints);
    connect(mBreakpointViewRemoveAllAction,&QAction::triggered,
            [this](){
        pMainWindow->debugger()->deleteBreakpoints();
    });

    //context menu signal for project view
    ui->projectView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->projectView,&QWidget::customContextMenuRequested,
             this, &MainWindow::onProjectViewContextMenu);
    mProject_Rename_Unit = createActionFor(
                tr("Rename File"),
                ui->projectView);
    connect(mProject_Rename_Unit, &QAction::triggered,
            [this](){
        if (ui->projectView->currentIndex().isValid())
            ui->projectView->edit(ui->projectView->currentIndex());
    });
    mProject_Add_Folder = createActionFor(
                tr("Add Folder"),
                ui->projectView);
    connect(mProject_Add_Folder, &QAction::triggered,
            [this](){
        if (!mProject)
            return;
        QModelIndex current = ui->projectView->currentIndex();
        if (!current.isValid()) {
            return;
        }
        FolderNode * node = static_cast<FolderNode*>(current.internalPointer());
        PFolderNode folderNode =  mProject->pointerToNode(node);
        if (!folderNode)
            folderNode = mProject->node();
        if (folderNode->unitIndex>=0)
            return;
        QString s=tr("New folder");
        bool ok;
        s = QInputDialog::getText(ui->projectView,
                              tr("Add Folder"),
                              tr("Folder name:"),
                              QLineEdit::Normal, s,
                              &ok).trimmed();
        if (ok && !s.isEmpty()) {
            QString path = mProject->getFolderPath(folderNode);
            if (path.isEmpty()) {
                mProject->addFolder(s);
            } else {
                mProject->addFolder(path + '/' +s);
            }
            mProject->saveOptions();
        }
    });
    mProject_Rename_Folder = createActionFor(
                tr("Rename Folder"),
                ui->projectView);
    connect(mProject_Rename_Folder, &QAction::triggered,
            [this](){
        if (ui->projectView->currentIndex().isValid())
            ui->projectView->edit(ui->projectView->currentIndex());
    });
    mProject_Remove_Folder = createActionFor(
                tr("Remove Folder"),
                ui->projectView);
    connect(mProject_Remove_Folder, &QAction::triggered,
            [this](){
        if (!mProject)
            return;
        QModelIndex current = ui->projectView->currentIndex();
        if (!current.isValid()) {
            return;
        }
        FolderNode * node = static_cast<FolderNode*>(current.internalPointer());
        PFolderNode folderNode =  mProject->pointerToNode(node);
        if (!folderNode)
            return;
        if (folderNode->unitIndex>=0)
            return;
        mProject->removeFolder(folderNode);
        mProject->saveOptions();
    });

    //context menu signal for class browser
    ui->tabStructure->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tabStructure,&QWidget::customContextMenuRequested,
             this, &MainWindow::onClassBrowserContextMenu);
    mClassBrowser_Sort_By_Type = createActionFor(
                tr("Sort By Type"),
                ui->tabStructure);
    mClassBrowser_Sort_By_Type->setCheckable(true);
    mClassBrowser_Sort_By_Type->setIcon(QIcon(":/icons/images/newlook24/077-sort-type.png"));
    mClassBrowser_Sort_By_Name = createActionFor(
                tr("Sort alphabetically"),
                ui->tabStructure);
    mClassBrowser_Sort_By_Name->setCheckable(true);
    mClassBrowser_Sort_By_Name->setIcon(QIcon(":/icons/images/newlook24/076-sort-alpha.png"));
    mClassBrowser_Show_Inherited = createActionFor(
                tr("Show inherited members"),
                ui->tabStructure);
    mClassBrowser_Show_Inherited->setCheckable(true);
    mClassBrowser_Show_Inherited->setIcon(QIcon(":/icons/images/newlook24/075-show-inheritance.png"));
    mClassBrowser_goto_declaration = createActionFor(
                tr("Goto declaration"),
                ui->tabStructure);
    mClassBrowser_goto_definition = createActionFor(
                tr("Goto definition"),
                ui->tabStructure);

    mClassBrowser_Sort_By_Name->setChecked(pSettings->ui().classBrowserSortAlpha());
    mClassBrowser_Sort_By_Type->setChecked(pSettings->ui().classBrowserSortType());
    mClassBrowser_Show_Inherited->setChecked(pSettings->ui().classBrowserShowInherited());
    connect(mClassBrowser_Sort_By_Name, &QAction::toggled,
            [this](){
        pSettings->ui().setClassBrowserSortAlpha(mClassBrowser_Sort_By_Name->isChecked());
        pSettings->ui().save();
        mClassBrowserModel.fillStatements();
    });
    connect(mClassBrowser_Sort_By_Type, &QAction::toggled,
            [this](){
        pSettings->ui().setClassBrowserSortType(mClassBrowser_Sort_By_Type->isChecked());
        pSettings->ui().save();
        mClassBrowserModel.fillStatements();
    });
    connect(mClassBrowser_Show_Inherited, &QAction::toggled,
            [this](){
        pSettings->ui().setClassBrowserShowInherited(mClassBrowser_Show_Inherited->isChecked());
        pSettings->ui().save();
        mClassBrowserModel.fillStatements();
    });

    connect(mClassBrowser_goto_definition,&QAction::triggered,
            [this](){
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
        Editor* e = pMainWindow->editorList()->getEditorByFilename(filename);
        if (e) {
            e->setCaretPositionAndActivate(line,1);
        }
    });

    connect(mClassBrowser_goto_declaration,&QAction::triggered,
            [this](){
        on_classBrowser_doubleClicked(ui->classBrowser->currentIndex());
    });

    //toolbar for class browser
    mClassBrowserToolbar = new QWidget();
    QVBoxLayout* layout = dynamic_cast<QVBoxLayout*>( ui->tabStructure->layout());
    layout->insertWidget(0,mClassBrowserToolbar);
    QHBoxLayout* hlayout =  new QHBoxLayout();
    hlayout->setContentsMargins(2,2,2,2);
    mClassBrowserToolbar->setLayout(hlayout);
    QToolButton * toolButton;
    toolButton = new QToolButton;
    toolButton->setDefaultAction(mClassBrowser_Sort_By_Type);
    hlayout->addWidget(toolButton);
    toolButton = new QToolButton;
    toolButton->setDefaultAction(mClassBrowser_Sort_By_Name);
    hlayout->addWidget(toolButton);
    QFrame * vLine = new QFrame();
    vLine->setFrameShape(QFrame::VLine);
    vLine->setFrameShadow(QFrame::Sunken);
    hlayout->addWidget(vLine);
    toolButton = new QToolButton;
    toolButton->setDefaultAction(mClassBrowser_Show_Inherited);
    hlayout->addWidget(toolButton);
    hlayout->addStretch();
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

    mMenuEncoding = new QMenu();
    mMenuEncoding->setTitle(tr("File Encoding"));
    mMenuEncoding->addAction(ui->actionAuto_Detect);
    mMenuEncoding->addAction(ui->actionEncode_in_ANSI);
    mMenuEncoding->addAction(ui->actionEncode_in_UTF_8);

    mMenuEncoding->addMenu(menuCharsets);
    mMenuEncoding->addSeparator();
    mMenuEncoding->addAction(ui->actionConvert_to_ANSI);
    mMenuEncoding->addAction(ui->actionConvert_to_UTF_8);

    ui->menuEdit->insertMenu(ui->actionFoldAll,mMenuEncoding);
    ui->menuEdit->insertSeparator(ui->actionFoldAll);
    ui->actionAuto_Detect->setCheckable(true);
    ui->actionEncode_in_ANSI->setCheckable(true);
    ui->actionEncode_in_UTF_8->setCheckable(true);
}

void MainWindow::maximizeEditor()
{
    if (mLeftPanelOpenned || mBottomPanelOpenned) {
        openCloseBottomPanel(false);
        openCloseLeftPanel(false);
    } else {
        openCloseBottomPanel(true);
        openCloseLeftPanel(true);
    }
}

void MainWindow::openShell(const QString &folder, const QString &shellCommand)
{
    QProcess process;
    process.setWorkingDirectory(folder);
    process.setProgram(shellCommand);
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments * args){
        args->flags |= CREATE_NEW_CONSOLE;
        args->startupInfo->dwFlags &=  ~STARTF_USESTDHANDLES; //
    });
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList pathAdded;
    if (pSettings->compilerSets().defaultSet()) {
        foreach(const QString& dir, pSettings->compilerSets().defaultSet()->binDirs()) {
            pathAdded.append(dir);
        }
    }
    pathAdded.append(pSettings->dirs().app());
    if (!path.isEmpty()) {
        path= pathAdded.join(PATH_SEPARATOR) + PATH_SEPARATOR + path;
    } else {
        path = pathAdded.join(PATH_SEPARATOR);
    }
    env.insert("PATH",path);
    process.setProcessEnvironment(env);
    process.startDetached();
}

void MainWindow::onAutoSaveTimeout()
{
    if (!pSettings->editor().enableAutoSave())
        return;
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
        //todo: auto save project files
        break;
    }
    updateStatusbarMessage(tr("%1 files autosaved").arg(updateCount));
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
    menu.exec(ui->searchHistoryPanel->mapToGlobal(pos));
}

void MainWindow::onBreakpointsViewContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(mBreakpointViewPropertyAction);
    menu.addAction(mBreakpointViewRemoveAllAction);
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
    int unitIndex = -1;
    QModelIndex current = ui->projectView->selectionModel()->currentIndex();
    if (current.isValid() && mProject) {
        FolderNode * node = static_cast<FolderNode*>(current.internalPointer());
        PFolderNode pNode = mProject->pointerToNode(node);
        if (pNode) {
            unitIndex = pNode->unitIndex;
            onFolder = (unitIndex<0);
            onUnit = (unitIndex >= 0);
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
    updateProjectActions();
    menu.addAction(ui->actionProject_New_File);
    menu.addAction(ui->actionAdd_to_project);
    if (!onFolder) {
        menu.addAction(ui->actionRemove_from_project);
    }
    if (onUnit) {
        menu.addAction(mProject_Rename_Unit);
    }
    menu.addSeparator();
    if (onFolder) {
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
    menu.addAction(ui->actionProject_options);

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

    menu.exec(ui->projectView->mapToGlobal(pos));
}

void MainWindow::onDebugConsoleContextMenu(const QPoint &pos)
{
    QMenu menu(this);

    bool oldBlock = mDebugConsole_ShowCommandLog->blockSignals(true);
    mDebugConsole_ShowCommandLog->setChecked(pSettings->debugger().showCommandLog());
    mDebugConsole_ShowCommandLog->blockSignals(oldBlock);

    menu.addAction(mDebugConsole_Copy);
    menu.addAction(mDebugConsole_Paste);
    menu.addAction(mDebugConsole_SelectAll);
    menu.addAction(mDebugConsole_Clear);
    menu.addSeparator();
    menu.addAction(mDebugConsole_ShowCommandLog);
    menu.exec(ui->debugConsole->mapToGlobal(pos));
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

void MainWindow::onEditorContextMenu(const QPoint &pos)
{
    Editor * editor = mEditorList->getEditor();
    if (!editor)
        return;
    QMenu menu(this);
    BufferCoord p;
    mEditorContextMenuPos = pos;
    if (editor->getPositionOfMouse(p)) {
        //mouse on editing area
        menu.addAction(ui->actionCompile_Run);
        menu.addAction(ui->actionDebug);
        menu.addSeparator();
        menu.addAction(ui->actionGoto_Declaration);
        menu.addAction(ui->actionGoto_Definition);
        menu.addAction(ui->actionFind_references);

        menu.addSeparator();
        menu.addAction(ui->actionOpen_Containing_Folder);
        menu.addAction(ui->actionOpen_Terminal);

        menu.addSeparator();
        menu.addAction(ui->actionReformat_Code);
        menu.addSeparator();
        menu.addAction(ui->actionCut);
        menu.addAction(ui->actionCopy);
        menu.addAction(ui->actionPaste);
        menu.addAction(ui->actionSelectAll);
        menu.addSeparator();
        menu.addAction(ui->actionAdd_Watch);
        menu.addAction(ui->actionToggle_Breakpoint);
        menu.addAction(ui->actionClear_all_breakpoints);
        menu.addSeparator();
        menu.addAction(ui->actionFile_Properties);

        //these actions needs parser
        ui->actionGoto_Declaration->setEnabled(!editor->parser()->parsing());
        ui->actionGoto_Definition->setEnabled(!editor->parser()->parsing());
        ui->actionFind_references->setEnabled(!editor->parser()->parsing());
    } else {
        //mouse on gutter
        int line;
        if (!editor->getLineOfMouse(line))
            line=-1;
        menu.addAction(ui->actionToggle_Breakpoint);
        menu.addAction(ui->actionBreakpoint_property);
        menu.addAction(ui->actionClear_all_breakpoints);
        ui->actionBreakpoint_property->setEnabled(editor->hasBreakpoint(line));
    }
    menu.exec(editor->viewport()->mapToGlobal(pos));

}

void MainWindow::onEditorTabContextMenu(const QPoint &pos)
{
    int index = ui->EditorTabsLeft->tabBar()->tabAt(pos);
    if (index<0)
        return;
    ui->EditorTabsLeft->setCurrentIndex(index);
    QMenu menu(this);
    QTabBar* tabBar = ui->EditorTabsLeft->tabBar();
    menu.addAction(ui->actionClose);
    menu.addAction(ui->actionClose_All);
    menu.addSeparator();
    menu.addAction(ui->actionOpen_Containing_Folder);
    menu.addAction(ui->actionOpen_Terminal);
    menu.addSeparator();
    menu.addAction(ui->actionFile_Properties);

    menu.exec(tabBar->mapToGlobal(pos));
}

void MainWindow::disableDebugActions()
{
    ui->actionStep_Into->setEnabled(false);
    ui->actionStep_Over->setEnabled(false);
    ui->actionStep_Out->setEnabled(false);
    ui->actionRun_To_Cursor->setEnabled(false);
    ui->actionContinue->setEnabled(false);
    ui->cbEvaluate->setEnabled(false);
    ui->cbMemoryAddress->setEnabled(false);
}

void MainWindow::enableDebugActions()
{
    ui->actionStep_Into->setEnabled(true);
    ui->actionStep_Over->setEnabled(true);
    ui->actionStep_Out->setEnabled(true);
    ui->actionRun_To_Cursor->setEnabled(true);
    ui->actionContinue->setEnabled(true);
    ui->cbEvaluate->setEnabled(true);
    ui->cbMemoryAddress->setEnabled(true);
}

void MainWindow::onTodoParseStarted()
{
    mTodoModel.clear();
}

void MainWindow::onTodoParsing(const QString &filename, int lineNo, int ch, const QString &line)
{
    mTodoModel.addItem(filename,lineNo,ch,line);
}

void MainWindow::onTodoParseFinished()
{

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
        // TODO: should we save watches?
        if (mProject->modified()) {
            QString s;
            if (mProject->name().isEmpty()) {
                s = mProject->filename();
            } else {
                s = mProject->name();
            }
            if (mSystemTurnedOff) {
                mProject->saveAll();
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
                    break;
                case QMessageBox::No:
                    mProject->setModified(false);
                    mProject->saveLayout();
                    break;
                case QMessageBox::Cancel:
                    mProject->saveLayout();
                    return;
                }
            }
        } else
            mProject->saveLayout(); // always save layout, but not when SaveAll has been called

        mClassBrowserModel.beginUpdate();
        {
            auto action2 = finally([this]{
                mClassBrowserModel.endUpdate();
            });
            // Remember it
            pSettings->history().addToOpenedProjects(mProject->filename());

            mEditorList->beginUpdate();
            {
                auto action3 = finally([this]{
                    mEditorList->endUpdate();
                });
                mProject.reset();

                if (!mQuitting && refreshEditor) {
                    //reset Class browsing
                    ui->tabInfos->setCurrentWidget(ui->tabStructure);
                    Editor * e = mEditorList->getEditor();
                    updateClassBrowserForEditor(e);
                } else {
                    mClassBrowserModel.setParser(nullptr);
                    mClassBrowserModel.setCurrentFile("");
                }
            }
        }
        if (!mQuitting) {
            // Clear error browser
            ui->tableIssues->clearIssues();
            updateProjectView();
        }
    }
}

void MainWindow::updateProjectView()
{
    if (mProject) {
        ui->projectView->setModel(mProject->model());
        connect(mProject->model(), &QAbstractItemModel::modelReset,
                ui->projectView,&QTreeView::expandAll);
        ui->projectView->expandAll();
        openCloseLeftPanel(true);
        ui->tabProject->setVisible(true);
        ui->tabInfos->setCurrentWidget(ui->tabProject);
    } else {
        // Clear project browser
        ui->projectView->setModel(nullptr);
        ui->tabProject->setVisible(false);
    }
    updateProjectActions();
}

void MainWindow::onFileChanged(const QString &path)
{
    Editor *e = mEditorList->getOpenedEditorByFilename(path);
    if (e) {
        if (fileExists(path)) {
            e->activate();
            if (QMessageBox::question(this,tr("Compile"),
                                      tr("File '%1' was changed.").arg(path)+"<BR /><BR />" + tr("Reload its content from disk?"),
                                      QMessageBox::Yes|QMessageBox::No,
                                      QMessageBox::No) == QMessageBox::Yes) {
                try {
                    e->loadFile();
                } catch(FileError e) {
                    QMessageBox::critical(this,tr("Error"),e.reason());
                }
            }
        } else {
            if (QMessageBox::question(this,tr("Compile"),
                                      tr("File '%1' was removed.").arg(path)+"<BR /><BR />" + tr("Keep it open?"),
                                      QMessageBox::Yes|QMessageBox::No,
                                      QMessageBox::Yes) == QMessageBox::No) {
                mEditorList->closeEditor(e);
            } else {
                e->setModified(true);
                e->updateCaption();
            }
        }
    }
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

SearchDialog *MainWindow::searchDialog() const
{
    return mSearchDialog;
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
    return mDebugger;
}

CPUDialog *MainWindow::cpuDialog() const
{
    return mCPUDialog;
}


void MainWindow::on_actionNew_triggered()
{
    newEditor();
}

void MainWindow::on_EditorTabsLeft_tabCloseRequested(int index)
{
    Editor* editor = mEditorList->getEditor(index,ui->EditorTabsLeft);
    mEditorList->closeEditor(editor);
}

void MainWindow::on_actionOpen_triggered()
{
    try {
        QString selectedFileFilter;
        selectedFileFilter = pSystemConsts->defaultAllFileFilter();
        QStringList files = QFileDialog::getOpenFileNames(pMainWindow,
            tr("Open"), QString(), pSystemConsts->defaultFileFilters().join(";;"),
            &selectedFileFilter);
        openFiles(files);
    }  catch (FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    Settings::UI& settings = pSettings->ui();
    settings.setMainWindowState(saveState());
    settings.setMainWindowGeometry(saveGeometry());
    settings.setBottomPanelHeight(mBottomPanelHeight);
    settings.setBottomPanelIndex(ui->tabMessages->currentIndex());
    settings.setBottomPanelOpenned(mBottomPanelOpenned);
    settings.setLeftPanelWidth(mLeftPanelWidth);
    settings.setLeftPanelIndex(ui->tabInfos->currentIndex());
    settings.setLeftPanelOpenned(mLeftPanelOpenned);
    settings.save();    

    if (pSettings->editor().autoLoadLastFiles()) {
        saveLastOpens();
    } else {
        //if don't save last open files, close project before editors, to save project openned editors;
        if (mProject) {
            closeProject(false);
        }
    }

    if (!mEditorList->closeAll(false)) {
        event->ignore();
        return ;
    }

    if (pSettings->editor().autoLoadLastFiles()) {
        if (mProject) {
            closeProject(false);
        }
    }


    mCompilerManager->stopCompile();
    mCompilerManager->stopRun();
    mSymbolUsageManager->save();
    event->accept();
    return;
}

void MainWindow::showEvent(QShowEvent *event)
{
    const Settings::UI& settings = pSettings->ui();
    ui->tabMessages->setCurrentIndex(settings.bottomPanelIndex());
    if (settings.bottomPanelOpenned()) {
        mBottomPanelHeight = settings.bottomPanelHeight();
        openCloseBottomPanel(true);
    } else {
        openCloseBottomPanel(false);
        mBottomPanelHeight = settings.bottomPanelHeight();
    }
    ui->tabInfos->setCurrentIndex(settings.leftPanelIndex());
    if (settings.leftPanelOpenned()) {
        mLeftPanelWidth = settings.leftPanelWidth();
        openCloseLeftPanel(true);
    } else {
        openCloseLeftPanel(false);
        mLeftPanelWidth = settings.leftPanelWidth();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()){
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        foreach(const QUrl& url, event->mimeData()->urls()){
            if (!url.isLocalFile())
                continue;
            QString file = url.toLocalFile();
            if (getFileType(file)==FileType::Project) {
                openProject(file);
                return;
            }
        }
        foreach(const QUrl& url, event->mimeData()->urls()){
            if (!url.isLocalFile())
                continue;
            QString file = url.toLocalFile();
            openFile(file);
        }
    }
}

void MainWindow::on_actionSave_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL) {
        try {
            editor->save();
            if (editor->inProject() && (mProject))
                mProject->saveAll();
        } catch(FileError e) {
            QMessageBox::critical(editor,tr("Error"),e.reason());
        }
    }    
}

void MainWindow::on_actionSaveAs_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL) {
        try {
            editor->saveAs();
        } catch(FileError e) {
            QMessageBox::critical(editor,tr("Error"),e.reason());
        }
    }
}

void MainWindow::on_actionOptions_triggered()
{
    bool oldCodeCompletion = pSettings->codeCompletion().enabled();
    PSettingsDialog settingsDialog = SettingsDialog::optionDialog();
    settingsDialog->exec();

    bool newCodeCompletion = pSettings->codeCompletion().enabled();
    if (!oldCodeCompletion && newCodeCompletion) {
        Editor *e = mEditorList->getEditor();
        if (mProject && !e) {
            scanActiveProject(true);
        } else if (mProject && e && e->inProject()) {
            scanActiveProject(true);
        } else if (e) {
            e->reparse();
        }
    }
}

void MainWindow::onCompilerSetChanged(int index)
{
    if (index<0)
        return;
    if (mProject) {
        Editor *e = mEditorList->getEditor();
        if (!e || e->inProject()) {
            mProject->options().compilerSet = index;
            mProject->saveOptions();
            return;
        }
    }
    pSettings->compilerSets().setDefaultIndex(index);
    pSettings->compilerSets().saveDefaultIndex();
}

void MainWindow::onCompileLog(const QString &msg)
{
    ui->txtCompilerOutput->appendPlainText(msg);
    ui->txtCompilerOutput->ensureCursorVisible();
}

void MainWindow::onCompileIssue(PCompileIssue issue)
{
    ui->tableIssues->addIssue(issue);

    // Update tab caption
//    if CompilerOutput.Items.Count = 1 then
//      CompSheet.Caption := Lang[ID_SHEET_COMP] + ' (' + IntToStr(CompilerOutput.Items.Count) + ')';

    if (issue->type == CompileIssueType::Error || issue->type ==
            CompileIssueType::Warning) {
        Editor* e = mEditorList->getOpenedEditorByFilename(issue->filename);
        if (e!=nullptr && (issue->line>0)) {
            int line = issue->line;
            if (line > e->lines()->count())
                return;
            int col = std::min(issue->column,e->lines()->getString(line-1).length()+1);
            if (col < 1)
                col = e->lines()->getString(line-1).length()+1;
            e->addSyntaxIssues(line,col,issue->endColumn,issue->type,issue->description);
        }
    }
}

void MainWindow::onCompileStarted()
{
    ui->txtCompilerOutput->clear();
}

void MainWindow::onCompileFinished()
{
    // Update tab caption
    int i = ui->tabMessages->indexOf(ui->tabIssues);
    if (i==-1)
        return;
    ui->tabMessages->setTabText(i, tr("Issues") +
                                QString(" (%1)").arg(ui->tableIssues->model()->rowCount()));

    // Close it if there's nothing to show
    if (mCheckSyntaxInBack) {
      // check syntax in back, don't change message panel
    } else if (
        (ui->tableIssues->count() == 0)
//        and (ResourceOutput.Items.Count = 0)
//        and devData.AutoCloseProgress
               ) {
        openCloseBottomPanel(false);
        // Or open it if there is anything to show
    } else {
        if (ui->tableIssues->count() > 0) {
            if (ui->tabMessages->currentIndex() != i) {
                ui->tabMessages->setCurrentIndex(i);
            }
//      end else if (ResourceOutput.Items.Count > 0) then begin
//        if MessageControl.ActivePage <> ResSheet then begin
//          MessageControl.ActivePage := ResSheet;
//          fMessageControlChanged := False;
//        end;
//      end;
            openCloseBottomPanel(true);
        }
    }

    Editor * e = mEditorList->getEditor();
    if (e!=nullptr) {
        e->invalidate();
    }

    //run succession task if there aren't any errors
    if (mCompileSuccessionTask && mCompilerManager->compileErrorCount()==0) {
        switch (mCompileSuccessionTask->type) {
        case MainWindow::CompileSuccessionTaskType::Run:
            runExecutable(mCompileSuccessionTask->filename);
            break;
        case MainWindow::CompileSuccessionTaskType::Debug:
            debug();
            break;
        }
        mCompileSuccessionTask.reset();
        // Jump to problem location, sorted by significance
    } else if ((mCompilerManager->compileIssueCount() > 0) && (!mCheckSyntaxInBack)) {
        // First try to find errors
        for (int i=0;i<ui->tableIssues->count();i++) {
            PCompileIssue issue = ui->tableIssues->issue(i);
            if (issue->type == CompileIssueType::Error) {
                if (e && e->filename() != issue->filename)
                    continue;
                ui->tableIssues->selectRow(i);
                QModelIndex index =ui->tableIssues->model()->index(i,0);
                emit ui->tableIssues->doubleClicked(index);
                break;
            }
        }

        // Then try to find warnings
        for (int i=0;i<ui->tableIssues->count();i++) {
            PCompileIssue issue = ui->tableIssues->issue(i);
            if (issue->type == CompileIssueType::Warning) {
                if (e && e->filename() != issue->filename)
                    continue;
                ui->tableIssues->selectRow(i);
                QModelIndex index =ui->tableIssues->model()->index(i,0);
                emit ui->tableIssues->doubleClicked(index);
            }
        }
    }
    mCheckSyntaxInBack=false;
    updateCompileActions();
    updateAppTitle();
}

void MainWindow::onCompileErrorOccured(const QString &reason)
{
    QMessageBox::critical(this,tr("Compile Failed"),reason);
}

void MainWindow::onRunErrorOccured(const QString &reason)
{
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

void MainWindow::cleanUpCPUDialog()
{
    CPUDialog* ptr=mCPUDialog;
    mCPUDialog=nullptr;
    ptr->deleteLater();
}

void MainWindow::onDebugCommandInput(const QString &command)
{
    if (mDebugger->executing()) {
        mDebugger->sendCommand(command,"",true,true);
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
    if (editor != NULL ) {
        editor->undo();
    }
}

void MainWindow::on_actionRedo_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->redo();
    }
}

void MainWindow::on_actionCut_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->cutToClipboard();
    }
}

void MainWindow::on_actionSelectAll_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->selectAll();
    }
}

void MainWindow::on_actionCopy_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->copyToClipboard();
    }
}

void MainWindow::on_actionPaste_triggered()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* data = clipboard->mimeData();
    if (!data)
        return;
    if (data->hasUrls()) {
        QStringList filesToOpen;
        foreach (const QUrl& url, data->urls()) {
            QString s = url.toLocalFile();
            if (!s.isEmpty()) {
                filesToOpen.append(s);
            }
        }
        if (!filesToOpen.isEmpty())
            openFiles(filesToOpen);
    } else {
        Editor * editor = mEditorList->getEditor();
        if (editor != NULL ) {
            editor->pasteFromClipboard();
        }
    }
}

void MainWindow::on_actionIndent_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->tab();
    }
}

void MainWindow::on_actionUnIndent_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->shifttab();
    }
}

void MainWindow::on_actionToggleComment_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->toggleComment();
    }
}

void MainWindow::on_actionUnfoldAll_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        //editor->clearFolds();
    }
}

void MainWindow::on_actionFoldAll_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        //editor->clearFolds();
        //editor->foldAll();
    }
}

void MainWindow::on_tableIssues_doubleClicked(const QModelIndex &index)
{
    PCompileIssue issue = ui->tableIssues->issue(index);
    if (!issue)
        return;

    Editor * editor = mEditorList->getEditorByFilename(issue->filename);
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
                   .arg(QString(QTextCodec::codecForLocale()->name())),
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
    if (index == ui->tabMessages->currentIndex()) {
        openCloseBottomPanel(!mBottomPanelOpenned);
    }
}

void MainWindow::on_tabMessages_currentChanged(int)
{
    openCloseBottomPanel(true);
}

void MainWindow::on_tabMessages_tabBarDoubleClicked(int )
{

}

void MainWindow::on_actionCompile_Run_triggered()
{
    mCompileSuccessionTask = std::make_shared<CompileSuccessionTask>();
    mCompileSuccessionTask->type = CompileSuccessionTaskType::Run;
    compile();
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
    if (!e->inProject()) {
        for (const PBreakpoint& breakpoint:mDebugger->breakpointModel()->breakpoints()) {
            if (e->filename() == breakpoint->filename) {
                return true;
            }
        }
    } else {
        for (const PBreakpoint& breakpoint:mDebugger->breakpointModel()->breakpoints()) {
            Editor* e1 = mEditorList->getOpenedEditorByFilename(breakpoint->filename);
            if (e1->inProject()) {
                return true;
            }
        }
    }
    return false;
}

void MainWindow::on_actionStep_Over_triggered()
{
    if (mDebugger->executing()) {
        //WatchView.Items.BeginUpdate();
        mDebugger->invalidateAllVars();
        mDebugger->sendCommand("next", "");
        mDebugger->updateDebugInfo();
        mDebugger->refreshWatchVars();
    }
}

void MainWindow::on_actionStep_Into_triggered()
{
    if (mDebugger->executing()) {
        //WatchView.Items.BeginUpdate();
        mDebugger->invalidateAllVars();
        mDebugger->sendCommand("step", "");
        mDebugger->updateDebugInfo();
        mDebugger->refreshWatchVars();
    }

}

void MainWindow::on_actionStep_Out_triggered()
{
    if (mDebugger->executing()) {
        //WatchView.Items.BeginUpdate();
        mDebugger->invalidateAllVars();
        mDebugger->sendCommand("finish", "");
        mDebugger->updateDebugInfo();
        mDebugger->refreshWatchVars();
    }

}

void MainWindow::on_actionRun_To_Cursor_triggered()
{
    if (mDebugger->executing()) {
        Editor *e=mEditorList->getEditor();
        if (e!=nullptr) {
            //WatchView.Items.BeginUpdate();
            mDebugger->invalidateAllVars();
            mDebugger->sendCommand("tbreak", QString(" %1").arg(e->caretY()));
            mDebugger->sendCommand("continue", "");
            mDebugger->updateDebugInfo();
            mDebugger->refreshWatchVars();
        }
    }

}

void MainWindow::on_actionContinue_triggered()
{
    if (mDebugger->executing()) {
        //WatchView.Items.BeginUpdate();
        mDebugger->invalidateAllVars();
        mDebugger->sendCommand("continue", "");
        mDebugger->updateDebugInfo();
        mDebugger->refreshWatchVars();
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
    if (mCPUDialog==nullptr) {
        mCPUDialog = new CPUDialog(this);
        connect(mCPUDialog, &CPUDialog::closed, this, &MainWindow::cleanUpCPUDialog);
    }
    mCPUDialog->show();
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::onDebugEvaluateInput()
{
    QString s=ui->cbEvaluate->currentText().trimmed();
    if (!s.isEmpty()) {
        connect(mDebugger, &Debugger::evalValueReady,
                   this, &MainWindow::onEvalValueReady);
        mDebugger->sendCommand("print",s,false);
    }
}

void MainWindow::onDebugMemoryAddressInput()
{
    QString s=ui->cbMemoryAddress->currentText().trimmed();
    if (!s.isEmpty()) {
        connect(mDebugger, &Debugger::memoryExamineReady,
                   this, &MainWindow::onMemoryExamineReady);
        mDebugger->sendCommand("x/64bx",s,false);
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

void MainWindow::onEvalValueReady(const QString &value)
{
    updateDebugEval(value);
    disconnect(mDebugger, &Debugger::evalValueReady,
               this, &MainWindow::onEvalValueReady);
}

void MainWindow::onMemoryExamineReady(const QStringList &value)
{
    ui->txtMemoryView->clear();
    foreach (QString s, value) {
        s.replace("\t","  ");
        ui->txtMemoryView->appendPlainText(s);
    }
    ui->txtMemoryView->moveCursor(QTextCursor::Start);
    disconnect(mDebugger, &Debugger::memoryExamineReady,
               this, &MainWindow::onMemoryExamineReady);
}

void MainWindow::onLocalsReady(const QStringList &value)
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
    if (mSearchDialog==nullptr) {
        mSearchDialog = new SearchDialog(this);
    }
    QString s = e->wordAtCursor();
    mSearchDialog->find(s);
}

void MainWindow::on_actionFind_in_files_triggered()
{
    if (mSearchDialog==nullptr) {
        mSearchDialog = new SearchDialog(this);
    }
    Editor *e = mEditorList->getEditor();
    if (e) {
        QString s = e->wordAtCursor();
        mSearchDialog->findInFiles(s);
    } else {
        mSearchDialog->findInFiles("");
    }
}

void MainWindow::on_actionReplace_triggered()
{
    Editor *e = mEditorList->getEditor();
    if (!e)
        return;
    if (mSearchDialog==nullptr) {
        mSearchDialog = new SearchDialog(this);
    }
    QString s = e->wordAtCursor();
    mSearchDialog->replace(s,s);
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
    if (results && results->searchType == SearchType::Search) {
        ui->btnSearchAgin->setEnabled(true);
    } else {
        ui->btnSearchAgin->setEnabled(false);
    }
}

void MainWindow::on_btnSearchAgin_clicked()
{
    if (mSearchDialog==nullptr) {
        mSearchDialog = new SearchDialog(this);
    }
    PSearchResults results=mSearchResultModel.currentResults();
    if (results){
        mSearchDialog->findInFiles(results->keyword,
                                   results->scope,
                                   results->options);
    }
}

void MainWindow::on_actionRemove_Watch_triggered()
{
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
}


void MainWindow::on_actionRemove_All_Watches_triggered()
{
    mDebugger->removeWatchVars(true);
}


void MainWindow::on_actionModify_Watch_triggered()
{

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


void MainWindow::on_tabInfos_tabBarClicked(int index)
{
    if (index == ui->tabInfos->currentIndex()) {
        openCloseLeftPanel(!mLeftPanelOpenned);
    }
}


void MainWindow::on_splitterInfos_splitterMoved(int, int)
{
    QList<int> sizes = ui->splitterInfos->sizes();
    mLeftPanelWidth = sizes[0];
}


void MainWindow::on_splitterMessages_splitterMoved(int, int)
{
    QList<int> sizes = ui->splitterMessages->sizes();
    mBottomPanelHeight = sizes[1];
}


void MainWindow::on_EditorTabsLeft_tabBarDoubleClicked(int index)
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
    mClosing = true;
    mEditorList->closeAll(mSystemTurnedOff);
    mClosing = false;
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
    int line;
    if (editor && editor->pointToLine(mEditorContextMenuPos,line))
        editor->toggleBreakpoint(line);
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
    int line;
    if (editor && editor->pointToLine(mEditorContextMenuPos,line)) {
        if (editor->hasBreakpoint(line))
            editor->modifyBreakpointProperty(line);
    }

}


void MainWindow::on_actionGoto_Declaration_triggered()
{
    Editor * editor = mEditorList->getEditor();
    BufferCoord pos;
    if (editor && editor->pointToCharLine(mEditorContextMenuPos,pos)) {
        editor->gotoDeclaration(pos);
    }
}


void MainWindow::on_actionGoto_Definition_triggered()
{
    Editor * editor = mEditorList->getEditor();
    BufferCoord pos;
    if (editor && editor->pointToCharLine(mEditorContextMenuPos,pos)) {
        editor->gotoDefinition(pos);
    }
}


void MainWindow::on_actionFind_references_triggered()
{
    Editor * editor = mEditorList->getEditor();
    BufferCoord pos;
    if (editor && editor->pointToCharLine(mEditorContextMenuPos,pos)) {
        CppRefacter refactor;
        refactor.findOccurence(editor,pos);
        showSearchPanel(true);
    }
}


void MainWindow::on_actionOpen_Containing_Folder_triggered()
{
    Editor* editor = mEditorList->getEditor();
    if (editor) {
        QFileInfo info(editor->filename());
        if (!info.path().isEmpty()) {
            QDesktopServices::openUrl(
                        QUrl("file:///"+
                             includeTrailingPathDelimiter(info.path()),QUrl::TolerantMode));
        }
    }
}


void MainWindow::on_actionOpen_Terminal_triggered()
{
    Editor* editor = mEditorList->getEditor();
    if (editor) {
        QFileInfo info(editor->filename());
        if (!info.path().isEmpty()) {
            openShell(info.path(),"cmd.exe");
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
        Editor *e = mEditorList->getEditorByFilename(filename);
        if (e) {
            e->setCaretPositionAndActivate(line,start);
        }
    }
}


void MainWindow::on_tblStackTrace_doubleClicked(const QModelIndex &index)
{
    PTrace trace = mDebugger->backtraceModel()->backtrace(index.row());
    if (trace) {
        Editor *e = mEditorList->getEditorByFilename(trace->filename);
        if (e) {
            e->setCaretPositionAndActivate(trace->line,1);
        }
    }
}


void MainWindow::on_tblBreakpoints_doubleClicked(const QModelIndex &index)
{
    PBreakpoint breakpoint = mDebugger->breakpointModel()->breakpoint(index.row());
    if (breakpoint) {
        Editor * e = mEditorList->getEditorByFilename(breakpoint->filename);
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
    if (!index.isValid())
        return;
    FolderNode * node = static_cast<FolderNode*>(index.internalPointer());
    if (!node)
        return;
    if (node->unitIndex>=0) {
        mProject->openUnit(node->unitIndex);
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
    QString oldName = mProject->name();
    PSettingsDialog dialog = SettingsDialog::projectOptionDialog();
    dialog->exec();
}


void MainWindow::on_actionNew_Project_triggered()
{
    NewProjectDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
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
        QDir dir(dialog.getLocation());
        if (!dir.exists()) {
            if (QMessageBox::question(this,
                                      tr("Folder not exist"),
                                      tr("Folder '%1' doesn't exist. Create it now?").arg(dialog.getLocation()),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::Yes) != QMessageBox::Yes) {
                return;
            }
            if (!dir.mkpath(dialog.getLocation())) {
                QMessageBox::critical(this,
                                      tr("Can't create folder"),
                                      tr("Failed to create folder '%1'.").arg(dialog.getLocation()),
                                      QMessageBox::Yes);
                return;
            }
        }

//     if cbDefault.Checked then
//        devData.DefCpp := rbCpp.Checked;

        s = includeTrailingPathDelimiter(dialog.getLocation())
                + dialog.getProjectName() + "." + DEV_PROJECT_EXT;

        if (fileExists(s)) {
            QString saveName = QFileDialog::getSaveFileName(
                        this,
                        tr("Save new project as"),
                        dialog.getLocation(),
                        tr("Red panda Dev-C++ project file (*.dev)"));
            if (!saveName.isEmpty()) {
                s = saveName;
            }
        }

        // Create an empty project
        mProject = std::make_shared<Project>(s,dialog.getProjectName());
        if (!mProject->assignTemplate(dialog.getTemplate())) {
            mProject = nullptr;
            QMessageBox::critical(this,
                                  tr("New project fail"),
                                  tr("Can't assign project template"),
                                  QMessageBox::Ok);
        }
        mProject->saveAll();
        updateProjectView();
    }
}


void MainWindow::on_actionSaveAll_triggered()
{
    // Pause the change notifier
    bool oldBlock = mFileSystemWatcher.blockSignals(true);
    auto action = finally([oldBlock,this] {
        mFileSystemWatcher.blockSignals(oldBlock);
    });
    if (mProject) {
        mProject->saveAll();
    }

    // Make changes to files
    for (int i=0;i<mEditorList->pageCount();i++) {
        Editor * e= (*mEditorList)[i];
        if (e->modified() && !e->inProject()) {
            if (!e->save())
                break;
        }
    }
    updateAppTitle();
}


void MainWindow::on_actionProject_New_File_triggered()
{
    int idx = -1;
    if (!mProject)
        return;
    QModelIndex current = ui->projectView->currentIndex();
    FolderNode * node = nullptr;
    if (current.isValid()) {
        node = static_cast<FolderNode*>(current.internalPointer());
    }
    PProjectUnit newUnit = mProject->newUnit(
                mProject->pointerToNode(node) );
    idx = mProject->units().count()-1;
    mProject->saveUnits();
    updateProjectView();
    Editor * editor = mProject->openUnit(idx);
    editor->setModified(true);
    editor->activate();
}


void MainWindow::on_actionAdd_to_project_triggered()
{
    if (!mProject)
        return;
    QFileDialog dialog(this,tr("Add to project"),
                       mProject->directory(),
                       pSystemConsts->defaultFileFilters().join(";;"));
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setOption(QFileDialog::DontConfirmOverwrite,true);
    if (dialog.exec()) {
        QModelIndex current = ui->projectView->currentIndex();
        FolderNode * node = nullptr;
        if (current.isValid()) {
            node = static_cast<FolderNode*>(current.internalPointer());
        }
        PFolderNode folderNode =  mProject->pointerToNode(node);
        foreach (const QString& filename, dialog.selectedFiles()) {
            mProject->addUnit(filename,folderNode,false);
            mProject->cppParser()->addFileToScan(filename);
        }
        mProject->rebuildNodes();
        mProject->saveUnits();
        parseFileList(mProject->cppParser());
        updateProjectView();
    }
}


void MainWindow::on_actionRemove_from_project_triggered()
{
    if (!mProject)
        return;
    if (!ui->projectView->selectionModel()->hasSelection())
        return;
    mProject->model()->beginUpdate();
    QSet<int> selected;
    foreach (const QModelIndex& index, ui->projectView->selectionModel()->selectedIndexes()){
        if (!index.isValid())
            continue;
        FolderNode * node = static_cast<FolderNode*>(index.internalPointer());
        PFolderNode folderNode =  mProject->pointerToNode(node);
        if (!folderNode)
            continue;
        selected.insert(folderNode->unitIndex);
    };
    for (int i=mProject->units().count()-1;i>=0;i--) {
        if (selected.contains(i)) {
            mProject->removeEditor(i,true);
        }
    }

    mProject->saveUnits();
    mProject->model()->endUpdate();
    updateProjectView();
}


void MainWindow::on_actionView_Makefile_triggered()
{
    if (!mProject)
        return;
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
    openShell(mProject->directory(),"cmd.exe");
}

const std::shared_ptr<QHash<StatementKind, QColor> > &MainWindow::statementColors() const
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
    filename = statement->fileName;
    line = statement->line;
    Editor* e = pMainWindow->editorList()->getEditorByFilename(filename);
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

void MainWindow::on_EditorTabsLeft_currentChanged(int index)
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->reparseTodo();
    }
}


void MainWindow::on_EditorTabsRight_currentChanged(int index)
{
    Editor * editor = mEditorList->getEditor();
    if (editor) {
        editor->reparseTodo();
    }
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
    editor->beginUpdate();
//    mClassBrowserModel.beginUpdate();
    QCursor oldCursor = editor->cursor();
    editor->setCursor(Qt::CursorShape::WaitCursor);
    auto action = finally([this,oldCursor,editor]{
        editor->endUpdate();
//        mClassBrowserModel.EndTreeUpdate;
        editor->setCursor(oldCursor);
    });
    QString word = editor->wordAtCursor();
    if (word.isEmpty())
        return;

//    if (!isIdentifier(word)) {
//        return;
//    }

    if (isKeyword(word)) {
        return;
    }

    BufferCoord oldCaretXY = editor->caretXY();
    if (editor->inProject() && mProject) {
        mProject->cppParser()->parseFileList();
        BufferCoord pBeginPos,pEndPos;
        QString phrase = getWordAtPosition(editor,oldCaretXY,pBeginPos,pEndPos,Editor::WordPurpose::wpInformation);
        // Find it's definition
        PStatement oldStatement = editor->parser()->findStatementOf(
                    editor->filename(),
                    phrase,
                    oldCaretXY.Line);
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

    bool ok;
    QString newWord = QInputDialog::getText(editor,
                                            tr("Rename Symbol"),
                                            tr("New Name"),
                                            QLineEdit::Normal,word, &ok);
    if (!ok)
        return;

    if (word == newWord)
        return;

    PCppParser parser = editor->parser();
    //here we must reparse the file in sync, or rename may fail
    parser->parseFile(editor->filename(), editor->inProject(), false, false);
    CppRefacter refactor;
    refactor.renameSymbol(editor,oldCaretXY,word,newWord);
    editor->reparse();

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

Ui::MainWindow *MainWindow::mainWidget() const
{
    return ui;
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
        QStringList contents;
        Editor* editor = mEditorList->getEditorByFilename(file->filename);
        if (!editor) {
            QMessageBox::critical(this,
                                  tr("Replace Error"),
                                  tr("Can't open file '%1' for replace!").arg(file->filename));
            return;
        }
        contents = editor->contents();
        for (int i=file->results.count()-1;i>=0;i--) {
            const PSearchResultTreeItem& item = file->results[i];
            QString line = contents[item->line-1];
            if (line.mid(item->start-1,results->keyword.length())!=results->keyword) {
                QMessageBox::critical(editor,
                            tr("Replace Error"),
                            tr("Contents has changed since last search!"));
                return;
            }
            line.remove(item->start-1,results->keyword.length());
            line.insert(item->start-1, newWord);
            contents[item->line-1] = line;
        }
        editor->selectAll();
        editor->setSelText(contents.join(editor->lineBreak()));
    }
    showSearchReplacePanel(false);
    openCloseBottomPanel(false);
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

