#include "editorsettings.h"
#include "../utils/font.h"

EditorSettings::EditorSettings(SettingsPersistor *persistor):
    BaseSettings{persistor, SETTING_EDITOR}
{

}

QByteArray EditorSettings::defaultEncoding() const
{
    return mDefaultEncoding;
}

void EditorSettings::setDefaultEncoding(const QByteArray &value)
{
    mDefaultEncoding = value;
}

bool EditorSettings::autoIndent() const
{
    return mAutoIndent;
}

void EditorSettings::setAutoIndent(bool value)
{
    mAutoIndent = value;
}

QColor EditorSettings::caretColor() const
{
    return mCaretColor;
}

void EditorSettings::setCaretColor(const QColor &caretColor)
{
    mCaretColor = caretColor;
}

bool EditorSettings::keepCaretX() const
{
    return mKeepCaretX;
}

void EditorSettings::setKeepCaretX(bool keepCaretX)
{
    mKeepCaretX = keepCaretX;
}

bool EditorSettings::halfPageScroll() const
{
    return mHalfPageScroll;
}

void EditorSettings::setHalfPageScroll(bool halfPageScroll)
{
    mHalfPageScroll = halfPageScroll;
}

bool EditorSettings::gutterFontOnlyMonospaced() const
{
    return mGutterFontOnlyMonospaced;
}

void EditorSettings::setGutterFontOnlyMonospaced(bool gutterFontOnlyMonospaced)
{
    mGutterFontOnlyMonospaced = gutterFontOnlyMonospaced;
}

int EditorSettings::gutterRightOffset() const
{
    return mGutterRightOffset;
}

void EditorSettings::setGutterRightOffset(int gutterRightOffset)
{
    mGutterRightOffset = gutterRightOffset;
}

int EditorSettings::copyWithFormatAs() const
{
    return mCopyWithFormatAs;
}

void EditorSettings::setCopyWithFormatAs(int copyWithFormatAs)
{
    mCopyWithFormatAs = copyWithFormatAs;
}

QString EditorSettings::colorScheme() const
{
    return mColorScheme;
}

void EditorSettings::setColorScheme(const QString &colorScheme)
{
    mColorScheme = colorScheme;
}

bool EditorSettings::removeSymbolPairs() const
{
    return mRemoveSymbolPairs;
}

void EditorSettings::setRemoveSymbolPairs(bool value)
{
    mRemoveSymbolPairs = value;
}

bool EditorSettings::syntaxCheckWhenLineChanged() const
{
    return mSyntaxCheckWhenLineChanged;
}

void EditorSettings::setSyntaxCheckWhenLineChanged(bool syntaxCheckWhenLineChanged)
{
    mSyntaxCheckWhenLineChanged = syntaxCheckWhenLineChanged;
}

bool EditorSettings::readOnlySytemHeader() const
{
    return mReadOnlySytemHeader;
}

void EditorSettings::setReadOnlySytemHeader(bool newReadOnlySytemHeader)
{
    mReadOnlySytemHeader = newReadOnlySytemHeader;
}

bool EditorSettings::defaultFileCpp() const
{
    return mDefaultFileCpp;
}

void EditorSettings::setDefaultFileCpp(bool newDefaultFileCpp)
{
    mDefaultFileCpp = newDefaultFileCpp;
}

bool EditorSettings::enableAutoSave() const
{
    return mEnableAutoSave;
}

void EditorSettings::setEnableAutoSave(bool newEnableAutoSave)
{
    mEnableAutoSave = newEnableAutoSave;
}

int EditorSettings::autoSaveInterval() const
{
    return mAutoSaveInterval;
}

void EditorSettings::setAutoSaveInterval(int newInterval)
{
    mAutoSaveInterval = newInterval;
}

AutoSaveStrategy EditorSettings::autoSaveStrategy() const
{
    return mAutoSaveStrategy;
}

void EditorSettings::setAutoSaveStrategy(AutoSaveStrategy newAutoSaveStrategy)
{
    mAutoSaveStrategy = newAutoSaveStrategy;
}

bool EditorSettings::enableAutolink() const
{
    return mEnableAutolink;
}

void EditorSettings::setEnableAutolink(bool newEnableAutolink)
{
    mEnableAutolink = newEnableAutolink;
}

const QColor &EditorSettings::rightEdgeLineColor() const
{
    return mRightEdgeLineColor;
}

void EditorSettings::setRightEdgeLineColor(const QColor &newRightMarginLineColor)
{
    mRightEdgeLineColor = newRightMarginLineColor;
}

bool EditorSettings::caretUseTextColor() const
{
    return mCaretUseTextColor;
}

void EditorSettings::setCaretUseTextColor(bool newUseIdentifierColor)
{
    mCaretUseTextColor = newUseIdentifierColor;
}

bool EditorSettings::rainbowParenthesis() const
{
    return mRainbowParenthesis;
}

void EditorSettings::setRainbowParenthesis(bool newRainbowParenthesis)
{
    mRainbowParenthesis = newRainbowParenthesis;
}

