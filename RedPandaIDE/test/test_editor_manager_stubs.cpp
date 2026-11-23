#include "src/mainwindow.h"
#include "src/settings.h"
#include "src/project.h"
#include "src/visithistorymanager.h"
#include "src/debugger/debugger.h"
#include "src/reformatter/astyleformatter.h"
#include "src/utils/parser.h"

MainWindow *pMainWindow = nullptr;
Settings *pSettings = nullptr;

const QMetaObject Debugger::staticMetaObject = QMetaObject{};

void Debugger::addBreakpoint(int, const QString &, bool) {}
void Debugger::removeBreakpoint(int, const QString &, bool) {}
void Debugger::deleteBreakpoints(const QString &, bool) {}
void Debugger::setIsForProject(bool) {}
void Debugger::evalValueReady(const QString &) {}
void Debugger::evalExpression(const QString &) {}
bool Debugger::executing() { return false; }
bool Debugger::inferiorRunning() { return false; }
bool Debugger::commandRunning() const { return false; }

std::shared_ptr<BreakpointModel> Debugger::breakpointModel()
{
    return {};
}

void reconfigureCppParser(std::shared_ptr<CppParser>, int) {}

SettingsPersistor &stubPersistor()
{
    static SettingsPersistor persistor("test-editor-manager-stubs.ini");
    return persistor;
}

EditorSettings &Settings::editor()
{
    static EditorSettings settings(&stubPersistor());
    return settings;
}

CodeCompletionSettings &Settings::codeCompletion()
{
    static CodeCompletionSettings settings(&stubPersistor());
    return settings;
}

CodeFormatterSettings &Settings::codeFormatter()
{
    static CodeFormatterSettings settings(&stubPersistor());
    return settings;
}

EnvironmentSettings &Settings::environment()
{
    static DirSettings dirs(&stubPersistor());
    static EnvironmentSettings settings(&stubPersistor(), &dirs);
    return settings;
}

Debugger *MainWindow::debugger() const { return nullptr; }
BookmarkModel *MainWindow::bookmarkModel() const { return nullptr; }
TodoModel *MainWindow::todoModel() { return nullptr; }
std::shared_ptr<Project> MainWindow::project() { return {}; }
QFileSystemWatcher *MainWindow::fileSystemWatcher() { return nullptr; }
OJProblemSetModel *MainWindow::getOJProblemSetModel() const { return nullptr; }
ColorManager *MainWindow::colorManager() const { return nullptr; }
IconsManager *MainWindow::iconsManager() const { return nullptr; }
CodeSnippetsManager *MainWindow::codeSnippetManager() const { return nullptr; }
FunctionTooltipWidget *MainWindow::functionTip() const { return nullptr; }
CodeCompletionPopup *MainWindow::completionPopup() const { return nullptr; }
HeaderCompletionPopup *MainWindow::headerCompletionPopup() const { return nullptr; }
SymbolUsageManager *MainWindow::symbolUsageManager() const { return nullptr; }

const PTodoParser &MainWindow::todoParser() const
{
    static const PTodoParser parser;
    return parser;
}

const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem>>> &MainWindow::statementColors() const
{
    static const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem>>> colors;
    return colors;
}

const std::shared_ptr<VisitHistoryManager> &MainWindow::visitHistoryManager() const
{
    static const std::shared_ptr<VisitHistoryManager> manager;
    return manager;
}

CaretList &MainWindow::caretList()
{
    static CaretList list;
    return list;
}

bool MainWindow::isClosingAll() const { return false; }
bool MainWindow::isQuitting() const { return false; }
bool MainWindow::openingFiles() const { return false; }
bool MainWindow::openingProject() const { return false; }
bool MainWindow::closingProject() const { return false; }
void MainWindow::updateStatusbarForLineCol(const Editor *) {}
void MainWindow::updateForStatusbarModeInfo(const Editor *) {}
void MainWindow::updateEditorActions(const Editor *) {}
void MainWindow::updateCaretActions() {}
void MainWindow::rebuildOpenedFileHisotryMenu() {}
void MainWindow::updateClassBrowserForEditor(Editor *) {}
void MainWindow::onFileSaved(const QString &, bool) {}
void MainWindow::updateEditorSettings() {}
void MainWindow::setProjectCurrentFile(const QString &) {}
void MainWindow::openFileWithContext(const QString &, FileType, QString,
                                     const QSynedit::CharPos &) {}
void MainWindow::onEditorContextMenu(const QPoint &) {}
void MainWindow::removeInfosForEditor() {}
void MainWindow::refreshInfosForEditor(Editor *) {}
void MainWindow::updateForEncodingInfo(const Editor *) {}
void MainWindow::checkSyntaxInBack(Editor *) {}
void MainWindow::logToolsOutput(const QString &) {}

QMap<QString, QString> MainWindow::macroVariables()
{
    return {};
}

void TodoParser::parseFile(const QString &, bool) {}

void TodoModel::setIsForProject(bool) {}

void BreakpointModel::onFileInsertLines(const QString &, int, int, bool) {}
void BreakpointModel::onFileDeleteLines(const QString &, int, int, bool) {}
void BreakpointModel::onFileLineMoved(const QString &, int, int, bool) {}
void BreakpointModel::renameBreakpointFilenames(const QString &, const QString &, bool) {}

void OJProblemSetModel::updateProblemAnswerFilename(const QString &, const QString &) {}

bool VisitHistoryManager::addFile(const VisitRecord &)
{
    return false;
}

bool Project::inProject(const QString &) const
{
    return false;
}

bool Project::inProject(const Editor *) const
{
    return false;
}

PProjectUnit Project::findUnit(const Editor *) const
{
    return {};
}

void Project::closeUnit(PProjectUnit &) {}

QString Project::directory() const
{
    return {};
}

PCppParser Project::cppParser()
{
    return {};
}

void ProjectUnit::setRealEncoding(const QByteArray &) {}


ProcessOutput runAndGetOutput(const QString &, const QString &, const QStringList &,
                              const QByteArray &, bool, bool,
                              const QProcessEnvironment &)
{
    return {};
}

QStringList getExecutableSearchPaths()
{
    return {};
}
