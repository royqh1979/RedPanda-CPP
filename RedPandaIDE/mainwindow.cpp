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


#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
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

    ui->actionIndent->setShortcut(Qt::Key_Tab);
    ui->actionUnIndent->setShortcut(Qt::Key_Tab | Qt::ShiftModifier);

    ui->tableIssues->setErrorColor(QColor("Red"));
    ui->tableIssues->setWarningColor(QColor("Orange"));


    mMenuEncoding = new QMenu();
    mMenuEncoding->setTitle(tr("File Encoding"));
    mMenuEncoding->addAction(ui->actionAuto_Detect);
    mMenuEncoding->addAction(ui->actionEncode_in_ANSI);
    mMenuEncoding->addAction(ui->actionEncode_in_UTF_8);
    mMenuEncoding->addSeparator();
    mMenuEncoding->addAction(ui->actionConvert_to_ANSI);
    mMenuEncoding->addAction(ui->actionConvert_to_UTF_8);
    ui->menuEdit->insertMenu(ui->actionFoldAll,mMenuEncoding);
    ui->menuEdit->insertSeparator(ui->actionFoldAll);
    ui->actionAuto_Detect->setCheckable(true);
    ui->actionEncode_in_ANSI->setCheckable(true);
    ui->actionEncode_in_UTF_8->setCheckable(true);

    mMenuRecentFiles = new QMenu();
    mMenuRecentFiles->setTitle(tr("Recent Files"));
    ui->menuFile->insertMenu(ui->actionExit, mMenuRecentFiles);
    ui->menuFile->insertSeparator(ui->actionExit);
    rebuildOpenedFileHisotryMenu();

    mCPUDialog = nullptr;

    updateEditorActions();
    updateCaretActions();
    applySettings();
    applyUISettings();


    connect(ui->debugConsole,&QConsole::commandInput,this,&MainWindow::onDebugCommandInput);
    connect(ui->cbEvaluate->lineEdit(), &QLineEdit::returnPressed,
            this, &MainWindow::onDebugEvaluateInput);

    mSearchResultTreeModel = std::make_shared<SearchResultTreeModel>(&mSearchResultModel);
    mSearchResultListModel = std::make_shared<SearchResultListModel>(&mSearchResultModel);
    mSearchViewDelegate = std::make_shared<SearchResultTreeViewDelegate>(mSearchResultTreeModel);
    ui->cbSearchHistory->setModel(mSearchResultListModel.get());
    ui->searchView->setModel(mSearchResultTreeModel.get());
    ui->searchView->setItemDelegate(mSearchViewDelegate.get());
    connect(mSearchResultTreeModel.get() , &QAbstractItemModel::modelReset,
            ui->searchView,&QTreeView::expandAll);
    ui->replacePanel->setVisible(false);

    //class browser
    ui->classBrowser->setModel(&mClassBrowserModel);

    connect(&mFileSystemWatcher,&QFileSystemWatcher::fileChanged,
            this, &MainWindow::onFileChanged);

    mCompletionPopup = std::make_shared<CodeCompletionPopup>();
    mHeaderCompletionPopup = std::make_shared<HeaderCompletionPopup>();

    updateAppTitle();

    connect(&mAutoSaveTimer, &QTimer::timeout,
            this, &MainWindow::onAutoSaveTimeout);
    resetAutoSaveTimer();

    connect(ui->menuFile, &QMenu::aboutToShow,
            this,&MainWindow::rebuildOpenedFileHisotryMenu);

    buildContextMenus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateForEncodingInfo() {
    Editor * editor = mEditorList->getEditor();
    if (editor!=NULL) {
        mFileEncodingStatus->setText(
                    QString("%1(%2)")
                    .arg(QString(editor->encodingOption())
                         ,QString(editor->fileEncoding())));
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
        ui->actionSaveAll->setEnabled(false);
        ui->actionSelectAll->setEnabled(false);
        ui->actionToggleComment->setEnabled(false);
        ui->actionUnIndent->setEnabled(false);
        ui->actionUndo->setEnabled(false);
        ui->actionUnfoldAll->setEnabled(false);

        ui->actionCompile->setEnabled(false);
        ui->actionCompile_Run->setEnabled(false);
        ui->actionRun->setEnabled(false);
        ui->actionRebuild->setEnabled(false);
        ui->actionStop_Execution->setEnabled(false);

        ui->actionDebug->setEnabled(false);
        ui->actionStep_Over->setEnabled(false);
        ui->actionStep_Into->setEnabled(false);
        ui->actionStep_Out->setEnabled(false);
        ui->actionContinue->setEnabled(false);
        ui->actionRun_To_Cursor->setEnabled(false);

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
        ui->actionSaveAll->setEnabled(true);
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

        updateCompileActions();
    }

}