bool EditorSettings::showFunctionTips() const
{
    return mShowFunctionTips;
}

void EditorSettings::setShowFunctionTips(bool newShowFunctionTips)
{
    mShowFunctionTips = newShowFunctionTips;
}

bool EditorSettings::fillIndents() const
{
    return mFillIndents;
}

void EditorSettings::setFillIndents(bool newFillIndents)
{
    mFillIndents = newFillIndents;
}

int EditorSettings::mouseWheelScrollSpeed() const
{
    return mMouseWheelScrollSpeed;
}

void EditorSettings::setMouseWheelScrollSpeed(int newMouseWheelScrollSpeed)
{
    mMouseWheelScrollSpeed = newMouseWheelScrollSpeed;
}

bool EditorSettings::highlightMathingBraces() const
{
    return mHighlightMathingBraces;
}

void EditorSettings::setHighlightMathingBraces(bool newHighlightMathingBraces)
{
    mHighlightMathingBraces = newHighlightMathingBraces;
}

bool EditorSettings::enableLigaturesSupport() const
{
    return mEnableLigaturesSupport;
}

void EditorSettings::setEnableLigaturesSupport(bool newEnableLigaturesSupport)
{
    mEnableLigaturesSupport = newEnableLigaturesSupport;
}

QStringList EditorSettings::fontFamilies() const
{
    return mFontFamilies;
}

void EditorSettings::setFontFamilies(const QStringList &newFontFamilies)
{
    mFontFamilies = newFontFamilies;
}

QStringList EditorSettings::fontFamiliesWithControlFont() const
{
    return mFontFamilies + QStringList{"Red Panda Control"};
}

int EditorSettings::mouseSelectionScrollSpeed() const
{
    return mMouseSelectionScrollSpeed;
}

void EditorSettings::setMouseSelectionScrollSpeed(int newMouseSelectionScrollSpeed)
{
    mMouseSelectionScrollSpeed = newMouseSelectionScrollSpeed;
}

bool EditorSettings::autoDetectFileEncoding() const
{
    return mAutoDetectFileEncoding;
}

void EditorSettings::setAutoDetectFileEncoding(bool newAutoDetectFileEncoding)
{
    mAutoDetectFileEncoding = newAutoDetectFileEncoding;
}

bool EditorSettings::autoFormatWhenSaved() const
{
    return mAutoFormatWhenSaved;
}

void EditorSettings::setAutoFormatWhenSaved(bool newAutoFormatWhenSaved)
{
    mAutoFormatWhenSaved = newAutoFormatWhenSaved;
}

bool EditorSettings::parseTodos() const
{
    return mParseTodos;
}

void EditorSettings::setParseTodos(bool newParseTodos)
{
    mParseTodos = newParseTodos;
}

const QStringList &EditorSettings::customCTypeKeywords() const
{
    return mCustomCTypeKeywords;
}

void EditorSettings::setCustomCTypeKeywords(const QStringList &newCustomTypeKeywords)
{
    mCustomCTypeKeywords = newCustomTypeKeywords;
}

bool EditorSettings::enableCustomCTypeKeywords() const
{
    return mEnableCustomCTypeKeywords;
}

void EditorSettings::setEnableCustomCTypeKeywords(bool newEnableCustomCTypeKeywords)
{
    mEnableCustomCTypeKeywords = newEnableCustomCTypeKeywords;
}

bool EditorSettings::removeTrailingSpacesWhenSaved() const
{
    return mRemoveTrailingSpacesWhenSaved;
}

void EditorSettings::setRemoveTrailingSpacesWhenSaved(bool newRemoveTrailingSpacesWhenSaved)
{
    mRemoveTrailingSpacesWhenSaved = newRemoveTrailingSpacesWhenSaved;
}

double EditorSettings::lineSpacing() const
{
    return mLineSpacing;
}

void EditorSettings::setLineSpacing(double newLineSpacing)
{
    mLineSpacing = newLineSpacing;
}

bool EditorSettings::showLeadingSpaces() const
{
    return mShowLeadingSpaces;
}

void EditorSettings::setShowLeadingSpaces(bool newShowStartSpaces)
{
    mShowLeadingSpaces = newShowStartSpaces;
}

bool EditorSettings::enableEditTempBackup() const
{
    return mEnableEditTempBackup;
}

void EditorSettings::setEnableEditTempBackup(bool newEnableEditTempBackup)
{
    mEnableEditTempBackup = newEnableEditTempBackup;
}

int EditorSettings::tipsDelay() const
{
    return mTipsDelay;
}

void EditorSettings::setTipsDelay(int newTipsDelay)
{
    mTipsDelay = newTipsDelay;
}

bool EditorSettings::forceFixedFontWidth() const
{
    return mForceFixedFontWidth;
}

void EditorSettings::setForceFixedFontWidth(bool newForceFixedWidth)
{
    mForceFixedFontWidth = newForceFixedWidth;
}

