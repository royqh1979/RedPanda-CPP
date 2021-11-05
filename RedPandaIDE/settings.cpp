#include "settings.h"
#include <QApplication>
#include <QTextCodec>
#include <algorithm>
#include "utils.h"
#include <QDir>
#include "systemconsts.h"
#include <QDebug>
#include <QMessageBox>
#include <QStandardPaths>

const char ValueToChar[28] = {'0', '1', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                              'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                              's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

Settings* pSettings;

Settings::Settings(const QString &filename):
    mFilename(filename),
    mSettings(filename,QSettings::IniFormat),
    mDirs(this),
    mEditor(this),
    mEnvironment(this),
    mCompilerSets(this),
    mExecutor(this),
    mDebugger(this),
    mCodeCompletion(this),
    mCodeFormatter(this),
    mHistory(this),
    mUI(this)
{
    load();
}

Settings::~Settings()
{
    mEditor.save();
}

void Settings::beginGroup(const QString &group)
{
    mSettings.beginGroup(group);
}

void Settings::endGroup()
{
    mSettings.endGroup();
}

void Settings::saveValue(const QString& group, const QString &key, const QVariant &value) {
    mSettings.beginGroup(group);
    auto act = finally([this] {
        this->mSettings.endGroup();
    });
    mSettings.setValue(key,value);
}

void Settings::saveValue(const QString &key, const QVariant &value)
{
    mSettings.setValue(key,value);
}

QVariant Settings::value(const QString &group, const QString &key, const QVariant &defaultValue)
{
    mSettings.beginGroup(group);
    auto act = finally([this] {
        this->mSettings.endGroup();
    });
    return mSettings.value(key,defaultValue);
}

QVariant Settings::value(const QString &key, const QVariant &defaultValue)
{
    return mSettings.value(key,defaultValue);
}

void Settings::load()
{

    mCompilerSets.loadSets();
    mEnvironment.load();
    mEditor.load();
    mExecutor.load();
    mDebugger.load();
    mHistory.load();
    mCodeCompletion.load();
    mCodeFormatter.load();
    mUI.load();
}

Settings::Dirs &Settings::dirs()
{
    return mDirs;
}

Settings::Editor &Settings::editor()
{
    return mEditor;
}

Settings::CompilerSets &Settings::compilerSets()
{
    return mCompilerSets;
}

Settings::Environment &Settings::environment()
{
    return mEnvironment;
}

Settings::Executor &Settings::executor()
{
    return mExecutor;
}

QString Settings::filename() const
{
    return mFilename;
}

Settings::CodeCompletion& Settings::codeCompletion()
{
    return mCodeCompletion;
}

Settings::CodeFormatter &Settings::codeFormatter()
{
    return mCodeFormatter;
}

Settings::UI &Settings::ui()
{
    return mUI;
}

Settings::History& Settings::history()
{
    return mHistory;
}

Settings::Debugger& Settings::debugger()
{
    return mDebugger;
}

Settings::Dirs::Dirs(Settings *settings):
    _Base(settings, SETTING_DIRS)
{
}

QString Settings::Dirs::app() const
{
    return QApplication::instance()->applicationDirPath();
}

QString Settings::Dirs::templateDir() const
{
    return includeTrailingPathDelimiter(app()) + "templates";
}

QString Settings::Dirs::projectDir() const
{
    if (isGreenEdition()) {
        return includeTrailingPathDelimiter(app()) + "projects";
    } else {
        return includeTrailingPathDelimiter(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0])
                         + "projects";
    }
}

QString Settings::Dirs::data(Settings::Dirs::DataType dataType) const
{
    using DataType = Settings::Dirs::DataType;
    QString dataDir = includeTrailingPathDelimiter(app())+"data";
    switch (dataType) {
    case DataType::None:
        return dataDir;
    case DataType::ColorSheme:
        return ":/colorschemes/colorschemes";
    }
    return "";
}

QString Settings::Dirs::config(Settings::Dirs::DataType dataType) const
{
    using DataType = Settings::Dirs::DataType;
    QFileInfo configFile(pSettings->filename());
    QString configDir = configFile.path();
    switch (dataType) {
    case DataType::None:
        return configDir;
    case DataType::ColorSheme:
        return includeTrailingPathDelimiter(configDir)+"scheme";
    }
    return "";
}

QString Settings::Dirs::executable() const
{
    QString s = QApplication::instance()->applicationFilePath();
    s.replace("/",QDir::separator());
    return s;
}

void Settings::Dirs::doSave()
{

}

void Settings::Dirs::doLoad()
{

}

Settings::_Base::_Base(Settings *settings, const QString &groupName):
    mSettings(settings),
    mGroup(groupName)
{

}

void Settings::_Base::beginGroup()
{
    mSettings->beginGroup(mGroup);
}

void Settings::_Base::endGroup()
{
    mSettings->endGroup();
}

void Settings::_Base::saveValue(const QString &key, const QVariant &value)
{
    mSettings->saveValue(key,value);
}

QVariant Settings::_Base::value(const QString &key, const QVariant &defaultValue)
{
    return mSettings->value(key,defaultValue);
}

bool Settings::_Base::boolValue(const QString &key, bool defaultValue)
{
    return value(key,defaultValue).toBool();
}

int Settings::_Base::intValue(const QString &key, int defaultValue)
{
    return value(key,defaultValue).toInt();
}

QStringList Settings::_Base::stringListValue(const QString &key, const QStringList &defaultValue)
{
    return value(key,defaultValue).toStringList();
}

QColor Settings::_Base::colorValue(const QString &key, const QColor& defaultValue)
{
    return value(key,defaultValue).value<QColor>();
}

QString Settings::_Base::stringValue(const QString &key, const QString& defaultValue)
{
    return value(key,defaultValue).toString();
}

void Settings::_Base::save()
{
    beginGroup();
    doSave();
    endGroup();
}

void Settings::_Base::load()
{
    beginGroup();
    doLoad();
    endGroup();
}

Settings::Editor::Editor(Settings *settings): _Base(settings, SETTING_EDITOR)
{

}

QByteArray Settings::Editor::defaultEncoding()
{
    return mDefaultEncoding;
}

void Settings::Editor::setDefaultEncoding(const QByteArray &value)
{
    mDefaultEncoding = value;
}

bool Settings::Editor::autoIndent()
{
    return mAutoIndent;
}

void Settings::Editor::setAutoIndent(bool value)
{
    mAutoIndent = value;
}

QColor Settings::Editor::caretColor() const
{
    return mCaretColor;
}

void Settings::Editor::setCaretColor(const QColor &caretColor)
{
    mCaretColor = caretColor;
}

bool Settings::Editor::keepCaretX() const
{
    return mKeepCaretX;
}

void Settings::Editor::setKeepCaretX(bool keepCaretX)
{
    mKeepCaretX = keepCaretX;
}

bool Settings::Editor::halfPageScroll() const
{
    return mHalfPageScroll;
}

void Settings::Editor::setHalfPageScroll(bool halfPageScroll)
{
    mHalfPageScroll = halfPageScroll;
}

bool Settings::Editor::gutterFontOnlyMonospaced() const
{
    return mGutterFontOnlyMonospaced;
}

void Settings::Editor::setGutterFontOnlyMonospaced(bool gutterFontOnlyMonospaced)
{
    mGutterFontOnlyMonospaced = gutterFontOnlyMonospaced;
}

int Settings::Editor::gutterRightOffset() const
{
    return mGutterRightOffset;
}

void Settings::Editor::setGutterRightOffset(int gutterRightOffset)
{
    mGutterRightOffset = gutterRightOffset;
}

int Settings::Editor::copyWithFormatAs() const
{
    return mCopyWithFormatAs;
}

void Settings::Editor::setCopyWithFormatAs(int copyWithFormatAs)
{
    mCopyWithFormatAs = copyWithFormatAs;
}

QString Settings::Editor::colorScheme() const
{
    return mColorScheme;
}

void Settings::Editor::setColorScheme(const QString &colorScheme)
{
    mColorScheme = colorScheme;
}

bool Settings::Editor::removeSymbolPairs() const
{
    return mRemoveSymbolPairs;
}

void Settings::Editor::setRemoveSymbolPairs(bool value)
{
    mRemoveSymbolPairs = value;
}

bool Settings::Editor::syntaxCheckWhenLineChanged() const
{
    return mSyntaxCheckWhenLineChanged;
}

void Settings::Editor::setSyntaxCheckWhenLineChanged(bool syntaxCheckWhenLineChanged)
{
    mSyntaxCheckWhenLineChanged = syntaxCheckWhenLineChanged;
}

bool Settings::Editor::readOnlySytemHeader() const
{
    return mReadOnlySytemHeader;
}

void Settings::Editor::setReadOnlySytemHeader(bool newReadOnlySytemHeader)
{
    mReadOnlySytemHeader = newReadOnlySytemHeader;
}

bool Settings::Editor::defaultFileCpp() const
{
    return mDefaultFileCpp;
}

void Settings::Editor::setDefaultFileCpp(bool newDefaultFileCpp)
{
    mDefaultFileCpp = newDefaultFileCpp;
}

bool Settings::Editor::enableAutoSave() const
{
    return mEnableAutoSave;
}

void Settings::Editor::setEnableAutoSave(bool newEnableAutoSave)
{
    mEnableAutoSave = newEnableAutoSave;
}

int Settings::Editor::autoSaveInterval() const
{
    return mAutoSaveInterval;
}

void Settings::Editor::setAutoSaveInterval(int newInterval)
{
    mAutoSaveInterval = newInterval;
}

AutoSaveStrategy Settings::Editor::autoSaveStrategy() const
{
    return mAutoSaveStrategy;
}

void Settings::Editor::setAutoSaveStrategy(AutoSaveStrategy newAutoSaveStrategy)
{
    mAutoSaveStrategy = newAutoSaveStrategy;
}

bool Settings::Editor::enableAutolink() const
{
    return mEnableAutolink;
}

void Settings::Editor::setEnableAutolink(bool newEnableAutolink)
{
    mEnableAutolink = newEnableAutolink;
}

const QColor &Settings::Editor::rightEdgeLineColor() const
{
    return mRightEdgeLineColor;
}

void Settings::Editor::setRightEdgeLineColor(const QColor &newRightMarginLineColor)
{
    mRightEdgeLineColor = newRightMarginLineColor;
}

bool Settings::Editor::caretUseTextColor() const
{
    return mCaretUseTextColor;
}

void Settings::Editor::setCaretUseTextColor(bool newUseIdentifierColor)
{
    mCaretUseTextColor = newUseIdentifierColor;
}

bool Settings::Editor::rainbowParenthesis() const
{
    return mRainbowParenthesis;
}

void Settings::Editor::setRainbowParenthesis(bool newRainbowParenthesis)
{
    mRainbowParenthesis = newRainbowParenthesis;
}

bool Settings::Editor::showFunctionTips() const
{
    return mShowFunctionTips;
}

void Settings::Editor::setShowFunctionTips(bool newShowFunctionTips)
{
    mShowFunctionTips = newShowFunctionTips;
}

bool Settings::Editor::fillIndents() const
{
    return mfillIndents;
}

void Settings::Editor::setFillIndents(bool newFillIndents)
{
    mfillIndents = newFillIndents;
}

int Settings::Editor::mouseWheelScrollSpeed() const
{
    return mMouseWheelScrollSpeed;
}

void Settings::Editor::setMouseWheelScrollSpeed(int newMouseWheelScrollSpeed)
{
    mMouseWheelScrollSpeed = newMouseWheelScrollSpeed;
}

bool Settings::Editor::useUTF8ByDefault() const
{
    return mUseUTF8ByDefault;
}

void Settings::Editor::setUseUTF8ByDefault(bool newUseUTF8ByDefault)
{
    mUseUTF8ByDefault = newUseUTF8ByDefault;
}

bool Settings::Editor::enableTooltips() const
{
    return mEnableTooltips;
}

void Settings::Editor::setEnableTooltips(bool newEnableTooltips)
{
    mEnableTooltips = newEnableTooltips;
}

bool Settings::Editor::enableDebugTooltips() const
{
    return mEnableDebugTooltips;
}

void Settings::Editor::setEnableDebugTooltips(bool newEnableDebugTooltips)
{
    mEnableDebugTooltips = newEnableDebugTooltips;
}

bool Settings::Editor::enableIdentifierToolTips() const
{
    return mEnableIdentifierToolTips;
}

void Settings::Editor::setEnableIdentifierToolTips(bool newEnableIdentifierToolTips)
{
    mEnableIdentifierToolTips = newEnableIdentifierToolTips;
}

bool Settings::Editor::enableHeaderToolTips() const
{
    return mEnableHeaderToolTips;
}

void Settings::Editor::setEnableHeaderToolTips(bool newEnableHeaderToolTips)
{
    mEnableHeaderToolTips = newEnableHeaderToolTips;
}

bool Settings::Editor::enableIssueToolTips() const
{
    return mEnableIssueToolTips;
}

void Settings::Editor::setEnableIssueToolTips(bool newEnableIssueToolTips)
{
    mEnableIssueToolTips = newEnableIssueToolTips;
}

int Settings::Editor::rightEdgeWidth() const
{
    return mRightEdgeWidth;
}

void Settings::Editor::setRightEdgeWidth(int newRightMarginWidth)
{
    mRightEdgeWidth = newRightMarginWidth;
}

bool Settings::Editor::showRightEdgeLine() const
{
    return mShowRightEdgeLine;
}

void Settings::Editor::setShowRightEdgeLine(bool newShowRightMarginLine)
{
    mShowRightEdgeLine = newShowRightMarginLine;
}

AutoSaveTarget Settings::Editor::autoSaveTarget() const
{
    return mAutoSaveTarget;
}

void Settings::Editor::setAutoSaveTarget(AutoSaveTarget newAutoSaveTarget)
{
    mAutoSaveTarget = newAutoSaveTarget;
}

bool Settings::Editor::autoLoadLastFiles() const
{
    return mAutoLoadLastFiles;
}

void Settings::Editor::setAutoLoadLastFiles(bool newAutoLoadLastFiles)
{
    mAutoLoadLastFiles = newAutoLoadLastFiles;
}

bool Settings::Editor::syntaxCheckWhenSave() const
{
    return mSyntaxCheckWhenSave;
}

void Settings::Editor::setSyntaxCheckWhenSave(bool syntaxCheckWhenSave)
{
    mSyntaxCheckWhenSave = syntaxCheckWhenSave;
}

bool Settings::Editor::syntaxCheck() const
{
    return mSyntaxCheck;
}

void Settings::Editor::setSyntaxCheck(bool syntaxCheck)
{
    mSyntaxCheck = syntaxCheck;
}

bool Settings::Editor::overwriteSymbols() const
{
    return mOverwriteSymbols;
}

void Settings::Editor::setOverwriteSymbols(bool overwriteSymbols)
{
    mOverwriteSymbols = overwriteSymbols;
}

bool Settings::Editor::completeGlobalInclude() const
{
    return mCompleteGlobalInclude;
}

void Settings::Editor::setCompleteGlobalInclude(bool completeGlobalInclude)
{
    mCompleteGlobalInclude = completeGlobalInclude;
}

bool Settings::Editor::completeDoubleQuote() const
{
    return mCompleteDoubleQuote;
}

void Settings::Editor::setCompleteDoubleQuote(bool completeDoubleQuote)
{
    mCompleteDoubleQuote = completeDoubleQuote;
}

bool Settings::Editor::completeSingleQuote() const
{
    return mCompleteSingleQuote;
}

void Settings::Editor::setCompleteSingleQuote(bool completeSingleQuote)
{
    mCompleteSingleQuote = completeSingleQuote;
}

bool Settings::Editor::completeComment() const
{
    return mCompleteComment;
}

void Settings::Editor::setCompleteComment(bool completeComment)
{
    mCompleteComment = completeComment;
}

