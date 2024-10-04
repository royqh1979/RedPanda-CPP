/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <vector>
#include <memory>
#include <QColor>
#include <QString>
#include <QPair>
#include "qsynedit/qsynedit.h"
#include "compiler/compilerinfo.h"
#include "utils.h"
#include "utils/font.h"

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
#define SETTING_VCS "VCS"
#define SETTING_LANGUAGES "Languages"
#define SETTING_CODE_COMPLETION "CodeCompletion"
#define SETTING_CODE_FORMATTER "CodeFormatter"
#define SETTING_COMPILTER_SETS "CompilerSets"
#define SETTING_COMPILTER_SETS_DEFAULT_INDEX "defaultIndex"
#define SETTING_COMPILTER_SETS_DEFAULT_INDEX_TIMESTAMP "defaultIndexTimestamp"
#define SETTING_COMPILTER_SETS_COUNT "count"
#define SETTING_COMPILTER_SET "CompilerSet_%1"
#define SETTING_EDITOR_DEFAULT_ENCODING "default_encoding"
#define SETTING_EDITOR_AUTO_INDENT "default_auto_indent"

extern const char ValueToChar[28];

class Settings;

class Settings
{
private:

    class _Base {
    public:
        explicit _Base(Settings* settings, const QString& groupName);
        void beginGroup();
        void endGroup();
        void remove(const QString &key);
        void saveValue(const QString &key, const QVariant &value);
        void saveValue(const QString &key, const QSet<QString>& set);
        QVariant value(const QString &key, const QVariant& defaultValue);
        bool boolValue(const QString &key, bool defaultValue);
        QSize sizeValue(const QString &key, const QSize& size=QSize());
        int intValue(const QString &key, int defaultValue);
        double doubleValue(const QString& key, double defaultValue);
        unsigned int uintValue(const QString &key, unsigned int defaultValue);
        QStringList stringListValue(const QString &key, const QStringList& defaultValue=QStringList());
        QSet<QString> stringSetValue(const QString &key);
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
            ColorScheme,
            IconSet,
            Theme,
            Template
        };
        explicit Dirs(Settings * settings);
        QString appDir() const;
        QString appResourceDir() const;
        QString appLibexecDir() const;
        QString projectDir() const;
        QString data(DataType dataType = DataType::None) const;
        QString config(DataType dataType = DataType::None) const;
        QString executable() const;

        void setProjectDir(const QString &newProjectDir);

    protected:
        void doSave() override;
        void doLoad() override;
    private:
        QString mProjectDir;
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

        QSynedit::EditCaretType caretForInsert() const;
        void setCaretForInsert(const QSynedit::EditCaretType &caretForInsert);

        QSynedit::EditCaretType caretForOverwrite() const;
        void setCaretForOverwrite(const QSynedit::EditCaretType &caretForOverwrite);

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

        bool halfPageScroll() const;
        void setHalfPageScroll(bool halfPageScroll);

        QString fontName() const;

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

        bool highlightCurrentWord() const;
        void setHighlightCurrentWord(bool newHighlightCurrentWord);

        bool highlightMathingBraces() const;
        void setHighlightMathingBraces(bool newHighlightMathingBraces);

        bool enableLigaturesSupport() const;
        void setEnableLigaturesSupport(bool newEnableLigaturesSupport);

        QStringList fontFamilies() const;
        void setFontFamilies(const QStringList &newFontFamilies);
        QStringList fontFamiliesWithControlFont() const;

        int mouseSelectionScrollSpeed() const;
        void setMouseSelectionScrollSpeed(int newMouseSelectionScrollSpeed);

        bool autoDetectFileEncoding() const;
        void setAutoDetectFileEncoding(bool newAutoDetectFileEncoding);

        bool autoFormatWhenSaved() const;
        void setAutoFormatWhenSaved(bool newAutoFormatWhenSaved);

        bool parseTodos() const;
        void setParseTodos(bool newParseTodos);

        const QStringList &customCTypeKeywords() const;
        void setCustomCTypeKeywords(const QStringList &newCustomTypeKeywords);

