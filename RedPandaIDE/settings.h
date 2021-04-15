#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <vector>
#include <memory>

/**
 * use the following command to get gcc's default bin/library folders:
 * gcc -print-search-dirs
 */

#define SETTING_DIRS "Dirs"
#define SETTING_EDITOR "Editor"
#define SETTING_COMPILTER_SETS "CompilerSets"
#define SETTING_COMPILTER_SETS_DEFAULT_INDEX "defaultIndex"
#define SETTING_COMPILTER_SETS_COUNT "count"
#define SETTING_COMPILTER_SET "CompilerSet_%1"
#define SETTING_EDITOR_DEFAULT_ENCODING "default_encoding"
#define SETTING_EDITOR_AUTO_INDENT "default_auto_indent"


extern const char ValueToChar[28];

class Settings;

typedef struct {
    QString name; // language table index of "Generate debugging info"
    QString section; // language table index of "C options"
    bool isC;
    bool isCpp; // True (C++ option?) - can be both C and C++ option...
    bool isLinker; // Is it a linker param
    int value; // True
    QString setting; // "-g3"
    QStringList choices; // replaces "Yes/No" standard choices (max 30 different choices)
} CompilerOption;

using PCompilerOption = std::shared_ptr<CompilerOption>;

using CompilerOptionList=std::vector<std::shared_ptr<CompilerOption>>;

class Settings
{
private:

    class _Base {
    public:
        explicit _Base(Settings* settings, const QString& groupName);
        void setDefault(const QString &key, const QVariant &value);
        void setValue(const QString &key, const QVariant &value);
        QVariant value(const QString &key);
    protected:
        Settings* mSettings;
        QString mGroup;
    };

public:
    class Dirs: public _Base {
    public:
        explicit Dirs(Settings * settings);
        const QString app() const;
    };

    class Editor: public _Base {
    public:
        explicit Editor(Settings * settings);
        QByteArray defaultEncoding();
        void setDefaultEncoding(const QByteArray& encoding);
        bool autoIndent();
        void setAutoIndent(bool indent);
    };

    class CompilerSet {
    public:
        explicit CompilerSet(const QString& compilerFolder = QString());
        explicit CompilerSet(const CompilerSet& set);

        CompilerSet& operator= (const CompilerSet& ) = delete;
        CompilerSet& operator= (const CompilerSet&& ) = delete;

        //properties
        void addOption(const QString& name, const QString section, bool isC,
                bool isCpp, bool isLinker,
                int value, const QString& setting,
                const QStringList& choices = QStringList());
        PCompilerOption findOption(const QString& setting);
        char getOptionValue(const QString& setting);
        void setOption(const QString& setting, char valueChar);
        void setOption(PCompilerOption& option, char valueChar);

        const QString& CCompilerName() const;
        void setCCompilerName(const QString& name);
        const QString& cppCompilerName() const;
        void setCppCompilerName(const QString& name);
        const QString& makeName() const;
        void setMakeName(const QString& name);
        const QString& debuggerName() const;
        void setDebuggerName(const QString& name);
        const QString& profilerName() const;
        void setProfilerName(const QString& name);
        const QString& resourceCompilerName() const;
        void setResourceCompilerName(const QString& name);

        QStringList& binDirs();
        QStringList& CIncludeDirs();
        QStringList& CppIncludeDirs();
        QStringList& LibDirs();

        const QString& dumpMachine();
        void setDumpMachine(const QString& value);
        const QString& version();
        void setVersion(const QString& value);
        const QString& type();
        void setType(const QString& value);
        const QString& name();
        void setName(const QString& value);
        const QString& folder();
        void setFolder(const QString& value);
        QStringList& defines();
        const QString& target();
        void setTarget(const QString& value);

        bool useCustomCompileParams();
        void setUseCustomCompileParams(bool value);
        bool useCustomLinkParams();
        void setUseCustomLinkParams(bool value);
        const QString& customCompileParams();
        void setCustomCompileParams(const QString& value);
        const QString& customLinkParams();
        void setCustomLinkParams(const QString& value);
        bool staticLink();
        void setStaticLink(bool value);
        bool autoAddCharsetParams();
        void setAutoAddCharsetParams(bool value);

        CompilerOptionList& options();

        //Converts options to and from memory format
        QByteArray iniOptions() const;
        void setIniOptions(const QByteArray& value);

    private:
        int charToValue(char valueChar);

        // Initialization
        void setProperties(const QString& binDir);
        void setExecutables();
        void setDirectories();
        void setUserInput();
        void setOptions();

        QByteArray getCompilerOutput(const QString& binDir, const QString& binFile,
                                     const QStringList& arguments);
    private:
        // Executables, most are hardcoded
        QString mCCompilerName;
        QString mCppCompilerName;
        QString mMakeName;
        QString mDebuggerName;
        QString mProfilerName;
        QString mResourceCompilerName;

        // Directories, mostly hardcoded too
        QStringList mBinDirs;
        QStringList mCIncludeDirs;
        QStringList mCppIncludeDirs;
        QStringList mLibDirs;

        // Misc. properties
        QString mDumpMachine; // "x86_64-w64-mingw32", "mingw32" etc
        QString mVersion; // "4.7.1"
        QString mType; // "TDM-GCC", "MinGW"
        QString mName; // "TDM-GCC 4.7.1 Release"
        QString mFolder; // MinGW64, MinGW32
        QStringList mDefines; // list of predefined constants
        QString mTarget; // 'X86_64' / 'i686'

        // User settings
        bool mUseCustomCompileParams;
        bool mUseCustomLinkParams;
        QString mCustomCompileParams;
        QString mCustomLinkParams;
        bool mStaticLink;
        bool mAutoAddCharsetParams;

        // Options
        CompilerOptionList mOptions;
    };

    typedef std::shared_ptr<CompilerSet> PCompilerSet;
    typedef std::vector<PCompilerSet> CompilerSetList;

    class CompilerSets {
    public:
        explicit CompilerSets(Settings* settings);

        PCompilerSet addSet(const CompilerSet& set);
        PCompilerSet addSet(const QString& folder=QString());

        void addSets(const QString& folder);
        void clearSets();
        void findSets();
        void saveSets();
        void loadSets();
        //properties
        CompilerSetList& list();
        int size() const;
        int defaultIndex() const;
        void setDefaultIndex(int value);
    private:
        void saveSet(int index);
        PCompilerSet loadSet(int index);
        CompilerSetList mList;
        int mDefaultIndex;
        Settings* mSettings;
    };

public:
    explicit Settings(const QString& filename);
    explicit Settings(Settings&& settings) = delete;
    explicit Settings(const Settings& settings) = delete;

    Settings& operator= (const Settings& settings) = delete;
    Settings& operator= (const Settings&& settings) = delete;
    void setDefault(const QString& group, const QString &key, const QVariant &value);
    void setValue(const QString& group, const QString &key, const QVariant &value);
    QVariant value(const QString& group, const QString &key);

    Dirs& dirs();
    Editor& editor();
    CompilerSets& compilerSets();
private:
    QSettings mSettings;
    Dirs mDirs;
    Editor mEditor;
    CompilerSets mCompilerSets;
};


extern Settings* pSettings;

#endif // SETTINGS_H