void MainWindow::updateCompileActions()
{
    if (mCompilerManager->compiling() || mCompilerManager->running() || mDebugger->executing()) {
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
    ui->actionStep_Into->setEnabled(mDebugger->executing());
    ui->actionStep_Out->setEnabled(mDebugger->executing());
    ui->actionStep_Over->setEnabled(mDebugger->executing());
    ui->actionContinue->setEnabled(mDebugger->executing());
    ui->actionRun_To_Cursor->setEnabled(mDebugger->executing());
    ui->actionStop_Execution->setEnabled(mCompilerManager->running() || mDebugger->executing());
}

void MainWindow::updateEditorColorSchemes()
{
    mEditorList->applyColorSchemes(pSettings->editor().colorScheme());
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
    QString str;
    QCoreApplication *app = QApplication::instance();
    if (e && !e->inProject()) {
        if (e->modified())
          str = e->filename() + " [*]";
        else
          str = e->filename();
        if (mDebugger->executing()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,appName,tr("Debugging"),DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,appName,tr("Debugging")));
        } else if (mCompilerManager->running()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,appName,tr("Running"),DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,appName,tr("Running")));
        } else if (mCompilerManager->compiling()) {
            setWindowTitle(QString("%1 - [%2] - %3 %4")
                           .arg(str,appName,tr("Compiling"),DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - [%2] - %3")
                                    .arg(str,appName,tr("Compiling")));
        } else {
            this->setWindowTitle(QString("%1 - %2 %3")
                                 .arg(str,appName,DEVCPP_VERSION));
            app->setApplicationName(QString("%1 - %2")
                                    .arg(str,appName));
        }
    }
// else if Assigned(fProject) then begin
//  if fDebugger.Executing then begin
//    Caption := Format('%s - [%s] - [Debugging] - %s %s',
//      [fProject.Name, ExtractFilename(fProject.Filename), appName, DEVCPP_VERSION]);
//    Application.Title := Format('%s - [Debugging] - %s', [fProject.Name, appName]);
//  end else if devExecutor.Running then begin
//    Caption := Format('%s - [%s] - [Executing] - %s %s',
//      [fProject.Name, ExtractFilename(fProject.Filename), appName, DEVCPP_VERSION]);
//    Application.Title := Format('%s - [Executing] - %s', [fProject.Name, appName]);
//  end else if fCompiler.Compiling then begin
//    Caption := Format('%s - [%s] - [Compiling] - %s %s',
//      [fProject.Name, ExtractFilename(fProject.Filename), appName, DEVCPP_VERSION]);
//    Application.Title := Format('%s - [Compiling] - %s', [fProject.Name, appName]);
//  end else begin
//    Caption := Format('%s - [%s] - %s %s',
//      [fProject.Name, ExtractFilename(fProject.Filename), appName, DEVCPP_VERSION]);
//    Application.Title := Format('%s - %s', [fProject.Name, appName]);
//  end;
    else {
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
}

void MainWindow::rebuildOpenedFileHisotryMenu()
{
    mMenuRecentFiles->clear();
    foreach (QAction* action,mRecentFileActions) {
        action->setParent(nullptr);
        action->deleteLater();
    }
    mRecentFileActions.clear();
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
        editor = mEditorList->newEditor(filename,ENCODING_AUTO_DETECT,
                                        false,false);
        editor->activate();
        this->updateForEncodingInfo();
    } catch (FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
}

void MainWindow::setupActions() {

}

void MainWindow::updateCompilerSet()
{
    mCompilerSet->clear();
    int index=pSettings->compilerSets().defaultIndex();
    for (size_t i=0;i<pSettings->compilerSets().list().size();i++) {
        mCompilerSet->addItem(pSettings->compilerSets().list()[i]->name());
    }
    if (index < 0 || index>=mCompilerSet->count()) {
        index = 0;
    }
    mCompilerSet->setCurrentIndex(index);
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
    e->clearSyntaxIssues();
    ui->tableIssues->clearIssues();
    mCompilerManager->checkSyntax(e->filename(),e->lines()->text(),
                                  e->fileEncoding() == ENCODING_ASCII);
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
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->clearSyntaxIssues();
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
        updateAppTitle();
        mCompilerManager->compile(editor->filename(),editor->fileEncoding(),rebuild);
        return true;
    }
    return false;
}