bool EditorSettings::copyHTMLRecalcLineNumber() const
{
    return mCopyHTMLRecalcLineNumber;
}

void EditorSettings::setCopyHTMLRecalcLineNumber(bool newCopyHTMLRecalcLineNumber)
{
    mCopyHTMLRecalcLineNumber = newCopyHTMLRecalcLineNumber;
}

bool EditorSettings::rainbowIndents() const
{
    return mRainbowIndents;
}

void EditorSettings::setRainbowIndents(bool newFillIndentsUsingRainbowColor)
{
    mRainbowIndents = newFillIndentsUsingRainbowColor;
}

bool EditorSettings::rainbowIndentGuides() const
{
    return mRainbowIndentGuides;
}

void EditorSettings::setRainbowIndentGuides(bool newIndentLineUsingRainbowColor)
{
    mRainbowIndentGuides = newIndentLineUsingRainbowColor;
}

bool EditorSettings::copyHTMLWithLineNumber() const
{
    return mCopyHTMLWithLineNumber;
}

void EditorSettings::setCopyHTMLWithLineNumber(bool newCopyHTMLWithLineNumber)
{
    mCopyHTMLWithLineNumber = newCopyHTMLWithLineNumber;
}

bool EditorSettings::showTrailingSpaces() const
{
    return mShowTrailingSpaces;
}

void EditorSettings::setShowTrailingSpaces(bool newShowEndSpaces)
{
    mShowTrailingSpaces = newShowEndSpaces;
}

bool EditorSettings::showInnerSpaces() const
{
    return mShowInnerSpaces;
}

void EditorSettings::setShowInnerSpaces(bool newShowMiddleSpaces)
{
    mShowInnerSpaces = newShowMiddleSpaces;
}

bool EditorSettings::showLineBreaks() const
{
    return mShowLineBreaks;
}

void EditorSettings::setShowLineBreaks(bool newShowLineBreaks)
{
    mShowLineBreaks = newShowLineBreaks;
}

bool EditorSettings::highlightCurrentWord() const
{
    return mHighlightCurrentWord;
}

void EditorSettings::setHighlightCurrentWord(bool newHighlightCurrentWord)
{
    mHighlightCurrentWord = newHighlightCurrentWord;
}

bool EditorSettings::enableTooltips() const
{
    return mEnableTooltips;
}

void EditorSettings::setEnableTooltips(bool newEnableTooltips)
{
    mEnableTooltips = newEnableTooltips;
}

bool EditorSettings::enableDebugTooltips() const
{
    return mEnableDebugTooltips;
}

void EditorSettings::setEnableDebugTooltips(bool newEnableDebugTooltips)
{
    mEnableDebugTooltips = newEnableDebugTooltips;
}

bool EditorSettings::enableIdentifierToolTips() const
{
    return mEnableIdentifierToolTips;
}

void EditorSettings::setEnableIdentifierToolTips(bool newEnableIdentifierToolTips)
{
    mEnableIdentifierToolTips = newEnableIdentifierToolTips;
}

bool EditorSettings::enableHeaderToolTips() const
{
    return mEnableHeaderToolTips;
}

void EditorSettings::setEnableHeaderToolTips(bool newEnableHeaderToolTips)
{
    mEnableHeaderToolTips = newEnableHeaderToolTips;
}

bool EditorSettings::enableIssueToolTips() const
{
    return mEnableIssueToolTips;
}

void EditorSettings::setEnableIssueToolTips(bool newEnableIssueToolTips)
{
    mEnableIssueToolTips = newEnableIssueToolTips;
}

int EditorSettings::rightEdgeWidth() const
{
    return mRightEdgeWidth;
}

void EditorSettings::setRightEdgeWidth(int newRightMarginWidth)
{
    mRightEdgeWidth = newRightMarginWidth;
}

bool EditorSettings::showRightEdgeLine() const
{
    return mShowRightEdgeLine;
}

void EditorSettings::setShowRightEdgeLine(bool newShowRightMarginLine)
{
    mShowRightEdgeLine = newShowRightMarginLine;
}

AutoSaveTarget EditorSettings::autoSaveTarget() const
{
    return mAutoSaveTarget;
}

void EditorSettings::setAutoSaveTarget(AutoSaveTarget newAutoSaveTarget)
{
    mAutoSaveTarget = newAutoSaveTarget;
}

bool EditorSettings::autoLoadLastFiles() const
{
    return mAutoLoadLastFiles;
}

void EditorSettings::setAutoLoadLastFiles(bool newAutoLoadLastFiles)
{
    mAutoLoadLastFiles = newAutoLoadLastFiles;
}

bool EditorSettings::syntaxCheckWhenSave() const
{
    return mSyntaxCheckWhenSave;
}

void EditorSettings::setSyntaxCheckWhenSave(bool syntaxCheckWhenSave)
{
    mSyntaxCheckWhenSave = syntaxCheckWhenSave;
}