bool Settings::Editor::completeBrace() const
{
    return mCompleteBrace;
}

void Settings::Editor::setCompleteBrace(bool completeBrace)
{
    mCompleteBrace = completeBrace;
}

bool Settings::Editor::completeBracket() const
{
    return mCompleteBracket;
}

void Settings::Editor::setCompleteBracket(bool completeBracket)
{
    mCompleteBracket = completeBracket;
}

bool Settings::Editor::completeParenthese() const
{
    return mCompleteParenthese;
}

void Settings::Editor::setCompleteParenthese(bool completeParenthese)
{
    mCompleteParenthese = completeParenthese;
}

bool Settings::Editor::completeSymbols() const
{
    return mCompleteSymbols;
}

void Settings::Editor::setCompleteSymbols(bool completeSymbols)
{
    mCompleteSymbols = completeSymbols;
}

QString Settings::Editor::copyHTMLColorScheme() const
{
    return mCopyHTMLColorScheme;
}

void Settings::Editor::setCopyHTMLColorScheme(const QString &copyHTMLColorScheme)
{
    mCopyHTMLColorScheme = copyHTMLColorScheme;
}

bool Settings::Editor::copyHTMLUseEditorColor() const
{
    return mCopyHTMLUseEditorColor;
}

void Settings::Editor::setCopyHTMLUseEditorColor(bool copyHTMLUseEditorColor)
{
    mCopyHTMLUseEditorColor = copyHTMLUseEditorColor;
}

bool Settings::Editor::copyHTMLUseBackground() const
{
    return mCopyHTMLUseBackground;
}

void Settings::Editor::setCopyHTMLUseBackground(bool copyHTMLUseBackground)
{
    mCopyHTMLUseBackground = copyHTMLUseBackground;
}

QString Settings::Editor::copyRTFColorScheme() const
{
    return mCopyRTFColorScheme;
}

void Settings::Editor::setCopyRTFColorScheme(const QString &copyRTFColorScheme)
{
    mCopyRTFColorScheme = copyRTFColorScheme;
}

bool Settings::Editor::copyRTFUseEditorColor() const
{
    return mCopyRTFUseEditorColor;
}

void Settings::Editor::setCopyRTFUseEditorColor(bool copyRTFUseEditorColor)
{
    mCopyRTFUseEditorColor = copyRTFUseEditorColor;
}

bool Settings::Editor::copyRTFUseBackground() const
{
    return mCopyRTFUseBackground;
}

void Settings::Editor::setCopyRTFUseBackground(bool copyRTFUseBackground)
{
    mCopyRTFUseBackground = copyRTFUseBackground;
}

int Settings::Editor::copyLineLimits() const
{
    return mCopyLineLimits;
}

void Settings::Editor::setCopyLineLimits(int copyLineLimits)
{
    mCopyLineLimits = copyLineLimits;
}

int Settings::Editor::copyCharLimits() const
{
    return mCopyCharLimits;
}

void Settings::Editor::setCopyCharLimits(int copyCharLimits)
{
    mCopyCharLimits = copyCharLimits;
}

bool Settings::Editor::copySizeLimit() const
{
    return mCopySizeLimit;
}

void Settings::Editor::setCopySizeLimit(bool copyLimit)
{
    mCopySizeLimit = copyLimit;
}

int Settings::Editor::gutterLeftOffset() const
{
    return mGutterLeftOffset;
}

void Settings::Editor::setGutterLeftOffset(int gutterLeftOffset)
{
    mGutterLeftOffset = gutterLeftOffset;
}

int Settings::Editor::gutterFontSize() const
{
    return mGutterFontSize;
}

void Settings::Editor::setGutterFontSize(int gutterFontSize)
{
    mGutterFontSize = gutterFontSize;
}

QString Settings::Editor::gutterFontName() const
{
    return mGutterFontName;
}

void Settings::Editor::setGutterFontName(const QString &gutterFontName)
{
    mGutterFontName = gutterFontName;
}

bool Settings::Editor::gutterUseCustomFont() const
{
    return mGutterUseCustomFont;
}

void Settings::Editor::setGutterUseCustomFont(bool gutterUseCustomFont)
{
    mGutterUseCustomFont = gutterUseCustomFont;
}

bool Settings::Editor::gutterLineNumbersStartZero() const
{
    return mGutterLineNumbersStartZero;
}

void Settings::Editor::setGutterLineNumbersStartZero(bool gutterLineNumbersStartZero)
{
    mGutterLineNumbersStartZero = gutterLineNumbersStartZero;
}

bool Settings::Editor::gutterAddLeadingZero() const
{
    return mGutterAddLeadingZero;
}

void Settings::Editor::setGutterAddLeadingZero(bool gutterAddLeadingZero)
{
    mGutterAddLeadingZero = gutterAddLeadingZero;
}

bool Settings::Editor::gutterShowLineNumbers() const
{
    return mGutterShowLineNumbers;
}

void Settings::Editor::setGutterShowLineNumbers(bool gutterShowLineNumbers)
{
    mGutterShowLineNumbers = gutterShowLineNumbers;
}

int Settings::Editor::gutterDigitsCount() const
{
    return mGutterDigitsCount;
}

void Settings::Editor::setGutterDigitsCount(int gutterDigitsCount)
{
    mGutterDigitsCount = gutterDigitsCount;
}

bool Settings::Editor::gutterAutoSize() const
{
    return mGutterAutoSize;
}

void Settings::Editor::setGutterAutoSize(bool gutterAutoSize)
{
    mGutterAutoSize = gutterAutoSize;
}

bool Settings::Editor::gutterVisible() const
{
    return mGutterVisible;
}

void Settings::Editor::setGutterVisible(bool gutterVisible)
{
    mGutterVisible = gutterVisible;
}

bool Settings::Editor::fontOnlyMonospaced() const
{
    return mFontOnlyMonospaced;
}

void Settings::Editor::setFontOnlyMonospaced(bool fontOnlyMonospaced)
{
    mFontOnlyMonospaced = fontOnlyMonospaced;
}

int Settings::Editor::fontSize() const
{
    return mFontSize;
}

void Settings::Editor::setFontSize(int fontSize)
{
    mFontSize = fontSize;
}

QString Settings::Editor::fontName() const
{
    return mFontName;
}

void Settings::Editor::setFontName(const QString &fontName)
{
    mFontName = fontName;
}

bool Settings::Editor::scrollByOneLess() const
{
    return mScrollByOneLess;
}

void Settings::Editor::setScrollByOneLess(bool scrollByOneLess)
{
    mScrollByOneLess = scrollByOneLess;
}

bool Settings::Editor::scrollPastEol() const
{
    return mScrollPastEol;
}

void Settings::Editor::setScrollPastEol(bool scrollPastEol)
{
    mScrollPastEol = scrollPastEol;
}

bool Settings::Editor::scrollPastEof() const
{
    return mScrollPastEof;
}

void Settings::Editor::setScrollPastEof(bool scrollPastEof)
{
    mScrollPastEof = scrollPastEof;
}

bool Settings::Editor::autoHideScrollbar() const
{
    return mAutoHideScrollbar;
}

void Settings::Editor::setAutoHideScrollbar(bool autoHideScrollbar)
{
    mAutoHideScrollbar = autoHideScrollbar;
}

void Settings::Editor::doSave()
{

    // indents
    saveValue("auto_indent", mAutoIndent);
    saveValue("tab_to_spaces", mTabToSpaces);
    saveValue("tab_width", mTabWidth);
    saveValue("show_indent_lines", mShowIndentLines);
    saveValue("indent_line_color",mIndentLineColor);
    saveValue("fill_indents",mfillIndents);
    // caret
    saveValue("enhance_home_key",mEnhanceHomeKey);
    saveValue("enhance_end_key",mEnhanceEndKey);
    saveValue("keep_caret_x",mKeepCaretX);
    saveValue("caret_for_insert",static_cast<int>(mCaretForInsert));
    saveValue("caret_for_overwrite",static_cast<int>(mCaretForOverwrite));
    saveValue("caret_use_text_color",mCaretUseTextColor);
    saveValue("caret_color",mCaretColor);

    //scroll
    saveValue("auto_hide_scroll_bar", mAutoHideScrollbar);
    saveValue("scroll_past_eof", mScrollPastEof);
    saveValue("scroll_past_eol", mScrollPastEol);
    saveValue("scroll_by_one_less", mScrollByOneLess);
    saveValue("half_page_scroll", mHalfPageScroll);
    saveValue("mouse_wheel_scroll_speed", mMouseWheelScrollSpeed);

    //right edge
    saveValue("show_right_edge_line",mShowRightEdgeLine);
    saveValue("right_edge_width",mRightEdgeWidth);
    saveValue("right_edge_line_color",mRightEdgeLineColor);

    //Font
    //font
    saveValue("font_name",mFontName);
    saveValue("font_size", mFontSize);
    saveValue("font_only_monospaced",mFontOnlyMonospaced);

    //gutter
    saveValue("gutter_visible", mGutterVisible);
    saveValue("gutter_auto_size", mGutterAutoSize);
    saveValue("gutter_digits_count", mGutterDigitsCount);
    saveValue("gutter_show_line_numbers",mGutterShowLineNumbers);
    saveValue("gutter_add_leading_zero",mGutterAddLeadingZero);
    saveValue("gutter_line_numbers_start_zero",mGutterLineNumbersStartZero);
    saveValue("gutter_use_custom_font",mGutterUseCustomFont);
    saveValue("gutter_font_name",mGutterFontName);
    saveValue("gutter_font_size",mGutterFontSize);
    saveValue("gutter_font_only_monospaced",mGutterFontOnlyMonospaced);

    //copy
    saveValue("copy_limit",mCopySizeLimit);
    saveValue("copy_char_limits",mCopyCharLimits);
    saveValue("copy_line_limits",mCopyLineLimits);
    saveValue("copy_with_format_as",mCopyWithFormatAs);
    saveValue("copy_rtf_use_background",mCopyRTFUseBackground);
    saveValue("copy_rtf_use_editor_color_scheme",mCopyRTFUseEditorColor);
    saveValue("copy_rtf_color_scheme",mCopyRTFColorScheme);
    saveValue("copy_html_use_background",mCopyHTMLUseBackground);
    saveValue("copy_html_use_editor_color_scheme",mCopyHTMLUseEditorColor);
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
    saveValue("use_utf8_by_default",mUseUTF8ByDefault);


    //tooltips
    saveValue("enable_tooltips",mEnableTooltips);
    saveValue("enable_debug_tooltips",mEnableDebugTooltips);
    saveValue("enable_identifier_tooltips",mEnableIdentifierToolTips);
    saveValue("enable_header_tooltips",mEnableHeaderToolTips);
    saveValue("enable_issue_tooltips",mEnableIssueToolTips);
    saveValue("show_function_tips",mShowFunctionTips);
}

void Settings::Editor::doLoad()
{

    // indents
    mAutoIndent = boolValue("auto_indent", true);
    mTabToSpaces = boolValue("tab_to_spaces",false);
    mTabWidth = intValue("tab_width",4);
    mShowIndentLines = boolValue("show_indent_lines",true);
    mIndentLineColor = colorValue("indent_line_color",QColorConstants::Svg::silver);
    mfillIndents = boolValue("fill_indents", false);
    // caret
    mEnhanceHomeKey = boolValue("enhance_home_key", true);
    mEnhanceEndKey = boolValue("enhance_end_key",true);
    mKeepCaretX = boolValue("keep_caret_x",true);
    mCaretForInsert = static_cast<SynEditCaretType>( intValue("caret_for_insert",static_cast<int>(SynEditCaretType::ctVerticalLine)));
    mCaretForOverwrite = static_cast<SynEditCaretType>( intValue("caret_for_overwrite",static_cast<int>(SynEditCaretType::ctBlock)));
    mCaretUseTextColor = boolValue("caret_use_text_color",true);
    mCaretColor = colorValue("caret_color",QColorConstants::Svg::yellow);

    //scroll
    mAutoHideScrollbar = boolValue("auto_hide_scroll_bar", false);
    mScrollPastEof = boolValue("scroll_past_eof", true);
    mScrollPastEol = boolValue("scroll_past_eol", true);
    mScrollByOneLess = boolValue("scroll_by_one_less", false);
    mHalfPageScroll = boolValue("half_page_scroll",false);
    mMouseWheelScrollSpeed = intValue("mouse_wheel_scroll_speed", 3);


    //right edge
    mShowRightEdgeLine = boolValue("show_right_edge_line",false);
    mRightEdgeWidth = intValue("right_edge_width",80);
    mRightEdgeLineColor = colorValue("right_edge_line_color",QColorConstants::Svg::yellow);

    //Font
    //font
    mFontName = stringValue("font_name","consolas");
    mFontSize = intValue("font_size",14);
    mFontOnlyMonospaced = boolValue("font_only_monospaced",true);

    //gutter
    mGutterVisible = boolValue("gutter_visible",true);
    mGutterAutoSize = boolValue("gutter_auto_size",true);
    mGutterLeftOffset = intValue("gutter_left_offset",28);
    mGutterRightOffset = intValue("gutter_right_offset",24);
    mGutterDigitsCount = intValue("gutter_digits_count",1);
    mGutterShowLineNumbers = boolValue("gutter_show_line_numbers",true);
    mGutterAddLeadingZero = boolValue("gutter_add_leading_zero",true);
    mGutterLineNumbersStartZero = boolValue("gutter_line_numbers_start_zero",false);
    mGutterUseCustomFont = boolValue("gutter_use_custom_font",false);
    mGutterFontName = stringValue("gutter_font_name","consolas");
    mGutterFontSize = intValue("gutter_font_size",QGuiApplication::font().pointSize());
    mGutterFontOnlyMonospaced = boolValue("gutter_font_only_monospaced",true);

    //copy
    mCopySizeLimit = boolValue("copy_limit",true);
    mCopyCharLimits = intValue("copy_char_limits",100);
    mCopyLineLimits = intValue("copy_line_limits",100000);
    mCopyWithFormatAs = intValue("copy_with_format_as",1);
    mCopyRTFUseBackground = boolValue("copy_rtf_use_background",false);
    mCopyRTFUseEditorColor = boolValue("copy_rtf_use_editor_color_scheme",false);
    mCopyRTFColorScheme = stringValue("copy_rtf_color_scheme","Intellij Classic");
    mCopyHTMLUseBackground = boolValue("copy_html_use_background",false);
    mCopyHTMLUseEditorColor = boolValue("copy_html_use_editor_color_scheme",false);
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
    mEnableAutoSave = boolValue("enable_auto_save",false);
    mAutoSaveInterval = intValue("auto_save_interal",10);
    mAutoSaveTarget = static_cast<enum AutoSaveTarget>(
                intValue("auto_save_target",AutoSaveTarget::astCurrentFile));
    mAutoSaveStrategy = static_cast<enum AutoSaveStrategy>(
                intValue("auto_save_strategy",AutoSaveStrategy::assOverwrite));

    //auto link
    mEnableAutolink = boolValue("enable_autolink",true);

    //misc
    mDefaultEncoding = value("default_encoding", ENCODING_SYSTEM_DEFAULT).toByteArray();
    mReadOnlySytemHeader = boolValue("readonly_system_header",true);
    mAutoLoadLastFiles = boolValue("auto_load_last_files",true);
    mDefaultFileCpp = boolValue("default_file_cpp",true);
    mUseUTF8ByDefault = boolValue("use_utf8_by_default",false);

    //tooltips
    mEnableTooltips = boolValue("enable_tooltips",true);
    mEnableDebugTooltips = boolValue("enable_debug_tooltips",true);
    mEnableIdentifierToolTips = boolValue("enable_identifier_tooltips",true);
    mEnableHeaderToolTips = boolValue("enable_header_tooltips", true);
    mEnableIssueToolTips = boolValue("enable_issue_tooltips", true);
    mShowFunctionTips = boolValue("show_function_tips",true);
}

