#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <QSettings>
#include <memory>

enum class ProjectType {
    GUI,
    Console,
    StaticLib,
    DynamicLib
};

class Project;
class Editor;
class CppParser;

struct FolderNode;
using PFolderNode = std::shared_ptr<FolderNode>;
struct FolderNode {
    QString text;
    FolderNode * parent;
    int unitIndex;
    QList<PFolderNode>  children;
};

class ProjectUnit {

public:
    explicit ProjectUnit(Project* parent);
    Project* parent() const;
    void setParent(Project* newParent);
    Editor *editor() const;
    void setEditor(Editor *newEditor);
    const QString &fileName() const;
    void setFileName(const QString &newFileName);
    bool isNew() const;
    void setNew(bool newNew);
    const QString &folder() const;
    void setFolder(const QString &newFolder);
    bool compile() const;
    void setCompile(bool newCompile);
    bool compileCpp() const;
    void setCompileCpp(bool newCompileCpp);
    bool overrideBuildCmd() const;
    void setOverrideBuildCmd(bool newOverrideBuildCmd);
    const QString &buildCmd() const;
    void setBuildCmd(const QString &newBuildCmd);
    bool link() const;
    void setLink(bool newLink);
    int priority() const;
    void setPriority(int newPriority);
    bool detectEncoding() const;
    void setDetectEncoding(bool newDetectEncoding);
    const QByteArray &encoding() const;
    void setEncoding(const QByteArray &newEncoding);
    bool modified() const;
    void setModified(bool value);
    bool save();

private:
    Project* mParent;
    Editor* mEditor;
    QString mFileName;
    bool mNew;
    QString mFolder;
    bool mCompile;
    bool mCompileCpp;
    bool mOverrideBuildCmd;
    QString mBuildCmd;
    bool mLink;
    int mPriority;
    bool mDetectEncoding;
    QByteArray mEncoding;
    PFolderNode mNode;
};

using PProjectUnit = std::shared_ptr<ProjectUnit>;

struct ProjectVersionInfo{
    int major;
    int minor;
    int release;
    int build;
    int languageID;
    int charsetID;
    QString companyName;
    QString fileVersion;
    QString fileDescription;
    QString internalName;
    QString legalCopyright;
    QString legalTrademarks;
    QString originalFilename;
    QString productName;
    QString productVersion;
    bool autoIncBuildNr;
    bool syncProduct;
};

struct ProjectOptions{
    ProjectType type;
    int version;
    bool useUTF8;
    QStringList objfiles;
    QString compilerCmd;
    QString cppCompilerCmd;
    QString linkerCmd;
    QStringList includes;
    QStringList libs;
    QString privateResource;
    QStringList resourceIncludes;
    QStringList makeIncludes;
    bool useGPP;
    QString icon;
    QString exeOutput;
    QString objectOutput;
    QString logOutput;
    bool logOutputEnabled;
    bool useCustomMakefile;
    QString customMakefile;
    bool usePrecompiledHeader;
    bool precompiledHeader;
    bool overrideOutput;
    QString overridenOutput;
    QString hostApplication;
    bool includeVersionInfo;
    bool supportXPThemes;
    int compilerSet;
    QString compilerOptions;
    ProjectVersionInfo VersionInfo;
    QString cmdLineArgs;
    bool mStaticLink;
    bool mAddCharset;
};

class Project : public QObject
{
    Q_OBJECT
public:
    explicit Project(QObject *parent = nullptr);
    QString directory();
    QString executableName();
    QString makeFileName();
    bool modified();
    void setFileName(const QString& value);
    void setModified(bool value);

    int  newUnit(bool newProject,
                 PFolderNode parentNode,
                 const QString customFileName);
    PProjectUnit addUnit(const QString& inFileName,
                PFolderNode parentNode,
                bool rebuild);
    function GetFolderPath(Node: TTreeNode): AnsiString;
    procedure UpdateFolders;
    procedure AddFolder(const s: AnsiString);
    function OpenUnit(index: integer): TEditor;
    procedure CloseUnit(index: integer);
    procedure DoAutoOpen;
    procedure SaveUnitAs(i: integer; sFileName: AnsiString); // save single [UnitX]
    procedure SaveAll; // save [Project] and  all [UnitX]
    procedure LoadLayout; // load all [UnitX]
    procedure LoadUnitLayout(e: TEditor; Index: integer); // load single [UnitX] cursor positions
    procedure SaveLayout; // save all [UnitX]
    procedure SaveUnitLayout(e: TEditor; Index: integer); // save single [UnitX] cursor positions
    function MakeProjectNode: TTreeNode;
    function MakeNewFileNode(const s: AnsiString; IsFolder: boolean; NewParent: TTreeNode): TTreeNode;
    procedure BuildPrivateResource(ForceSave: boolean = False);
    procedure LoadOptions;
    procedure SaveOptions;
    function SaveUnits: Boolean;
//    procedure Open;
    function FileAlreadyExists(const s: AnsiString): boolean;
    function RemoveFolder(Node: TTreeNode): boolean;
    function RemoveEditor(index: integer; DoClose: boolean): boolean;
    function GetUnitFromString(const s: AnsiString): integer;
    procedure RebuildNodes;
    function ListUnitStr(Separator: char): AnsiString;
    procedure ExportToHTML;
    function ShowOptions: Integer;
    function AssignTemplate(const aFileName: AnsiString; aTemplate: TTemplate): boolean;
    function FolderNodeFromName(const name: AnsiString): TTreeNode;
    procedure CreateFolderNodes;
    procedure UpdateNodeIndexes;
    procedure SetNodeValue(value: TTreeNode);
    procedure CheckProjectFileForUpdate;
    procedure IncrementBuildNumber;
    function GetCompilerOption(const OptionString: AnsiString): Char;
    procedure SetCompilerOption(const OptionString: AnsiString; Value: Char);
    procedure SaveToLog;
signals:
    void changed();
private:
    void open();
    void sortUnitsByPriority();
private:
    QList<PProjectUnit> mUnits;
    ProjectOptions mOptions;
    std::shared_ptr<QSettings> mIniFile;
    QString mFilename;
    QString mName;
    bool mModified;
    QStringList mFolders;
    std::shared_ptr<CppParser> mParser;
    PFolderNode mNode;
};

#endif // PROJECT_H
