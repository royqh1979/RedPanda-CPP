#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <vector>
#include <memory>
#include <QColor>
#include <QString>
#include "qsynedit/SynEdit.h"

/**
 * use the following command to get gcc's default bin/library folders:
 * gcc -print-search-dirs
 */

#define SETTING_DIRS "Dirs"
#define SETTING_EDITOR "Editor"
#define SETTING_ENVIRONMENT "Environment"
#define SETTING_EXECUTOR "Executor"
#define SETTING_DEBUGGER "Debugger"
#define SETTING_HISTORY "History"
#define SETTING_UI "UI"
#define SETTING_CODE_COMPLETION "CodeCompletion"
#define SETTING_CODE_FORMATTER "CodeFormatter"
#define SETTING_COMPILTER_SETS "CompilerSets"
#define SETTING_COMPILTER_SETS_DEFAULT_INDEX "defaultIndex"
#define SETTING_COMPILTER_SETS_COUNT "count"
#define SETTING_COMPILTER_SET "CompilerSet_%1"
#define SETTING_EDITOR_DEFAULT_ENCODING "default_encoding"
#define SETTING_EDITOR_AUTO_INDENT "default_auto_indent"


extern const char ValueToChar[28];

class Settings;

enum CompilerSetType {
    CST_RELEASE,
    CST_DEBUG,
    CST_PROFILING
};

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