SynEditCaretType Settings::Editor::caretForOverwrite() const
{
    return mCaretForOverwrite;
}

void Settings::Editor::setCaretForOverwrite(const SynEditCaretType &caretForOverwrite)
{
    mCaretForOverwrite = caretForOverwrite;
}

SynEditCaretType Settings::Editor::caretForInsert() const
{
    return mCaretForInsert;
}

void Settings::Editor::setCaretForInsert(const SynEditCaretType &caretForInsert)
{
    mCaretForInsert = caretForInsert;
}

bool Settings::Editor::enhanceEndKey() const
{
    return mEnhanceEndKey;
}

void Settings::Editor::setEnhanceEndKey(bool enhanceEndKey)
{
    mEnhanceEndKey = enhanceEndKey;
}

bool Settings::Editor::enhanceHomeKey() const
{
    return mEnhanceHomeKey;
}

void Settings::Editor::setEnhanceHomeKey(bool enhanceHomeKey)
{
    mEnhanceHomeKey = enhanceHomeKey;
}

QColor Settings::Editor::indentLineColor() const
{
    return mIndentLineColor;
}

void Settings::Editor::setIndentLineColor(const QColor &indentLineColor)
{
    mIndentLineColor = indentLineColor;
}

bool Settings::Editor::showIndentLines() const
{
    return mShowIndentLines;
}

void Settings::Editor::setShowIndentLines(bool showIndentLines)
{
    mShowIndentLines = showIndentLines;
}

int Settings::Editor::tabWidth() const
{
    return mTabWidth;
}

void Settings::Editor::setTabWidth(int tabWidth)
{
    mTabWidth = tabWidth;
}

bool Settings::Editor::tabToSpaces() const
{
    return mTabToSpaces;
}

void Settings::Editor::setTabToSpaces(bool tabToSpaces)
{
    mTabToSpaces = tabToSpaces;
}

Settings::CompilerSet::CompilerSet(const QString& compilerFolder):
    mAutoAddCharsetParams(true),
    mStaticLink(true)
{
    if (!compilerFolder.isEmpty()) {
        setProperties(compilerFolder+"/bin");

        //manually set the directories
        setDirectories(compilerFolder+"/bin");

        setExecutables();

        setUserInput();

        setDefines();
    }
    setOptions();
}

Settings::CompilerSet::CompilerSet(const Settings::CompilerSet &set):
    mCCompiler(set.mCCompiler),
    mCppCompiler(set.mCppCompiler),
    mMake(set.mMake),
    mDebugger(set.mDebugger),
    mProfiler(set.mProfiler),
    mResourceCompiler(set.mResourceCompiler),
    mBinDirs(set.mBinDirs),
    mCIncludeDirs(set.mCIncludeDirs),
    mCppIncludeDirs(set.mCppIncludeDirs),
    mLibDirs(set.mLibDirs),
    mDumpMachine(set.mDumpMachine),
    mVersion(set.mVersion),
    mType(set.mType),
    mName(set.mName),
    mDefines(set.mDefines),
    mTarget(set.mTarget),
    mCompilerType(set.mCompilerType),
    mUseCustomCompileParams(set.mUseCustomCompileParams),
    mUseCustomLinkParams(set.mUseCustomLinkParams),
    mCustomCompileParams(set.mCustomCompileParams),
    mCustomLinkParams(set.mCustomLinkParams),
    mAutoAddCharsetParams(set.mAutoAddCharsetParams)
{
    // Executables, most are hardcoded
    for (PCompilerOption pOption:set.mOptions) {
        PCompilerOption p=std::make_shared<CompilerOption>();
        *p=*pOption;
        mOptions.push_back(pOption);
    }
}

void Settings::CompilerSet::addOption(const QString &name, const QString section, bool isC,
    bool isCpp, bool isLinker, int value, const QString &setting, const QStringList &choices)
{
    PCompilerOption pOption = std::make_shared<CompilerOption>();
    pOption->name = name;
    pOption->section = section;
    pOption->isC = isC;
    pOption->isCpp = isCpp;
    pOption->isLinker = isLinker;
    pOption->value = value;
    pOption->setting= setting;
    pOption->choices = choices;
    mOptions.push_back(pOption);
}

PCompilerOption Settings::CompilerSet::findOption(const QString &setting)
{
    for (PCompilerOption pOption : mOptions) {
        if (pOption->setting == setting) {
            return pOption;
        }
    }
    return PCompilerOption();
}

int Settings::CompilerSet::findOptionIndex(const QString &setting)
{
    for (int i=0;i<mOptions.size();i++) {
         PCompilerOption pOption = mOptions[i];
        if (pOption->setting == setting) {
            return i;
        }
    }
    return -1;
}

char Settings::CompilerSet::getOptionValue(const QString &setting)
{
    PCompilerOption pOption = findOption(setting);
    if (pOption) {
        return ValueToChar[pOption->value];
    } else {
        return '0';
    }
}

void Settings::CompilerSet::setOption(const QString &setting, char valueChar)
{
    PCompilerOption pOption = findOption(setting);
    if (pOption) {
        setOption(pOption,valueChar);
    }
}

void Settings::CompilerSet::setOption(PCompilerOption &option, char valueChar)
{
    option->value = charToValue(valueChar);
}

static void checkDirs(const QStringList& dirlist, QString& gooddirs, QString& baddirs) {
    gooddirs = "";
    baddirs = "";

    for (int i=0; i<dirlist.count();i++) {
        QDir dir(dirlist[i]);
        if (!dir.exists()) {
            if (baddirs.isEmpty()) {
                baddirs = dirlist[i];
            } else {
                baddirs += ";" + dirlist[i];
            }
        } else {
            if (gooddirs.isEmpty()) {
                gooddirs = dirlist[i];
            } else {
                gooddirs += ";" + dirlist[i];
            }
        }
    }
}


bool Settings::CompilerSet::dirsValid(QString &msg)
{
    QString goodbin, badbin, goodlib, badlib, goodinc, badinc, goodinccpp, badinccpp;
    msg = "";

    if (mBinDirs.count()>0) {// we need some bin dir, so treat count=0 as an error too
        checkDirs(mBinDirs,goodbin,badbin);
        if (!badbin.isEmpty()) {
            msg += QObject::tr("The following %1 directories don't exist:").arg(
                        QObject::tr("binary")
                        );
            msg += "<br />";
            msg += badbin.replace(';',"<br />");
            msg += "<br />";
            msg += "<br />";
            return false;
        }
    } else {
        msg += QObject::tr("No %1 directories have been specified.").arg(
                    QObject::tr("binary")
                    );
        msg += "<br />";
        msg += "<br />";
        return false;
    }
    checkDirs(mCIncludeDirs,goodbin,badbin);
    if (!badbin.isEmpty()) {
        msg += QObject::tr("The following %1 directories don't exist:").arg(
                    QObject::tr("C include")
                    );
        msg += "<br />";
        msg += badbin.replace(';',"<br />");
        msg += "<br />";
        msg += "<br />";
        return false;
    }

    checkDirs(mCppIncludeDirs,goodbin,badbin);
    if (!badbin.isEmpty()) {
        msg += QObject::tr("The following %1 directories don't exist:").arg(
                    QObject::tr("C++ include")
                    );
        msg += "<br />";
        msg += badbin.replace(';',"<br />");
        msg += "<br />";
        msg += "<br />";
        return false;
    }

    checkDirs(mLibDirs,goodbin,badbin);
    if (!badbin.isEmpty()) {
        msg += QObject::tr("The following %1 directories don't exist:").arg(
                    QObject::tr("C++ include")
                    );
        msg += "<br />";
        msg += badbin.replace(';',"<br />");
        msg += "<br />";
        msg += "<br />";
        return false;
    }

    if (!msg.isEmpty())
        return false;
    else
        return true;
}

bool Settings::CompilerSet::validateExes(QString &msg)
{
    msg ="";
    if (!fileExists(mCCompiler)) {
        msg += QObject::tr("Cannot find the %1 \"%2\"")
                .arg("C Compiler")
                .arg(mCCompiler);
    }
    if (!fileExists(mCppCompiler)) {
        msg += QObject::tr("Cannot find the %1 \"%2\"")
                .arg("C++ Compiler")
                .arg(mCppCompiler);
    }
    if (!fileExists(mMake)) {
        msg += QObject::tr("Cannot find the %1 \"%2\"")
                .arg("Maker")
                .arg(mMake);
    }
    if (!fileExists(mDebugger)) {
        msg += QObject::tr("Cannot find the %1 \"%2\"")
                .arg("Maker")
                .arg(mDebugger);
    }
    if (!msg.isEmpty())
        return false;
    else
        return true;
}

const QString &Settings::CompilerSet::CCompiler() const
{
    return mCCompiler;
}

void Settings::CompilerSet::setCCompiler(const QString &name)
{
    mCCompiler = name;
}

const QString &Settings::CompilerSet::cppCompiler() const
{
    return mCppCompiler;
}

void Settings::CompilerSet::setCppCompiler(const QString &name)
{
    mCppCompiler = name;
}

const QString &Settings::CompilerSet::make() const
{
    return mMake;
}

void Settings::CompilerSet::setMake(const QString &name)
{
    mMake = name;
}

const QString &Settings::CompilerSet::debugger() const
{
    return mDebugger;
}

void Settings::CompilerSet::setDebugger(const QString &name)
{
    mDebugger = name;
}

const QString &Settings::CompilerSet::profiler() const
{
    return mProfiler;
}

void Settings::CompilerSet::setProfiler(const QString &name)
{
    mProfiler = name;
}

const QString &Settings::CompilerSet::resourceCompiler() const
{
    return mResourceCompiler;
}

void Settings::CompilerSet::setResourceCompiler(const QString &name)
{
    mResourceCompiler = name;
}

QStringList &Settings::CompilerSet::binDirs()
{
    return mBinDirs;
}

QStringList &Settings::CompilerSet::CIncludeDirs()
{
    return mCIncludeDirs;
}

QStringList &Settings::CompilerSet::CppIncludeDirs()
{
    return mCppIncludeDirs;
}

QStringList &Settings::CompilerSet::libDirs()
{
    return mLibDirs;
}

QStringList &Settings::CompilerSet::defaultCIncludeDirs()
{
    return mDefaultCIncludeDirs;
}

QStringList &Settings::CompilerSet::defaultCppIncludeDirs()
{
    return mDefaultCppIncludeDirs;
}

QStringList &Settings::CompilerSet::defaultLibDirs()
{
    return mLibDirs;
}

const QString &Settings::CompilerSet::dumpMachine() const
{
    return mDumpMachine;
}

void Settings::CompilerSet::setDumpMachine(const QString &value)
{
    mDumpMachine = value;
}

const QString &Settings::CompilerSet::version() const
{
    return mVersion;
}

void Settings::CompilerSet::setVersion(const QString &value)
{
    mVersion = value;
}

const QString &Settings::CompilerSet::type() const
{
    return mType;
}

void Settings::CompilerSet::setType(const QString& value)
{
    mType = value;
}

const QString &Settings::CompilerSet::name() const
{
    return mName;
}

void Settings::CompilerSet::setName(const QString &value)
{
    mName = value;
}

const QStringList& Settings::CompilerSet::defines() const
{
    return mDefines;
}

const QString &Settings::CompilerSet::target() const
{
    return mTarget;
}

void Settings::CompilerSet::setTarget(const QString &value)
{
    mTarget = value;
}

void Settings::CompilerSet::setUseCustomCompileParams(bool value)
{
    mUseCustomCompileParams = value;
}

bool Settings::CompilerSet::useCustomLinkParams() const
{
    return mUseCustomLinkParams;
}

void Settings::CompilerSet::setUseCustomLinkParams(bool value)
{
    mUseCustomLinkParams = value;
}

const QString &Settings::CompilerSet::customCompileParams() const
{
    return mCustomCompileParams;
}

void Settings::CompilerSet::setCustomCompileParams(const QString &value)
{
    mCustomCompileParams = value;
}

const QString &Settings::CompilerSet::customLinkParams() const
{
    return mCustomLinkParams;
}

void Settings::CompilerSet::setCustomLinkParams(const QString &value)
{
    mCustomLinkParams = value;
}

bool Settings::CompilerSet::autoAddCharsetParams() const
{
    return mAutoAddCharsetParams;
}

void Settings::CompilerSet::setAutoAddCharsetParams(bool value)
{
    mAutoAddCharsetParams = value;
}

CompilerOptionList &Settings::CompilerSet::options()
{
    return mOptions;
}

int Settings::CompilerSet::charToValue(char valueChar)
{
    if (valueChar == '1') {
        return 1;
    } else if ( (valueChar>='a') && (valueChar<='z')) {
        return (valueChar-'a')+2;
    } else {
        return 0;
    }
}

char Settings::CompilerSet::valueToChar(int val)
{
    return ValueToChar[val];
}

static void addExistingDirectory(QStringList& dirs, const QString& directory) {
    if (!directoryExists(directory))
        return;
    QFileInfo dirInfo(directory);
    QString dirPath = dirInfo.absoluteFilePath();
    if (dirs.contains(dirPath))
        return;
    dirs.append(dirPath);
}

void Settings::CompilerSet::setProperties(const QString &binDir)
{
    if (!fileExists(binDir,GCC_PROGRAM))
        return;
    // Obtain version number and compiler distro etc
    QStringList arguments;
    arguments.append("-v");
    QByteArray output = getCompilerOutput(binDir,GCC_PROGRAM,arguments);

    //Target
    QByteArray targetStr = "Target: ";
    int delimPos1 = output.indexOf(targetStr);
    if (delimPos1<0)
        return; // unknown binary
    delimPos1+=strlen(targetStr);
    int delimPos2 = delimPos1;
    while (delimPos2<output.length() && !isNonPrintableAsciiChar(output[delimPos2]))
        delimPos2++;
    mTarget = output.mid(delimPos1,delimPos2-delimPos1);

    if (mTarget.contains("x86_64"))
        mTarget = "x86_64";
    else
        mTarget = "i686";

    //Find version number
    targetStr = "clang version ";
    delimPos1 = output.indexOf(targetStr);
    if (delimPos1>=0) {
        mCompilerType = "Clang";
        delimPos1+=strlen(targetStr);
        delimPos2 = delimPos1;
        while (delimPos2<output.length() && !isNonPrintableAsciiChar(output[delimPos2]))
            delimPos2++;
        mVersion = output.mid(delimPos1,delimPos2-delimPos1);

        mName = "Clang " + mVersion;
    } else {
        mCompilerType = "GCC";
        targetStr = "gcc version ";
        delimPos1 = output.indexOf(targetStr);
        if (delimPos1<0)
            return; // unknown binary
        delimPos1+=strlen(targetStr);
        delimPos2 = delimPos1;
        while (delimPos2<output.length() && !isNonPrintableAsciiChar(output[delimPos2]))
            delimPos2++;
        mVersion = output.mid(delimPos1,delimPos2-delimPos1);

        // Find compiler builder
        delimPos1 = delimPos2;
        while ((delimPos1 < output.length()) && !(output[delimPos1] == '('))
            delimPos1++;
        while ((delimPos2 < output.length()) && !(output[delimPos2] == ')'))
            delimPos2++;
        mType = output.mid(delimPos1 + 1, delimPos2 - delimPos1 - 1);

        // Assemble user friendly name if we don't have one yet
        if (mName == "") {
            if (mType.contains("tdm64")) {
                mName = "TDM-GCC " + mVersion;
            } else if (mType.contains("tdm")) {
                mName = "TDM-GCC " + mVersion;
            } else if (mType.contains("MSYS2")) {
                mName = "MinGW-w64 GCC " + mVersion;
            } else if (mType.contains("GCC")) {
                mName = "MinGW GCC " + mVersion;
            } else {
                mName = "MinGW GCC " + mVersion;
            }
        }
    }

    // Set compiler folder
    QDir tmpDir(binDir);
    tmpDir.cdUp();
    QString folder = tmpDir.path();

    // Obtain compiler target
    arguments.clear();
    arguments.append("-dumpmachine");
    mDumpMachine = getCompilerOutput(binDir, GCC_PROGRAM, arguments);

    // Add the default directories
    addExistingDirectory(mBinDirs, includeTrailingPathDelimiter(folder) +  "bin");
//    addExistingDirectory(mDefaultLibDirs, includeTrailingPathDelimiter(folder) + "lib");
//    addExistingDirectory(mDefaultCIncludeDirs, includeTrailingPathDelimiter(folder) + "include");
//    addExistingDirectory(mDefaultCppIncludeDirs, includeTrailingPathDelimiter(folder) + "include");

    if (!mDumpMachine.isEmpty()) {
        //mingw-w64 bin folder
        addExistingDirectory(mBinDirs,
            includeTrailingPathDelimiter(folder) + "lib/"
            "gcc/" + mDumpMachine
            + "/" + mVersion);
    }
}