bool EditorSettings::syntaxCheck() const
{
    return mSyntaxCheck;
}

void EditorSettings::setSyntaxCheck(bool syntaxCheck)
{
    mSyntaxCheck = syntaxCheck;
}

bool EditorSettings::overwriteSymbols() const
{
    return mOverwriteSymbols;
}

void EditorSettings::setOverwriteSymbols(bool overwriteSymbols)
{
    mOverwriteSymbols = overwriteSymbols;
}

bool EditorSettings::completeGlobalInclude() const
{
    return mCompleteGlobalInclude;
}

void EditorSettings::setCompleteGlobalInclude(bool completeGlobalInclude)
{
    mCompleteGlobalInclude = completeGlobalInclude;
}

bool EditorSettings::completeDoubleQuote() const
{
    return mCompleteDoubleQuote;
}

void EditorSettings::setCompleteDoubleQuote(bool completeDoubleQuote)
{
    mCompleteDoubleQuote = completeDoubleQuote;
}

bool EditorSettings::completeSingleQuote() const
{
    return mCompleteSingleQuote;
}

void EditorSettings::setCompleteSingleQuote(bool completeSingleQuote)
{
    mCompleteSingleQuote = completeSingleQuote;
}

bool EditorSettings::completeComment() const
{
    return mCompleteComment;
}

void EditorSettings::setCompleteComment(bool completeComment)
{
    mCompleteComment = completeComment;
}

bool EditorSettings::completeBrace() const
{
    return mCompleteBrace;
}

void EditorSettings::setCompleteBrace(bool completeBrace)
{
    mCompleteBrace = completeBrace;
}

bool EditorSettings::completeBracket() const
{
    return mCompleteBracket;
}

void EditorSettings::setCompleteBracket(bool completeBracket)
{
    mCompleteBracket = completeBracket;
}

bool EditorSettings::completeParenthese() const
{
    return mCompleteParenthese;
}

void EditorSettings::setCompleteParenthese(bool completeParenthese)
{
    mCompleteParenthese = completeParenthese;
}

bool EditorSettings::completeSymbols() const
{
    return mCompleteSymbols;
}

void EditorSettings::setCompleteSymbols(bool completeSymbols)
{
    mCompleteSymbols = completeSymbols;
}

QString EditorSettings::copyHTMLColorScheme() const
{
    return mCopyHTMLColorScheme;
}

void EditorSettings::setCopyHTMLColorScheme(const QString &copyHTMLColorScheme)
{
    mCopyHTMLColorScheme = copyHTMLColorScheme;
}

bool EditorSettings::copyHTMLUseEditorColor() const
{
    return mCopyHTMLUseEditorColor;
}

void EditorSettings::setCopyHTMLUseEditorColor(bool copyHTMLUseEditorColor)
{
    mCopyHTMLUseEditorColor = copyHTMLUseEditorColor;
}

bool EditorSettings::copyHTMLUseBackground() const
{
    return mCopyHTMLUseBackground;
}

void EditorSettings::setCopyHTMLUseBackground(bool copyHTMLUseBackground)
{
    mCopyHTMLUseBackground = copyHTMLUseBackground;
}

QString EditorSettings::copyRTFColorScheme() const
{
    return mCopyRTFColorScheme;
}

void EditorSettings::setCopyRTFColorScheme(const QString &copyRTFColorScheme)
{
    mCopyRTFColorScheme = copyRTFColorScheme;
}

bool EditorSettings::copyRTFUseEditorColor() const
{
    return mCopyRTFUseEditorColor;
}

void EditorSettings::setCopyRTFUseEditorColor(bool copyRTFUseEditorColor)
{
    mCopyRTFUseEditorColor = copyRTFUseEditorColor;
}

bool EditorSettings::copyRTFUseBackground() const
{
    return mCopyRTFUseBackground;
}

void EditorSettings::setCopyRTFUseBackground(bool copyRTFUseBackground)
{
    mCopyRTFUseBackground = copyRTFUseBackground;
}

int EditorSettings::gutterLeftOffset() const
{
    return mGutterLeftOffset;
}

void EditorSettings::setGutterLeftOffset(int gutterLeftOffset)
{
    mGutterLeftOffset = gutterLeftOffset;
}

int EditorSettings::gutterFontSize() const
{
    return mGutterFontSize;
}

void EditorSettings::setGutterFontSize(int gutterFontSize)
{
    mGutterFontSize = gutterFontSize;
}

QString EditorSettings::gutterFontName() const
{
    return mGutterFontName;
}

void EditorSettings::setGutterFontName(const QString &gutterFontName)
{
    mGutterFontName = gutterFontName;
}

bool EditorSettings::gutterUseCustomFont() const
{
    return mGutterUseCustomFont;
}

void EditorSettings::setGutterUseCustomFont(bool gutterUseCustomFont)
{
    mGutterUseCustomFont = gutterUseCustomFont;
}

bool EditorSettings::gutterLineNumbersStartZero() const
{
    return mGutterLineNumbersStartZero;
}

