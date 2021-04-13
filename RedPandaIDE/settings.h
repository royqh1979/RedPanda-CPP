#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <vector>
#include <memory>

#define SETTING_DIRS "dirs"
#define SETTING_EDITOR "editor"
#define SETTING_EDITOR_DEFAULT_ENCODING "default_encoding"
#define SETTING_EDITOR_AUTO_INDENT "default_auto_indent"

class Settings;

typedef struct {
    int name; // language table index of "Generate debugging info"
    int section; // language table index of "C options"
    bool isC;
    bool isCpp; // True (C++ option?) - can be both C and C++ option...
    bool isLinker; // Is it a linker param
    int value; // True
    QString Setting; // "-g3"
    QStringList Choices; // replaces "Yes/No" standard choices (max 30 different choices)
} CompilerOption, *PCompilerOption;

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

    class CompilerSet: public _Base {
    public:
        explicit CompilerSet(Settings *settings, int index, const QString& compilerFolder = QString());

    private:
        int mIndex;
        // Executables, most are hardcoded
        QString mCCompilerName;
        QString mCPPCompilerName;
        QString mMakeName;
        QString mDebuggerName;
        QString mProfilerName;
        QString mResourceCompilerName;

        // Directories, mostly hardcoded too
        QStringList mBinDirs;
        QStringList mCIncludeDirs;
        QStringList mCPPIncludeDirs;
        QStringList mLibDirs;

        // Misc. properties
        QString mDumpMachine; // "x86_64-w64-mingw32", "mingw32" etc
        QString mVersion; // "4.7.1"
        QString mType; // "TDM-GCC", "MinGW"
        QString mName; // "TDM-GCC 4.7.1 Release"
        QString mFolder; // MinGW64, MinGW32
        QStringList mDefIncludes; // default include dir
        QStringList fDefines; // list of predefined constants
        QString mTarget; // 'X86_64' / 'i686'

        // User settings
        bool mAddCustomCompileParams;
        bool mAddCustomLinkParams;
        QString mCustomCompileParams;
        QString mCustomLinkParams;
        bool mStaticLink;
        bool mAddCharsetParams;

        // Options
        std::vector<std::shared_ptr<CompilerOption>> mOptions;
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
private:
    QSettings mSettings;
    Dirs mDirs;
    Editor mEditor;
};


extern Settings* pSettings;

#endif // SETTINGS_H