void Settings::CompilerSet::setDefines() {
    // get default defines
    QStringList arguments;
    arguments.append("-dM");
    arguments.append("-E");
    arguments.append("-x");
    arguments.append("c++");
    arguments.append("-std=c++17");
    arguments.append(NULL_FILE);
    QFileInfo ccompiler(mCCompiler);
    QByteArray output = getCompilerOutput(ccompiler.absolutePath(),ccompiler.fileName(),arguments);
    // 'cpp.exe -dM -E -x c++ -std=c++17 NUL'

    mDefines.clear();
    QList<QByteArray> lines = output.split('\n');
    for (QByteArray& line:lines) {
        QByteArray trimmedLine = line.trimmed();
        if (!trimmedLine.isEmpty()) {
            mDefines.append(trimmedLine);
        }
    }
}

void Settings::CompilerSet::setExecutables()
{
    mCCompiler = findProgramInBinDirs(GCC_PROGRAM);
    mCppCompiler = findProgramInBinDirs(GPP_PROGRAM);
    mDebugger = findProgramInBinDirs(GDB_PROGRAM);
    mMake = findProgramInBinDirs(MAKE_PROGRAM);
    mResourceCompiler = findProgramInBinDirs(WINDRES_PROGRAM);
    mProfiler = findProgramInBinDirs(GPROF_PROGRAM);
}

void Settings::CompilerSet::setDirectories(const QString& binDir)
{
    QString folder = QFileInfo(binDir).absolutePath();
    // Find default directories
    // C include dirs
    QStringList arguments;
    arguments.clear();
    arguments.append("-xc");
    arguments.append("-v");
    arguments.append("-E");
    arguments.append(NULL_FILE);
    QByteArray output = getCompilerOutput(binDir,GCC_PROGRAM,arguments);

    int delimPos1 = output.indexOf("#include <...> search starts here:");
    int delimPos2 = output.indexOf("End of search list.");
    if (delimPos1 >0 && delimPos2>0 ) {
        delimPos1 += QByteArray("#include <...> search starts here:").length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mDefaultCIncludeDirs,trimmedLine);
            }
        }
    }

    // Find default directories
    // C++ include dirs
    arguments.clear();
    arguments.append("-xc++");
    arguments.append("-E");
    arguments.append("-v");
    arguments.append(NULL_FILE);
    output = getCompilerOutput(binDir,GCC_PROGRAM,arguments);
    //gcc -xc++ -E -v NUL

    delimPos1 = output.indexOf("#include <...> search starts here:");
    delimPos2 = output.indexOf("End of search list.");
    if (delimPos1 >0 && delimPos2>0 ) {
        delimPos1 += QByteArray("#include <...> search starts here:").length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mDefaultCppIncludeDirs,trimmedLine);
            }
        }
    }

    // Find default directories
    arguments.clear();
    arguments.append("-print-search-dirs");
    arguments.append(NULL_FILE);
    output = getCompilerOutput(binDir,GCC_PROGRAM,arguments);
    // bin dirs
    QByteArray targetStr = QByteArray("programs: =");
    delimPos1 = output.indexOf(targetStr);
    if (delimPos1>=0) {
        delimPos1+=targetStr.length();
        delimPos2 = delimPos1;
        while (delimPos2 < output.length() && output[delimPos2]!='\n')
            delimPos2+=1;
        QList<QByteArray> lines = output.mid(delimPos1,delimPos2-delimPos1).split(';');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty())
                addExistingDirectory(mBinDirs,trimmedLine);
        }
    }
    // lib dirs
    targetStr = QByteArray("libraries: =");
    delimPos1 = output.indexOf(targetStr);
    if (delimPos1>=0) {
        delimPos1+=targetStr.length();
        delimPos2 = delimPos1;
        while (delimPos2 < output.length() && output[delimPos2]!='\n')
            delimPos2+=1;
        QList<QByteArray> lines = output.mid(delimPos1,delimPos2-delimPos1).split(';');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty())
                addExistingDirectory(mDefaultLibDirs,trimmedLine);
        }
    }

    // Try to obtain our target/autoconf folder
    if (!mDumpMachine.isEmpty()) {
        //mingw-w64 bin folder
        addExistingDirectory(mBinDirs,
            includeTrailingPathDelimiter(folder) + "lib/"
            "gcc/" + mDumpMachine
            + "/" + mVersion);

        // Regular include folder
        addExistingDirectory(mDefaultCIncludeDirs, includeTrailingPathDelimiter(folder) + mDumpMachine + "/include");
        addExistingDirectory(mDefaultCppIncludeDirs, includeTrailingPathDelimiter(folder)+ mDumpMachine + "/include");

        // Other include folder?
        addExistingDirectory(mDefaultCIncludeDirs,
            includeTrailingPathDelimiter(folder) + "lib/gcc/"
            + mDumpMachine + "/" + mVersion + "/include");
        addExistingDirectory(mDefaultCppIncludeDirs,
            includeTrailingPathDelimiter(folder) + "lib/gcc/"
            + mDumpMachine + "/" + mVersion + "/include");

        addExistingDirectory(mDefaultCIncludeDirs,
            includeTrailingPathDelimiter(folder) + "lib/gcc/"
             + mDumpMachine + "/" + mVersion + "/include-fixed");
        addExistingDirectory(mDefaultCppIncludeDirs,
            includeTrailingPathDelimiter(folder) + "lib/gcc/"
                + mDumpMachine + "/" + mVersion + "/include-fixed");

        // C++ only folder (mingw.org)
        addExistingDirectory(mDefaultCppIncludeDirs,
            includeTrailingPathDelimiter(folder)  + "lib/gcc/"
                + mDumpMachine + "/" + mVersion + "/include/c++");
        addExistingDirectory(mDefaultCppIncludeDirs,
             includeTrailingPathDelimiter(folder)  + "lib/gcc/"
                 + mDumpMachine + "/" + mVersion + "/include/c++/"
                 + mDumpMachine);
        addExistingDirectory(mDefaultCppIncludeDirs,
             includeTrailingPathDelimiter(folder)  + "lib/gcc/"
                 + mDumpMachine + "/" + mVersion + "/include/c++/backward");

        // C++ only folder (Mingw-w64)
        addExistingDirectory(mDefaultCppIncludeDirs,
            includeTrailingPathDelimiter(folder)  + "include/c++/"
            + mVersion );
        addExistingDirectory(mDefaultCppIncludeDirs,
            includeTrailingPathDelimiter(folder)  + "include/c++/"
            + mVersion + "/backward");
        addExistingDirectory(mDefaultCppIncludeDirs,
            includeTrailingPathDelimiter(folder)  + "include/c++/"
            + mVersion + "/" + mDumpMachine);
    }
}

int Settings::CompilerSet::mainVersion()
{
    int i = mVersion.indexOf('.');
    if (i<0)
        return -1;
    bool ok;
    int num = mVersion.left(i).toInt(&ok);
    if (!ok)
        return -1;
    return num;

}

void Settings::CompilerSet::setUserInput()
{
    mUseCustomCompileParams = false;
    mUseCustomLinkParams = false;
    mAutoAddCharsetParams = true;
    mStaticLink = true;
}

void Settings::CompilerSet::setOptions()
{
    // C options
    QString groupName = QObject::tr("C options");
    addOption(QObject::tr("Support all ANSI standard C programs (-ansi)"), groupName, true, true, false, 0, "-ansi");
    addOption(QObject::tr("Do not recognize asm,inline or typeof as a keyword (-fno-asm)"), groupName, true, true, false, 0, "-fno-asm");
    addOption(QObject::tr("Imitate traditional C preprocessors (-traditional-cpp)"), groupName, true, true, false, 0, "-traditional-cpp");

    // Optimization for cpu type
    groupName = QObject::tr("Code Generation");
    QStringList sl;
    sl.append(""); // /!\ Must contain a starting empty value in order to do not have always to pass the parameter
    sl.append("This CPU=native");
    sl.append("i386=i386");
    sl.append("i486=i486");
    sl.append("i586=i586");
    sl.append("i686=i686");
    sl.append("Pentium=pentium");
    sl.append("Pentium MMX=pentium-mmx");
    sl.append("Pentium Pro=pentiumpro");
    sl.append("Pentium 2=pentium2");
    sl.append("Pentium 3=pentium3");
    sl.append("Pentium 4=pentium4");
    sl.append("Conroe=core2");
    sl.append("Nehalem=corei7");
    sl.append("Sandy=corei7-avx");
    sl.append("K6=k6");
    sl.append("K6-2=k6-2");
    sl.append("K6-3=k6-3");
    sl.append("Athlon=athlon");
    sl.append("Athlon Tbird=athlon-tbird");
    sl.append("Athlon 4=athlon-4");
    sl.append("Athlon XP=athlon-xp");
    sl.append("Athlon MP=athlon-mp");
    sl.append("K8=k8");
    sl.append("K8 Rev.E=k8-sse3");
    sl.append("K10=barcelona");
    sl.append("Bulldozer=bdver1");
    addOption(QObject::tr("Optimize for the following machine (-march)"), groupName, true, true, false, 0, "-march=", sl);
    addOption(QObject::tr("Optimize less, while maintaining full compatibility (-tune)"), groupName, true, true, false, 0, "-mtune=", sl);

    // Enable use of the specific instructions
    sl.clear();
    sl.append(""); // /!\ Must contain a starting empty value in order to do not have always to pass the parameter
    sl.append("MMX=mmx");
    sl.append("3D Now=3dnow");
    sl.append("SSE=sse");
    sl.append("SSE2=sse2");
    sl.append("SSE3=sse3");
    sl.append("SSSE3=ssse3");
    sl.append("SSE4=sse4");
    sl.append("SSE4A=sse4a");
    sl.append("SSE4.1=sse4.1");
    sl.append("SSE4.2=sse4.2");
    sl.append("AVX=avx");
    sl.append("AVX2=avx2");
    sl.append("FMA4=fma4");
    sl.append("XOP=xop");
    sl.append("AES=aes");
    addOption(QObject::tr("Enable use of specific instructions (-mx)"), groupName, true, true, false, 0, "-m", sl);

    // Optimization
    sl.clear();
    sl.append("");
    sl.append("Low=1");
    sl.append("Med=2");
    sl.append("High=3");
    sl.append("Highest (fast)=fast");
    sl.append("Size (s)=s");
    sl.append("Debug (g)=g");
    addOption(QObject::tr("Optimization level (-Ox)"), groupName, true, true, false, 0, "-O", sl);

    // 32bit/64bit
    sl.clear();
    sl.append("");
    sl.append("32bit=m32");
    sl.append("64bit=m64");
    addOption(QObject::tr("Compile with the following pointer size (-mx)"), groupName, true, true, true, 0, "-", sl);

    // Language Standards
    sl.clear();
    sl.append(""); // Passing nothing effectively lets the compiler decide
    sl.append("ISO C90=c90");
    sl.append("ISO C99=c99");
    sl.append("ISO C11=c11");
    sl.append("ISO C17=c17");
    sl.append("ISO C++=c++98");
    sl.append("ISO C++11=c++11");
    sl.append("ISO C++14=c++14");
    sl.append("ISO C++17=c++17");
    sl.append("ISO C++20=c++2a");
    sl.append("GNU C90=gnu90");
    sl.append("GNU C99=gnu99");
    sl.append("GNU C11=gnu11");
    sl.append("GNU C17=gnu17");
    sl.append("GNU C++=gnu++98");
    sl.append("GNU C++11=gnu++11");
    sl.append("GNU C++14=gnu++14");
    sl.append("GNU C++17=gnu++17");
    sl.append("GNU C++20=gnu++20");
    addOption(QObject::tr("Language standard (-std)"), groupName, true, true, false, 0, "-std=", sl);

    // Warnings
    groupName = QObject::tr("Warnings");
    addOption(QObject::tr("Inhibit all warning messages (-w)"), groupName, true, true, false, 0, "-w");
    addOption(QObject::tr("Show most warnings (-Wall)"), groupName, true, true, false, 0, "-Wall");
    addOption(QObject::tr("Show some more warnings (-Wextra)"), groupName, true, true, false, 0, "-Wextra");
    addOption(QObject::tr("Check ISO C/C++/C++0x conformance (-pedantic)"), groupName, true, true, false, 0, "-pedantic");
    addOption(QObject::tr("Only check the code for syntax errors (-fsyntax-only)"), groupName, true, true, false, 0, "-fsyntax-only");
    addOption(QObject::tr("Make all warnings into errors (-Werror)"), groupName, true, true, false, 0, "-Werror");
    addOption(QObject::tr("Abort compilation on first error (-Wfatal-errors)"), groupName, true, true, false, 0, "-Wfatal-errors");

    // Profile
    groupName = QObject::tr("Profile");
    addOption(QObject::tr("Generate profiling info for analysis (-pg)"), groupName, true, true, true, 0, "-pg");

    // Linker
    groupName = QObject::tr("Linker");
    addOption(QObject::tr("Link an Objective C program (-lobjc)"), groupName, false, false, true, 0, "-lobjc");
    addOption(QObject::tr("Do not use standard system libraries (-nostdlib)"), groupName, false, false, true, 0, "-nostdlib");
    addOption(QObject::tr("Do not create a console window (-mwindows)"), groupName,false, false, true, 0, "-mwindows");
    addOption(QObject::tr("Strip executable (-s)"), groupName, false, false, true, 0, "-s");
    addOption(QObject::tr("Generate debugging information (-g3)"), groupName, true, true, false, 0, "-g3");

    // Output
    groupName = QObject::tr("Output");
    addOption(QObject::tr("Put comments in generated assembly code (-fverbose-asm)"), groupName, true, true, false, 0, "-fverbose-asm");
    addOption(QObject::tr("Do not assemble, compile and generate the assemble code (-S)"), groupName, true, true, false, 0, "-S");
    addOption(QObject::tr("Use pipes instead of temporary files during compilation (-pipe)"), groupName, true, true, false, 0, "-pipe");
}

QString Settings::CompilerSet::findProgramInBinDirs(const QString name)
{
    for (const QString& dir : mBinDirs) {
        QFileInfo f(includeTrailingPathDelimiter(dir) + name);
        if (f.exists() && f.isExecutable()) {
            return f.absoluteFilePath();
        }
    }
    return QString();
}

QByteArray Settings::CompilerSet::iniOptions() const
{
    QByteArray result;
    for (const PCompilerOption& p:mOptions) {
        result.append(ValueToChar[p->value]);
    }
    return result;
}

void Settings::CompilerSet::setIniOptions(const QByteArray &value)
{
   int i=0;
   for (PCompilerOption p:mOptions) {
       if (i>=value.length()) {
           break;
       }
       p->value = charToValue(value[i]);
       i++;
   }
}