void EditorSettings::setGutterLineNumbersStartZero(bool gutterLineNumbersStartZero)
{
    mGutterLineNumbersStartZero = gutterLineNumbersStartZero;
}

bool EditorSettings::gutterAddLeadingZero() const
{
    return mGutterAddLeadingZero;
}

void EditorSettings::setGutterAddLeadingZero(bool gutterAddLeadingZero)
{
    mGutterAddLeadingZero = gutterAddLeadingZero;
}

bool EditorSettings::gutterShowLineNumbers() const
{
    return mGutterShowLineNumbers;
}

void EditorSettings::setGutterShowLineNumbers(bool gutterShowLineNumbers)
{
    mGutterShowLineNumbers = gutterShowLineNumbers;
}

int EditorSettings::gutterDigitsCount() const
{
    return mGutterDigitsCount;
}

void EditorSettings::setGutterDigitsCount(int gutterDigitsCount)
{
    mGutterDigitsCount = gutterDigitsCount;
}

bool EditorSettings::gutterAutoSize() const
{
    return mGutterAutoSize;
}

void EditorSettings::setGutterAutoSize(bool gutterAutoSize)
{
    mGutterAutoSize = gutterAutoSize;
}

bool EditorSettings::gutterVisible() const
{
    return mGutterVisible;
}

void EditorSettings::setGutterVisible(bool gutterVisible)
{
    mGutterVisible = gutterVisible;
}

int EditorSettings::fontSize() const
{
    return mFontSize;
}

void EditorSettings::setFontSize(int fontSize)
{
    mFontSize = fontSize;
}

QString EditorSettings::fontName() const
{
    return mFontFamilies.length() > 0 ? mFontFamilies[0] : "";
}

bool EditorSettings::scrollPastEol() const
{
    return mScrollPastEol;
}

void EditorSettings::setScrollPastEol(bool scrollPastEol)
{
    mScrollPastEol = scrollPastEol;
}

bool EditorSettings::scrollPastEof() const
{
    return mScrollPastEof;
}

void EditorSettings::setScrollPastEof(bool scrollPastEof)
{
    mScrollPastEof = scrollPastEof;
}

bool EditorSettings::autoHideScrollbar() const
{
    return mAutoHideScrollbar;
}

void EditorSettings::setAutoHideScrollbar(bool autoHideScrollbar)
{
    mAutoHideScrollbar = autoHideScrollbar;
}