        bool enableCustomCTypeKeywords() const;
        void setEnableCustomCTypeKeywords(bool newEnableCustomCTypeKeywords);

        bool removeTrailingSpacesWhenSaved() const;
        void setRemoveTrailingSpacesWhenSaved(bool newRemoveTrailingSpacesWhenSaved);

        double lineSpacing() const;
        void setLineSpacing(double newLineSpacing);

        bool showLineBreaks() const;
        void setShowLineBreaks(bool newShowLineBreaks);

        bool showInnerSpaces() const;
        void setShowInnerSpaces(bool newShowMiddleSpaces);

        bool showTrailingSpaces() const;
        void setShowTrailingSpaces(bool newShowEndSpaces);

        bool showLeadingSpaces() const;
        void setShowLeadingSpaces(bool newShowStartSpaces);

        bool enableEditTempBackup() const;
        void setEnableEditTempBackup(bool newEnableEditTempBackup);

        int tipsDelay() const;
        void setTipsDelay(int newTipsDelay);

        bool forceFixedFontWidth() const;
        void setForceFixedFontWidth(bool newForceFixedWidth);

        bool copyHTMLWithLineNumber() const;
        void setCopyHTMLWithLineNumber(bool newCopyHTMLWithLineNumber);

        bool copyHTMLRecalcLineNumber() const;
        void setCopyHTMLRecalcLineNumber(bool newCopyHTMLRecalcLineNumber);

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
        QSynedit::EditCaretType mCaretForInsert;
        QSynedit::EditCaretType mCaretForOverwrite;
        bool mCaretUseTextColor;
        QColor mCaretColor;


        //highlights
        bool mHighlightCurrentWord;
        bool mHighlightMathingBraces;

        //scroll
        bool mAutoHideScrollbar;
        bool mScrollPastEof;
        bool mScrollPastEol;
        bool mHalfPageScroll;
        int mMouseWheelScrollSpeed;
        int mMouseSelectionScrollSpeed;

        //right margin
        bool mShowRightEdgeLine;
        int mRightEdgeWidth;
        QColor mRightEdgeLineColor;

        //Font
        //font
        QStringList mFontFamilies;
        int mFontSize;
        double mLineSpacing;
        bool mEnableLigaturesSupport;
        bool mForceFixedFontWidth;

        bool mShowLeadingSpaces;
        bool mShowTrailingSpaces;
        bool mShowInnerSpaces;
        bool mShowLineBreaks;

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
        int mCopyWithFormatAs;
        bool mCopyRTFUseBackground;
        bool mCopyRTFUseEditorColor;
        QString mCopyRTFColorScheme;
        bool mCopyHTMLUseBackground;
        bool mCopyHTMLUseEditorColor;
        bool mCopyHTMLWithLineNumber;
        bool mCopyHTMLRecalcLineNumber;
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
        bool mEnableEditTempBackup;
        bool mEnableAutoSave;
        int mAutoSaveInterval;
        enum AutoSaveTarget mAutoSaveTarget;
        enum AutoSaveStrategy mAutoSaveStrategy;

        //auto link
        bool mEnableAutolink;

        //Misc
        QByteArray mDefaultEncoding;
        bool mAutoDetectFileEncoding;
        bool mReadOnlySytemHeader;
        bool mAutoLoadLastFiles;
        bool mDefaultFileCpp;
        bool mAutoFormatWhenSaved;
        bool mRemoveTrailingSpacesWhenSaved;
        bool mParseTodos;

        QStringList mCustomCTypeKeywords;
        bool mEnableCustomCTypeKeywords;


        //hints tooltip
        int mTipsDelay;
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
        struct TerminalItem {
            QString name;
            QString terminal;
            QString param;
        };

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

        const QString &defaultOpenFolder() const;
        void setDefaultOpenFolder(const QString &newDefaultOpenFolder);

        const QString &iconSet() const;
        void setIconSet(const QString &newIconSet);

        QString terminalPath() const;
        void setTerminalPath(const QString &terminalPath);

        QString AStylePath() const;
        void setAStylePath(const QString &aStylePath);