QByteArray Settings::CompilerSet::getCompilerOutput(const QString &binDir, const QString &binFile, const QStringList &arguments)
{
    QByteArray result = runAndGetOutput(includeTrailingPathDelimiter(binDir)+binFile, binDir, arguments);
    return result.trimmed();
}

int Settings::CompilerSet::compilerSetType() const
{
    return mCompilerSetType;
}

void Settings::CompilerSet::setCompilerSetType(int newCompilerSetType)
{
    mCompilerSetType = newCompilerSetType;
}

void Settings::CompilerSet::setCompilerType(const QString &newCompilerType)
{
    mCompilerType = newCompilerType;
}

const QString &Settings::CompilerSet::compilerType() const
{
    return mCompilerType;
}

bool Settings::CompilerSet::staticLink() const
{
    return mStaticLink;
}

void Settings::CompilerSet::setStaticLink(bool newStaticLink)
{
    mStaticLink = newStaticLink;
}

bool Settings::CompilerSet::useCustomCompileParams() const
{
    return mUseCustomCompileParams;
}

Settings::CompilerSets::CompilerSets(Settings *settings):
    mDefaultIndex(-1),
    mSettings(settings)
{

}

Settings::PCompilerSet Settings::CompilerSets::addSet(const Settings::CompilerSet& set)
{
    PCompilerSet p=std::make_shared<CompilerSet>(set);
    mList.push_back(p);
    return p;
}

Settings::PCompilerSet Settings::CompilerSets::addSet(const QString &folder)
{
    PCompilerSet p=std::make_shared<CompilerSet>(folder);
    mList.push_back(p);
    return p;
}

static void setReleaseOptions(Settings::PCompilerSet pSet) {
    PCompilerOption pOption = pSet->findOption("-O");
    if (pOption) {
        pSet->setOption(pOption,'a');
    }

    pOption = pSet->findOption("-s");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }

    pOption = pSet->findOption("-pipe");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }

//    pOption = pSet->findOption("-static");
//    if (pOption) {
//        pSet->setOption(pOption,'1');
//    }
    pSet->setStaticLink(true);
}

static void setDebugOptions(Settings::PCompilerSet pSet) {
    PCompilerOption pOption = pSet->findOption("-g3");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }
    pOption = pSet->findOption("-Wall");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }
    pOption = pSet->findOption("-Wextra");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }
    pOption = pSet->findOption("-pipe");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }

//    pOption = pSet->findOption("-static");
//    if (pOption) {
//        pSet->setOption(pOption,'1');
//    }
    pSet->setStaticLink(false);
}

static void setProfileOptions(Settings::PCompilerSet pSet) {
    PCompilerOption pOption = pSet->findOption("-pg");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }

    pOption = pSet->findOption("-pipe");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }

//    pOption = pSet->findOption("-static");
//    if (pOption) {
//        pSet->setOption(pOption,'1');
//    }
    pSet->setStaticLink(false);
}

void Settings::CompilerSets::addSets(const QString &folder)
{
    if (!directoryExists(folder))
        return;
    if (!fileExists(includeTrailingPathDelimiter(folder)+"bin"+QDir::separator()+GCC_PROGRAM)) {
        return;
    }
    // Default, release profile
    PCompilerSet baseSet = addSet(folder);
    QString baseName = baseSet->name();
    QString platformName;
    if (baseSet->target() == "x86_64") {
        platformName = "64-bit";
    } else {
        platformName = "32-bit";
    }
    baseSet->setName(baseName + " " + platformName + " Release");
    baseSet->setCompilerSetType(CompilerSetType::CST_RELEASE);
    setReleaseOptions(baseSet);

    baseSet = addSet(folder);
    baseSet->setName(baseName + " " + platformName + " Debug");
    baseSet->setCompilerSetType(CompilerSetType::CST_DEBUG);
    setDebugOptions(baseSet);

    baseSet = addSet(folder);
    baseSet->setName(baseName + " " + platformName + " Profiling");
    baseSet->setCompilerSetType(CompilerSetType::CST_PROFILING);
    setProfileOptions(baseSet);

    mDefaultIndex = mList.size() - 2;
}

void Settings::CompilerSets::clearSets()
{
    for (size_t i=0;i<mList.size();i++) {
        mSettings->mSettings.beginGroup(QString(SETTING_COMPILTER_SET).arg(i));
        mSettings->mSettings.remove("");
        mSettings->mSettings.endGroup();
    }
    mList.clear();
    mDefaultIndex = -1;
}

void Settings::CompilerSets::findSets()
{
    clearSets();
    addSets(includeTrailingPathDelimiter(mSettings->dirs().app())+"MinGW32");
    addSets(includeTrailingPathDelimiter(mSettings->dirs().app())+"MinGW64");
}

void Settings::CompilerSets::saveSets()
{
    for (size_t i=0;i<mList.size();i++) {
        saveSet(i);
    }
    if (mDefaultIndex>=mList.size()) {
        mDefaultIndex = mList.size()-1;
    }
    mSettings->mSettings.beginGroup(SETTING_COMPILTER_SETS);
    mSettings->mSettings.setValue(SETTING_COMPILTER_SETS_DEFAULT_INDEX,mDefaultIndex);
    mSettings->mSettings.setValue(SETTING_COMPILTER_SETS_COUNT,mList.size());
    mSettings->mSettings.endGroup();
}