void EditorSettings::doSave()
{

    // indents
    saveValue("auto_indent", mAutoIndent);
    saveValue("tab_to_spaces", mTabToSpaces);
    saveValue("tab_width", mTabWidth);
    saveValue("show_indent_lines", mShowIndentLines);
    saveValue("fill_indents",mFillIndents);
    saveValue("rainbow_indent_guides",mRainbowIndentGuides);
    saveValue("rainbow_indents",mRainbowIndents);


    // caret
    saveValue("enhance_home_key",mEnhanceHomeKey);
    saveValue("enhance_end_key",mEnhanceEndKey);
    saveValue("keep_caret_x",mKeepCaretX);
    saveValue("caret_for_insert",static_cast<int>(mCaretForInsert));
    saveValue("caret_for_overwrite",static_cast<int>(mCaretForOverwrite));
    saveValue("caret_use_text_color",mCaretUseTextColor);
    saveValue("caret_color",mCaretColor);

    //highlight
    saveValue("highlight_matching_braces",mHighlightMathingBraces);
    saveValue("highlight_current_word",mHighlightCurrentWord);

    //scroll
    saveValue("auto_hide_scroll_bar", mAutoHideScrollbar);
    saveValue("scroll_past_eof", mScrollPastEof);
    saveValue("scroll_past_eol", mScrollPastEol);
    saveValue("half_page_scroll", mHalfPageScroll);
    saveValue("mouse_wheel_scroll_speed", mMouseWheelScrollSpeed);
    saveValue("mouse_drag_scroll_speed",mMouseSelectionScrollSpeed);

    //right edge
    saveValue("show_right_edge_line",mShowRightEdgeLine);
    saveValue("right_edge_width",mRightEdgeWidth);
    saveValue("right_edge_line_color",mRightEdgeLineColor);

    //Font
    //font
    saveValue("font_families", mFontFamilies);
    saveValue("font_size", mFontSize);
    saveValue("line_spacing",mLineSpacing);
    saveValue("enable_ligatures_support", mEnableLigaturesSupport);
    saveValue("force_fixed_font_width", mForceFixedFontWidth);

    saveValue("show_leading_spaces", mShowLeadingSpaces);
    saveValue("show_trailing_spaces", mShowTrailingSpaces);
    saveValue("show_inner_spaces", mShowInnerSpaces);
    saveValue("show_line_breaks", mShowLineBreaks);

    //gutter
    saveValue("gutter_visible", mGutterVisible);
    saveValue("gutter_auto_size", mGutterAutoSize);
    saveValue("gutter_left_offset",mGutterLeftOffset);
    saveValue("gutter_right_offset2",mGutterRightOffset);
    saveValue("gutter_digits_count", mGutterDigitsCount);
    saveValue("gutter_show_line_numbers",mGutterShowLineNumbers);
    saveValue("gutter_add_leading_zero",mGutterAddLeadingZero);
    saveValue("gutter_line_numbers_start_zero",mGutterLineNumbersStartZero);
    saveValue("gutter_use_custom_font",mGutterUseCustomFont);
    saveValue("gutter_font_name",mGutterFontName);
    saveValue("gutter_font_size",mGutterFontSize);
    saveValue("gutter_font_only_monospaced",mGutterFontOnlyMonospaced);

    //copy
    saveValue("copy_with_format_as",mCopyWithFormatAs);
    saveValue("copy_rtf_use_background",mCopyRTFUseBackground);
    saveValue("copy_rtf_use_editor_color_scheme",mCopyRTFUseEditorColor);
    saveValue("copy_rtf_color_scheme",mCopyRTFColorScheme);
    saveValue("copy_html_use_background",mCopyHTMLUseBackground);
    saveValue("copy_html_use_editor_color_scheme",mCopyHTMLUseEditorColor);
    saveValue("copy_html_with_line_number",mCopyHTMLWithLineNumber);
    saveValue("copy_html_recalc_line_number",mCopyHTMLRecalcLineNumber);

    saveValue("copy_html_color_scheme", mCopyHTMLColorScheme);

    //color scheme
    saveValue("color_scheme", mColorScheme);
    saveValue("rainbow_parenthesis",mRainbowParenthesis);

    //Symbol Completion
    saveValue("complete_symbols", mCompleteSymbols);
    saveValue("complete_parenthese", mCompleteParenthese);
    saveValue("complete_bracket", mCompleteBracket);
    saveValue("complete_brace", mCompleteBrace);
    saveValue("complete_comment", mCompleteComment);
    saveValue("complete_single_quote", mCompleteSingleQuote);
    saveValue("complete_double_quote", mCompleteDoubleQuote);
    saveValue("complete_global_include", mCompleteGlobalInclude);
    saveValue("overwrite_symbols", mOverwriteSymbols);
    saveValue("remove_symbol_pairs",mRemoveSymbolPairs);

    //Auto Syntax Check
    saveValue("check_syntax",mSyntaxCheck);
    saveValue("check_syntax_when_save",mSyntaxCheckWhenSave);
    saveValue("check_syntax_when_line_changed",mSyntaxCheckWhenLineChanged);

    //auto save
    saveValue("enable_edit_temp_backup",mEnableEditTempBackup);
    saveValue("enable_auto_save",mEnableAutoSave);
    saveValue("auto_save_interal",mAutoSaveInterval);
    saveValue("auto_save_target",mAutoSaveTarget);
    saveValue("auto_save_strategy",mAutoSaveStrategy);

    //auto link
    saveValue("enable_autolink",mEnableAutolink);

    //misc
    saveValue("default_encoding",mDefaultEncoding);
    saveValue("readonly_system_header",mReadOnlySytemHeader);
    saveValue("auto_load_last_files",mAutoLoadLastFiles);
    saveValue("default_file_cpp",mDefaultFileCpp);
    saveValue("auto_detect_file_encoding",mAutoDetectFileEncoding);
    saveValue("auto_format_when_saved", mAutoFormatWhenSaved);
    saveValue("remove_trailing_spaces_when_saved",mRemoveTrailingSpacesWhenSaved);
    saveValue("parse_todos",mParseTodos);

    saveValue("custom_c_type_keywords", mCustomCTypeKeywords);
    saveValue("enable_custom_c_type_keywords",mEnableCustomCTypeKeywords);

    //tooltips
    saveValue("enable_tooltips",mEnableTooltips);
    saveValue("enable_debug_tooltips",mEnableDebugTooltips);
    saveValue("enable_identifier_tooltips",mEnableIdentifierToolTips);
    saveValue("enable_header_tooltips",mEnableHeaderToolTips);
    saveValue("enable_issue_tooltips",mEnableIssueToolTips);
    saveValue("show_function_tips",mShowFunctionTips);
    saveValue("tips_delay",mTipsDelay);
}