using CompilerOptionList=QVector<std::shared_ptr<CompilerOption>>;

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
        QStringList stringListValue(const QString &key, const QStringList& defaultValue=QStringList());
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
        QString templateDir() const;
        QString projectDir() const;
        QString data(DataType dataType = DataType::None) const;
        QString config(DataType dataType = DataType::None) const;
        QString executable() const;

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

        bool completeSymbols() const;
        void setCompleteSymbols(bool completeSymbols);

        bool completeParenthese() const;
        void setCompleteParenthese(bool completeParenthese);

        bool completeBracket() const;
        void setCompleteBracket(bool completeBracket);

        bool completeBrace() const;
        void setCompleteBrace(bool completeBrace);

        bool completeComment() const;
        void setCompleteComment(bool completeComment);

        bool completeSingleQuote() const;
        void setCompleteSingleQuote(bool completeSingleQuote);

        bool completeDoubleQuote() const;
        void setCompleteDoubleQuote(bool completeDoubleQuote);

        bool completeGlobalInclude() const;
        void setCompleteGlobalInclude(bool completeGlobalInclude);

        bool overwriteSymbols() const;
        void setOverwriteSymbols(bool overwriteSymbols);

        bool removeSymbolPairs() const;
        void setRemoveSymbolPairs(bool value);

        bool syntaxCheck() const;
        void setSyntaxCheck(bool syntaxCheck);

        bool syntaxCheckWhenSave() const;
        void setSyntaxCheckWhenSave(bool syntaxCheckWhenSave);

        bool syntaxCheckWhenLineChanged() const;
        void setSyntaxCheckWhenLineChanged(bool syntaxCheckWhenLineChanged);

        bool readOnlySytemHeader() const;
        void setReadOnlySytemHeader(bool newReadOnlySytemHeader);

        bool autoLoadLastFiles() const;
        void setAutoLoadLastFiles(bool newAutoLoadLastFiles);

        bool defaultFileCpp() const;
        void setDefaultFileCpp(bool newDefaultFileCpp);

        bool enableAutoSave() const;
        void setEnableAutoSave(bool newEnableAutoSave);

        int autoSaveInterval() const;
        void setAutoSaveInterval(int newInterval);

        AutoSaveTarget autoSaveTarget() const;
        void setAutoSaveTarget(AutoSaveTarget newAutoSaveTarget);

        AutoSaveStrategy autoSaveStrategy() const;
        void setAutoSaveStrategy(AutoSaveStrategy newAutoSaveStrategy);

        bool enableAutolink() const;
        void setEnableAutolink(bool newEnableAutolink);

        bool showRightEdgeLine() const;
        void setShowRightEdgeLine(bool newShowRightMarginLine);

        int rightEdgeWidth() const;
        void setRightEdgeWidth(int newRightMarginWidth);

        const QColor &rightEdgeLineColor() const;
        void setRightEdgeLineColor(const QColor &newRightMarginLineColor);

        bool caretUseTextColor() const;
        void setCaretUseTextColor(bool newUseIdentifierColor);

        bool rainbowParenthesis() const;
        void setRainbowParenthesis(bool newRainbowParenthesis);

        bool enableTooltips() const;
        void setEnableTooltips(bool newEnableTooltips);
        bool enableDebugTooltips() const;
        void setEnableDebugTooltips(bool newEnableDebugTooltips);
        bool enableIdentifierToolTips() const;
        void setEnableIdentifierToolTips(bool newEnableIdentifierToolTips);
        bool enableHeaderToolTips() const;
        void setEnableHeaderToolTips(bool newEnableHeaderToolTips);
        bool enableIssueToolTips() const;
        void setEnableIssueToolTips(bool newEnableIssueToolTips);

        bool showFunctionTips() const;
        void setShowFunctionTips(bool newShowFunctionTips);

        bool fillIndents() const;
        void setFillIndents(bool newFillIndents);

        int mouseWheelScrollSpeed() const;
        void setMouseWheelScrollSpeed(int newMouseWheelScrollSpeed);

        bool useUTF8ByDefault() const;
        void setUseUTF8ByDefault(bool newUseUTF8ByDefault);

    private:
        //General
        // indents
        bool mAutoIndent;
        bool mTabToSpaces;
        int mTabWidth;
        bool mShowIndentLines;
        QColor mIndentLineColor;
        bool mfillIndents;
        // caret
        bool mEnhanceHomeKey;
        bool mEnhanceEndKey;
        bool mKeepCaretX;
        SynEditCaretType mCaretForInsert;
        SynEditCaretType mCaretForOverwrite;
        bool mCaretUseTextColor;
        QColor mCaretColor;

        //scroll
        bool mAutoHideScrollbar;
        bool mScrollPastEof;
        bool mScrollPastEol;
        bool mScrollByOneLess;
        bool mHalfPageScroll;
        int mMouseWheelScrollSpeed;

        //right margin
        bool mShowRightEdgeLine;
        int mRightEdgeWidth;
        QColor mRightEdgeLineColor;

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
        bool mRainbowParenthesis;

        //Symbol Completion
        bool mCompleteSymbols;
        bool mCompleteParenthese;
        bool mCompleteBracket;
        bool mCompleteBrace;
        bool mCompleteComment;
        bool mCompleteSingleQuote;
        bool mCompleteDoubleQuote;
        bool mCompleteGlobalInclude;
        bool mOverwriteSymbols;
        bool mRemoveSymbolPairs;

        //Auto Syntax Check
        bool mSyntaxCheck;
        bool mSyntaxCheckWhenSave;
        bool mSyntaxCheckWhenLineChanged;

        //auto save
        bool mEnableAutoSave;
        int mAutoSaveInterval;
        enum AutoSaveTarget mAutoSaveTarget;
        enum AutoSaveStrategy mAutoSaveStrategy;

        //auto link
        bool mEnableAutolink;

        //Misc
        QByteArray mDefaultEncoding;
        bool mReadOnlySytemHeader;
        bool mAutoLoadLastFiles;
        bool mDefaultFileCpp;
        bool mUseUTF8ByDefault;

        //hints tooltip
        bool mEnableTooltips;
        bool mEnableDebugTooltips;
        bool mEnableIdentifierToolTips;
        bool mEnableHeaderToolTips;
        bool mEnableIssueToolTips;
        bool mShowFunctionTips;

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

        const QString &currentFolder() const;
        void setCurrentFolder(const QString &newCurrentFolder);

    private:

        //Appearence
        QString mTheme;
        QString mInterfaceFont;
        int mInterfaceFontSize;
        QString mLanguage;
        QString mCurrentFolder;
        // _Base interface
    protected:
        void doSave() override;
        void doLoad() override;
    };

    class CodeCompletion: public _Base {
    public:
        explicit CodeCompletion(Settings *settings);
        int width() const;
        void setWidth(int newWidth);

        int height() const;
        void setHeight(int newHeight);

        bool enabled() const;
        void setEnabled(bool newEnabled);

        bool parseLocalHeaders() const;
        void setParseLocalHeaders(bool newParseLocalHeaders);

        bool parseGlobalHeaders() const;
        void setParseGlobalHeaders(bool newParseGlobalHeaders);

        bool showCompletionWhileInput() const;
        void setShowCompletionWhileInput(bool newShowCompletionWhileInput);

        bool recordUsage() const;
        void setRecordUsage(bool newRecordUsage);

        bool sortByScope() const;
        void setSortByScope(bool newSortByScope);

        bool showKeywords() const;
        void setShowKeywords(bool newShowKeywords);

        bool ignoreCase() const;
        void setIgnoreCase(bool newIgnoreCase);

        bool appendFunc() const;
        void setAppendFunc(bool newAppendFunc);

        bool showCodeIns() const;
        void setShowCodeIns(bool newShowCodeIns);

    private:
        int mWidth;
        int mHeight;
        bool mEnabled;
        bool mParseLocalHeaders;
        bool mParseGlobalHeaders;
        bool mShowCompletionWhileInput;
        bool mRecordUsage;
        bool mSortByScope;
        bool mShowKeywords;
        bool mIgnoreCase;
        bool mAppendFunc;
        bool mShowCodeIns;

        // _Base interface
    protected:
        void doSave() override;
        void doLoad() override;

    };

    class CodeFormatter: public _Base {
    public:
        explicit CodeFormatter(Settings* settings);
        QStringList getArguments();
        int braceStyle() const;
        void setBraceStyle(int newBraceStyle);

        int indentStyle() const;
        void setIndentStyle(int newIndentStyle);
        int tabWidth() const;
        void setTabWidth(int newTabWidth);
        bool attachNamespaces() const;
        void setAttachNamespaces(bool newAttachNamespaces);
        bool attachClasses() const;
        void setAttachClasses(bool newAttachClasses);
        bool attachInlines() const;
        void setAttachInlines(bool newAttachInlines);
        bool attachExternC() const;
        void setAttachExternC(bool newAttachExternC);
        bool attachClosingWhile() const;
        void setAttachClosingWhile(bool newAttachClosingWhile);
        bool indentClasses() const;
        void setIndentClasses(bool newIndentClasses);
        bool indentModifiers() const;
        void setIndentModifiers(bool newIndentModifiers);
        bool indentCases() const;
        void setIndentCases(bool newIndentCases);
        bool indentNamespaces() const;
        void setIndentNamespaces(bool newIndentNamespaces);
        int indentContinuation() const;
        void setIndentContinuation(int newIndentContinuation);
        bool indentLabels() const;
        void setIndentLabels(bool newIndentLabels);
        bool indentPreprocBlock() const;
        void setIndentPreprocBlock(bool newIndentPreprocBlock);
        bool indentPreprocCond() const;
        void setIndentPreprocCond(bool newIndentPreprocCond);
        bool indentPreprocDefine() const;
        void setIndentPreprocDefine(bool newIndentPreprocDefine);
        bool indentCol1Comments() const;
        void setIndentCol1Comments(bool newIndentCol1Comments);
        int minConditionalIndent() const;
        void setMinConditionalIndent(int newMinConditionalIndent);
        int maxContinuationIndent() const;
        void setMaxContinuationIndent(int newMaxContinuationIndent);
        bool breakBlocks() const;
        void setBreakBlocks(bool newBreakBlocks);
        bool breakBlocksAll() const;
        void setBreakBlocksAll(bool newBreakBlocksAll);
        bool padOper() const;
        void setPadOper(bool newPadOper);
        bool padComma() const;
        void setPadComma(bool newPadComma);
        bool padParen() const;
        void setPadParen(bool newPadParen);
        bool padParenOut() const;
        void setPadParenOut(bool newPadParenOut);
        bool padFirstParenOut() const;
        void setPadFirstParenOut(bool newPadFirstParenOut);
        bool padParenIn() const;
        void setPadParenIn(bool newPadParenIn);
        bool padHeader() const;
        void setPadHeader(bool newPadHeader);
        bool unpadParen() const;
        void setUnpadParen(bool newUnpadParen);
        bool deleteEmptyLines() const;
        void setDeleteEmptyLines(bool newDeleteEmptyLines);
        bool deleteMultipleEmptyLines() const;
        void setDeleteMultipleEmptyLines(bool newDeleteMultipleEmptyLines);
        bool fillEmptyLines() const;
        void setFillEmptyLines(bool newFillEmptyLines);
        int alignPointerStyle() const;
        void setAlignPointerStyle(int newAlignPointerStyle);
        int alignReferenceStyle() const;
        void setAlignReferenceStyle(int newAlignReferenceStyle);
        bool breakClosingBraces() const;
        void setBreakClosingBraces(bool newBreakClosingBraces);
        bool breakElseIf() const;
        void setBreakElseIf(bool newBreakElseIf);
        bool breakOneLineHeaders() const;
        void setBreakOneLineHeaders(bool newBreakOneLineHeaders);
        bool addBraces() const;
        void setAddBraces(bool newAddBraces);
        bool addOneLineBraces() const;
        void setAddOneLineBraces(bool newAddOneLineBraces);
        bool removeBraces() const;
        void setRemoveBraces(bool newRemoveBraces);
        bool breakReturnTypeDecl() const;
        void setBreakReturnTypeDecl(bool newBreakReturnTypeDecl);
        bool attachReturnType() const;
        void setAttachReturnType(bool newAttachReturnType);
        bool attachReturnTypeDecl() const;
        void setAttachReturnTypeDecl(bool newAttachReturnTypeDecl);
        bool keepOneLineBlocks() const;
        void setKeepOneLineBlocks(bool newKeepOneLineBlocks);
        bool keepOneLineStatements() const;
        void setKeepOneLineStatements(bool newKeepOneLineStatements);
        bool convertTabs() const;
        void setConvertTabs(bool newConvertTabs);
        bool closeTemplates() const;
        void setCloseTemplates(bool newCloseTemplates);
        bool removeCommentPrefix() const;
        void setRemoveCommentPrefix(bool newRemoveCommentPrefix);
        int maxCodeLength() const;
        void setMaxCodeLength(int newMaxCodeLength);
        bool breakAfterLogical() const;
        void setBreakAfterLogical(bool newBreakAfterLogical);

        bool breakReturnType() const;
        void setBreakReturnType(bool newBreakReturnType);

        bool breakMaxCodeLength() const;
        void setBreakMaxCodeLength(bool newBreakMaxCodeLength);

        bool indentSwitches() const;
        void setIndentSwitches(bool newIndentSwitches);

        bool indentAfterParens() const;
        void setIndentAfterParens(bool newIndentAfterParens);

    private:
        int mBraceStyle;
        int mIndentStyle;
        int mTabWidth;
        bool mAttachNamespaces;
        bool mAttachClasses;
        bool mAttachInlines;
        bool mAttachExternC;
        bool mAttachClosingWhile;
        bool mIndentClasses;
        bool mIndentModifiers;
        bool mIndentSwitches;
        bool mIndentCases;
        bool mIndentNamespaces;
        bool mIndentAfterParens;
        int mIndentContinuation;
        bool mIndentLabels;
        bool mIndentPreprocBlock;
        bool mIndentPreprocCond;
        bool mIndentPreprocDefine;
        bool mIndentCol1Comments;
        int mMinConditionalIndent;
        int mMaxContinuationIndent;
        bool mBreakBlocks;
        bool mBreakBlocksAll;
        bool mPadOper;
        bool mPadComma;
        bool mPadParen;
        bool mPadParenOut;
        bool mPadFirstParenOut;
        bool mPadParenIn;
        bool mPadHeader;
        bool mUnpadParen;
        bool mDeleteEmptyLines;
        bool mDeleteMultipleEmptyLines;
        bool mFillEmptyLines;
        int mAlignPointerStyle;
        int mAlignReferenceStyle;
        bool mBreakClosingBraces;
        bool mBreakElseIf;
        bool mBreakOneLineHeaders;
        bool mAddBraces;
        bool mAddOneLineBraces;
        bool mRemoveBraces;
        bool mBreakReturnType;
        bool mBreakReturnTypeDecl;
        bool mAttachReturnType;
        bool mAttachReturnTypeDecl;
        bool mKeepOneLineBlocks;
        bool mKeepOneLineStatements;
        bool mConvertTabs;
        bool mCloseTemplates;
        bool mRemoveCommentPrefix;
        bool mBreakMaxCodeLength;
        int mMaxCodeLength;
        bool mBreakAfterLogical;
        // _Base interface
    protected:
        void doSave() override;
        void doLoad() override;
    };

    class History: public _Base {
    public:
        explicit History(Settings *settings);

        const QStringList& openedFiles() const;
        const QStringList& openedProjects() const;
        bool addToOpenedFiles(const QString& filename);
        void removeFile(const QString& filename);
        bool addToOpenedProjects(const QString& filename);
        void removeProject(const QString& filename);
    private:
        QStringList mOpenedFiles;
        QStringList mOpenedProjects;

        // _Base interface
    protected:
        void doSave() override;
        void doLoad() override;
    };

    class Executor: public _Base {
    public:
        explicit Executor(Settings * settings);

        bool pauseConsole() const;
        void setPauseConsole(bool pauseConsole);

        bool minimizeOnRun() const;
        void setMinimizeOnRun(bool minimizeOnRun);

        bool useParams() const;
        void setUseParams(bool newUseParams);
        const QString &params() const;
        void setParams(const QString &newParams);
        bool redirectInput() const;
        void setRedirectInput(bool newRedirectInput);
        const QString &inputFilename() const;
        void setInputFilename(const QString &newInputFilename);

        bool enableProblemSet() const;
        void setEnableProblemSet(bool newEnableProblemSet);

        bool enableCompetitiveCompanion() const;
        void setEnableCompetitiveCompanion(bool newEnableCompetitiveCompanion);

        int competivieCompanionPort() const;
        void setCompetivieCompanionPort(int newCompetivieCompanionPort);

    private:
        // general
        bool mPauseConsole;
        bool mMinimizeOnRun;
        bool mUseParams;
        QString mParams;
        bool mRedirectInput;
        QString mInputFilename;

        //Problem Set
        bool mEnableProblemSet;
        bool mEnableCompetitiveCompanion;
        int mCompetivieCompanionPort;

    protected:
        void doSave() override;
        void doLoad() override;
    };

    class UI: public _Base {
    public:
        explicit UI(Settings *settings);

        const QByteArray &mainWindowState() const;
        void setMainWindowState(const QByteArray &newMainWindowState);

        const QByteArray &mainWindowGeometry() const;
        void setMainWindowGeometry(const QByteArray &newMainWindowGeometry);

        bool bottomPanelOpenned() const;
        void setBottomPanelOpenned(bool newBottomPanelOpenned);
        int bottomPanelHeight() const;
        void setBottomPanelHeight(int newBottomPanelHeight);
        int bottomPanelIndex() const;
        void setBottomPanelIndex(int newBottomPanelIndex);
        bool leftPanelOpenned() const;
        void setLeftPanelOpenned(bool newLeftPanelOpenned);
        int leftPanelWidth() const;
        void setLeftPanelWidth(int newLeftPanelWidth);
        int leftPanelIndex() const;
        void setLeftPanelIndex(int newLeftPanelIndex);

        bool classBrowserSortAlpha() const;
        void setClassBrowserSortAlpha(bool newClassBrowserSortAlpha);

        bool classBrowserSortType() const;
        void setClassBrowserSortType(bool newClassBrowserSortType);

        bool classBrowserShowInherited() const;
        void setClassBrowserShowInherited(bool newClassBrowserShowInherited);

    private:
        QByteArray mMainWindowState;
        QByteArray mMainWindowGeometry;
        bool mBottomPanelOpenned;
        int mBottomPanelHeight;
        int mBottomPanelIndex;
        bool mLeftPanelOpenned;
        int mLeftPanelWidth;
        int mLeftPanelIndex;
        bool mClassBrowserSortAlpha;
        bool mClassBrowserSortType;
        bool mClassBrowserShowInherited;

    protected:
        void doSave() override;
        void doLoad() override;
    };

    class Debugger: public _Base {
    public:
        explicit Debugger(Settings* settings);
        bool showCommandLog() const;
        void setShowCommandLog(bool showCommandLog);

        bool showAnnotations() const;
        void setShowAnnotations(bool showAnnotations);

        bool onlyShowMono() const;
        void setOnlyShowMono(bool onlyShowMono);

        int fontSize() const;
        void setFontSize(int fontSize);

        bool useIntelStyle() const;
        void setUseIntelStyle(bool useIntelStyle);

        QString fontName() const;
        void setFontName(const QString &fontName);

        bool blendMode() const;
        void setBlendMode(bool blendMode);

        bool skipSystemLibraries() const;
        void setSkipSystemLibraries(bool newSkipSystemLibraries);
        bool skipProjectLibraries() const;
        void setSkipProjectLibraries(bool newSkipProjectLibraries);
        bool skipCustomLibraries() const;
        void setSkipCustomLibraries(bool newSkipCustomLibraries);

        bool autosaveBreakpoints() const;
        void setAutosaveBreakpoints(bool newAutosaveBreakpoints);

        bool autosaveWatches() const;
        void setAutosaveWatches(bool newAutosaveWatches);

    private:
        bool mShowCommandLog;
        bool mShowAnnotations;
        QString mFontName;
        bool mOnlyShowMono;
        int mFontSize;
        bool mUseIntelStyle;
        bool mBlendMode;
        bool mSkipSystemLibraries;
        bool mSkipProjectLibraries;
        bool mSkipCustomLibraries;
        bool mAutosaveBreakpoints;
        bool mAutosaveWatches;

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
        int findOptionIndex(const QString& setting);
        char getOptionValue(const QString& setting);
        void setOption(const QString& setting, char valueChar);
        void setOption(PCompilerOption& option, char valueChar);
        void setProperties(const QString& binDir);
        void setDirectories(const QString& binDir);
        int mainVersion();

        bool dirsValid(QString& msg);
        bool validateExes(QString& msg);
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
        QStringList& defaultCIncludeDirs();
        QStringList& defaultCppIncludeDirs();
        QStringList& defaultLibDirs();

        const QString& dumpMachine() const;
        void setDumpMachine(const QString& value);
        const QString& version() const;
        void setVersion(const QString& value);
        const QString& type() const;
        void setType(const QString& value);
        const QString& name() const;
        void setName(const QString& value);
        const QStringList& defines() const;
        const QString& target() const;
        void setTarget(const QString& value);

        bool useCustomCompileParams() const;
        void setUseCustomCompileParams(bool value);
        bool useCustomLinkParams() const;
        void setUseCustomLinkParams(bool value);
        const QString& customCompileParams() const;
        void setCustomCompileParams(const QString& value);
        const QString& customLinkParams() const;
        void setCustomLinkParams(const QString& value);
        bool autoAddCharsetParams() const;
        void setAutoAddCharsetParams(bool value);

        CompilerOptionList& options();

        //Converts options to and from memory format
        QByteArray iniOptions() const;
        void setIniOptions(const QByteArray& value);

        //load hard defines
        void setDefines();

        bool staticLink() const;
        void setStaticLink(bool newStaticLink);


        static int charToValue(char valueChar);
        static char valueToChar(int val);
        const QString &compilerType() const;

        void setCompilerType(const QString &newCompilerType);

        int compilerSetType() const;
        void setCompilerSetType(int newCompilerSetType);

    private:
        // Initialization
        void setExecutables();
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
        QStringList mDefaultLibDirs;
        QStringList mDefaultCIncludeDirs;
        QStringList mDefaultCppIncludeDirs;

        // Misc. properties
        QString mDumpMachine; // "x86_64-w64-mingw32", "mingw32" etc
        QString mVersion; // "4.7.1"
        QString mType; // "TDM-GCC", "MinGW"
        QString mName; // "TDM-GCC 4.7.1 Release"
        QStringList mDefines; // list of predefined constants
        QString mTarget; // 'X86_64' / 'i686'
        QString mCompilerType; // 'Clang' / 'GCC'
        int mCompilerSetType; // RELEASE/ DEBUG/ Profile

        // User settings
        bool mUseCustomCompileParams;
        bool mUseCustomLinkParams;
        QString mCustomCompileParams;
        QString mCustomLinkParams;
        bool mAutoAddCharsetParams;
        bool mStaticLink;

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
        void saveSet(int index);
        //properties
        CompilerSetList& list();
        int size() const;
        int defaultIndex() const;
        void setDefaultIndex(int value);
        PCompilerSet defaultSet();
        PCompilerSet getSet(int index);
    private:
        void savePath(const QString& name, const QString& path);
        void savePathList(const QString& name, const QStringList& pathList);

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
    void load();

    Dirs& dirs();
    Editor& editor();
    CompilerSets& compilerSets();
    Environment& environment();
    Executor& executor();
    Debugger& debugger();
    History& history();
    CodeCompletion &codeCompletion();
    CodeFormatter &codeFormatter();
    UI &ui();
    QString filename() const;

private:
    QString mFilename;
    QSettings mSettings;
    Dirs mDirs;
    Editor mEditor;
    Environment mEnvironment;
    CompilerSets mCompilerSets;
    Executor mExecutor;
    Debugger mDebugger;
    CodeCompletion mCodeCompletion;
    CodeFormatter mCodeFormatter;
    History mHistory;
    UI mUI;
};


extern Settings* pSettings;

#endif // SETTINGS_H