        QString terminalArgumentsPattern() const;
        void setTerminalArgumentsPattern(const QString &argsPattern);

        bool useCustomIconSet() const;
        void setUseCustomIconSet(bool newUseCustomIconSet);

        bool hideNonSupportFilesInFileView() const;
        void setHideNonSupportFilesInFileView(bool newHideNonSupportFilesInFileView);

        bool openFilesInSingleInstance() const;
        void setOpenFilesInSingleInstance(bool newOpenFilesInSingleInstance);

        double iconZoomFactor() const;
        void setIconZoomFactor(double newIconZoomFactor);

        QJsonArray availableTerminals() const;
        void setAvailableTerminals(const QJsonArray &availableTerminals);

        QString queryPredefinedTerminalArgumentsPattern(const QString &executable) const;

        bool useCustomTerminal() const;
        void setUseCustomTerminal(bool newUseCustomTerminal);

        QList<TerminalItem> loadTerminalList() const;

        static QMap<QString, QString> terminalArgsPatternMagicVariables();

    private:
        bool isTerminalValid();
        void checkAndSetTerminal();

        //Appearance
        QString mTheme;
        QString mInterfaceFont;
        int mInterfaceFontSize;
        double mIconZoomFactor;
        QString mLanguage;
        QString mCurrentFolder;
        QString mIconSet;
        bool mUseCustomIconSet;
        bool mUseCustomTheme;

        QString mDefaultOpenFolder;
        QString mTerminalPath;
        QString mAStylePath;
        QString mTerminalArgumentsPattern;
        bool mUseCustomTerminal;
        bool mHideNonSupportFilesInFileView;
        bool mOpenFilesInSingleInstance;

        static const QMap<QString, QString> mTerminalArgsPatternMagicVariables;
        // _Base interface
    protected:
        void doSave() override;
        void doLoad() override;
    };

    class CodeCompletion: public _Base {
    public:
        explicit CodeCompletion(Settings *settings);
        int widthInColumns() const;
        void setWidthInColumns(int newWidth);

        int heightInLines() const;
        void setHeightInLines(int newHeight);

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

        //bool clearWhenEditorHidden();
        //void setClearWhenEditorHidden(bool newClearWhenEditorHidden);

        int minCharRequired() const;
        void setMinCharRequired(int newMinCharRequired);

        bool hideSymbolsStartsWithUnderLine() const;
        void setHideSymbolsStartsWithUnderLine(bool newHideSymbolsStartsWithOneUnderLine);

        bool hideSymbolsStartsWithTwoUnderLine() const;
        void setHideSymbolsStartsWithTwoUnderLine(bool newHideSymbolsStartsWithTwoUnderLine);

        bool shareParser();
        void setShareParser(bool newShareParser);

    private:
        int mWidthInColumns;
        int mHeightInLines;
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
        int mMinCharRequired;
        bool mHideSymbolsStartsWithTwoUnderLine;
        bool mHideSymbolsStartsWithUnderLine;
        //bool mClearWhenEditorHidden;
        bool mShareParser;

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

        bool squeezeLines() const;
        void setSqueezeLines(bool newSqueezeLines);

        int squeezeLinesNumber() const;
        void setSqueezeLinesNumber(int newSqueezeLinesNumber);

        bool squeezeWhitespace() const;
        void setSqueezeWhitespace(bool newSqueezeWhitespace);

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
        bool mFillEmptyLines;
        bool mSqueezeLines;
        int mSqueezeLinesNumber;
        bool mSqueezeWhitespace;
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

        const QString &caseEditorFontName() const;
        void setCaseEditorFontName(const QString &newCaseEditorFontName);

        int caseEditorFontSize() const;
        void setCaseEditorFontSize(int newCaseEditorFontSize);

        bool caseEditorFontOnlyMonospaced() const;
        void setCaseEditorFontOnlyMonospaced(bool newCaseEditorFontOnlyMonospaced);

        bool enableCaseLimit() const;
        void setEnableCaseLimit(bool newValue);

        size_t caseTimeout() const;
        void setCaseTimeout(size_t newCaseTimeout);

        size_t caseMemoryLimit() const;
        void setCaseMemoryLimit(size_t newCaseMemoryLimit);

        bool convertHTMLToTextForInput() const;
        void setConvertHTMLToTextForInput(bool newConvertHTMLToTextForInput);

        bool convertHTMLToTextForExpected() const;
        void setConvertHTMLToTextForExpected(bool newConvertHTMLToTextForExpected);

        bool redirectStderrToToolLog() const;
        void setRedirectStderrToToolLog(bool newRedirectStderrToToolLog);

        ProblemCaseValidateType problemCaseValidateType() const;
        void setProblemCaseValidateType(ProblemCaseValidateType newProblemCaseValidateType);

        bool enableVirualTerminalSequence() const;
        void setEnableVirualTerminalSequence(bool newEnableVirualTerminalSequence);
    private:
        // general
        bool mPauseConsole;
        bool mMinimizeOnRun;
        bool mUseParams;
        QString mParams;
        bool mRedirectInput;
        QString mInputFilename;
        bool mEnableVirualTerminalSequence;

        //Problem Set
        bool mEnableProblemSet;
        bool mEnableCompetitiveCompanion;
        int mCompetivieCompanionPort;
        bool mConvertHTMLToTextForInput;
        bool mConvertHTMLToTextForExpected;
        bool mIgnoreSpacesWhenValidatingCases;
        ProblemCaseValidateType mProblemCaseValidateType;
        bool mRedirectStderrToToolLog;
        QString mCaseEditorFontName;
        int mCaseEditorFontSize;
        bool mCaseEditorFontOnlyMonospaced;
        bool mEnableCaseLimit;
        qulonglong mCaseTimeout; //ms
        qulonglong mCaseMemoryLimit; //kb

    protected:
        void doSave() override;
        void doLoad() override;
    };