void Settings::CompilerSets::loadSets()
{
    mList.clear();
    mSettings->mSettings.beginGroup(SETTING_COMPILTER_SETS);
    mDefaultIndex =mSettings->mSettings.value(SETTING_COMPILTER_SETS_DEFAULT_INDEX,-1).toInt();
    int listSize = mSettings->mSettings.value(SETTING_COMPILTER_SETS_COUNT,0).toInt();
    mSettings->mSettings.endGroup();
    for (int i=0;i<listSize;i++) {
        PCompilerSet pSet=loadSet(i);
        mList.push_back(pSet);
    }

    PCompilerSet pCurrentSet = defaultSet();
    if (pCurrentSet) {
        QString msg;
        if (!pCurrentSet->dirsValid(msg) || !pCurrentSet->validateExes(msg)) {
            if (QMessageBox::warning(nullptr,QObject::tr("Confirm"),
                       QObject::tr("The following problems were found during validation of compiler set \"%1\":")
                                     .arg(pCurrentSet->name())
                                     +"<br /><br />"
                                     +msg
                                     +QObject::tr("Would you like Red Panda C++ to remove them for you and add the default paths to the valid paths?")
                                     +"<br /><br />"
                                     +QObject::tr("Leaving those directories will lead to problems during compilation.<br /><br />Unless you know exactly what you're doing, it is recommended that you click Yes."),
                                     QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
                return;
            }
            findSets();
            saveSets();
            if ( mList.size() <= mDefaultIndex)
                mDefaultIndex =  mList.size()-1;
            pCurrentSet = defaultSet();
            if (!pCurrentSet) {
                return;
            }
            saveSet(mDefaultIndex);
            if (pCurrentSet->binDirs().count()>0) {
                pCurrentSet->setProperties(pCurrentSet->binDirs()[0]);
            }
        } else {
            return;
        }
    } else {
        if (QMessageBox::warning(nullptr,QObject::tr("Confirm"),
                   QObject::tr("Compiler set not configuared.")
                                 +"<br /><br />"
                                 +QObject::tr("Would you like Red Panda C++ to search for compilers in the following locations: <BR />'%1'<BR />'%2'? ")
                                 .arg(includeTrailingPathDelimiter(pSettings->dirs().app()) + "MinGW32")
                                 .arg(includeTrailingPathDelimiter(pSettings->dirs().app()) + "MinGW64"),
                                 QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        clearSets();
        findSets();
        pCurrentSet = defaultSet();
        if (!pCurrentSet) {
            return;
        }
        saveSets();
    }

}

void Settings::CompilerSets::saveDefaultIndex()
{
    mSettings->mSettings.beginGroup(SETTING_COMPILTER_SETS);
    mSettings->mSettings.setValue(SETTING_COMPILTER_SETS_DEFAULT_INDEX,mDefaultIndex);
    mSettings->mSettings.endGroup();
}

void Settings::CompilerSets::deleteSet(int index)
{
    // Erase all sections at and above from disk
    for (size_t i=index;i<mList.size();i++) {
        mSettings->mSettings.beginGroup(QString(SETTING_COMPILTER_SET).arg(i));
        mSettings->mSettings.remove("");
        mSettings->mSettings.endGroup();
    }
    mList.erase(std::begin(mList)+index);
    for (size_t i=index;i<mList.size();i++) {
        saveSet(i);
    }
    if (mDefaultIndex>=mList.size()) {
        mDefaultIndex = mList.size()-1;
    }
}

Settings::CompilerSetList &Settings::CompilerSets::list()
{
    return mList;
}

int Settings::CompilerSets::size() const
{
    return mList.size();
}

int Settings::CompilerSets::defaultIndex() const
{
    return mDefaultIndex;
}

void Settings::CompilerSets::setDefaultIndex(int value)
{
    mDefaultIndex = value;
}

Settings::PCompilerSet Settings::CompilerSets::defaultSet()
{
    return getSet(mDefaultIndex);
}

Settings::PCompilerSet Settings::CompilerSets::getSet(int index)
{
    if (index>=0 && index<mList.size()) {
        return mList[index];
    }
    return PCompilerSet();
}

void Settings::CompilerSets::savePath(const QString& name, const QString& path) {
    QString s;
    QString prefix1 = excludeTrailingPathDelimiter(mSettings->mDirs.app()) + "/";
    QString prefix2 = excludeTrailingPathDelimiter(mSettings->mDirs.app()) + QDir::separator();
    if (path.startsWith(prefix1, PATH_SENSITIVITY)) {
        s = "%AppPath%/"+ path.mid(prefix1.length());
    } else if (path.startsWith(prefix2, PATH_SENSITIVITY)) {
        s = "%AppPath%/"+ path.mid(prefix2.length());
    } else {
        s= path;
    }
    mSettings->mSettings.setValue(name,s);
}

void Settings::CompilerSets::savePathList(const QString& name, const QStringList& pathList) {
    QStringList sl;
    for (const QString& path: pathList) {
        QString s;
        QString prefix1 = excludeTrailingPathDelimiter(mSettings->mDirs.app()) + "/";
        QString prefix2 = excludeTrailingPathDelimiter(mSettings->mDirs.app()) + QDir::separator();
        if (path.startsWith(prefix1, PATH_SENSITIVITY)) {
            s = "%AppPath%/"+ path.mid(prefix1.length());
        } else if (path.startsWith(prefix2, PATH_SENSITIVITY)) {
            s = "%AppPath%/" + path.mid(prefix2.length());
        } else {
            s= path;
        }
        sl.append(s);
    }
    mSettings->mSettings.setValue(name,sl);
}

void Settings::CompilerSets::saveSet(int index)
{
    PCompilerSet pSet = mList[index];
    mSettings->mSettings.beginGroup(QString(SETTING_COMPILTER_SET).arg(index));

    savePath("ccompiler", pSet->CCompiler());
    savePath("cppcompiler", pSet->cppCompiler());
    savePath("debugger", pSet->debugger());
    savePath("make", pSet->make());
    savePath("windres", pSet->resourceCompiler());
    savePath("profiler", pSet->profiler());

    // Save option string
    mSettings->mSettings.setValue("Options", pSet->iniOptions());

    // Save extra 'general' options
    mSettings->mSettings.setValue("useCustomCompileParams", pSet->useCustomCompileParams());
    mSettings->mSettings.setValue("customCompileParams", pSet->customCompileParams());
    mSettings->mSettings.setValue("useCustomLinkParams", pSet->useCustomLinkParams());
    mSettings->mSettings.setValue("customLinkParams", pSet->customLinkParams());
    mSettings->mSettings.setValue("AddCharset", pSet->autoAddCharsetParams());
    mSettings->mSettings.setValue("StaticLink", pSet->staticLink());

    // Misc. properties
    mSettings->mSettings.setValue("DumpMachine", pSet->dumpMachine());
    mSettings->mSettings.setValue("Version", pSet->version());
    mSettings->mSettings.setValue("Type", pSet->type());
    mSettings->mSettings.setValue("Name", pSet->name());
    mSettings->mSettings.setValue("Target", pSet->target());
    mSettings->mSettings.setValue("CompilerType", pSet->compilerType());
    mSettings->mSettings.setValue("CompilerSetType", pSet->compilerSetType());

    // Paths
    savePathList("Bins",pSet->binDirs());
    savePathList("C",pSet->CIncludeDirs());
    savePathList("Cpp",pSet->CppIncludeDirs());
    savePathList("Libs",pSet->libDirs());

    mSettings->mSettings.endGroup();
}

QString Settings::CompilerSets::loadPath(const QString &name)
{
    QString s =  mSettings->mSettings.value(name).toString();
    QString prefix = "%AppPath%/";
    if (s.startsWith(prefix)) {
        s = includeTrailingPathDelimiter(mSettings->mDirs.app()) + s.mid(prefix.length());
    }
    return QFileInfo(s).absoluteFilePath();
}

void Settings::CompilerSets::loadPathList(const QString &name, QStringList& list)
{
    list.clear();
    QStringList sl = mSettings->mSettings.value(name).toStringList();
    QString prefix = "%AppPath%/";
    for (QString& s:sl) {
        if (s.startsWith(prefix)) {
            s = includeTrailingPathDelimiter(mSettings->mDirs.app()) + s.mid(prefix.length());
        }
        list.append(QFileInfo(s).absoluteFilePath());
    }
}

Settings::PCompilerSet Settings::CompilerSets::loadSet(int index)
{
    PCompilerSet pSet = std::make_shared<CompilerSet>();
    mSettings->mSettings.beginGroup(QString(SETTING_COMPILTER_SET).arg(index));

    pSet->setCCompiler(loadPath("ccompiler"));
    pSet->setCppCompiler(loadPath("cppcompiler"));
    pSet->setDebugger(loadPath("debugger"));
    pSet->setMake(loadPath("make"));
    pSet->setResourceCompiler(loadPath("windres"));
    pSet->setProfiler(loadPath("profiler"));

    // Save option string
    pSet->setIniOptions(mSettings->mSettings.value("Options").toByteArray());

    // Save extra 'general' options
    pSet->setUseCustomCompileParams(mSettings->mSettings.value("useCustomCompileParams").toBool());
    pSet->setCustomCompileParams(mSettings->mSettings.value("customCompileParams").toString());
    pSet->setUseCustomLinkParams(mSettings->mSettings.value("useCustomLinkParams").toBool());
    pSet->setCustomLinkParams(mSettings->mSettings.value("customLinkParams").toString());
    pSet->setAutoAddCharsetParams(mSettings->mSettings.value("AddCharset").toBool());
    pSet->setStaticLink(mSettings->mSettings.value("StaticLink").toBool());

    pSet->setDumpMachine(mSettings->mSettings.value("DumpMachine").toString());
    pSet->setVersion(mSettings->mSettings.value("Version").toString());
    pSet->setType(mSettings->mSettings.value("Type").toString());
    pSet->setName(mSettings->mSettings.value("Name").toString());
    pSet->setTarget(mSettings->mSettings.value("Target").toString());
    pSet->setCompilerType(mSettings->mSettings.value("CompilerType").toString());
    pSet->setCompilerSetType(mSettings->mSettings.value("CompilerSetType").toInt());

    // Paths
    loadPathList("Bins",pSet->binDirs());
    loadPathList("C",pSet->CIncludeDirs());
    loadPathList("Cpp",pSet->CppIncludeDirs());
    loadPathList("Libs",pSet->libDirs());

    mSettings->mSettings.endGroup();

    pSet->setDirectories(pSet->binDirs()[0]);
    pSet->setDefines();
    return pSet;
}

Settings::Environment::Environment(Settings *settings):_Base(settings, SETTING_ENVIRONMENT)
{

}

void Settings::Environment::doLoad()
{
    //Appearence
    mTheme = stringValue("theme","dark");
    mInterfaceFont = stringValue("interface font","Segoe UI");
    mInterfaceFontSize = intValue("interface font size",10);
    mLanguage = stringValue("language", QLocale::system().name());
    mCurrentFolder = stringValue("current_folder",QDir::currentPath());
    if (!fileExists(mCurrentFolder)) {
        mCurrentFolder = QDir::currentPath();
    }
}

int Settings::Environment::interfaceFontSize() const
{
    return mInterfaceFontSize;
}

void Settings::Environment::setInterfaceFontSize(int interfaceFontSize)
{
    mInterfaceFontSize = interfaceFontSize;
}

QString Settings::Environment::language() const
{
    return mLanguage;
}

void Settings::Environment::setLanguage(const QString &language)
{
    mLanguage = language;
}

const QString &Settings::Environment::currentFolder() const
{
    return mCurrentFolder;
}

void Settings::Environment::setCurrentFolder(const QString &newCurrentFolder)
{
    mCurrentFolder = newCurrentFolder;
}

void Settings::Environment::doSave()
{
    //Appearence
    saveValue("theme", mTheme);
    saveValue("interface font", mInterfaceFont);
    saveValue("interface font size", mInterfaceFontSize);
    saveValue("language", mLanguage);
    saveValue("current_folder",mCurrentFolder);
}

QString Settings::Environment::interfaceFont() const
{
    return mInterfaceFont;
}

void Settings::Environment::setInterfaceFont(const QString &interfaceFont)
{
    mInterfaceFont = interfaceFont;
}

QString Settings::Environment::theme() const
{
    return mTheme;
}

void Settings::Environment::setTheme(const QString &theme)
{
    mTheme = theme;
}

Settings::Executor::Executor(Settings *settings):_Base(settings, SETTING_EXECUTOR)
{

}

bool Settings::Executor::minimizeOnRun() const
{
    return mMinimizeOnRun;
}

void Settings::Executor::setMinimizeOnRun(bool minimizeOnRun)
{
    mMinimizeOnRun = minimizeOnRun;
}

bool Settings::Executor::useParams() const
{
    return mUseParams;
}

void Settings::Executor::setUseParams(bool newUseParams)
{
    mUseParams = newUseParams;
}

const QString &Settings::Executor::params() const
{
    return mParams;
}

void Settings::Executor::setParams(const QString &newParams)
{
    mParams = newParams;
}

bool Settings::Executor::redirectInput() const
{
    return mRedirectInput;
}

void Settings::Executor::setRedirectInput(bool newRedirectInput)
{
    mRedirectInput = newRedirectInput;
}

const QString &Settings::Executor::inputFilename() const
{
    return mInputFilename;
}

void Settings::Executor::setInputFilename(const QString &newInputFilename)
{
    mInputFilename = newInputFilename;
}

int Settings::Executor::competivieCompanionPort() const
{
    return mCompetivieCompanionPort;
}

void Settings::Executor::setCompetivieCompanionPort(int newCompetivieCompanionPort)
{
    mCompetivieCompanionPort = newCompetivieCompanionPort;
}

bool Settings::Executor::enableCompetitiveCompanion() const
{
    return mEnableCompetitiveCompanion;
}

void Settings::Executor::setEnableCompetitiveCompanion(bool newEnableCompetitiveCompanion)
{
    mEnableCompetitiveCompanion = newEnableCompetitiveCompanion;
}

bool Settings::Executor::enableProblemSet() const
{
    return mEnableProblemSet;
}

void Settings::Executor::setEnableProblemSet(bool newEnableProblemSet)
{
    mEnableProblemSet = newEnableProblemSet;
}

void Settings::Executor::doSave()
{
    saveValue("pause_console", mPauseConsole);
    saveValue("minimize_on_run", mMinimizeOnRun);
    saveValue("use_params",mUseParams);
    saveValue("params",mParams);
    saveValue("redirect_input",mRedirectInput);
    saveValue("input_filename",mInputFilename);
    //problem set

}

bool Settings::Executor::pauseConsole() const
{
    return mPauseConsole;
}

void Settings::Executor::setPauseConsole(bool pauseConsole)
{
    mPauseConsole = pauseConsole;
}

void Settings::Executor::doLoad()
{
    mPauseConsole = boolValue("pause_console",true);
    mMinimizeOnRun = boolValue("minimize_on_run",false);
    mUseParams = boolValue("use_params",false);
    mParams = stringValue("params", "");
    mRedirectInput = boolValue("redirect_input",false);
    mInputFilename = stringValue("input_filename","");

    mEnableProblemSet = boolValue("enable_proble_set",true);
    mEnableCompetitiveCompanion = boolValue("enable_competivie_companion",true);
    mCompetivieCompanionPort = intValue("competitive_companion_port",10045);
}


Settings::Debugger::Debugger(Settings *settings):_Base(settings, SETTING_DEBUGGER)
{

}

bool Settings::Debugger::showCommandLog() const
{
    return mShowCommandLog;
}

void Settings::Debugger::setShowCommandLog(bool showCommandLog)
{
    mShowCommandLog = showCommandLog;
}

bool Settings::Debugger::showAnnotations() const
{
    return mShowAnnotations;
}

void Settings::Debugger::setShowAnnotations(bool showAnnotations)
{
    mShowAnnotations = showAnnotations;
}

QString Settings::Debugger::fontName() const
{
    return mFontName;
}

void Settings::Debugger::setFontName(const QString &fontName)
{
    mFontName = fontName;
}

bool Settings::Debugger::blendMode() const
{
    return mBlendMode;
}

void Settings::Debugger::setBlendMode(bool blendMode)
{
    mBlendMode = blendMode;
}

bool Settings::Debugger::skipSystemLibraries() const
{
    return mSkipSystemLibraries;
}

void Settings::Debugger::setSkipSystemLibraries(bool newSkipSystemLibraries)
{
    mSkipSystemLibraries = newSkipSystemLibraries;
}

bool Settings::Debugger::skipProjectLibraries() const
{
    return mSkipProjectLibraries;
}

void Settings::Debugger::setSkipProjectLibraries(bool newSkipProjectLibraries)
{
    mSkipProjectLibraries = newSkipProjectLibraries;
}

bool Settings::Debugger::skipCustomLibraries() const
{
    return mSkipCustomLibraries;
}

void Settings::Debugger::setSkipCustomLibraries(bool newSkipCustomLibraries)
{
    mSkipCustomLibraries = newSkipCustomLibraries;
}

bool Settings::Debugger::autosaveWatches() const
{
    return mAutosaveWatches;
}

void Settings::Debugger::setAutosaveWatches(bool newAutosaveWatches)
{
    mAutosaveWatches = newAutosaveWatches;
}

bool Settings::Debugger::autosaveBreakpoints() const
{
    return mAutosaveBreakpoints;
}

void Settings::Debugger::setAutosaveBreakpoints(bool newAutosaveBreakpoints)
{
    mAutosaveBreakpoints = newAutosaveBreakpoints;
}

bool Settings::Debugger::useIntelStyle() const
{
    return mUseIntelStyle;
}

void Settings::Debugger::setUseIntelStyle(bool useIntelStyle)
{
    mUseIntelStyle = useIntelStyle;
}

int Settings::Debugger::fontSize() const
{
    return mFontSize;
}

void Settings::Debugger::setFontSize(int fontSize)
{
    mFontSize = fontSize;
}

bool Settings::Debugger::onlyShowMono() const
{
    return mOnlyShowMono;
}

void Settings::Debugger::setOnlyShowMono(bool onlyShowMono)
{
    mOnlyShowMono = onlyShowMono;
}

void Settings::Debugger::doSave()
{
    saveValue("show_command_log", mShowCommandLog);
    saveValue("show_annotations", mShowAnnotations);
    saveValue("font_name",mFontName);
    saveValue("only_show_mono",mOnlyShowMono);
    saveValue("font_size",mFontSize);
    saveValue("use_intel_style",mUseIntelStyle);
    saveValue("blend_mode",mBlendMode);
    saveValue("skip_system_lib", mSkipSystemLibraries);
    saveValue("skip_project_lib", mSkipProjectLibraries);
    saveValue("skip_custom_lib", mSkipCustomLibraries);
    saveValue("autosave_breakpoints",mAutosaveBreakpoints);
    saveValue("autosave_watches",mAutosaveWatches);

}

void Settings::Debugger::doLoad()
{
    mShowCommandLog = boolValue("show_command_log",true);
    mShowAnnotations = boolValue("show_annotations",false);
    mFontName = stringValue("font_name","Consolas");
    mOnlyShowMono = boolValue("only_show_mono",true);
    mFontSize = intValue("font_size",10);
    mUseIntelStyle = boolValue("use_intel_style",true);
    mBlendMode = boolValue("blend_mode",true);
    mSkipSystemLibraries = boolValue("skip_system_lib",true);
    mSkipProjectLibraries = boolValue("skip_project_lib",true);
    mSkipCustomLibraries = boolValue("skip_custom_lib",false);
    mAutosaveBreakpoints = boolValue("autosave_breakpoints",true);
    mAutosaveWatches = boolValue("autosave_watches",true);
}

Settings::History::History(Settings *settings):_Base(settings, SETTING_HISTORY)
{

}

const QStringList &Settings::History::openedFiles() const
{
    return mOpenedFiles;
}

const QStringList &Settings::History::openedProjects() const
{
    return mOpenedProjects;
}

bool Settings::History::addToOpenedFiles(const QString &filename)
{
    if (!QFile(filename).exists())
        return false;
    int index = mOpenedFiles.indexOf(filename);
    if (index>=0) {
        mOpenedFiles.removeAt(index);
    }
    if (mOpenedFiles.size()>=15) {
        mOpenedFiles.pop_back();
    }
    mOpenedFiles.push_front(filename);
    save();
    return true;

}

void Settings::History::removeFile(const QString &filename)
{
    int index = mOpenedFiles.indexOf(filename);
    if (index>=0) {
        mOpenedFiles.removeAt(index);
    }
    save();
    return;
}

bool Settings::History::addToOpenedProjects(const QString &filename)
{
    if (!QFile(filename).exists())
        return false;
    int index = mOpenedProjects.indexOf(filename);
    if (index>=0) {
        mOpenedProjects.removeAt(index);
    }
    if (mOpenedProjects.size()>=15) {
        mOpenedProjects.pop_back();
    }
    mOpenedProjects.push_front(filename);
    save();
    return true;
}

void Settings::History::removeProject(const QString &filename)
{
    int index = mOpenedProjects.indexOf(filename);
    if (index>=0) {
        mOpenedProjects.removeAt(index);
    }
    save();
    return;
}

void Settings::History::doSave()
{
    saveValue("opened_files", mOpenedFiles);
    saveValue("opened_projects", mOpenedProjects);
}

static QStringList filterValidPathes(const QStringList& files) {
    QStringList lst;
    foreach (const QString& filePath, files) {
        if (fileExists(filePath)) {
            lst.append(QFileInfo(filePath).absoluteFilePath());
        }
    }
    return lst;
}

void Settings::History::doLoad()
{
    mOpenedFiles = filterValidPathes(stringListValue("opened_files"));
    mOpenedProjects = filterValidPathes(stringListValue("opened_projects"));
}

Settings::CodeCompletion::CodeCompletion(Settings *settings):_Base(settings, SETTING_CODE_COMPLETION)
{

}

bool Settings::CodeCompletion::showCodeIns() const
{
    return mShowCodeIns;
}

void Settings::CodeCompletion::setShowCodeIns(bool newShowCodeIns)
{
    mShowCodeIns = newShowCodeIns;
}

bool Settings::CodeCompletion::appendFunc() const
{
    return mAppendFunc;
}

void Settings::CodeCompletion::setAppendFunc(bool newAppendFunc)
{
    mAppendFunc = newAppendFunc;
}

bool Settings::CodeCompletion::ignoreCase() const
{
    return mIgnoreCase;
}

void Settings::CodeCompletion::setIgnoreCase(bool newIgnoreCase)
{
    mIgnoreCase = newIgnoreCase;
}

bool Settings::CodeCompletion::showKeywords() const
{
    return mShowKeywords;
}

void Settings::CodeCompletion::setShowKeywords(bool newShowKeywords)
{
    mShowKeywords = newShowKeywords;
}

bool Settings::CodeCompletion::sortByScope() const
{
    return mSortByScope;
}

void Settings::CodeCompletion::setSortByScope(bool newSortByScope)
{
    mSortByScope = newSortByScope;
}

bool Settings::CodeCompletion::recordUsage() const
{
    return mRecordUsage;
}

void Settings::CodeCompletion::setRecordUsage(bool newRecordUsage)
{
    mRecordUsage = newRecordUsage;
}

bool Settings::CodeCompletion::showCompletionWhileInput() const
{
    return mShowCompletionWhileInput;
}

void Settings::CodeCompletion::setShowCompletionWhileInput(bool newShowCompletionWhileInput)
{
    mShowCompletionWhileInput = newShowCompletionWhileInput;
}

bool Settings::CodeCompletion::parseGlobalHeaders() const
{
    return mParseGlobalHeaders;
}

void Settings::CodeCompletion::setParseGlobalHeaders(bool newParseGlobalHeaders)
{
    mParseGlobalHeaders = newParseGlobalHeaders;
}

bool Settings::CodeCompletion::parseLocalHeaders() const
{
    return mParseLocalHeaders;
}

void Settings::CodeCompletion::setParseLocalHeaders(bool newParseLocalHeaders)
{
    mParseLocalHeaders = newParseLocalHeaders;
}

bool Settings::CodeCompletion::enabled() const
{
    return mEnabled;
}

void Settings::CodeCompletion::setEnabled(bool newEnabled)
{
    mEnabled = newEnabled;
}

int Settings::CodeCompletion::height() const
{
    return mHeight;
}

void Settings::CodeCompletion::setHeight(int newHeight)
{
    mHeight = newHeight;
}

int Settings::CodeCompletion::width() const
{
    return mWidth;
}

void Settings::CodeCompletion::setWidth(int newWidth)
{
    mWidth = newWidth;
}

void Settings::CodeCompletion::doSave()
{
    saveValue("width",mWidth);
    saveValue("height",mHeight);
    saveValue("enabled",mEnabled);
    saveValue("parse_local_headers",mParseLocalHeaders);
    saveValue("parse_global_headers",mParseGlobalHeaders);
    saveValue("show_completion_while_input",mShowCompletionWhileInput);
    saveValue("record_usage",mRecordUsage);
    saveValue("sort_by_scope",mSortByScope);
    saveValue("show_keywords",mShowKeywords);
    saveValue("ignore_case",mIgnoreCase);
    saveValue("append_func",mAppendFunc);
    saveValue("show_code_ins",mShowCodeIns);
}


void Settings::CodeCompletion::doLoad()
{
    //Appearence
    mWidth = intValue("width",700);
    mHeight = intValue("height",400);
    mEnabled = boolValue("enabled",true);
    mParseLocalHeaders = boolValue("parse_local_headers",true);
    mParseGlobalHeaders = boolValue("parse_global_headers",true);
    mShowCompletionWhileInput = boolValue("show_completion_while_input",true);
    mRecordUsage = boolValue("record_usage",true);
    mSortByScope = boolValue("sort_by_scope",true);
    mShowKeywords = boolValue("show_keywords",true);
    mIgnoreCase = boolValue("ignore_case",true);
    mAppendFunc = boolValue("append_func",true);
    mShowCodeIns = boolValue("show_code_ins",true);
}

Settings::CodeFormatter::CodeFormatter(Settings *settings):
    _Base(settings,SETTING_CODE_FORMATTER)
{

}

QStringList Settings::CodeFormatter::getArguments()
{
    QStringList result;
    switch(mBraceStyle) {
    case FormatterBraceStyle::fbsDefault:
        break;
    case FormatterBraceStyle::fbsAllman:
        result.append("--style=allman");
        break;
    case FormatterBraceStyle::fbsJava:
        result.append("--style=java");
        break;
    case FormatterBraceStyle::fbsKR:
        result.append("--style=kr");
        break;
    case FormatterBraceStyle::fbsStroustrup:
        result.append("--style=stroustrup");
        break;
    case FormatterBraceStyle::fbsWitesmith:
        result.append("--style=whitesmith");
        break;
    case FormatterBraceStyle::fbsVtk:
        result.append("--style=vtk");
        break;
    case FormatterBraceStyle::fbsRatliff:
        result.append("--style=ratliff");
        break;
    case FormatterBraceStyle::fbsGNU:
        result.append("--style=gnu");
        break;
    case FormatterBraceStyle::fbsLinux:
        result.append("--style=linux");
        break;
    case FormatterBraceStyle::fbsHorstmann:
        result.append("--style=horstmann");
        break;
    case FormatterBraceStyle::fbs1TBS:
        result.append("--style=1tbs");
        break;
    case FormatterBraceStyle::fbsGoogle:
        result.append("--style=google");
        break;
    case FormatterBraceStyle::fbsMozilla:
        result.append("--style=mozilla");
        break;
    case FormatterBraceStyle::fbsWebkit:
        result.append("--style=webkit");
        break;
    case FormatterBraceStyle::fbsPico:
        result.append("--style=pico");
        break;
    case FormatterBraceStyle::fbsLisp:
        result.append("--style=lisp");
        break;
    };
    switch(mIndentStyle) {
    case FormatterIndentType::fitTab:
        result.append(QString("--indent=tab=%1").arg(mTabWidth));
        break;
    case FormatterIndentType::fitSpace:
        result.append(QString("--indent=spaces=%1").arg(mTabWidth));
        break;
    }
    if (mAttachNamespaces)
        result.append("--attach-namespaces");
    if (mAttachClasses)
        result.append("--attach-classes");
    if (mAttachInlines)
        result.append("--attach-inlines");
    if (mAttachExternC)
        result.append("--attach-extern-c");
    if (mAttachClosingWhile)
        result.append("--attach-closing-while");
    if (mIndentClasses)
        result.append("--indent-classes");
    if (mIndentModifiers)
        result.append("--indent-modifiers");
    if (mIndentSwitches)
        result.append("--indent-switches");
    if (mIndentCases)
        result.append("--indent-cases");
    if (mIndentNamespaces)
        result.append("--indent-namespaces");
    if (mIndentAfterParens)
        result.append("--indent-after-parens");
    if (mIndentContinuation!=1)
        result.append(QString("--indent-continuation=%1").arg(mIndentContinuation));
    if (mIndentLabels)
        result.append("--indent-labels");
    if (mIndentPreprocBlock)
        result.append("--indent-preproc-block");
    if (mIndentPreprocCond)
        result.append("--indent-preproc-cond");
    if (mIndentPreprocDefine)
        result.append("--indent-preproc-define");
    if (mIndentCol1Comments)
        result.append("--indent-col1-comments");
    if (mMinConditionalIndent!=2)
        result.append(QString("--min-conditional-indent=%1").arg(mMinConditionalIndent));
    if (mMaxContinuationIndent!=40)
        result.append(QString("--max-continuation-indent=%1").arg(mMaxContinuationIndent));
    if (mBreakBlocks)
        result.append("--break-blocks");
    if (mBreakBlocksAll)
        result.append("--break-blocks=all");
    if (mPadOper)
        result.append("--pad-oper");
    if (mPadComma)
        result.append("--pad-comma");
    if (mPadParen)
        result.append("--pad-paren");
    if (mPadParenOut)
        result.append("--pad-paren-out");
    if (mPadFirstParenOut)
        result.append("--pad-first-paren-out");
    if (mPadParenIn)
        result.append("--pad-paren-in");
    if (mPadHeader)
        result.append("--pad-header");
    if (mUnpadParen)
        result.append("--unpad-paren");
    if (mDeleteEmptyLines)
        result.append("--delete-empty-lines");
    if (mDeleteMultipleEmptyLines)
        result.append("--delete-multiple-empty-lines");
    if (mFillEmptyLines)
        result.append("--fill-empty-lines");
    switch(mAlignPointerStyle) {
    case FormatterOperatorAlign::foaNone:
        break;
    case FormatterOperatorAlign::foaType:
        result.append("--align-pointer=type");
        break;
    case FormatterOperatorAlign::foaMiddle:
        result.append("--align-pointer=middle");
        break;
    case FormatterOperatorAlign::foaName:
        result.append("--align-pointer=name");
        break;
    }
    switch(mAlignReferenceStyle) {
    case FormatterOperatorAlign::foaNone:
        break;
    case FormatterOperatorAlign::foaType:
        result.append("--align-reference=type");
        break;
    case FormatterOperatorAlign::foaMiddle:
        result.append("--align-reference=middle");
        break;
    case FormatterOperatorAlign::foaName:
        result.append("--align-reference=name");
        break;
    }

    if (mBreakClosingBraces)
        result.append("--break-closing-braces");
    if (mBreakElseIf)
        result.append("--break-elseifs");
    if (mBreakOneLineHeaders)
        result.append("--break-one-line-headers");
    if (mAddBraces)
        result.append("--add-braces");
    if (mAddOneLineBraces)
        result.append("--add-one-line-braces");
    if (mRemoveBraces)
        result.append("--remove-braces");
    if (mBreakReturnType)
        result.append("--break-return-type");
    if (mBreakReturnTypeDecl)
        result.append("--break-return-type-decl");
    if (mAttachReturnType)
        result.append("--attach-return-type");
    if (mAttachReturnTypeDecl)
        result.append("--attach-return-type-decl");
    if (mKeepOneLineBlocks)
        result.append("--keep-one-line-blocks");
    if (mKeepOneLineStatements)
        result.append("--keep-one-line-statements");
    if (mConvertTabs)
        result.append("--convert-tabs");
    if (mCloseTemplates)
        result.append("--close-templates");
    if (mRemoveCommentPrefix)
        result.append("--remove-comment-prefix");
    if (mBreakMaxCodeLength) {
        result.append(QString("--max-code-length=%1").arg(mMaxCodeLength));
        if (mBreakAfterLogical)
            result.append("--break-after-logical");
    }

    return result;
}

int Settings::CodeFormatter::indentStyle() const
{
    return mIndentStyle;
}

void Settings::CodeFormatter::setIndentStyle(int newIndentStyle)
{
    mIndentStyle = newIndentStyle;
}

int Settings::CodeFormatter::tabWidth() const
{
    return mTabWidth;
}

void Settings::CodeFormatter::setTabWidth(int newTabWidth)
{
    mTabWidth = newTabWidth;
}

bool Settings::CodeFormatter::attachNamespaces() const
{
    return mAttachNamespaces;
}

void Settings::CodeFormatter::setAttachNamespaces(bool newAttachNamespaces)
{
    mAttachNamespaces = newAttachNamespaces;
}

bool Settings::CodeFormatter::attachClasses() const
{
    return mAttachClasses;
}

void Settings::CodeFormatter::setAttachClasses(bool newAttachClasses)
{
    mAttachClasses = newAttachClasses;
}

bool Settings::CodeFormatter::attachInlines() const
{
    return mAttachInlines;
}

void Settings::CodeFormatter::setAttachInlines(bool newAttachInlines)
{
    mAttachInlines = newAttachInlines;
}

bool Settings::CodeFormatter::attachExternC() const
{
    return mAttachExternC;
}

void Settings::CodeFormatter::setAttachExternC(bool newAttachExternC)
{
    mAttachExternC = newAttachExternC;
}

bool Settings::CodeFormatter::attachClosingWhile() const
{
    return mAttachClosingWhile;
}

void Settings::CodeFormatter::setAttachClosingWhile(bool newAttachClosingWhile)
{
    mAttachClosingWhile = newAttachClosingWhile;
}

bool Settings::CodeFormatter::indentClasses() const
{
    return mIndentClasses;
}

void Settings::CodeFormatter::setIndentClasses(bool newIndentClasses)
{
    mIndentClasses = newIndentClasses;
}

bool Settings::CodeFormatter::indentModifiers() const
{
    return mIndentModifiers;
}

void Settings::CodeFormatter::setIndentModifiers(bool newIndentModifiers)
{
    mIndentModifiers = newIndentModifiers;
}

bool Settings::CodeFormatter::indentCases() const
{
    return mIndentCases;
}

void Settings::CodeFormatter::setIndentCases(bool newIndentCases)
{
    mIndentCases = newIndentCases;
}

bool Settings::CodeFormatter::indentNamespaces() const
{
    return mIndentNamespaces;
}

void Settings::CodeFormatter::setIndentNamespaces(bool newIndentNamespaces)
{
    mIndentNamespaces = newIndentNamespaces;
}

int Settings::CodeFormatter::indentContinuation() const
{
    return mIndentContinuation;
}

void Settings::CodeFormatter::setIndentContinuation(int newIndentContinuation)
{
    mIndentContinuation = newIndentContinuation;
}

bool Settings::CodeFormatter::indentLabels() const
{
    return mIndentLabels;
}

void Settings::CodeFormatter::setIndentLabels(bool newIndentLabels)
{
    mIndentLabels = newIndentLabels;
}

bool Settings::CodeFormatter::indentPreprocBlock() const
{
    return mIndentPreprocBlock;
}

void Settings::CodeFormatter::setIndentPreprocBlock(bool newIndentPreprocBlock)
{
    mIndentPreprocBlock = newIndentPreprocBlock;
}

bool Settings::CodeFormatter::indentPreprocCond() const
{
    return mIndentPreprocCond;
}

void Settings::CodeFormatter::setIndentPreprocCond(bool newIndentPreprocCond)
{
    mIndentPreprocCond = newIndentPreprocCond;
}

bool Settings::CodeFormatter::indentPreprocDefine() const
{
    return mIndentPreprocDefine;
}

void Settings::CodeFormatter::setIndentPreprocDefine(bool newIndentPreprocDefine)
{
    mIndentPreprocDefine = newIndentPreprocDefine;
}

bool Settings::CodeFormatter::indentCol1Comments() const
{
    return mIndentCol1Comments;
}

void Settings::CodeFormatter::setIndentCol1Comments(bool newIndentCol1Comments)
{
    mIndentCol1Comments = newIndentCol1Comments;
}

int Settings::CodeFormatter::minConditionalIndent() const
{
    return mMinConditionalIndent;
}

void Settings::CodeFormatter::setMinConditionalIndent(int newMinConditionalIndent)
{
    mMinConditionalIndent = newMinConditionalIndent;
}

int Settings::CodeFormatter::maxContinuationIndent() const
{
    return mMaxContinuationIndent;
}

void Settings::CodeFormatter::setMaxContinuationIndent(int newMaxContinuationIndent)
{
    mMaxContinuationIndent = newMaxContinuationIndent;
}

bool Settings::CodeFormatter::breakBlocks() const
{
    return mBreakBlocks;
}

void Settings::CodeFormatter::setBreakBlocks(bool newBreakBlocks)
{
    mBreakBlocks = newBreakBlocks;
}

bool Settings::CodeFormatter::breakBlocksAll() const
{
    return mBreakBlocksAll;
}

void Settings::CodeFormatter::setBreakBlocksAll(bool newBreakBlocksAll)
{
    mBreakBlocksAll = newBreakBlocksAll;
}

bool Settings::CodeFormatter::padOper() const
{
    return mPadOper;
}

void Settings::CodeFormatter::setPadOper(bool newPadOper)
{
    mPadOper = newPadOper;
}

bool Settings::CodeFormatter::padComma() const
{
    return mPadComma;
}

void Settings::CodeFormatter::setPadComma(bool newPadComma)
{
    mPadComma = newPadComma;
}

bool Settings::CodeFormatter::padParen() const
{
    return mPadParen;
}

void Settings::CodeFormatter::setPadParen(bool newPadParen)
{
    mPadParen = newPadParen;
}

bool Settings::CodeFormatter::padParenOut() const
{
    return mPadParenOut;
}

void Settings::CodeFormatter::setPadParenOut(bool newPadParenOut)
{
    mPadParenOut = newPadParenOut;
}

bool Settings::CodeFormatter::padFirstParenOut() const
{
    return mPadFirstParenOut;
}

void Settings::CodeFormatter::setPadFirstParenOut(bool newPadFirstParenOut)
{
    mPadFirstParenOut = newPadFirstParenOut;
}

bool Settings::CodeFormatter::padParenIn() const
{
    return mPadParenIn;
}

void Settings::CodeFormatter::setPadParenIn(bool newPadParenIn)
{
    mPadParenIn = newPadParenIn;
}

bool Settings::CodeFormatter::padHeader() const
{
    return mPadHeader;
}

void Settings::CodeFormatter::setPadHeader(bool newPadHeader)
{
    mPadHeader = newPadHeader;
}

bool Settings::CodeFormatter::unpadParen() const
{
    return mUnpadParen;
}

void Settings::CodeFormatter::setUnpadParen(bool newUnpadParen)
{
    mUnpadParen = newUnpadParen;
}

bool Settings::CodeFormatter::deleteEmptyLines() const
{
    return mDeleteEmptyLines;
}

void Settings::CodeFormatter::setDeleteEmptyLines(bool newDeleteEmptyLines)
{
    mDeleteEmptyLines = newDeleteEmptyLines;
}

bool Settings::CodeFormatter::deleteMultipleEmptyLines() const
{
    return mDeleteMultipleEmptyLines;
}

void Settings::CodeFormatter::setDeleteMultipleEmptyLines(bool newDeleteMultipleEmptyLines)
{
    mDeleteMultipleEmptyLines = newDeleteMultipleEmptyLines;
}

bool Settings::CodeFormatter::fillEmptyLines() const
{
    return mFillEmptyLines;
}

void Settings::CodeFormatter::setFillEmptyLines(bool newFillEmptyLines)
{
    mFillEmptyLines = newFillEmptyLines;
}

int Settings::CodeFormatter::alignPointerStyle() const
{
    return mAlignPointerStyle;
}

void Settings::CodeFormatter::setAlignPointerStyle(int newAlignPointerStyle)
{
    mAlignPointerStyle = newAlignPointerStyle;
}

int Settings::CodeFormatter::alignReferenceStyle() const
{
    return mAlignReferenceStyle;
}

void Settings::CodeFormatter::setAlignReferenceStyle(int newAlignReferenceStyle)
{
    mAlignReferenceStyle = newAlignReferenceStyle;
}

bool Settings::CodeFormatter::breakClosingBraces() const
{
    return mBreakClosingBraces;
}

void Settings::CodeFormatter::setBreakClosingBraces(bool newBreakClosingBraces)
{
    mBreakClosingBraces = newBreakClosingBraces;
}

bool Settings::CodeFormatter::breakElseIf() const
{
    return mBreakElseIf;
}

void Settings::CodeFormatter::setBreakElseIf(bool newBreakElseIf)
{
    mBreakElseIf = newBreakElseIf;
}

bool Settings::CodeFormatter::breakOneLineHeaders() const
{
    return mBreakOneLineHeaders;
}

void Settings::CodeFormatter::setBreakOneLineHeaders(bool newBreakOneLineHeaders)
{
    mBreakOneLineHeaders = newBreakOneLineHeaders;
}

bool Settings::CodeFormatter::addBraces() const
{
    return mAddBraces;
}

void Settings::CodeFormatter::setAddBraces(bool newAddBraces)
{
    mAddBraces = newAddBraces;
}

bool Settings::CodeFormatter::addOneLineBraces() const
{
    return mAddOneLineBraces;
}

void Settings::CodeFormatter::setAddOneLineBraces(bool newAddOneLineBraces)
{
    mAddOneLineBraces = newAddOneLineBraces;
}

bool Settings::CodeFormatter::removeBraces() const
{
    return mRemoveBraces;
}

void Settings::CodeFormatter::setRemoveBraces(bool newRemoveBraces)
{
    mRemoveBraces = newRemoveBraces;
}

bool Settings::CodeFormatter::breakReturnTypeDecl() const
{
    return mBreakReturnTypeDecl;
}

void Settings::CodeFormatter::setBreakReturnTypeDecl(bool newBreakReturnTypeDecl)
{
    mBreakReturnTypeDecl = newBreakReturnTypeDecl;
}

bool Settings::CodeFormatter::attachReturnType() const
{
    return mAttachReturnType;
}

void Settings::CodeFormatter::setAttachReturnType(bool newAttachReturnType)
{
    mAttachReturnType = newAttachReturnType;
}

bool Settings::CodeFormatter::attachReturnTypeDecl() const
{
    return mAttachReturnTypeDecl;
}

void Settings::CodeFormatter::setAttachReturnTypeDecl(bool newAttachReturnTypeDecl)
{
    mAttachReturnTypeDecl = newAttachReturnTypeDecl;
}

bool Settings::CodeFormatter::keepOneLineBlocks() const
{
    return mKeepOneLineBlocks;
}

void Settings::CodeFormatter::setKeepOneLineBlocks(bool newKeepOneLineBlocks)
{
    mKeepOneLineBlocks = newKeepOneLineBlocks;
}

bool Settings::CodeFormatter::keepOneLineStatements() const
{
    return mKeepOneLineStatements;
}

void Settings::CodeFormatter::setKeepOneLineStatements(bool newKeepOneLineStatements)
{
    mKeepOneLineStatements = newKeepOneLineStatements;
}

bool Settings::CodeFormatter::convertTabs() const
{
    return mConvertTabs;
}

void Settings::CodeFormatter::setConvertTabs(bool newConvertTabs)
{
    mConvertTabs = newConvertTabs;
}

bool Settings::CodeFormatter::closeTemplates() const
{
    return mCloseTemplates;
}

void Settings::CodeFormatter::setCloseTemplates(bool newCloseTemplates)
{
    mCloseTemplates = newCloseTemplates;
}

bool Settings::CodeFormatter::removeCommentPrefix() const
{
    return mRemoveCommentPrefix;
}

void Settings::CodeFormatter::setRemoveCommentPrefix(bool newRemoveCommentPrefix)
{
    mRemoveCommentPrefix = newRemoveCommentPrefix;
}

int Settings::CodeFormatter::maxCodeLength() const
{
    return mMaxCodeLength;
}

void Settings::CodeFormatter::setMaxCodeLength(int newMaxCodeLength)
{
    mMaxCodeLength = newMaxCodeLength;
}

bool Settings::CodeFormatter::breakAfterLogical() const
{
    return mBreakAfterLogical;
}

void Settings::CodeFormatter::setBreakAfterLogical(bool newBreakAfterLogical)
{
    mBreakAfterLogical = newBreakAfterLogical;
}

bool Settings::CodeFormatter::breakReturnType() const
{
    return mBreakReturnType;
}

void Settings::CodeFormatter::setBreakReturnType(bool newBreakReturnType)
{
    mBreakReturnType = newBreakReturnType;
}

bool Settings::CodeFormatter::breakMaxCodeLength() const
{
    return mBreakMaxCodeLength;
}

void Settings::CodeFormatter::setBreakMaxCodeLength(bool newBreakMaxCodeLength)
{
    mBreakMaxCodeLength = newBreakMaxCodeLength;
}

bool Settings::CodeFormatter::indentAfterParens() const
{
    return mIndentAfterParens;
}

void Settings::CodeFormatter::setIndentAfterParens(bool newIndentAfterParens)
{
    mIndentAfterParens = newIndentAfterParens;
}

bool Settings::CodeFormatter::indentSwitches() const
{
    return mIndentSwitches;
}

void Settings::CodeFormatter::setIndentSwitches(bool newIndentSwitches)
{
    mIndentSwitches = newIndentSwitches;
}

void Settings::CodeFormatter::doSave()
{
    saveValue("brace_style",mBraceStyle);
    saveValue("indent_style",mIndentStyle);
    saveValue("tab_width",mTabWidth);
    saveValue("attach_namespaces",mAttachNamespaces);
    saveValue("attach_classes",mAttachClasses);
    saveValue("attach_inlines",mAttachInlines);
    saveValue("attach_extern_c",mAttachExternC);
    saveValue("attach_closing_while",mAttachClosingWhile);
    saveValue("indent_classes",mIndentClasses);
    saveValue("indent_modifiers",mIndentModifiers);
    saveValue("indent_switches",mIndentSwitches);
    saveValue("indent_cases",mIndentCases);
    saveValue("indent_namespaces",mIndentNamespaces);
    saveValue("indent_after_parents",mIndentAfterParens);
    saveValue("indent_continuation",mIndentContinuation);
    saveValue("indent_labels",mIndentLabels);
    saveValue("indent_preproc_block",mIndentPreprocBlock);
    saveValue("indent_preproc_cond",mIndentPreprocCond);
    saveValue("indent_preproc_define",mIndentPreprocDefine);
    saveValue("indent_col1_comments",mIndentCol1Comments);
    saveValue("min_conditional_indent",mMinConditionalIndent);
    saveValue("max_continuation_indent",mMaxContinuationIndent);
    saveValue("break_blocks",mBreakBlocks);
    saveValue("break_blocks_all",mBreakBlocksAll);
    saveValue("pad_oper",mPadOper);
    saveValue("pad_comma",mPadComma);
    saveValue("pad_paren",mPadParen);
    saveValue("pad_paren_out",mPadParenOut);
    saveValue("pad_first_paren_out",mPadFirstParenOut);
    saveValue("pad_parent_in",mPadParenIn);
    saveValue("pad_header",mPadHeader);
    saveValue("unpad_paren",mUnpadParen);
    saveValue("delete_empty_lines",mDeleteEmptyLines);
    saveValue("delete_multiple_empty_lines",mDeleteMultipleEmptyLines);
    saveValue("fill_empty_lines",mFillEmptyLines);
    saveValue("align_pointer_style",mAlignPointerStyle);
    saveValue("align_reference_style",mAlignReferenceStyle);
    saveValue("break_closing_braces",mBreakClosingBraces);
    saveValue("break_else_if",mBreakElseIf);
    saveValue("break_one_line_headers",mBreakOneLineHeaders);
    saveValue("add_braces",mAddBraces);
    saveValue("add_one_line_braces",mAddOneLineBraces);
    saveValue("remove_braces",mRemoveBraces);
    saveValue("break_return_type",mBreakReturnType);
    saveValue("break_return_type_decl",mBreakReturnTypeDecl);
    saveValue("attach_return_type",mAttachReturnType);
    saveValue("attach_return_type_decl",mAttachReturnTypeDecl);
    saveValue("keep_one_line_blocks",mKeepOneLineBlocks);
    saveValue("keep_one_line_statements",mKeepOneLineStatements);
    saveValue("convert_tabs",mConvertTabs);
    saveValue("close_templates",mCloseTemplates);
    saveValue("remove_comment_prefix",mRemoveCommentPrefix);
    saveValue("break_max_code_length",mBreakMaxCodeLength);
    saveValue("max_code_length",mMaxCodeLength);
    saveValue("break_after_logical",mBreakAfterLogical);
}

void Settings::CodeFormatter::doLoad()
{
    mBraceStyle = intValue("brace_style", FormatterBraceStyle::fbsJava);
    mIndentStyle = intValue("indent_style",FormatterIndentType::fitTab); // 0 isspaces, 1 is tab
    mTabWidth = intValue("tab_width",4);
    mAttachNamespaces = boolValue("attach_namespaces",false);
    mAttachClasses = boolValue("attach_classes",false);
    mAttachInlines = boolValue("attach_inlines",false);
    mAttachExternC = boolValue("attach_extern_c",false);
    mAttachClosingWhile = boolValue("attach_closing_while",false);
    mIndentClasses = boolValue("indent_classes",true);
    mIndentModifiers = boolValue("indent_modifiers",false);
    mIndentSwitches = boolValue("indent_switches",true);
    mIndentCases = boolValue("indent_cases",false);
    mIndentNamespaces = boolValue("indent_namespaces",true);
    mIndentAfterParens = boolValue("indent_after_parents",false);
    mIndentContinuation = boolValue("indent_continuation",false);
    mIndentLabels = boolValue("indent_labels",false);
    mIndentPreprocBlock = boolValue("indent_preproc_block",true);
    mIndentPreprocCond = boolValue("indent_preproc_cond",false);
    mIndentPreprocDefine = boolValue("indent_preproc_define",false);
    mIndentCol1Comments = boolValue("indent_col1_comments",false);
    mMinConditionalIndent = intValue("min_conditional_indent",1);
    mMaxContinuationIndent = intValue("max_continuation_indent",40);
    mBreakBlocks = boolValue("break_blocks",false);
    mBreakBlocksAll = boolValue("break_blocks_all",false);
    mPadOper = boolValue("pad_oper",true);
    mPadComma = boolValue("pad_comma",true);
    mPadParen = boolValue("pad_paren",false);
    mPadParenOut = boolValue("pad_paren_out",false);
    mPadFirstParenOut = boolValue("pad_first_paren_out",false);
    mPadParenIn = boolValue("pad_parent_in",false);
    mPadHeader = boolValue("pad_header",true);
    mUnpadParen = boolValue("unpad_paren",false);
    mDeleteEmptyLines = boolValue("delete_empty_lines",false);
    mDeleteMultipleEmptyLines = boolValue("delete_multiple_empty_lines",false);
    mFillEmptyLines = boolValue("fill_empty_lines",false);
    mAlignPointerStyle = intValue("align_pointer_style", FormatterOperatorAlign::foaNone);
    mAlignReferenceStyle = intValue("align_reference_style", FormatterOperatorAlign::foaNone);
    mBreakClosingBraces = boolValue("break_closing_braces",false);
    mBreakElseIf = boolValue("break_else_if",false);
    mBreakOneLineHeaders = boolValue("break_one_line_headers",false);
    mAddBraces = boolValue("add_braces",false);
    mAddOneLineBraces = boolValue("add_one_line_braces",false);
    mRemoveBraces = boolValue("remove_braces",false);
    mBreakReturnType = boolValue("break_return_type",false);
    mBreakReturnTypeDecl = boolValue("break_return_type_decl",false);
    mAttachReturnType = boolValue("attach_return_type",false);
    mAttachReturnTypeDecl = boolValue("attach_return_type_decl",false);
    mKeepOneLineBlocks = boolValue("keep_one_line_blocks",false);
    mKeepOneLineStatements = boolValue("keep_one_line_statements",false);
    mConvertTabs = boolValue("convert_tabs",false);
    mCloseTemplates = boolValue("close_templates",false);
    mRemoveCommentPrefix = boolValue("remove_comment_prefix",false);
    mBreakMaxCodeLength = boolValue("break_max_code_length",false);
    mMaxCodeLength = intValue("max_code_length",80);
    mBreakAfterLogical = boolValue("break_after_logical",false);
}

int Settings::CodeFormatter::braceStyle() const
{
    return mBraceStyle;
}

void Settings::CodeFormatter::setBraceStyle(int newBraceStyle)
{
    mBraceStyle = newBraceStyle;
}

Settings::UI::UI(Settings *settings):_Base(settings,SETTING_UI)
{

}

const QByteArray &Settings::UI::mainWindowGeometry() const
{
    return mMainWindowGeometry;
}

void Settings::UI::setMainWindowGeometry(const QByteArray &newMainWindowGeometry)
{
    mMainWindowGeometry = newMainWindowGeometry;
}

bool Settings::UI::bottomPanelOpenned() const
{
    return mBottomPanelOpenned;
}

void Settings::UI::setBottomPanelOpenned(bool newBottomPanelOpenned)
{
    mBottomPanelOpenned = newBottomPanelOpenned;
}

int Settings::UI::bottomPanelHeight() const
{
    return mBottomPanelHeight;
}

void Settings::UI::setBottomPanelHeight(int newBottomPanelHeight)
{
    mBottomPanelHeight = newBottomPanelHeight;
}

int Settings::UI::bottomPanelIndex() const
{
    return mBottomPanelIndex;
}

void Settings::UI::setBottomPanelIndex(int newBottomPanelIndex)
{
    mBottomPanelIndex = newBottomPanelIndex;
}

bool Settings::UI::leftPanelOpenned() const
{
    return mLeftPanelOpenned;
}

void Settings::UI::setLeftPanelOpenned(bool newLeftPanelOpenned)
{
    mLeftPanelOpenned = newLeftPanelOpenned;
}

int Settings::UI::leftPanelWidth() const
{
    return mLeftPanelWidth;
}

void Settings::UI::setLeftPanelWidth(int newLeftPanelWidth)
{
    mLeftPanelWidth = newLeftPanelWidth;
}

int Settings::UI::leftPanelIndex() const
{
    return mLeftPanelIndex;
}

void Settings::UI::setLeftPanelIndex(int newLeftPanelIndex)
{
    mLeftPanelIndex = newLeftPanelIndex;
}

bool Settings::UI::classBrowserShowInherited() const
{
    return mClassBrowserShowInherited;
}

void Settings::UI::setClassBrowserShowInherited(bool newClassBrowserShowInherited)
{
    mClassBrowserShowInherited = newClassBrowserShowInherited;
}

bool Settings::UI::classBrowserSortType() const
{
    return mClassBrowserSortType;
}

void Settings::UI::setClassBrowserSortType(bool newClassBrowserSortType)
{
    mClassBrowserSortType = newClassBrowserSortType;
}

bool Settings::UI::classBrowserSortAlpha() const
{
    return mClassBrowserSortAlpha;
}

void Settings::UI::setClassBrowserSortAlpha(bool newClassBrowserSortAlpha)
{
    mClassBrowserSortAlpha = newClassBrowserSortAlpha;
}

const QByteArray &Settings::UI::mainWindowState() const
{
    return mMainWindowState;
}

void Settings::UI::setMainWindowState(const QByteArray &newMainWindowState)
{
    mMainWindowState = newMainWindowState;
}

void Settings::UI::doSave()
{
    saveValue("main_window_state",mMainWindowState);
    saveValue("main_window_geometry",mMainWindowGeometry);
    saveValue("bottom_panel_openned",mBottomPanelOpenned);
    saveValue("bottom_panel_height",mBottomPanelHeight);
    saveValue("bottom_panel_index",mBottomPanelIndex);
    saveValue("left_panel_openned",mLeftPanelOpenned);
    saveValue("left_panel_width",mLeftPanelWidth);
    saveValue("left_panel_index",mLeftPanelIndex);
    saveValue("class_browser_sort_alphabetically",mClassBrowserSortAlpha);
    saveValue("class_browser_sort_by_type",mClassBrowserSortType);
    saveValue("class_browser_show_inherited",mClassBrowserShowInherited);
}

void Settings::UI::doLoad()
{
    mMainWindowState = value("main_window_state",QByteArray()).toByteArray();
    mMainWindowGeometry = value("main_window_geometry",QByteArray()).toByteArray();
    mBottomPanelOpenned = boolValue("bottom_panel_openned",false);
    mBottomPanelHeight = intValue("bottom_panel_height",220);
    mBottomPanelIndex = intValue("bottom_panel_index",0);
    mLeftPanelOpenned = boolValue("left_panel_openned",true);
    mLeftPanelWidth = intValue("left_panel_width",250);
    mLeftPanelIndex = intValue("left_panel_index",2);
    mClassBrowserSortAlpha = boolValue("class_browser_sort_alphabetically",true);
    mClassBrowserSortType = boolValue("class_browser_sort_by_type",true);
    mClassBrowserShowInherited = boolValue("class_browser_show_inherited",true);
}