void MainWindow::runExecutable(const QString &exeName,const QString &filename)
{
    // Check if it exists
    if (!QFile(exeName).exists()) {
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
    mCompilerManager->run(exeName,"",QFileInfo(exeName).absolutePath());
}

void MainWindow::runExecutable()
{
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
        break;
//      cttProject: begin
//          // Check if we enabled proper options
//          DebugEnabled := fProject.GetCompilerOption('-g3') <> '0';
//          StripEnabled := fProject.GetCompilerOption('-s') <> '0';

//          // Ask the user if he wants to enable debugging...
//          if (not DebugEnabled or StripEnabled) then begin
//            if  (MessageDlg(Lang[ID_MSG_NODEBUGSYMBOLS], mtConfirmation, [mbYes,
//              mbNo], 0) = mrYes) then begin

//              // Enable debugging, disable stripping
//              fProject.SetCompilerOption('-g3', '1');
//              fProject.SetCompilerOption('-s', '0');

//              fCompSuccessAction := csaDebug;
//              actRebuildExecute(nil);
//            end;
//            Exit;
//          end;

//          // Did we compile?
//          if not FileExists(fProject.Executable) then begin
//            if MessageDlg(Lang[ID_ERR_PROJECTNOTCOMPILEDSUGGEST], mtConfirmation, [mbYes, mbNo], 0) = mrYes then begin
//              fCompSuccessAction := csaDebug;
//              actCompileExecute(nil);
//            end;
//            Exit;
//          end;


//          // Did we choose a host application for our DLL?
//          if fProject.Options.typ = dptDyn then begin
//            if fProject.Options.HostApplication = '' then begin
//              MessageDlg(Lang[ID_ERR_HOSTMISSING], mtWarning, [mbOK], 0);
//              exit;
//            end else if not FileExists(fProject.Options.HostApplication) then begin
//              MessageDlg(Lang[ID_ERR_HOSTNOTEXIST], mtWarning, [mbOK], 0);
//              exit;
//            end;
//          end;

//          // Reset UI, remove invalid breakpoints
//          PrepareDebugger;

//          filepath := fProject.Executable;

//        if (!mDebugger->start())
//            return;
//          fDebugger.SendCommand('file', '"' + StringReplace(filepath, '\', '/', [rfReplaceAll]) + '"');

//          if fProject.Options.typ = dptDyn then
//            fDebugger.SendCommand('exec-file', '"' + StringReplace(fProject.Options.HostApplication, '\', '/',
//              [rfReplaceAll])
//              + '"');

//          for i:=0 to fProject.Units.Count-1 do begin
//            fDebugger.SendCommand('dir', '"'+StringReplace(
//              ExtractFilePath(fProject.Units[i].FileName),'\', '/',[rfReplaceAll])
//              + '"');
//          end;
//          for i:=0 to fProject.Options.Includes.Count-1 do begin
//            fDebugger.SendCommand('dir', '"'+StringReplace(
//              fProject.Options.Includes[i],'\', '/',[rfReplaceAll])
//              + '"');
//          end;
//          for i:=0 to fProject.Options.Libs.Count-1 do begin
//            fDebugger.SendCommand('dir', '"'+StringReplace(
//              fProject.Options.Includes[i],'\', '/',[rfReplaceAll])
//              + '"');
//          end;

//        end;
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
    foreach (QString dir,compilerSet->libDirs()) {
        mDebugger->sendCommand("dir",
                               QString("\"%1\"").arg(dir.replace('\\','/')));
    }
    foreach (QString dir,compilerSet->defaultLibDirs()) {
        mDebugger->sendCommand("dir",
                               QString("\"%1\"").arg(dir.replace('\\','/')));
    }
    // Add include folders
    foreach (QString dir,compilerSet->CIncludeDirs()) {
        mDebugger->sendCommand("dir",
                               QString("\"%1\"").arg(dir.replace('\\','/')));
    }
    foreach (QString dir,compilerSet->CppIncludeDirs()) {
        mDebugger->sendCommand("dir",
                               QString("\"%1\"").arg(dir.replace('\\','/')));
    }
    foreach (QString dir,compilerSet->defaultCIncludeDirs()) {
        mDebugger->sendCommand("dir",
                               QString("\"%1\"").arg(dir.replace('\\','/')));
    }
    foreach (QString dir,compilerSet->defaultCppIncludeDirs()) {
        mDebugger->sendCommand("dir",
                               QString("\"%1\"").arg(dir.replace('\\','/')));
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
//params := '';
//if fCompiler.UseRunParams then
//  params := params + ' ' + fProject.Options.CmdLineArgs;
//if fCompiler.UseInputFile then
//  params := params + ' < "' + fCompiler.InputFile + '"';

//fDebugger.SendCommand('start', params);
//UpdateDebugInfo;
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

//fDebugger.SendCommand('run', params);
//UpdateDebugInfo;
            break;
        }
    }
}