#ifdef ENABLE_VCS
    class VCS: public _Base {
    public:
        explicit VCS(Settings *settings);
        const QString &gitPath() const;
        void setGitPath(const QString &newGitPath);
        bool gitOk() const;
        void detectGitInPath();
    private:
        void validateGit();
    private:
        QString mGitPath;
        bool mGitOk;
    protected:
        void doSave() override;
        void doLoad() override;
    };
#endif

    class Languages: public _Base {
    public:
        enum class X86ASMDialect {
            ATT,
            Intel
        };
        explicit Languages(Settings *settings);
        bool noDebugDirectivesWhenGenerateASM() const;
        void setNoDebugDirectivesWhenGenerateASM(bool newNoDebugDirectivesWhenGenerateASM);

        bool noSEHDirectivesWhenGenerateASM() const;
        void setNoSEHDirectivesWhenGenerateASM(bool newNoSEHDirectivesWhenGenerateASM);

        X86ASMDialect x86DialectOfASMGenerated() const;
        void setX86DialectOfASMGenerated(X86ASMDialect newX86DialectOfASMGenerated);

        bool indentCClassMemberVisibilityKeywords() const;
        void setIndentCClassMemberVisibilityKeywords(bool newIndentCClassMemberVisibilityKeywords);

        bool indentCSwitchCaseKeywords() const;
        void setIndentCSwitchCaseKeywords(bool newIndentCSwitchCaseKeywords);

    private:
        bool mNoDebugDirectivesWhenGenerateASM;
        bool mNoSEHDirectivesWhenGenerateASM;
        X86ASMDialect mX86DialectOfASMGenerated;
        bool mIndentCClassMemberVisibilityKeywords;
        bool mIndentCSwitchCaseKeywords;
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

        int bottomPanelIndex() const;
        void setBottomPanelIndex(int newBottomPanelIndex);
        int leftPanelIndex() const;
        void setLeftPanelIndex(int newLeftPanelIndex);

        bool classBrowserSortAlpha() const;
        void setClassBrowserSortAlpha(bool newClassBrowserSortAlpha);

        bool classBrowserSortType() const;
        void setClassBrowserSortType(bool newClassBrowserSortType);

        bool classBrowserShowInherited() const;
        void setClassBrowserShowInherited(bool newClassBrowserShowInherited);

        bool showToolbar() const;
        void setShowToolbar(bool newShowToolbar);

        bool showStatusBar() const;
        void setShowStatusBar(bool newShowStatusBar);

        bool showToolWindowBars() const;
        void setShowToolWindowBars(bool newShowToolWindowBars);

        bool showProject() const;
        void setShowProject(bool newShowProject);

        bool showWatch() const;
        void setShowWatch(bool newShowWatch);

        bool showStructure() const;
        void setShowStructure(bool newShowStructure);

        bool showFiles() const;
        void setShowFiles(bool newShowFiles);

        bool showProblemSet() const;
        void setShowProblemSet(bool newShowProblemSet);

        bool showIssues() const;
        void setShowIssues(bool newShowIssues);

        bool showCompileLog() const;
        void setShowCompileLog(bool newShowCompileLog);

        bool showDebug() const;
        void setShowDebug(bool newShowDebug);

        bool showSearch() const;
        void setShowSearch(bool newShowSearch);

        bool showTODO() const;
        void setShowTODO(bool newShowTODO);

        bool showBookmark() const;
        void setShowBookmark(bool newShowBookmark);

        bool showProblem() const;
        void setShowProblem(bool newShowProblem);

        int CPUDialogWidth() const;
        void setCPUDialogWidth(int newCPUDialogWidth);

        int CPUDialogHeight() const;
        void setCPUDialogHeight(int newCPUDialogHeight);

        int CPUDialogSplitterPos() const;
        void setCPUDialogSplitterPos(int newCPUDialogSplitterPos);

        int settingsDialogWidth() const;
        void setSettingsDialogWidth(int newSettingsDialogWidth);

        int settingsDialogHeight() const;
        void setSettingsDialogHeight(int newSettingsDialogHeight);

        int settingsDialogSplitterPos() const;
        void setSettingsDialogSplitterPos(int newSettingsDialogSplitterPos);

        int newProjectDialogWidth() const;
        void setNewProjectDialogWidth(int newNewProjectDialogWidth);

        int newProjectDialogHeight() const;
        void setNewProjectDialogHeight(int newNewProjectDialogHeight);

        int newClassDialogWidth() const;
        void setNewClassDialogWidth(int newNewClassDialogWidth);

        int newClassDialogHeight() const;
        void setNewClassDialogHeight(int newNewClassDialogHeight);

        int newHeaderDialogWidth() const;
        void setNewHeaderDialogWidth(int newNewFileDialogWidth);

        int newHeaderDialogHeight() const;
        void setNewHeaderDialogHeight(int newNewFileDialogHeight);

        bool shrinkExplorerTabs() const;
        void setShrinkExplorerTabs(bool newShrinkExplorerTabs);

        bool shrinkMessagesTabs() const;
        void setShrinkMessagesTabs(bool newShrinkMessagesTabs);

        const QSize &explorerTabsSize() const;
        void setExplorerTabsSize(const QSize &newExplorerTabsSize);

        const QSize &messagesTabsSize() const;
        void setMessagesTabsSize(const QSize &newMessagesTabsSize);

        int debugPanelIndex() const;
        void setDebugPanelIndex(int newDebugPanelIndex);

        int projectOrder() const;
        void setProjectOrder(int newProjectOrder);

        int watchOrder() const;
        void setWatchOrder(int newWatchOrder);

        int structureOrder() const;
        void setStructureOrder(int newStructureOrder);

        int filesOrder() const;
        void setFilesOrder(int newFilesOrder);

        int problemSetOrder() const;
        void setProblemSetOrder(int newProblemSetOrder);

        int issuesOrder() const;
        void setIssuesOrder(int newIssuesOrder);

        int compileLogOrder() const;
        void setCompileLogOrder(int newCompileLogOrder);

        int debugOrder() const;
        void setDebugOrder(int newDebugOrder);

        int searchOrder() const;
        void setSearchOrder(int newSearchOrder);

        int TODOOrder() const;
        void setTODOOrder(int newTODOOrder);

        int bookmarkOrder() const;
        void setBookmarkOrder(int newBookmarkOrder);

        int problemOrder() const;
        void setProblemOrder(int newProblemOrder);

        bool openEditorsWhenReplace() const;
        void setOpenEditorsWhenReplace(bool newOpenEditorsWhenReplace);

    private:
        bool mOpenEditorsWhenReplace;
        QByteArray mMainWindowState;
        QByteArray mMainWindowGeometry;
        int mBottomPanelIndex;
        int mLeftPanelIndex;
        int mDebugPanelIndex;
        bool mClassBrowserSortAlpha;
        bool mClassBrowserSortType;
        bool mClassBrowserShowInherited;

        bool mShrinkExplorerTabs;
        bool mShrinkMessagesTabs;
        QSize mExplorerTabsSize;
        QSize mMessagesTabsSize;
        //view
        bool mShowToolbar;
        bool mShowStatusBar;
        bool mShowToolWindowBars;

        bool mShowProject;
        bool mShowWatch;
        bool mShowStructure;
        bool mShowFiles;
        bool mShowProblemSet;
        int mProjectOrder;
        int mWatchOrder;
        int mStructureOrder;
        int mFilesOrder;
        int mProblemSetOrder;

        bool mShowIssues;
        bool mShowCompileLog;
        bool mShowDebug;
        bool mShowSearch;
        bool mShowTODO;
        bool mShowBookmark;
        bool mShowProblem;
        int mIssuesOrder;
        int mCompileLogOrder;
        int mDebugOrder;
        int mSearchOrder;
        int mTODOOrder;
        int mBookmarkOrder;
        int mProblemOrder;

        //dialogs
        int mCPUDialogWidth;
        int mCPUDialogHeight;
        int mCPUDialogSplitterPos;
        int mSettingsDialogWidth;
        int mSettingsDialogHeight;
        int mSettingsDialogSplitterPos;
        int mNewProjectDialogWidth;
        int mNewProjectDialogHeight;
        int mNewClassDialogWidth;
        int mNewClassDialogHeight;
        int mNewHeaderDialogWidth;
        int mNewHeaderDialogHeight;

    protected:
        void doSave() override;
        void doLoad() override;
    };

    class Debugger: public _Base {
    public:
        explicit Debugger(Settings* settings);
        bool enableDebugConsole() const;
        void setEnableDebugConsole(bool showCommandLog);

        bool showDetailLog() const;
        void setShowDetailLog(bool showAnnotations);

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

        bool openCPUInfoWhenSignaled() const;
        void setOpenCPUInfoWhenSignaled(bool newOpenCPUInfoWhenSignaled);

        bool useGDBServer() const;
        void setUseGDBServer(bool newUseGDBServer);
        int GDBServerPort() const;
        void setGDBServerPort(int newGDBServerPort);

        int memoryViewRows() const;
        void setMemoryViewRows(int newMemoryViewRows);

        int memoryViewColumns() const;
        void setMemoryViewColumns(int newMemoryViewColumns);

        bool autosave() const;
        void setAutosave(bool newAutosave);

        int arrayElements() const;
        void setArrayElements(int newArrayElements);

        int characters() const;
        void setCharacters(int newCharacters);

    private:
        bool mEnableDebugConsole;
        bool mShowDetailLog;
        QString mFontName;
        bool mOnlyShowMono;
        int mFontSize;
        bool mUseIntelStyle;
        bool mBlendMode;
        bool mSkipSystemLibraries;
        bool mSkipProjectLibraries;
        bool mSkipCustomLibraries;
        bool mAutosave;
        bool mOpenCPUInfoWhenSignaled;
        bool mUseGDBServer;
        int mGDBServerPort;
        int mMemoryViewRows;
        int mMemoryViewColumns;
        int mArrayElements;
        int mCharacters;

        // _Base interface
    protected:
        void doSave() override;
        void doLoad() override;
    };

    class CompilerSet {
    public:
        enum class CompilationStage {
            PreprocessingOnly,
            CompilationProperOnly,
            AssemblingOnly,
            GenerateExecutable
        };

        explicit CompilerSet();
        explicit CompilerSet(const QString& compilerFolder, const QString& c_prog);
        explicit CompilerSet(const CompilerSet& set);
        explicit CompilerSet(const QJsonObject& set);

        CompilerSet& operator= (const CompilerSet& ) = delete;
        CompilerSet& operator= (const CompilerSet&& ) = delete;

        // Initialization
        void setProperties(const QString& binDir, const QString& c_prog);
        QStringList x86MultilibList(const QString &folder, const QString &c_prog) const;

        void resetCompileOptionts();
        bool setCompileOption(const QString& key, int valIndex);
        bool setCompileOption(const QString& key, const QString& value);
        void unsetCompileOption(const QString& key);
        void setCompileOptions(const QMap<QString, QString> options);

        QString getCompileOptionValue(const QString& key) const;

        int mainVersion() const;
        QString findProgramInBinDirs(const QString name) const;

        bool canCompileC() const;
        bool canCompileCPP() const;
        bool canMake() const;
        bool canDebug() const;
//        bool dirsValid(QString& msg);
//        bool validateExes(QString& msg);
        //properties
        const QString& CCompiler() const;
        void setCCompiler(const QString& name);
        const QString& cppCompiler() const;
        void setCppCompiler(const QString& name);
        const QString& make() const;
        void setMake(const QString& name);
        const QString& debugger() const;
        void setDebugger(const QString& name);
        const QString& resourceCompiler() const;
        void setResourceCompiler(const QString& name);
        const QString &debugServer() const;
        void setDebugServer(const QString &newDebugServer);

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
        const QString& name() const;
        void setName(const QString& value);
        QStringList defines(bool isCpp);
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

        //Converts options to and from memory format ( for old settings compatibility)
        void setIniOptions(const QByteArray& value);

        bool staticLink() const;
        void setStaticLink(bool newStaticLink);


        static int charToValue(char valueChar);
        static char valueToChar(int val);
        CompilerType compilerType() const;

        void setCompilerType(CompilerType newCompilerType);

        const QString &execCharset() const;
        void setExecCharset(const QString &newExecCharset);

        const QMap<QString, QString> &compileOptions() const;

        const QString &executableSuffix() const;
        void setExecutableSuffix(const QString &newExecutableSuffix);

        const QString &preprocessingSuffix() const;
        void setPreprocessingSuffix(const QString &newPreprocessingSuffix);

        const QString &compilationProperSuffix() const;
        void setCompilationProperSuffix(const QString &newCompilationProperSuffix);

        const QString &assemblingSuffix() const;
        void setAssemblingSuffix(const QString &newAssemblingSuffix);

        CompilationStage compilationStage() const;
        void setCompilationStage(CompilationStage newCompilationStage);

        QString getOutputFilename(const QString& sourceFilename);
        QString getOutputFilename(const QString& sourceFilename,Settings::CompilerSet::CompilationStage stage);
        bool isOutputExecutable();
        bool isOutputExecutable(Settings::CompilerSet::CompilationStage stage);

#ifdef Q_OS_WINDOWS
        bool isDebugInfoUsingUTF8() const;
        bool forceUTF8() const;
        bool isCompilerInfoUsingUTF8() const;
#else
        constexpr bool isDebugInfoUsingUTF8() const { return true; }
        constexpr bool forceUTF8() const { return true; }
        constexpr bool isCompilerInfoUsingUTF8() const { return true; }
#endif

        bool persistInAutoFind() const;
        void setPersistInAutoFind(bool newPersistInAutoFind);

        bool forceEnglishOutput() const;
        void setForceEnglishOutput(bool newForceEnglishOutput);

    private:
        void setGCCProperties(const QString& binDir, const QString& c_prog);
        void setDirectories(const QString& binDir);
        void setGCCDirectories(const QString& binDir);
#ifdef ENABLE_SDCC
        void setSDCCProperties(const QString& binDir, const QString& c_prog);
        void setSDCCDirectories(const QString& binDir);
#endif
        //load hard defines
        void setExecutables();
        void setUserInput();


        QByteArray getCompilerOutput(const QString& binDir, const QString& binFile,
                                     const QStringList& arguments) const;
    private:
        bool mFullLoaded;
        // Executables, most are hardcoded
        QString mCCompiler;
        QString mCppCompiler;
        QString mMake;
        QString mDebugger;
        QString mResourceCompiler;
        QString mDebugServer;

        // Directories, mostly hardcoded too
        QStringList mBinDirs;
        QStringList mCIncludeDirs;
        QStringList mCppIncludeDirs;
        QStringList mLibDirs;
        QStringList mDefaultLibDirs;
        QStringList mDefaultCIncludeDirs;
        QStringList mDefaultCppIncludeDirs;

        // Misc. properties
        QString mDumpMachine; // "x86_64-w64-mingw32", "x86_64-pc-linux-gnu", etc
        QString mVersion; // "14.2.0", "14" (--with-gcc-major-version-only)
        QString mName; // "MinGW-w64 GCC 14.2.0 Release"
        QString mTarget; // "x86_64", "i686", etc
        CompilerType mCompilerType;

        // User settings
        bool mUseCustomCompileParams;
        bool mUseCustomLinkParams;
        QString mCustomCompileParams;
        QString mCustomLinkParams;
        bool mAutoAddCharsetParams;
        QString mExecCharset;
        bool mStaticLink;
        bool mPersistInAutoFind;
        bool mForceEnglishOutput;

        QString mPreprocessingSuffix;
        QString mCompilationProperSuffix;
        QString mAssemblingSuffix;
        QString mExecutableSuffix;

        CompilationStage mCompilationStage;

        // Options
        QMap<QString,QString> mCompileOptions;
    };

    typedef std::shared_ptr<CompilerSet> PCompilerSet;
    typedef std::vector<PCompilerSet> CompilerSetList;

    class CompilerSets {
    public:
        explicit CompilerSets(Settings* settings);
        PCompilerSet addSet();
        bool addSets(const QString& folder);
        bool addSets(const QString& folder, const QString& c_prog);
        CompilerSetList clearSets();
        void findSets();
        void saveSets();
        void loadSets();
        void saveDefaultIndex();
        void deleteSet(int index);
        void saveSet(int index);
        size_t size() const;
        int defaultIndex() const;
        void setDefaultIndex(int value);
        qint64 defaultIndexTimestamp() const;
        PCompilerSet defaultSet();
        PCompilerSet getSet(int index);

        QString getKeyFromCompilerCompatibleIndex(int idx) const;

        static bool isTarget64Bit(const QString &target);
    private:
        PCompilerSet addSet(const QString& folder, const QString& c_prog);
        PCompilerSet addSet(const PCompilerSet &pSet);
        PCompilerSet addSet(const QJsonObject &set);
        void savePath(const QString& name, const QString& path);
        void savePathList(const QString& name, const QStringList& pathList);

        QString loadPath(const QString& name);
        void loadPathList(const QString& name, QStringList& list);
        PCompilerSet loadSet(int index);
        void prepareCompatibleIndex();
    private:
        CompilerSetList mList;
        int mDefaultIndex;
        qint64 mDefaultIndexTimeStamp;
        Settings* mSettings;
        QStringList mCompilerCompatibleIndex; // index for old settings compatibility
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
    void remove(const QString &key);
    void saveValue(const QString& group, const QString &key, const QVariant &value);
    void saveValue(const QString &key, const QVariant &value);
    QVariant value(const QString& group, const QString &key, const QVariant& defaultValue);
    QVariant value(const QString &key, const QVariant& defaultValue);
    void load();
    QSettings::Status sync();

    Dirs& dirs();
    Editor& editor();
    CompilerSets& compilerSets();
    Environment& environment();
    Executor& executor();
    Debugger& debugger();
    CodeCompletion &codeCompletion();
    CodeFormatter &codeFormatter();
    UI &ui();
#ifdef ENABLE_VCS
    VCS &vcs();
#endif
    Languages &languages();
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
    UI mUI;
#ifdef ENABLE_VCS
    VCS mVCS;
#endif
    Languages mLanguages;
};

extern Settings* pSettings;

#endif // SETTINGS_H