void EditorSettings::doLoad()
{

    // indents
    mAutoIndent = boolValue("auto_indent", true);
    mTabToSpaces = boolValue("tab_to_spaces",false);
    mTabWidth = intValue("tab_width",4);
    mShowIndentLines = boolValue("show_indent_lines",true);
    mFillIndents = boolValue("fill_indents", false);
    mRainbowIndentGuides = boolValue("rainbow_indent_guides", true);
    mRainbowIndents = boolValue("rainbow_indents", true);

    // caret
    mEnhanceHomeKey = boolValue("enhance_home_key", true);
    mEnhanceEndKey = boolValue("enhance_end_key",true);
    mKeepCaretX = boolValue("keep_caret_x",true);
    mCaretForInsert = static_cast<QSynedit::EditCaretType>( intValue("caret_for_insert",static_cast<int>(QSynedit::EditCaretType::VerticalLine)));
    mCaretForOverwrite = static_cast<QSynedit::EditCaretType>( intValue("caret_for_overwrite",static_cast<int>(QSynedit::EditCaretType::Block)));
    mCaretUseTextColor = boolValue("caret_use_text_color",true);
    mCaretColor = colorValue("caret_color",Qt::yellow);

    //highlight
    mHighlightMathingBraces = boolValue("highlight_matching_braces",true);
    mHighlightCurrentWord = boolValue("highlight_current_word",true);

    //scroll
    mAutoHideScrollbar = boolValue("auto_hide_scroll_bar", false);
    mScrollPastEof = boolValue("scroll_past_eof", true);
    mScrollPastEol = boolValue("scroll_past_eol", false);
    mHalfPageScroll = boolValue("half_page_scroll",false);
    mMouseWheelScrollSpeed = intValue("mouse_wheel_scroll_speed", 3);
    mMouseSelectionScrollSpeed = intValue("mouse_drag_scroll_speed",10);


    //right edge
    mShowRightEdgeLine = boolValue("show_right_edge_line",false);
    mRightEdgeWidth = intValue("right_edge_width",80);
    mRightEdgeLineColor = colorValue("right_edge_line_color",Qt::yellow);

    //Editor font
    QStringList fontFamilies = stringListValue("font_families", QStringList());
    if (fontFamilies.empty()) {
        // backward compatibility: try old font settings
        QString fontName = stringValue("font_name", "");
        if (!fontName.isEmpty())
            fontFamilies.append(fontName);
        QString nonAsciiFontName = stringValue("non_ascii_font_name", "");
        if (!nonAsciiFontName.isEmpty())
            fontFamilies.append(nonAsciiFontName);
        mFontFamilies = fontFamilies.empty() ? defaultEditorFonts() : fontFamilies;
    } else {
        mFontFamilies = fontFamilies;
    }
    mFontSize = intValue("font_size",12);
    mLineSpacing = doubleValue("line_spacing",1.1);
    mForceFixedFontWidth = boolValue("force_fixed_font_width", isCjk());
    // if (mForceFixedFontWidth)
    //     mEnableLigaturesSupport = false;
    // else
    //     mEnableLigaturesSupport = boolValue("enable_ligatures_support", !isZhJa);
    mEnableLigaturesSupport = boolValue("enable_ligatures_support", false);


    mShowLeadingSpaces = boolValue("show_leading_spaces", false);
    mShowTrailingSpaces = boolValue("show_trailing_spaces", false);
    mShowInnerSpaces = boolValue("show_inner_spaces", false);
    mShowLineBreaks = boolValue("show_line_breaks", false);

    //gutter
    mGutterVisible = boolValue("gutter_visible",true);
    mGutterAutoSize = boolValue("gutter_auto_size",true);
    mGutterLeftOffset = intValue("gutter_left_offset",6);
    mGutterRightOffset = intValue("gutter_right_offset",-1);
    if (mGutterRightOffset>0)
        mGutterRightOffset = std::max(0, mGutterRightOffset-20);
    else
        mGutterRightOffset = intValue("gutter_right_offset2",4);
    mGutterDigitsCount = intValue("gutter_digits_count",1);
    mGutterShowLineNumbers = boolValue("gutter_show_line_numbers",true);
    mGutterAddLeadingZero = boolValue("gutter_add_leading_zero",false);
    mGutterLineNumbersStartZero = boolValue("gutter_line_numbers_start_zero",false);
    mGutterUseCustomFont = boolValue("gutter_use_custom_font",false);

    mGutterFontName = stringValue("gutter_font_name", defaultMonoFont());
    mGutterFontSize = intValue("gutter_font_size",12);
    mGutterFontOnlyMonospaced = boolValue("gutter_font_only_monospaced",true);

    //copy
#ifdef Q_OS_WIN
    mCopyWithFormatAs = intValue("copy_with_format_as",1); //html
#else
    mCopyWithFormatAs = intValue("copy_with_format_as",0); //none
#endif
    mCopyRTFUseBackground = boolValue("copy_rtf_use_background",false);
    mCopyRTFUseEditorColor = boolValue("copy_rtf_use_editor_color_scheme",false);
    mCopyRTFColorScheme = stringValue("copy_rtf_color_scheme","Intellij Classic");
    mCopyHTMLUseBackground = boolValue("copy_html_use_background",false);
    mCopyHTMLUseEditorColor = boolValue("copy_html_use_editor_color_scheme",false);
    mCopyHTMLWithLineNumber = boolValue("copy_html_with_line_number", false);
    mCopyHTMLRecalcLineNumber = boolValue("copy_html_recalc_line_number", true);

    mCopyHTMLColorScheme = stringValue("copy_html_color_scheme","Intellij Classic");

    //color
    mColorScheme = stringValue("color_scheme", "VS Code");
    mRainbowParenthesis = boolValue("rainbow_parenthesis", true);

    //Symbol Completion
    mCompleteSymbols = boolValue("complete_symbols",true);
    mCompleteParenthese = boolValue("complete_parenthese",true);
    mCompleteBracket = boolValue("complete_bracket",true);
    mCompleteBrace = boolValue("complete_brace",true);
    mCompleteComment = boolValue("complete_comment",true);
    mCompleteSingleQuote = boolValue("complete_single_quote",true);
    mCompleteDoubleQuote = boolValue("complete_double_quote",true);
    mCompleteGlobalInclude = boolValue("complete_global_include",true);
    mOverwriteSymbols = boolValue("overwrite_symbols",true);
    mRemoveSymbolPairs = boolValue("remove_symbol_pairs",true);

    //Auto Syntax Check
    mSyntaxCheck = boolValue("check_syntax",true);
    mSyntaxCheckWhenSave = boolValue("check_syntax_when_save",true);
    mSyntaxCheckWhenLineChanged = boolValue("check_syntax_when_line_changed",true);

    //auto save
    mEnableEditTempBackup = boolValue("enable_edit_temp_backup", false);
    mEnableAutoSave = boolValue("enable_auto_save",false);
    mAutoSaveInterval = intValue("auto_save_interal",10);
    mAutoSaveTarget = static_cast<enum AutoSaveTarget>(
                intValue("auto_save_target",AutoSaveTarget::astCurrentFile));
    mAutoSaveStrategy = static_cast<enum AutoSaveStrategy>(
                intValue("auto_save_strategy",AutoSaveStrategy::assOverwrite));

    //auto link
    mEnableAutolink = boolValue("enable_autolink",true);

    //misc
    mReadOnlySytemHeader = boolValue("readonly_system_header",true);
    mAutoLoadLastFiles = boolValue("auto_load_last_files",true);
    mDefaultFileCpp = boolValue("default_file_cpp",true);
    bool useUTF8ByDefault = boolValue("use_utf8_by_default",false);
    if (useUTF8ByDefault)
        mDefaultEncoding = ENCODING_UTF8;
    else
        mDefaultEncoding = value("default_encoding", ENCODING_UTF8).toByteArray();
    mAutoDetectFileEncoding = boolValue("auto_detect_file_encoding",true);
    mAutoFormatWhenSaved = boolValue("auto_format_when_saved", false);
    mRemoveTrailingSpacesWhenSaved = boolValue("remove_trailing_spaces_when_saved",false);
    mParseTodos = boolValue("parse_todos",true);

    mCustomCTypeKeywords = stringListValue("custom_c_type_keywords");
    mEnableCustomCTypeKeywords = boolValue("enable_custom_c_type_keywords",false);

    //tooltips
    mEnableTooltips = boolValue("enable_tooltips",true);
    mEnableDebugTooltips = boolValue("enable_debug_tooltips",true);
    mEnableIdentifierToolTips = boolValue("enable_identifier_tooltips",true);
    mEnableHeaderToolTips = boolValue("enable_header_tooltips", true);
    mEnableIssueToolTips = boolValue("enable_issue_tooltips", true);
    mShowFunctionTips = boolValue("show_function_tips",true);
    mTipsDelay = intValue("tips_delay",500);
}