void MainWindow::showSearchPanel()
{
    openCloseBottomPanel(true);
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
    int idxClose = ui->tabMessages->indexOf(ui->tabClose);
    ui->tabMessages->setTabVisible(idxClose,open);
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

void MainWindow::buildContextMenus()
{
    ui->watchView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->watchView,&QWidget::customContextMenuRequested,
            this, &MainWindow::onWatchViewContextMenu);

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

    if (pSettings->compilerSets().defaultSet()) {
        foreach(const QString& dir, pSettings->compilerSets().defaultSet()->binDirs()) {
#ifdef Q_OS_WIN
            path+=";";
#else
            path+=":";
#endif
            path+=dir;
        }
    }
#ifdef Q_OS_WIN
            path+=";";
#else
            path+=":";
#endif
    path+=pSettings->dirs().app();
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

void MainWindow::onEditorContextMenu(const QPoint &pos)
{
    Editor * editor = mEditorList->getEditor();
    if (!editor)
        return;
    QMenu menu(this);
    BufferCoord p;
    mContextMenuPos = pos;
    if (editor->GetPositionOfMouse(p)) {
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
        if (!editor->GetLineOfMouse(line))
            line=-1;
        menu.addAction(ui->actionToggle_Breakpoint);
        menu.addAction(ui->actionBreakpoint_property);
        menu.addAction(ui->actionClear_all_breakpoints);
        ui->actionBreakpoint_property->setEnabled(editor->hasBreakpoint(line));
    }
    menu.exec(editor->viewport()->mapToGlobal(pos));

}

void MainWindow::onFileChanged(const QString &path)
{
    Editor *e = mEditorList->getOpenedEditorByFilename(path);
    if (e) {
        if (QFile(path).exists()) {
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
    try {
        Editor * editor=mEditorList->newEditor("",ENCODING_AUTO_DETECT,false,true);
        editor->activate();
        updateForEncodingInfo();
    }  catch (FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
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
        if (pSettings->editor().defaultFileCpp()){
            selectedFileFilter = pSystemConsts->defaultCPPFileFilter();
        } else {
            selectedFileFilter = pSystemConsts->defaultCFileFilter();
        }
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

    if (!mEditorList->closeAll(false)) {
        event->ignore();
        return ;
    }

    delete mEditorList;
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

void MainWindow::on_actionSave_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL) {
        try {
            editor->save();
        } catch(FileError e) {
            QMessageBox::critical(this,tr("Error"),e.reason());
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
            QMessageBox::critical(this,tr("Error"),e.reason());
        }
    }
}

void MainWindow::on_actionOptions_triggered()
{
    SettingsDialog settingsDialog;
    settingsDialog.exec();
}

void MainWindow::onCompilerSetChanged(int index)
{
    if (index<0)
        return;
    pSettings->compilerSets().setDefaultIndex(index);
    pSettings->compilerSets().saveDefaultIndex();
}

void MainWindow::onCompileLog(const QString &msg)
{
    ui->txtCompilerOutput->appendPlainText(msg);
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

    // Jump to problem location, sorted by significance
    if ((mCompilerManager->compileErrorCount() > 0) && (!mCheckSyntaxInBack)) {
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
        // Then try to find anything with a line number...
//      for I := 0 to CompilerOutput.Items.Count - 1 do begin
//        if not SameStr(CompilerOutput.Items[I].Caption, '') then begin
//          CompilerOutput.Selected := CompilerOutput.Items[I];
//          CompilerOutput.Selected.MakeVisible(False);
//          CompilerOutputDblClick(CompilerOutput);
//          Exit;
//        end;
//      end;

      // Then try to find a resource error
//      if ResourceOutput.Items.Count > 0 then begin
//        ResourceOutput.Selected := ResourceOutput.Items[0];
//        ResourceOutput.Selected.MakeVisible(False);
//        CompilerOutputDblClick(ResourceOutput);
//      end;
    } else {
        if (mCompileSuccessionTask) {
            switch (mCompileSuccessionTask->type) {
            case MainWindow::CompileSuccessionTaskType::Run:
                runExecutable(mCompileSuccessionTask->filename);
                break;
            case MainWindow::CompileSuccessionTaskType::Debug:
                debug();
                break;
            }
            mCompileSuccessionTask.reset();
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
        mDebugger->sendCommand(command,"");
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
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->pasteFromClipboard();
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
        editor->untab();
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

void MainWindow::on_tabMessages_currentChanged(int index)
{
    int idxClose = ui->tabMessages->indexOf(ui->tabClose);
    if (index == idxClose) {
        openCloseBottomPanel(false);
    } else {
        openCloseBottomPanel(true);
    }
}

void MainWindow::on_tabMessages_tabBarDoubleClicked(int index)
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
//        if ((mProject) and (e.InProject or (fProject.MakeFileName = e.FileName)) then begin
//            Result := cttProject;
//        end else begin
//          Result := cttFile;
//        end;
        target = CompileTarget::File;
    }
//      // No editors have been opened. Check if a project is open
//    end else if Assigned(fProject) then begin
//      Result := cttProject;

//      // No project, no editor...
//    end else begin
//      Result := cttNone;
//    end;
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
    if (e==nullptr)
        return;
    if (e->selAvail()) {
        s = e->selText();
    } else {
        s = e->WordAtCursor();
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

void MainWindow::on_actionFind_triggered()
{
    Editor *e = mEditorList->getEditor();
    if (!e)
        return;
    if (mSearchDialog==nullptr) {
        mSearchDialog = new SearchDialog(this);
    }
    QString s = e->WordAtCursor();
    mSearchDialog->find(s);
}

void MainWindow::on_actionFind_in_files_triggered()
{
    if (mSearchDialog==nullptr) {
        mSearchDialog = new SearchDialog(this);
    }
    Editor *e = mEditorList->getEditor();
    if (e) {
        QString s = e->WordAtCursor();
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
    QString s = e->WordAtCursor();
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
    QList<int> sizes = ui->splitterMessages->sizes();
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
    if (editor && editor->PointToLine(mContextMenuPos,line))
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
    if (editor && editor->PointToLine(mContextMenuPos,line)) {
        if (editor->hasBreakpoint(line))
            editor->modifyBreakpointProperty(line);
    }

}


void MainWindow::on_actionGoto_Declaration_triggered()
{
    Editor * editor = mEditorList->getEditor();
    BufferCoord pos;
    if (editor && editor->PointToCharLine(mContextMenuPos,pos)) {
        editor->gotoDeclaration(pos);
    }
}


void MainWindow::on_actionGoto_Definition_triggered()
{
    Editor * editor = mEditorList->getEditor();
    BufferCoord pos;
    if (editor && editor->PointToCharLine(mContextMenuPos,pos)) {
        editor->gotoDefinition(pos);
    }
}


void MainWindow::on_actionFind_references_triggered()
{
    Editor * editor = mEditorList->getEditor();
    BufferCoord pos;
    if (editor && editor->PointToCharLine(mContextMenuPos,pos)) {
        CppRefacter refactor;
        refactor.findOccurence(editor,pos);
        ui->tabMessages->setCurrentWidget(ui->tabSearch);
        openCloseBottomPanel(true);
    }
}


void MainWindow::on_actionOpen_Containing_Folder_triggered()
{
    Editor* editor = mEditorList->getEditor();
    if (editor) {
        QFileInfo info(editor->filename());
        if (!info.path().isEmpty()) {
            QDesktopServices::openUrl(info.path());
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
    FilePropertiesDialog dialog(this);
    dialog.exec();
    dialog.setParent(nullptr);
}

