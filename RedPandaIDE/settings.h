#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <vector>
#include <memory>
#include <QColor>
#include "qsynedit/SynEdit.h"

/**
 * use the following command to get gcc's default bin/library folders:
 * gcc -print-search-dirs
 */

#define SETTING_DIRS "Dirs"
#define SETTING_EDITOR "Editor"
#define SETTING_ENVIRONMENT "Environment"
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
        void beginGroup();
        void endGroup();
        void saveValue(const QString &key, const QVariant &value);
        QVariant value(const QString &key, const QVariant& defaultValue);
        bool boolValue(const QString &key, bool defaultValue);
        int intValue(const QString &key, int defaultValue);
        QColor colorValue(const QString &key, const QColor& defaultValue);
        QString stringValue(const QString &key, const QString& defaultValue);
        void save();
        void load();
    protected:
        virtual void doSave() = 0;
        virtual void doLoad() = 0;
    protected:
        Settings* mSettings;
        QString mGroup;
    };

public:
    class Dirs: public _Base {
    public:
        enum class DataType {
            None,
            ColorSheme
        };
        explicit Dirs(Settings * settings);
        QString app() const;
        QString data(DataType dataType = DataType::None) const;
        QString config(DataType dataType = DataType::None) const;

        // _Base interface
    protected:
        void doSave() override;
        void doLoad() override;
    };

    class Editor: public _Base {
    public:
        explicit Editor(Settings * settings);
        QByteArray defaultEncoding();
        void setDefaultEncoding(const QByteArray& value);
        bool autoIndent();
        void setAutoIndent(bool value);
        bool addIndent() const;
        void setAddIndent(bool addIndent);

        bool tabToSpaces() const;
        void setTabToSpaces(bool tabToSpaces);

        int tabWidth() const;
        void setTabWidth(int tabWidth);

        bool showIndentLines() const;
        void setShowIndentLines(bool showIndentLines);

        QColor indentLineColor() const;
        void setIndentLineColor(const QColor &indentLineColor);

        bool enhanceHomeKey() const;
        void setEnhanceHomeKey(bool enhanceHomeKey);

        bool enhanceEndKey() const;
        void setEnhanceEndKey(bool enhanceEndKey);

        SynEditCaretType caretForInsert() const;
        void setCaretForInsert(const SynEditCaretType &caretForInsert);

        SynEditCaretType caretForOverwrite() const;
        void setCaretForOverwrite(const SynEditCaretType &caretForOverwrite);

        QColor caretColor() const;
        void setCaretColor(const QColor &caretColor);

        bool keepCaretX() const;
        void setKeepCaretX(bool keepCaretX);

        bool autoHideScrollbar() const;
        void setAutoHideScrollbar(bool autoHideScrollbar);

        bool scrollPastEof() const;
        void setScrollPastEof(bool scrollPastEof);

        bool scrollPastEol() const;
        void setScrollPastEol(bool scrollPastEol);

        bool scrollByOneLess() const;
        void setScrollByOneLess(bool scrollByOneLess);

        bool halfPageScroll() const;
        void setHalfPageScroll(bool halfPageScroll);

        QString fontName() const;
        void setFontName(const QString &fontName);

        int fontSize() const;
        void setFontSize(int fontSize);

        bool fontOnlyMonospaced() const;
        void setFontOnlyMonospaced(bool fontOnlyMonospaced);

        bool gutterVisible() const;
        void setGutterVisible(bool gutterVisible);

        bool gutterAutoSize() const;
        void setGutterAutoSize(bool gutterAutoSize);

        int gutterDigitsCount() const;
        void setGutterDigitsCount(int gutterDigitsCount);

        bool gutterShowLineNumbers() const;
        void setGutterShowLineNumbers(bool gutterShowLineNumbers);

        bool gutterAddLeadingZero() const;
        void setGutterAddLeadingZero(bool gutterAddLeadingZero);

        bool gutterLineNumbersStartZero() const;
        void setGutterLineNumbersStartZero(bool gutterLineNumbersStartZero);

        bool gutterUseCustomFont() const;
        void setGutterUseCustomFont(bool gutterUseCustomFont);

        QString gutterFontName() const;
        void setGutterFontName(const QString &gutterFontName);

        int gutterFontSize() const;
        void setGutterFontSize(int gutterFontSize);

        bool gutterFontOnlyMonospaced() const;
        void setGutterFontOnlyMonospaced(bool gutterFontOnlyMonospaced);

        int gutterLeftOffset() const;
        void setGutterLeftOffset(int gutterLeftOffset);

        int gutterRightOffset() const;
        void setGutterRightOffset(int gutterRightOffset);

        bool copySizeLimit() const;
        void setCopySizeLimit(bool copyLimit);

        int copyCharLimits() const;
        void setCopyCharLimits(int copyCharLimits);

        int copyLineLimits() const;
        void setCopyLineLimits(int copyLineLimits);

        bool copyRTFUseBackground() const;
        void setCopyRTFUseBackground(bool copyRTFUseBackground);

        bool copyRTFUseEditorColor() const;
        void setCopyRTFUseEditorColor(bool copyRTFUseEditorColor);

        QString copyRTFColorScheme() const;
        void setCopyRTFColorScheme(const QString &copyRTFColorScheme);

        bool copyHTMLUseBackground() const;
        void setCopyHTMLUseBackground(bool copyHTMLUseBackground);

        bool copyHTMLUseEditorColor() const;
        void setCopyHTMLUseEditorColor(bool copyHTMLUseEditorColor);

        QString copyHTMLColorScheme() const;
        void setCopyHTMLColorScheme(const QString &copyHTMLColorScheme);

        int copyWithFormatAs() const;
        void setCopyWithFormatAs(int copyWithFormatAs);

        QString colorScheme() const;
        void setColorScheme(const QString &colorScheme);

    private:
        QByteArray mDefaultEncoding;
        //General
        // indents
        bool mAutoIndent;
        bool mAddIndent;
        bool mTabToSpaces;
        int mTabWidth;
        bool mShowIndentLines;
        QColor mIndentLineColor;
        // caret
        bool mEnhanceHomeKey;
        bool mEnhanceEndKey;
        bool mKeepCaretX;
        SynEditCaretType mCaretForInsert;
        SynEditCaretType mCaretForOverwrite;
        QColor mCaretColor;
        //scroll
        bool mAutoHideScrollbar;
        bool mScrollPastEof;
        bool mScrollPastEol;
        bool mScrollByOneLess;
        bool mHalfPageScroll;

        //Font
        //font
        QString mFontName;
        int mFontSize;
        bool mFontOnlyMonospaced;

        //gutter
        bool mGutterVisible;
        bool mGutterAutoSize;
        int mGutterLeftOffset;
        int mGutterRightOffset;
        int mGutterDigitsCount;
        bool mGutterShowLineNumbers;
        bool mGutterAddLeadingZero;
        bool mGutterLineNumbersStartZero;
        bool mGutterUseCustomFont;
        QString mGutterFontName;
        int mGutterFontSize;
        bool mGutterFontOnlyMonospaced;

        //copy
        bool mCopySizeLimit;
        int mCopyCharLimits;
        int mCopyLineLimits;
        int mCopyWithFormatAs;
        bool mCopyRTFUseBackground;
        bool mCopyRTFUseEditorColor;
        QString mCopyRTFColorScheme;
        bool mCopyHTMLUseBackground;
        bool mCopyHTMLUseEditorColor;
        QString mCopyHTMLColorScheme;

        //Color
        QString mColorScheme;

        // _Base interface
    protected:
        void doSave() override;
        void doLoad() override;
    };

    class Environment: public _Base {
    public:
        explicit Environment(Settings * settings);
        QString theme() const;
        void setTheme(const QString &theme);

        QString interfaceFont() const;
        void setInterfaceFont(const QString &interfaceFont);

        int interfaceFontSize() const;
        void setInterfaceFontSize(int interfaceFontSize);

        QString language() const;
        void setLanguage(const QString &language);

    private:

        //Appearence
        QString mTheme;
        QString mInterfaceFont;
        int mInterfaceFontSize;
        QString mLanguage;
        // _Base interface
    protected:
        void doSave() override;
        void doLoad() override;
    };

    class CompilerSet {
    public:
        explicit CompilerSet(const QString& compilerFolder = QString());
        explicit CompilerSet(const CompilerSet& set);

        CompilerSet& operator= (const CompilerSet& ) = delete;
        CompilerSet& operator= (const CompilerSet&& ) = delete;


        void addOption(const QString& name, const QString section, bool isC,
                bool isCpp, bool isLinker,
                int value, const QString& setting,
                const QStringList& choices = QStringList());
        PCompilerOption findOption(const QString& setting);
        char getOptionValue(const QString& setting);
        void setOption(const QString& setting, char valueChar);
        void setOption(PCompilerOption& option, char valueChar);

        bool dirsValid(QString& msg);
        //properties
        const QString& CCompiler() const;
        void setCCompiler(const QString& name);
        const QString& cppCompiler() const;
        void setCppCompiler(const QString& name);
        const QString& make() const;
        void setMake(const QString& name);
        const QString& debugger() const;
        void setDebugger(const QString& name);
        const QString& profiler() const;
        void setProfiler(const QString& name);
        const QString& resourceCompiler() const;
        void setResourceCompiler(const QString& name);

        QStringList& binDirs();
        QStringList& CIncludeDirs();
        QStringList& CppIncludeDirs();
        QStringList& libDirs();

        const QString& dumpMachine();
        void setDumpMachine(const QString& value);
        const QString& version();
        void setVersion(const QString& value);
        const QString& type();
        void setType(const QString& value);
        const QString& name();
        void setName(const QString& value);
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
        bool autoAddCharsetParams();
        void setAutoAddCharsetParams(bool value);

        CompilerOptionList& options();

        //Converts options to and from memory format
        QByteArray iniOptions() const;
        void setIniOptions(const QByteArray& value);

        //load hard defines
        void setDefines();

    private:
        int charToValue(char valueChar);

        // Initialization
        void setProperties(const QString& binDir);
        void setExecutables();
        void setDirectories(const QString& folder);
        void setUserInput();
        void setOptions();

        QString findProgramInBinDirs(const QString name);

        QByteArray getCompilerOutput(const QString& binDir, const QString& binFile,
                                     const QStringList& arguments);
    private:
        // Executables, most are hardcoded
        QString mCCompiler;
        QString mCppCompiler;
        QString mMake;
        QString mDebugger;
        QString mProfiler;
        QString mResourceCompiler;

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
        QStringList mDefines; // list of predefined constants
        QString mTarget; // 'X86_64' / 'i686'

        // User settings
        bool mUseCustomCompileParams;
        bool mUseCustomLinkParams;
        QString mCustomCompileParams;
        QString mCustomLinkParams;
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
        void saveDefaultIndex();
        void deleteSet(int index);
        //properties
        CompilerSetList& list();
        int size() const;
        int defaultIndex() const;
        void setDefaultIndex(int value);
        PCompilerSet defaultSet();
    private:
        void savePath(const QString& name, const QString& path);
        void savePathList(const QString& name, const QStringList& pathList);
        void saveSet(int index);
        QString loadPath(const QString& name);
        void loadPathList(const QString& name, QStringList& list);
        PCompilerSet loadSet(int index);
        CompilerSetList mList;
        int mDefaultIndex;
        Settings* mSettings;
    };

public:
    explicit Settings(const QString& filename);
    explicit Settings(Settings&& settings) = delete;
    explicit Settings(const Settings& settings) = delete;
    ~Settings();

    Settings& operator= (const Settings& settings) = delete;
    Settings& operator= (const Settings&& settings) = delete;
    void beginGroup(const QString& group);
    void endGroup();
    void saveValue(const QString& group, const QString &key, const QVariant &value);
    void saveValue(const QString &key, const QVariant &value);
    QVariant value(const QString& group, const QString &key, const QVariant& defaultValue);
    QVariant value(const QString &key, const QVariant& defaultValue);

    Dirs& dirs();
    Editor& editor();
    CompilerSets& compilerSets();
    Environment& environment();
    QString filename() const;

private:
    QString mFilename;
    QSettings mSettings;
    Dirs mDirs;
    Editor mEditor;
    Environment mEnvironment;
    CompilerSets mCompilerSets;
};


extern Settings* pSettings;

#endif // SETTINGS_H
