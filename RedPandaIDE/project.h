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

class ProjectUnit {

public:
    const std::weak_ptr<Project> &parent() const;
    void setParent(const std::weak_ptr<Project> &newParent);
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
    std::weak_ptr<Project> mParent;
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
    void open();
    procedure SetFileName(const value: AnsiString);
    procedure SetModified(value: boolean);
    procedure SortUnitsByPriority;
    procedure Open;
signals:
    void changed();
private:
    QList<PProjectUnit> mUnits;
    ProjectOptions mOptions;
    QSettings mIniFile;
    QString mFilename;
    QString mName;
    bool mModified;
    QStringList mFolders;
    std::shared_ptr<CppParser> mParser;
};

#endif // PROJECT_H