QSynedit::EditCaretType EditorSettings::caretForOverwrite() const
{
    return mCaretForOverwrite;
}

void EditorSettings::setCaretForOverwrite(const QSynedit::EditCaretType &caretForOverwrite)
{
    mCaretForOverwrite = caretForOverwrite;
}

QSynedit::EditCaretType EditorSettings::caretForInsert() const
{
    return mCaretForInsert;
}

void EditorSettings::setCaretForInsert(const QSynedit::EditCaretType &caretForInsert)
{
    mCaretForInsert = caretForInsert;
}

bool EditorSettings::enhanceEndKey() const
{
    return mEnhanceEndKey;
}

void EditorSettings::setEnhanceEndKey(bool enhanceEndKey)
{
    mEnhanceEndKey = enhanceEndKey;
}

bool EditorSettings::enhanceHomeKey() const
{
    return mEnhanceHomeKey;
}

void EditorSettings::setEnhanceHomeKey(bool enhanceHomeKey)
{
    mEnhanceHomeKey = enhanceHomeKey;
}

bool EditorSettings::showIndentLines() const
{
    return mShowIndentLines;
}

void EditorSettings::setShowIndentLines(bool showIndentLines)
{
    mShowIndentLines = showIndentLines;
}

int EditorSettings::tabWidth() const
{
    return mTabWidth;
}

void EditorSettings::setTabWidth(int tabWidth)
{
    mTabWidth = tabWidth;
}

bool EditorSettings::tabToSpaces() const
{
    return mTabToSpaces;
}

void EditorSettings::setTabToSpaces(bool tabToSpaces)
{
    mTabToSpaces = tabToSpaces;
}
