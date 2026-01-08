/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#ifndef EDITOR_SETTINGS_H
#define EDITOR_SETTINGS_H
#include "basesettings.h"
#include "../utils.h"
#include "qsynedit/types.h"

#define SETTING_EDITOR "Editor"

class EditorSettings: public BaseSettings {
public:
    explicit EditorSettings(SettingsPersistor * persistor);
    QByteArray defaultEncoding() const;
    void setDefaultEncoding(const QByteArray& value);
    bool autoIndent() const;
    void setAutoIndent(bool value);
    bool addIndent() const;
    void setAddIndent(bool addIndent);

    bool tabToSpaces() const;
    void setTabToSpaces(bool tabToSpaces);

    int tabWidth() const;
    void setTabWidth(int tabWidth);

    bool showIndentLines() const;
    void setShowIndentLines(bool showIndentLines);

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

    bool rainbowIndentGuides() const;
    void setRainbowIndentGuides(bool newIndentLineUsingRainbowColor);

    bool rainbowIndents() const;
    void setRainbowIndents(bool newFillIndentsUsingRainbowColor);

private:
    //General
    // indents
    bool mAutoIndent;
    bool mTabToSpaces;
    int mTabWidth;
    bool mShowIndentLines;
    bool mFillIndents;
    bool mRainbowIndentGuides;
    bool mRainbowIndents;
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
    Q_PROPERTY(bool rainbowIndents READ rainbowIndents WRITE setRainbowIndents NOTIFY fillIndentsUsingRainbowColorChanged)

protected:
    void doSave() override;
    void doLoad() override;
};



#endif
//EDITOR_SETTINGS_H
