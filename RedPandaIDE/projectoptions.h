#ifndef PROJECTOPTIONS_H
#define PROJECTOPTIONS_H

#include <QWidget>

enum class ProjectType {
    GUI=0,
    Console=1,
    StaticLib=2,
    DynamicLib=3
};

struct ProjectVersionInfo{
    explicit ProjectVersionInfo();
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
    explicit ProjectOptions();
    ProjectType type;
    int version;
    QStringList objFiles;
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
    QString precompiledHeader;
    bool overrideOutput;
    QString overridenOutput;
    QString hostApplication;
    bool includeVersionInfo;
    bool supportXPThemes;
    int compilerSet;
    int compilerSetType;
    QByteArray compilerOptions;
    ProjectVersionInfo versionInfo;
    QString cmdLineArgs;
    bool staticLink;
    bool addCharset;
    QString encoding;
};
#endif // PROJECTOPTIONS_H
