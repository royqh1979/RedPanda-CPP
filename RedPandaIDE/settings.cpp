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
#include "settings.h"
#include <QApplication>
#include <algorithm>
#include "utils.h"
#include "utils/escape.h"
#include "utils/font.h"
#include "utils/parsearg.h"
#include <QDir>
#include "systemconsts.h"
#include <QDebug>
#include <QMessageBox>
#include <QStandardPaths>
#include <QScreen>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#ifdef Q_OS_LINUX
#include <sys/sysinfo.h>
#endif
#ifdef ENABLE_LUA_ADDON
# include "addon/executor.h"
# include "addon/runtime.h"
#endif

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
    mUI(this),
#ifdef ENABLE_VCS
    mVCS(this),
#endif
    mLanguages(this)
{
    //load();
}

Settings::~Settings()
{
    //mEditor.save();
}

void Settings::beginGroup(const QString &group)
{
    mSettings.beginGroup(group);
}

void Settings::endGroup()
{
    mSettings.endGroup();
}

void Settings::remove(const QString &key)
{
    mSettings.remove(key);
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
    mCodeCompletion.load();
    mCodeFormatter.load();
    mUI.load();
    mDirs.load();
#ifdef ENABLE_VCS
    mVCS.load();
#endif
    mLanguages.load();
}

QSettings::Status Settings::sync()
{
    mSettings.sync();
    return mSettings.status();
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

Settings::Languages &Settings::languages()
{
    return mLanguages;
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

#ifdef ENABLE_VCS
Settings::VCS &Settings::vcs()
{
    return mVCS;
}
#endif

Settings::Debugger& Settings::debugger()
{
    return mDebugger;
}

Settings::Dirs::Dirs(Settings *settings):
    _Base(settings, SETTING_DIRS)
{
}

QString Settings::Dirs::appDir() const
{
    return QApplication::instance()->applicationDirPath();
}

QString Settings::Dirs::appResourceDir() const
{
#ifdef Q_OS_WIN
    return appDir();
#elif defined(Q_OS_MACOS)
//    return QApplication::instance()->applicationDirPath();
    return "";
#else // XDG desktop
    // in AppImage or tarball PREFIX is not true, resolve from relative path
    const static QString absoluteResourceDir(QDir(appDir()).absoluteFilePath("../share/" APP_NAME));
    return absoluteResourceDir;
#endif
}


QString Settings::Dirs::appLibexecDir() const
{
#ifdef Q_OS_WIN
    return appDir();
#elif defined(Q_OS_MACOS)
    return QApplication::instance()->applicationDirPath();
#else // XDG desktop
    // in AppImage or tarball LIBEXECDIR is not true, resolve from relative path
    const static QString relativeLibExecDir(QDir(PREFIX "/bin").relativeFilePath(LIBEXECDIR "/" APP_NAME));
    const static QString absoluteLibExecDir(QDir(appDir()).absoluteFilePath(relativeLibExecDir));
    return absoluteLibExecDir;
#endif
}

QString Settings::Dirs::projectDir() const
{
    return mProjectDir;
}

QString Settings::Dirs::data(Settings::Dirs::DataType dataType) const
{
    using DataType = Settings::Dirs::DataType;
    QString dataDir = includeTrailingPathDelimiter(appDir())+"data";
    switch (dataType) {
    case DataType::None:
        return dataDir;
    case DataType::ColorScheme:
        return ":/resources/colorschemes";
    case DataType::IconSet:
        return ":/resources/iconsets";
    case DataType::Theme:
        return ":/resources/themes";
    case DataType::Template:
        return includeTrailingPathDelimiter(appResourceDir()) + "templates";
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
    case DataType::ColorScheme:
        return QFileInfo{includeTrailingPathDelimiter(configDir)+"scheme"}.absoluteFilePath();
    case DataType::IconSet:
        return QFileInfo{includeTrailingPathDelimiter(configDir)+"iconsets"}.absoluteFilePath();
    case DataType::Theme:
        return QFileInfo{includeTrailingPathDelimiter(configDir)+"themes"}.absoluteFilePath();
    case DataType::Template:
        return QFileInfo{includeTrailingPathDelimiter(configDir) + "templates"}.absoluteFilePath();
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
    saveValue("projectDir",mProjectDir);
}

void Settings::Dirs::doLoad()
{
    QString defaultProjectDir;
    if (isGreenEdition()) {
        defaultProjectDir = includeTrailingPathDelimiter(appDir()) + "projects";
    } else {
        defaultProjectDir = includeTrailingPathDelimiter(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0])
                         + "projects";
    }
    mProjectDir = stringValue("projectDir",defaultProjectDir);
}

void Settings::Dirs::setProjectDir(const QString &newProjectDir)
{
    mProjectDir = newProjectDir;
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

void Settings::_Base::remove(const QString &key)
{
    mSettings->remove(key);
}

void Settings::_Base::saveValue(const QString &key, const QVariant &value)
{
    mSettings->saveValue(key,value);
}

void Settings::_Base::saveValue(const QString &key, const QSet<QString> &set)
{
    QStringList val;
    foreach(const QString& s,set) {
        val.append(s);
    }
    mSettings->saveValue(key,val);
}

QVariant Settings::_Base::value(const QString &key, const QVariant &defaultValue)
{
    return mSettings->value(key,defaultValue);
}

bool Settings::_Base::boolValue(const QString &key, bool defaultValue)
{
    return value(key,defaultValue).toBool();
}

QSize Settings::_Base::sizeValue(const QString &key, const QSize& size)
{
    return value(key,size).toSize();
}

int Settings::_Base::intValue(const QString &key, int defaultValue)
{
    return value(key,defaultValue).toInt();
}

double Settings::_Base::doubleValue(const QString &key, double defaultValue)
{
    return value(key,defaultValue).toDouble();
}

unsigned int Settings::_Base::uintValue(const QString &key, unsigned int defaultValue)
{
    return value(key,defaultValue).toUInt();
}

QStringList Settings::_Base::stringListValue(const QString &key, const QStringList &defaultValue)
{
    return value(key,defaultValue).toStringList();
}

QSet<QString> Settings::_Base::stringSetValue(const QString &key)
{
    QStringList lst=value(key,QStringList()).toStringList();
    QSet<QString> result;
    foreach(const QString& s, lst)
        result.insert(s);
    return result;
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

bool Settings::Editor::highlightMathingBraces() const
{
    return mHighlightMathingBraces;
}

void Settings::Editor::setHighlightMathingBraces(bool newHighlightMathingBraces)
{
    mHighlightMathingBraces = newHighlightMathingBraces;
}

bool Settings::Editor::enableLigaturesSupport() const
{
    return mEnableLigaturesSupport;
}

void Settings::Editor::setEnableLigaturesSupport(bool newEnableLigaturesSupport)
{
    mEnableLigaturesSupport = newEnableLigaturesSupport;
}

QStringList Settings::Editor::fontFamilies() const
{
    return mFontFamilies;
}

void Settings::Editor::setFontFamilies(const QStringList &newFontFamilies)
{
    mFontFamilies = newFontFamilies;
}

QStringList Settings::Editor::fontFamiliesWithControlFont() const
{
    return mFontFamilies + QStringList{"Red Panda Control"};
}

int Settings::Editor::mouseSelectionScrollSpeed() const
{
    return mMouseSelectionScrollSpeed;
}

void Settings::Editor::setMouseSelectionScrollSpeed(int newMouseSelectionScrollSpeed)
{
    mMouseSelectionScrollSpeed = newMouseSelectionScrollSpeed;
}

bool Settings::Editor::autoDetectFileEncoding() const
{
    return mAutoDetectFileEncoding;
}

void Settings::Editor::setAutoDetectFileEncoding(bool newAutoDetectFileEncoding)
{
    mAutoDetectFileEncoding = newAutoDetectFileEncoding;
}

bool Settings::Editor::autoFormatWhenSaved() const
{
    return mAutoFormatWhenSaved;
}

void Settings::Editor::setAutoFormatWhenSaved(bool newAutoFormatWhenSaved)
{
    mAutoFormatWhenSaved = newAutoFormatWhenSaved;
}

bool Settings::Editor::parseTodos() const
{
    return mParseTodos;
}

void Settings::Editor::setParseTodos(bool newParseTodos)
{
    mParseTodos = newParseTodos;
}

const QStringList &Settings::Editor::customCTypeKeywords() const
{
    return mCustomCTypeKeywords;
}

void Settings::Editor::setCustomCTypeKeywords(const QStringList &newCustomTypeKeywords)
{
    mCustomCTypeKeywords = newCustomTypeKeywords;
}

bool Settings::Editor::enableCustomCTypeKeywords() const
{
    return mEnableCustomCTypeKeywords;
}

void Settings::Editor::setEnableCustomCTypeKeywords(bool newEnableCustomCTypeKeywords)
{
    mEnableCustomCTypeKeywords = newEnableCustomCTypeKeywords;
}

bool Settings::Editor::removeTrailingSpacesWhenSaved() const
{
    return mRemoveTrailingSpacesWhenSaved;
}

void Settings::Editor::setRemoveTrailingSpacesWhenSaved(bool newRemoveTrailingSpacesWhenSaved)
{
    mRemoveTrailingSpacesWhenSaved = newRemoveTrailingSpacesWhenSaved;
}

double Settings::Editor::lineSpacing() const
{
    return mLineSpacing;
}

void Settings::Editor::setLineSpacing(double newLineSpacing)
{
    mLineSpacing = newLineSpacing;
}

bool Settings::Editor::showLeadingSpaces() const
{
    return mShowLeadingSpaces;
}

void Settings::Editor::setShowLeadingSpaces(bool newShowStartSpaces)
{
    mShowLeadingSpaces = newShowStartSpaces;
}

bool Settings::Editor::enableEditTempBackup() const
{
    return mEnableEditTempBackup;
}

void Settings::Editor::setEnableEditTempBackup(bool newEnableEditTempBackup)
{
    mEnableEditTempBackup = newEnableEditTempBackup;
}

int Settings::Editor::tipsDelay() const
{
    return mTipsDelay;
}

void Settings::Editor::setTipsDelay(int newTipsDelay)
{
    mTipsDelay = newTipsDelay;
}

bool Settings::Editor::forceFixedFontWidth() const
{
    return mForceFixedFontWidth;
}

void Settings::Editor::setForceFixedFontWidth(bool newForceFixedWidth)
{
    mForceFixedFontWidth = newForceFixedWidth;
}

bool Settings::Editor::copyHTMLRecalcLineNumber() const
{
    return mCopyHTMLRecalcLineNumber;
}

void Settings::Editor::setCopyHTMLRecalcLineNumber(bool newCopyHTMLRecalcLineNumber)
{
    mCopyHTMLRecalcLineNumber = newCopyHTMLRecalcLineNumber;
}

bool Settings::Editor::copyHTMLWithLineNumber() const
{
    return mCopyHTMLWithLineNumber;
}

void Settings::Editor::setCopyHTMLWithLineNumber(bool newCopyHTMLWithLineNumber)
{
    mCopyHTMLWithLineNumber = newCopyHTMLWithLineNumber;
}

bool Settings::Editor::showTrailingSpaces() const
{
    return mShowTrailingSpaces;
}

void Settings::Editor::setShowTrailingSpaces(bool newShowEndSpaces)
{
    mShowTrailingSpaces = newShowEndSpaces;
}

bool Settings::Editor::showInnerSpaces() const
{
    return mShowInnerSpaces;
}

void Settings::Editor::setShowInnerSpaces(bool newShowMiddleSpaces)
{
    mShowInnerSpaces = newShowMiddleSpaces;
}

bool Settings::Editor::showLineBreaks() const
{
    return mShowLineBreaks;
}

void Settings::Editor::setShowLineBreaks(bool newShowLineBreaks)
{
    mShowLineBreaks = newShowLineBreaks;
}

bool Settings::Editor::highlightCurrentWord() const
{
    return mHighlightCurrentWord;
}

void Settings::Editor::setHighlightCurrentWord(bool newHighlightCurrentWord)
{
    mHighlightCurrentWord = newHighlightCurrentWord;
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
    return mFontFamilies.length() > 0 ? mFontFamilies[0] : "";
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

void Settings::Editor::doLoad()
{

    // indents
    mAutoIndent = boolValue("auto_indent", true);
    mTabToSpaces = boolValue("tab_to_spaces",false);
    mTabWidth = intValue("tab_width",4);
    mShowIndentLines = boolValue("show_indent_lines",true);
    mIndentLineColor = colorValue("indent_line_color",Qt::lightGray);
    mfillIndents = boolValue("fill_indents", false);
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

QSynedit::EditCaretType Settings::Editor::caretForOverwrite() const
{
    return mCaretForOverwrite;
}

void Settings::Editor::setCaretForOverwrite(const QSynedit::EditCaretType &caretForOverwrite)
{
    mCaretForOverwrite = caretForOverwrite;
}

QSynedit::EditCaretType Settings::Editor::caretForInsert() const
{
    return mCaretForInsert;
}

void Settings::Editor::setCaretForInsert(const QSynedit::EditCaretType &caretForInsert)
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

Settings::CompilerSet::CompilerSet():
    mFullLoaded{false},
    mCompilerType{CompilerType::Unknown},
    mAutoAddCharsetParams{false},
    mExecCharset{ENCODING_SYSTEM_DEFAULT},
    mStaticLink{false},
    mPersistInAutoFind{false},
    mForceEnglishOutput{false},
    mPreprocessingSuffix{DEFAULT_PREPROCESSING_SUFFIX},
    mCompilationProperSuffix{DEFAULT_COMPILATION_SUFFIX},
    mAssemblingSuffix{DEFAULT_ASSEMBLING_SUFFIX},
    mExecutableSuffix{DEFAULT_EXECUTABLE_SUFFIX},
    mCompilationStage{Settings::CompilerSet::CompilationStage::GenerateExecutable}
{

}


Settings::CompilerSet::CompilerSet(const QString& compilerFolder, const QString& c_prog):
    mAutoAddCharsetParams{true},
    mExecCharset{ENCODING_SYSTEM_DEFAULT},
    mStaticLink{true},
    mPersistInAutoFind{false},
    mForceEnglishOutput{false},
    mPreprocessingSuffix{DEFAULT_PREPROCESSING_SUFFIX},
    mCompilationProperSuffix{DEFAULT_COMPILATION_SUFFIX},
    mAssemblingSuffix{DEFAULT_ASSEMBLING_SUFFIX},
    mExecutableSuffix{DEFAULT_EXECUTABLE_SUFFIX},
    mCompilationStage{Settings::CompilerSet::CompilationStage::GenerateExecutable}
{
    QDir dir(compilerFolder);
    if (dir.exists(c_prog)) {

        setProperties(compilerFolder,c_prog);

        if (mName.isEmpty()) {
            mFullLoaded = false;
            return;
        }

        //manually set the directories
        setDirectories(compilerFolder);

        setExecutables();

        setUserInput();

#ifdef ENABLE_SDCC
        if (mCompilerType == CompilerType::SDCC) {
            mExecutableSuffix = SDCC_HEX_SUFFIX;
        }
#endif
        mFullLoaded = true;
    } else {
        mFullLoaded = false;
    }
}

Settings::CompilerSet::CompilerSet(const Settings::CompilerSet &set):
    mFullLoaded{set.mFullLoaded},
    mCCompiler{set.mCCompiler},
    mCppCompiler{set.mCppCompiler},
    mMake{set.mMake},
    mDebugger{set.mDebugger},
    mResourceCompiler{set.mResourceCompiler},
    mDebugServer{set.mDebugServer},

    mBinDirs{set.mBinDirs},
    mCIncludeDirs{set.mCIncludeDirs},
    mCppIncludeDirs{set.mCppIncludeDirs},
    mLibDirs{set.mLibDirs},
    mDefaultLibDirs{set.mDefaultLibDirs},
    mDefaultCIncludeDirs{set.mDefaultCIncludeDirs},
    mDefaultCppIncludeDirs{set.mDefaultCppIncludeDirs},

    mDumpMachine{set.mDumpMachine},
    mVersion{set.mVersion},
    mName{set.mName},
    mTarget{set.mTarget},
    mCompilerType{set.mCompilerType},


    mUseCustomCompileParams{set.mUseCustomCompileParams},
    mUseCustomLinkParams{set.mUseCustomLinkParams},
    mCustomCompileParams{set.mCustomCompileParams},
    mCustomLinkParams{set.mCustomLinkParams},
    mAutoAddCharsetParams{set.mAutoAddCharsetParams},
    mExecCharset{set.mExecCharset},
    mStaticLink{set.mStaticLink},
    mPersistInAutoFind{set.mPersistInAutoFind},
    mForceEnglishOutput{set.mForceEnglishOutput},

    mPreprocessingSuffix{set.mPreprocessingSuffix},
    mCompilationProperSuffix{set.mCompilationProperSuffix},
    mAssemblingSuffix{set.mAssemblingSuffix},
    mExecutableSuffix{set.mExecutableSuffix},
    mCompilationStage{set.mCompilationStage},
    mCompileOptions{set.mCompileOptions}
{

}

Settings::CompilerSet::CompilerSet(const QJsonObject &set) :
    mFullLoaded{true},
    mCCompiler{set["cCompiler"].toString()},
    mCppCompiler{set["cxxCompiler"].toString()},
    mMake{set["make"].toString()},
    mDebugger{set["debugger"].toString()},
    mResourceCompiler{set["resourceCompiler"].toString()},
    mDebugServer{set["debugServer"].toString()},

    mBinDirs{},               // handle later
    mCIncludeDirs{},          // handle later
    mCppIncludeDirs{},        // handle later
    mLibDirs{},               // handle later
    mDefaultLibDirs{},        // handle later
    mDefaultCIncludeDirs{},   // handle later
    mDefaultCppIncludeDirs{}, // handle later

    mDumpMachine{set["dumpMachine"].toString()},
    mVersion{set["version"].toString()},
    mName{set["name"].toString()},
    mTarget{set["target"].toString()},
    mCompilerType{}, // handle later

    mUseCustomCompileParams{!set["customCompileParams"].toArray().isEmpty()},
    mUseCustomLinkParams{!set["customLinkParams"].toArray().isEmpty()},
    mCustomCompileParams{}, // handle later
    mCustomLinkParams{},    // handle later
    mAutoAddCharsetParams{!set["execCharset"].toString().isEmpty()},
    mExecCharset{}, // handle later
    mStaticLink{set["staticLink"].toBool()},
    mPersistInAutoFind{false},
    mForceEnglishOutput{false},

    mPreprocessingSuffix{set["preprocessingSuffix"].toString()},
    mCompilationProperSuffix{set["compilationProperSuffix"].toString()},
    mAssemblingSuffix{set["assemblingSuffix"].toString()},
    mExecutableSuffix{set["executableSuffix"].toString()},
    mCompilationStage{CompilationStage(set["compilationStage"].toInt())},
    mCompileOptions{} // handle later
{
    for (const QJsonValue &dir : set["binDirs"].toArray())
        mBinDirs.append(dir.toString());
    for (const QJsonValue &dir : set["cIncludeDirs"].toArray())
        mCIncludeDirs.append(dir.toString());
    for (const QJsonValue &dir : set["cxxIncludeDirs"].toArray())
        mCppIncludeDirs.append(dir.toString());
    for (const QJsonValue &dir : set["libDirs"].toArray())
        mLibDirs.append(dir.toString());
    for (const QJsonValue &dir : set["defaultLibDirs"].toArray())
        mDefaultLibDirs.append(dir.toString());
    for (const QJsonValue &dir : set["defaultCIncludeDirs"].toArray())
        mDefaultCIncludeDirs.append(dir.toString());
    for (const QJsonValue &dir : set["defaultCxxIncludeDirs"].toArray())
        mDefaultCppIncludeDirs.append(dir.toString());

    QString compilerType = set["compilerType"].toString();
    if (compilerType == "GCC") {
        mCompilerType = CompilerType::GCC;
    } else if (compilerType == "GCC_UTF8") {
        mCompilerType = CompilerType::GCC_UTF8;
    } else if (compilerType == "Clang") {
        mCompilerType = CompilerType::Clang;
    }
#if ENABLE_SDCC
    else if (compilerType == "SDCC") {
        mCompilerType = CompilerType::SDCC;
    }
#endif
    else {
        mCompilerType = CompilerType::Unknown;
        mFullLoaded = false;
    }

    QStringList compileParams;
    for (const QJsonValue &param : set["customCompileParams"].toArray())
        compileParams << param.toString();
    mCustomCompileParams = escapeArgumentsForInputField(compileParams);
    QStringList linkParams;
    for (const QJsonValue &param : set["customLinkParams"].toArray())
        linkParams << param.toString();
    mCustomLinkParams = escapeArgumentsForInputField(linkParams);

    if (!mAutoAddCharsetParams)
        mExecCharset = "UTF-8";
    else
        mExecCharset = set["execCharset"].toString();

    const static QMap<QString, QString> optionMap = {
                                                     {CC_CMD_OPT_OPTIMIZE, "ccCmdOptOptimize"},
                                                     {CC_CMD_OPT_STD, "ccCmdOptStd"},
                                                     {C_CMD_OPT_STD, "cCmdOptStd"},
                                                     {CC_CMD_OPT_INSTRUCTION, "ccCmdOptInstruction"},

                                                     {CC_CMD_OPT_POINTER_SIZE, "ccCmdOptPointerSize"},

                                                     {CC_CMD_OPT_DEBUG_INFO, "ccCmdOptDebugInfo"},
                                                     {CC_CMD_OPT_PROFILE_INFO, "ccCmdOptProfileInfo"},
                                                     {CC_CMD_OPT_SYNTAX_ONLY, "ccCmdOptSyntaxOnly"},

                                                     {CC_CMD_OPT_INHIBIT_ALL_WARNING, "ccCmdOptInhibitAllWarning"},
                                                     {CC_CMD_OPT_WARNING_ALL, "ccCmdOptWarningAll"},
                                                     {CC_CMD_OPT_WARNING_EXTRA, "ccCmdOptWarningExtra"},
                                                     {CC_CMD_OPT_CHECK_ISO_CONFORMANCE, "ccCmdOptCheckIsoConformance"},
                                                     {CC_CMD_OPT_WARNING_AS_ERROR, "ccCmdOptWarningAsError"},
                                                     {CC_CMD_OPT_ABORT_ON_ERROR, "ccCmdOptAbortOnError"},
                                                     {CC_CMD_OPT_STACK_PROTECTOR, "ccCmdOptStackProtector"},
                                                     {CC_CMD_OPT_ADDRESS_SANITIZER, "ccCmdOptAddressSanitizer"},

                                                     {CC_CMD_OPT_USE_PIPE, "ccCmdOptUsePipe"},
                                                     {LINK_CMD_OPT_NO_LINK_STDLIB, "linkCmdOptNoLinkStdlib"},
                                                     {LINK_CMD_OPT_NO_CONSOLE, "linkCmdOptNoConsole"},
                                                     {LINK_CMD_OPT_STRIP_EXE, "linkCmdOptStripExe"},
                                                     };
    for (const QString &key : optionMap.keys()) {
        const QString &jsonKey = optionMap[key];
        QString value = set[jsonKey].toString();
        if (!value.isEmpty())
            setCompileOption(key, value);
    }
}

void Settings::CompilerSet::resetCompileOptionts()
{
      mCompileOptions.clear();
}

bool Settings::CompilerSet::setCompileOption(const QString &key, int valIndex)
{
    PCompilerOption op = CompilerInfoManager::getCompilerOption(mCompilerType, key);
    if (!op)
        return false;
    if (op->choices.isEmpty()) {
        if (valIndex==1)
            mCompileOptions.insert(key,COMPILER_OPTION_ON);
        else
            mCompileOptions.remove(key);
        return true;
    } else if (valIndex>0 && valIndex <= op->choices.length()) {
        mCompileOptions.insert(key,op->choices[valIndex-1].second);
        return true;
    } else {
        mCompileOptions.remove(key);
        return true;
    }
    return false;
}

bool Settings::CompilerSet::setCompileOption(const QString &key, const QString &value)
{
    PCompilerOption op = CompilerInfoManager::getCompilerOption(mCompilerType,key);
    if (!op)
        return false;
    mCompileOptions.insert(key,value);
    return true;
}

void Settings::CompilerSet::unsetCompileOption(const QString &key)
{
    mCompileOptions.remove(key);
}

void Settings::CompilerSet::setCompileOptions(const QMap<QString, QString> options)
{
    mCompileOptions=options;
}

QString Settings::CompilerSet::getCompileOptionValue(const QString &key) const
{
    return mCompileOptions.value(key,QString());
}

//static void checkDirs(const QStringList& dirlist, QString& gooddirs, QString& baddirs) {
//    gooddirs = "";
//    baddirs = "";

//    for (int i=0; i<dirlist.count();i++) {
//        QDir dir(dirlist[i]);
//        if (!dir.exists()) {
//            if (baddirs.isEmpty()) {
//                baddirs = dirlist[i];
//            } else {
//                baddirs += ";" + dirlist[i];
//            }
//        } else {
//            if (gooddirs.isEmpty()) {
//                gooddirs = dirlist[i];
//            } else {
//                gooddirs += ";" + dirlist[i];
//            }
//        }
//    }
//}


//bool Settings::CompilerSet::dirsValid(QString &msg)
//{
//    QString goodbin, badbin, goodlib, badlib, goodinc, badinc, goodinccpp, badinccpp;
//    msg = "";

//    if (mBinDirs.count()>0) {// we need some bin dir, so treat count=0 as an error too
//        checkDirs(mBinDirs,goodbin,badbin);
//        if (!badbin.isEmpty()) {
//            msg += QObject::tr("The following %1 directories don't exist:").arg(
//                        QObject::tr("binary")
//                        );
//            msg += "<br />";
//            msg += badbin.replace(';',"<br />");
//            msg += "<br />";
//            msg += "<br />";
//            return false;
//        }
//    } else {
//        msg += QObject::tr("No %1 directories have been specified.").arg(
//                    QObject::tr("binary")
//                    );
//        msg += "<br />";
//        msg += "<br />";
//        return false;
//    }
//    checkDirs(mCIncludeDirs,goodbin,badbin);
//    if (!badbin.isEmpty()) {
//        msg += QObject::tr("The following %1 directories don't exist:").arg(
//                    QObject::tr("C include")
//                    );
//        msg += "<br />";
//        msg += badbin.replace(';',"<br />");
//        msg += "<br />";
//        msg += "<br />";
//        return false;
//    }

//    checkDirs(mCppIncludeDirs,goodbin,badbin);
//    if (!badbin.isEmpty()) {
//        msg += QObject::tr("The following %1 directories don't exist:").arg(
//                    QObject::tr("C++ include")
//                    );
//        msg += "<br />";
//        msg += badbin.replace(';',"<br />");
//        msg += "<br />";
//        msg += "<br />";
//        return false;
//    }

//    checkDirs(mLibDirs,goodbin,badbin);
//    if (!badbin.isEmpty()) {
//        msg += QObject::tr("The following %1 directories don't exist:").arg(
//                    QObject::tr("C++ include")
//                    );
//        msg += "<br />";
//        msg += badbin.replace(';',"<br />");
//        msg += "<br />";
//        msg += "<br />";
//        return false;
//    }

//    if (!msg.isEmpty())
//        return false;
//    else
//        return true;
//}

//bool Settings::CompilerSet::validateExes(QString &msg)
//{
//    msg ="";
//    if (!fileExists(mCCompiler)) {
//        msg += QObject::tr("Cannot find the %1 \"%2\"")
//                .arg(QObject::tr("C Compiler"))
//                .arg(mCCompiler);
//    }
//    if (!fileExists(mCppCompiler)) {
//        msg += QObject::tr("Cannot find the %1 \"%2\"")
//                .arg(QObject::tr("C++ Compiler"))
//                .arg(mCppCompiler);
//    }
//    if (!mMake.isEmpty() && !fileExists(mMake)) {
//        msg += QObject::tr("Cannot find the %1 \"%2\"")
//                .arg(QObject::tr("Maker"))
//                .arg(mMake);
//    }
//    if (!fileExists(mDebugger)) {
//        msg += QObject::tr("Cannot find the %1 \"%2\"")
//                .arg(QObject::tr("Debugger"))
//                .arg(mDebugger);
//    }
//    if (!msg.isEmpty())
//        return false;
//    else
//        return true;
//}

const QString &Settings::CompilerSet::CCompiler() const
{
    return mCCompiler;
}

void Settings::CompilerSet::setCCompiler(const QString &name)
{
    if (mCCompiler!=name) {
        mCCompiler = name;
        if (mCompilerType == CompilerType::Unknown) {
            QString temp=extractFileName(mCCompiler);
            if (temp == CLANG_PROGRAM) {
                setCompilerType(CompilerType::Clang);
            } else if (temp == GCC_PROGRAM) {
                setCompilerType(CompilerType::GCC);
            }
        }
    }
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
    if (!mFullLoaded && !binDirs().isEmpty()) {
        mFullLoaded=true;
        setDirectories(binDirs()[0]);
    }
    return mDefaultCIncludeDirs;
}

QStringList &Settings::CompilerSet::defaultCppIncludeDirs()
{
    if (!mFullLoaded && !binDirs().isEmpty()) {
        mFullLoaded=true;
        setDirectories(binDirs()[0]);
    }
    return mDefaultCppIncludeDirs;
}

QStringList &Settings::CompilerSet::defaultLibDirs()
{
    if (!mFullLoaded && !binDirs().isEmpty()) {
        mFullLoaded=true;
        setDirectories(binDirs()[0]);
    }
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

const QString &Settings::CompilerSet::name() const
{
    return mName;
}

void Settings::CompilerSet::setName(const QString &value)
{
    mName = value;
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

void Settings::CompilerSet::setProperties(const QString& binDir, const QString& c_prog)
{
#ifdef ENABLE_SDCC
    if (c_prog == SDCC_PROGRAM) {
        setSDCCProperties(binDir,c_prog);
    } else {
#endif

        setGCCProperties(binDir,c_prog);
#ifdef ENABLE_SDCC
    }
#endif
}

void Settings::CompilerSet::setGCCProperties(const QString& binDir, const QString& c_prog)
{
    // We have tested before the call
//    if (!fileExists(c_prog))
//        return;

    // Obtain basic info
    QByteArray dumpMachine = getCompilerOutput(binDir, c_prog, {"-dumpmachine"});
    mDumpMachine = QString(dumpMachine).trimmed();
    if (mDumpMachine.isEmpty())
        // unknown binary
        return;
    if (mDumpMachine == "mingw32") {
        // MinGW.org uses bare `mingw32` “triplet”, not conforming to GCC's document.
        // here we change it to MinGW-w64’s compatibility mode triplet.
        // ref 1. https://gcc.gnu.org/install/specific.html
        // ref 2. https://sourceforge.net/p/mingw-w64/wiki2/Feature%20list/
        mDumpMachine = "i386-pc-mingw32";
    }
    mTarget = mDumpMachine.mid(0, mDumpMachine.indexOf('-'));
    QByteArray version = getCompilerOutput(binDir, c_prog, {"-dumpversion"});
    mVersion = QString(version).trimmed();

    // Obtain compiler distro
    QByteArray verboseOut = getCompilerOutput(binDir, c_prog, {"-v"});
    QByteArray targetStr = "clang version ";
    int clangVersionPos = verboseOut.indexOf(targetStr);
    if (clangVersionPos >= 0) {
        mCompilerType = CompilerType::Clang;
        QRegularExpression ntPosixPattern = QRegularExpression("^(.*)-pc-windows-msys$");
        QRegularExpression mingwW64Pattern = QRegularExpression("^(.*)-w64-windows-gnu$");
        if (mName.isEmpty()) {
            if (auto m = ntPosixPattern.match(mDumpMachine); m.hasMatch()) {
                mName = "MSYS2 Clang " + mVersion;
            } else if (m = mingwW64Pattern.match(mDumpMachine); m.hasMatch()) {
                mName = "MinGW-w64 Clang " + mVersion;
            } else {
                mName = "Clang " + mVersion;
            }
        }
    } else {
        mCompilerType = CompilerType::GCC;
        QRegularExpression ntPosixPattern = QRegularExpression("^(.*)-(.*)-(msys|cygwin)$");
        QRegularExpression mingwW64Pattern = QRegularExpression("^(.*)-w64-mingw32$");
        QRegularExpression mingwOrgPattern("^(.*)-(.*)-mingw32$");
        if (auto m = ntPosixPattern.match(mDumpMachine); m.hasMatch()) {
            mCompilerType = CompilerType::GCC_UTF8;
            if (mName.isEmpty()) {
                if (m.capturedTexts()[3] == "msys")
                    mName = "MSYS2 GCC " + mVersion;
                else
                    mName = "Cygwin GCC " + mVersion;
            }
        }
        // Assemble user friendly name if we don't have one yet
        if (mName.isEmpty()) {
            if (auto m = mingwW64Pattern.match(mDumpMachine); m.hasMatch()) {
                mName = "MinGW-w64 GCC " + mVersion;
            } else if (m = mingwOrgPattern.match(mDumpMachine); m.hasMatch()) {
                mName = "MinGW.org GCC " + mVersion;
            } else {
                mName = "GCC " + mVersion;
            }
        }
    }

    // Set compiler folder
    QDir tmpDir(binDir);
    tmpDir.cdUp();
    QString folder = tmpDir.path();

    // Add the default directories
    addExistingDirectory(mBinDirs, includeTrailingPathDelimiter(folder) +  "bin");
    if (!mDumpMachine.isEmpty()) {
        //mingw-w64 bin folder
        addExistingDirectory(mBinDirs,
            includeTrailingPathDelimiter(folder) + "lib/"
            "gcc/" + mDumpMachine
            + "/" + mVersion);
    }
}

#ifdef ENABLE_SDCC
void Settings::CompilerSet::setSDCCProperties(const QString& binDir, const QString& c_prog)
{
    // We have tested before the call
//    if (!fileExists(c_prog))
//        return;
    // Obtain version number and compiler distro etc
    QStringList arguments;
    arguments.append("-v");
    QByteArray output = getCompilerOutput(binDir, c_prog,arguments);

    if (!output.startsWith("SDCC"))
        return;

    //Target
    int delimPos = 0;
    while (delimPos<output.length() && (output[delimPos]>=((char)32)))
        delimPos++;
    QString triplet = output.mid(0,delimPos);

    QRegularExpression re("\\s+(\\d+\\.\\d+\\.\\d+)\\s+");
    QRegularExpressionMatch match = re.match(triplet);
    if (match.hasMatch())
        mVersion = match.captured(1);
    if (mVersion.isEmpty())
        mName = "SDCC";
    else
        mName = "SDCC "+mVersion;
    mCompilerType=CompilerType::SDCC;

    addExistingDirectory(mBinDirs, binDir);
}
#endif

QStringList Settings::CompilerSet::x86MultilibList(const QString &folder, const QString &c_prog) const
{
    QByteArray multilibOutput = getCompilerOutput(folder, c_prog, {"-print-multi-lib"});
    QStringList result;
    for (QByteArray rawLine : multilibOutput.split('\n')) {
        // man gcc:
        //   -print-multi-lib
        //     Print the mapping from multilib directory names to compiler switches that enable them.
        //     The directory name is separated from the switches by `;`, and each switch starts with
        //     an `@` instead of the `-`, without spaces between multiple switches.

        // Example 1 (GCC):
        //   .;
        //   32;@m32

        // Example 2 (GCC Debian):
        //   .;
        //   32;@m32
        //   x32;@mx32

        // Example 3 (Clang):
        //   .;@m64
        //   32;@m32

        QString line = QString(rawLine).trimmed();
        int sep = line.indexOf(';');
        if (sep < 0)
            continue;
        QString dir = line.left(sep);
        if (dir == ".")
            // native ABI
            continue;
        QString switches = line.mid(sep+1);
        if (switches == "@m32")
            result.append("32");
        else if (switches == "@mx32")
            result.append("x32");
        else if (switches == "@m64")
            // possible for Debian x32 port
            result.append("64");
    }
    return result;
}

QStringList Settings::CompilerSet::defines(bool isCpp) {
    // get default defines
    QStringList arguments;
    arguments.append("-dM");
    arguments.append("-E");
    arguments.append("-x");
    QString key;
#ifdef ENABLE_SDCC
    if (mCompilerType==CompilerType::SDCC) {
        arguments.append("c");
        arguments.append("-V");
        key=SDCC_CMD_OPT_PROCESSOR;
        //language standard
        PCompilerOption pOption = CompilerInfoManager::getCompilerOption(compilerType(), key);
        if (pOption) {
            if (!mCompileOptions[key].isEmpty())
                arguments.append(pOption->setting + mCompileOptions[key]);
        }
        key=SDCC_CMD_OPT_STD;
        //language standard
        pOption = CompilerInfoManager::getCompilerOption(compilerType(), key);
        if (pOption) {
            if (!mCompileOptions[key].isEmpty())
                arguments.append(pOption->setting + mCompileOptions[key]);
        }
    } else {
#endif
        if (isCpp) {
            arguments.append("c++");
            key=CC_CMD_OPT_STD;
        } else {
            arguments.append("c");
            key=C_CMD_OPT_STD;
        }
        //language standard
        PCompilerOption pOption = CompilerInfoManager::getCompilerOption(compilerType(), key);
        if (pOption) {
            if (!mCompileOptions[key].isEmpty())
                arguments.append(pOption->setting + mCompileOptions[key]);
        }
        pOption = CompilerInfoManager::getCompilerOption(compilerType(), CC_CMD_OPT_DEBUG_INFO);
        if (pOption && mCompileOptions.contains(CC_CMD_OPT_DEBUG_INFO)) {
            arguments.append(pOption->setting);
        }
#ifdef ENABLE_SDCC
    }
#endif

    if (mUseCustomCompileParams) {
        QStringList extraParams = parseArgumentsWithoutVariables(mCustomCompileParams);
        arguments.append(extraParams);
    }
    if (arguments.contains("-g3"))
        arguments.append("-D_DEBUG");
    arguments.append(NULL_FILE);

    QFileInfo ccompiler(mCCompiler);
    QByteArray output = getCompilerOutput(ccompiler.absolutePath(),ccompiler.fileName(),arguments);
    // 'cpp.exe -dM -E -x c++ -std=c++17 NUL'
//    qDebug()<<"------------------";
    QStringList result;
#ifdef ENABLE_SDCC
    if (mCompilerType==CompilerType::SDCC) {
        QList<QByteArray> lines = output.split('\n');
        QByteArray currentLine;
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (trimmedLine.startsWith("+")) {
                currentLine = line;
                break;
            }
        }
        lines = currentLine.split(' ');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (trimmedLine.startsWith("-D")) {
                trimmedLine = trimmedLine.mid(2);
                if (trimmedLine.contains("=")) {
                    QList<QByteArray> items=trimmedLine.split('=');
                    result.append(QString("#define %1 %2").arg(QString(items[0]),QString(items[1])));
                } else {
                    result.append("#define "+trimmedLine);
                }
            }
        }
    } else {
#else
    {
#endif
        QList<QByteArray> lines = output.split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                result.append(trimmedLine);
            }
        }
    }
    return result;
}

void Settings::CompilerSet::setExecutables()
{
    if (mCompilerType == CompilerType::Clang) {
        mCCompiler =  findProgramInBinDirs(CLANG_PROGRAM);
        mCppCompiler = findProgramInBinDirs(CLANG_CPP_PROGRAM);
        mDebugger = findProgramInBinDirs(GDB_PROGRAM);
        if (mDebugger.isEmpty()) {
            mDebugger = findProgramInBinDirs(LLDB_MI_PROGRAM);
            mDebugServer = findProgramInBinDirs(LLDB_SERVER_PROGRAM);
        } else {
            mDebugServer = findProgramInBinDirs(GDB_SERVER_PROGRAM);
        }
        if (mCCompiler.isEmpty())
            mCCompiler =  findProgramInBinDirs(GCC_PROGRAM);
        if (mCppCompiler.isEmpty())
            mCppCompiler = findProgramInBinDirs(GPP_PROGRAM);
#ifdef ENABLE_SDCC
    } else if (mCompilerType == CompilerType::SDCC) {
        mCCompiler =  findProgramInBinDirs(SDCC_PROGRAM);
        if (mCCompiler.isEmpty())
            mCCompiler =  findProgramInBinDirs(SDCC_PROGRAM);
#endif
    } else {
        mCCompiler =  findProgramInBinDirs(GCC_PROGRAM);
        mCppCompiler = findProgramInBinDirs(GPP_PROGRAM);
        mDebugger = findProgramInBinDirs(GDB_PROGRAM);
        mDebugServer = findProgramInBinDirs(GDB_SERVER_PROGRAM);
    }
    mMake = findProgramInBinDirs(MAKE_PROGRAM);
#ifdef Q_OS_WIN
    mResourceCompiler = findProgramInBinDirs(WINDRES_PROGRAM);
#endif
}

void Settings::CompilerSet::setDirectories(const QString& binDir)
{
#ifdef ENABLE_SDCC
    if (mCompilerType == CompilerType::SDCC) {
        setSDCCDirectories(binDir);
    } else {
#endif
        setGCCDirectories(binDir);
#ifdef ENABLE_SDCC
    }
#endif
}

void Settings::CompilerSet::setGCCDirectories(const QString& binDir)
{
    QString folder = QFileInfo(binDir).absolutePath();
    QString c_prog;
    if (mCompilerType==CompilerType::Clang)
        c_prog = CLANG_PROGRAM;
    else
        c_prog = GCC_PROGRAM;
    // Find default directories
    // C include dirs
    QStringList arguments;
    arguments.clear();
    arguments.append("-xc");
    arguments.append("-v");
    arguments.append("-E");
    arguments.append(NULL_FILE);
    QByteArray output = getCompilerOutput(binDir,c_prog,arguments);

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
    output = getCompilerOutput(binDir,c_prog,arguments);
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
    output = getCompilerOutput(binDir,c_prog,arguments);
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

#ifdef ENABLE_SDCC
void Settings::CompilerSet::setSDCCDirectories(const QString& binDir)
{
    QString folder = QFileInfo(binDir).absolutePath();
    QString c_prog = SDCC_PROGRAM;
    // Find default directories
    // C include dirs
    QStringList arguments;
    arguments.clear();
    arguments.append("--print-search-dirs");
    QString key = SDCC_CMD_OPT_PROCESSOR;
    PCompilerOption pOption = CompilerInfoManager::getCompilerOption(compilerType(), key);
    if (pOption) {
        if (!mCompileOptions[key].isEmpty())
            arguments.append(pOption->setting + mCompileOptions[key]);
    }
    QByteArray output = getCompilerOutput(binDir,c_prog,arguments);

    //bindirs
    QByteArray targetStr = QByteArray("programs:");
    int delimPos1 = output.indexOf(targetStr);
    int delimPos2 = output.indexOf("datadir:");
    if (delimPos1 >0 && delimPos2>delimPos1 ) {
        delimPos1 += targetStr.length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mBinDirs,trimmedLine);
            }
        }
    }

    targetStr = QByteArray("includedir:");
    delimPos1 = output.indexOf(targetStr);
    delimPos2 = output.indexOf("libdir:");
    if (delimPos1 >0 && delimPos2>delimPos1 ) {
        delimPos1 += targetStr.length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mDefaultCIncludeDirs,trimmedLine);
            }
        }
    }

    targetStr = QByteArray("libdir:");
    delimPos1 = output.indexOf(targetStr);
    delimPos2 = output.indexOf("libpath:");
    if (delimPos1 >0 && delimPos2>delimPos1 ) {
        delimPos1 += targetStr.length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mDefaultLibDirs,trimmedLine);
            }
        }
    }

}
#endif

int Settings::CompilerSet::mainVersion() const
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

bool Settings::CompilerSet::canCompileC() const
{
    return fileExists(mCCompiler);
}

bool Settings::CompilerSet::canCompileCPP() const
{
#ifdef ENABLE_SDCC
    if (mCompilerType==CompilerType::SDCC)
        return false;
#endif
    return fileExists(mCppCompiler);
}

bool Settings::CompilerSet::canMake() const
{
    return fileExists(mMake);
}

bool Settings::CompilerSet::canDebug() const
{
#ifdef ENABLE_SDCC
    if (mCompilerType==CompilerType::SDCC)
        return false;
#endif
    return fileExists(mDebugger);
}

void Settings::CompilerSet::setUserInput()
{
    mUseCustomCompileParams = false;
    mUseCustomLinkParams = false;
#ifdef ENABLE_SDCC
    if (mCompilerType==CompilerType::SDCC) {
        mAutoAddCharsetParams = false;
        mStaticLink = false;
    } else {
#else
    {
#endif
        mAutoAddCharsetParams = true;
        mStaticLink = true;
    }
}


QString Settings::CompilerSet::findProgramInBinDirs(const QString name) const
{
    for (const QString& dir : mBinDirs) {
        QFileInfo f(includeTrailingPathDelimiter(dir) + name);
        if (f.exists() && f.isExecutable()) {
            return f.absoluteFilePath();
        }
    }
    return QString();
}

void Settings::CompilerSet::setIniOptions(const QByteArray &value)
{
   if (value.isEmpty())
       return;
   mCompileOptions.clear();
   for (int i=0;i<value.length();i++) {
       QString key = pSettings->compilerSets().getKeyFromCompilerCompatibleIndex(i);
       setCompileOption(key,charToValue(value[i]));
   }
}

QByteArray Settings::CompilerSet::getCompilerOutput(const QString &binDir, const QString &binFile, const QStringList &arguments) const
{
    QProcessEnvironment env;
    env.insert("LANG","en");
    QString path = binDir;
    env.insert("PATH",path);
    auto [result, _, errorMessage] = runAndGetOutput(
                includeTrailingPathDelimiter(binDir)+binFile,
                binDir,
                arguments,
                QByteArray(),
                false,
                false,
                env);
    return result.trimmed();
}

bool Settings::CompilerSet::forceEnglishOutput() const
{
    return mForceEnglishOutput;
}

void Settings::CompilerSet::setForceEnglishOutput(bool newForceEnglishOutput)
{
    mForceEnglishOutput = newForceEnglishOutput;
}

bool Settings::CompilerSet::persistInAutoFind() const
{
    return mPersistInAutoFind;
}

void Settings::CompilerSet::setPersistInAutoFind(bool newPersistInAutoFind)
{
    mPersistInAutoFind = newPersistInAutoFind;
}

Settings::CompilerSet::CompilationStage Settings::CompilerSet::compilationStage() const
{
    return mCompilationStage;
}

void Settings::CompilerSet::setCompilationStage(CompilationStage newCompilationStage)
{
    mCompilationStage = newCompilationStage;
}

QString Settings::CompilerSet::getOutputFilename(const QString &sourceFilename)
{
    return getOutputFilename(sourceFilename, compilationStage());
}

QString Settings::CompilerSet::getOutputFilename(const QString &sourceFilename, CompilationStage stage)
{
    switch(stage) {
    case Settings::CompilerSet::CompilationStage::PreprocessingOnly:
        return changeFileExt(sourceFilename, preprocessingSuffix());
    case Settings::CompilerSet::CompilationStage::CompilationProperOnly:
        return changeFileExt(sourceFilename, compilationProperSuffix());
    case Settings::CompilerSet::CompilationStage::AssemblingOnly:
        return changeFileExt(sourceFilename, assemblingSuffix());
    case Settings::CompilerSet::CompilationStage::GenerateExecutable:
        return changeFileExt(sourceFilename, executableSuffix());
    }
    return changeFileExt(sourceFilename,DEFAULT_EXECUTABLE_SUFFIX);
}

bool Settings::CompilerSet::isOutputExecutable()
{
    return isOutputExecutable(mCompilationStage);
}

bool Settings::CompilerSet::isOutputExecutable(CompilationStage stage)
{
    return stage == CompilationStage::GenerateExecutable;
}

#ifdef Q_OS_WINDOWS
bool Settings::CompilerSet::isDebugInfoUsingUTF8() const
{
    switch(mCompilerType) {
    case CompilerType::Clang:
    case CompilerType::GCC_UTF8:
        return true;
    case CompilerType::GCC:
        return applicationIsUtf8(mCCompiler);
    default:
        return false;
    }
}

bool Settings::CompilerSet::forceUTF8() const
{
    return CompilerInfoManager::forceUTF8InDebugger(mCompilerType);
}

bool Settings::CompilerSet::isCompilerInfoUsingUTF8() const
{
    return isDebugInfoUsingUTF8();
}
#endif

const QString &Settings::CompilerSet::assemblingSuffix() const
{
    return mAssemblingSuffix;
}

void Settings::CompilerSet::setAssemblingSuffix(const QString &newAssemblingSuffix)
{
    mAssemblingSuffix = newAssemblingSuffix;
}

const QString &Settings::CompilerSet::compilationProperSuffix() const
{
    return mCompilationProperSuffix;
}

void Settings::CompilerSet::setCompilationProperSuffix(const QString &newCompilationProperSuffix)
{
    mCompilationProperSuffix = newCompilationProperSuffix;
}

const QString &Settings::CompilerSet::preprocessingSuffix() const
{
    return mPreprocessingSuffix;
}

void Settings::CompilerSet::setPreprocessingSuffix(const QString &newPreprocessingSuffix)
{
    mPreprocessingSuffix = newPreprocessingSuffix;
}

const QString &Settings::CompilerSet::executableSuffix() const
{
    return mExecutableSuffix;
}

void Settings::CompilerSet::setExecutableSuffix(const QString &newExecutableSuffix)
{
    mExecutableSuffix = newExecutableSuffix;
}

const QMap<QString, QString> &Settings::CompilerSet::compileOptions() const
{
    return mCompileOptions;
}

const QString &Settings::CompilerSet::execCharset() const
{
    return mExecCharset;
}

void Settings::CompilerSet::setExecCharset(const QString &newExecCharset)
{
    mExecCharset = newExecCharset;
}

const QString &Settings::CompilerSet::debugServer() const
{
    return mDebugServer;
}

void Settings::CompilerSet::setDebugServer(const QString &newDebugServer)
{
    mDebugServer = newDebugServer;
}

void Settings::CompilerSet::setCompilerType(CompilerType newCompilerType)
{
    mCompilerType = newCompilerType;
}

CompilerType Settings::CompilerSet::compilerType() const
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
    prepareCompatibleIndex();
}

Settings::PCompilerSet Settings::CompilerSets::addSet()
{
    PCompilerSet p=std::make_shared<CompilerSet>();
    mList.push_back(p);
    return p;
}

Settings::PCompilerSet Settings::CompilerSets::addSet(const QString &folder, const QString& c_prog)
{
    PCompilerSet p=std::make_shared<CompilerSet>(folder,c_prog);
    if (c_prog==GCC_PROGRAM && p->compilerType()==CompilerType::Clang)
        return PCompilerSet();
    mList.push_back(p);
    return p;
}

Settings::PCompilerSet Settings::CompilerSets::addSet(const PCompilerSet &pSet)
{
    PCompilerSet p=std::make_shared<CompilerSet>(*pSet);
    mList.push_back(p);
    return p;
}

Settings::PCompilerSet Settings::CompilerSets::addSet(const QJsonObject &set)
{
    PCompilerSet p = std::make_shared<CompilerSet>(set);
    mList.push_back(p);
    return p;
}

static void setX86MultilibOptions(Settings::PCompilerSet pSet, const QString &value) {
    pSet->setCompileOption(CC_CMD_OPT_POINTER_SIZE, value);
}

static void setReleaseOptions(Settings::PCompilerSet pSet) {
    pSet->setCompileOption(CC_CMD_OPT_OPTIMIZE,"2");
    pSet->setCompileOption(LINK_CMD_OPT_STRIP_EXE, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_USE_PIPE, COMPILER_OPTION_ON);
    pSet->setStaticLink(true);
}

static void setDebugOptions(Settings::PCompilerSet pSet, bool enableAsan = false) {
    //pSet->setCompileOption(CC_CMD_OPT_OPTIMIZE,"g");
    pSet->setCompileOption(CC_CMD_OPT_DEBUG_INFO, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_WARNING_ALL, COMPILER_OPTION_ON);
    //pSet->setCompileOption(CC_CMD_OPT_WARNING_EXTRA, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_USE_PIPE, COMPILER_OPTION_ON);

    if (enableAsan) {
#ifdef __aarch64__
        pSet->setCompileOption(CC_CMD_OPT_ADDRESS_SANITIZER, "hwaddress");
#else
        pSet->setCompileOption(CC_CMD_OPT_ADDRESS_SANITIZER, "address");
#endif
    }
    //Some windows gcc don't correctly support this
    //pSet->setCompileOption(CC_CMD_OPT_STACK_PROTECTOR, "-strong");
    pSet->setStaticLink(false);

}

bool Settings::CompilerSets::addSets(const QString &folder, const QString& c_prog) {
    foreach (const PCompilerSet& set, mList) {
        if (set->binDirs().contains(folder) && extractFileName(set->CCompiler())==c_prog)
            return false;
    }
    // Default, release profile
    PCompilerSet baseSet = addSet(folder,c_prog);
    if (!baseSet || baseSet->name().isEmpty())
        return false;
#if ENABLE_SDCC
    if (c_prog == SDCC_PROGRAM) {
        baseSet->setCompileOption(SDCC_OPT_NOSTARTUP,COMPILER_OPTION_ON);
    } else {
#else
    {
#endif
        QString baseName = baseSet->name();
        QString platformName;
        if (isTarget64Bit(baseSet->target())) {
            platformName = "64-bit";
        } else {
            platformName = "32-bit";
        }

        // handling x86 multilib
        if (baseSet->target() == "x86_64") {
            auto multilibs = baseSet->x86MultilibList(folder, c_prog);
            for (const QString &value : multilibs) {
                PCompilerSet set= addSet(baseSet);
                set->setName(baseName + " multilib " + value + " Release");
                setX86MultilibOptions(set, value);
                setReleaseOptions(set);

                set = addSet(baseSet);
                set->setName(baseName + " multilib " + value + " Debug");
                setX86MultilibOptions(set, value);
                setDebugOptions(set);
            }
        }


        PCompilerSet debugSet = addSet(baseSet);
        debugSet->setName(baseName + " " + platformName + " Debug");
        setDebugOptions(debugSet);

        // Enable ASan compiler set if it is supported and gdb works with ASan.
#ifdef Q_OS_LINUX
        PCompilerSet debugAsanSet = addSet(baseSet);
        debugAsanSet->setName(baseName + " " + platformName + " Debug with ASan");
        setDebugOptions(debugAsanSet, true);
#endif

        baseSet->setName(baseName + " " + platformName + " Release");
        setReleaseOptions(baseSet);
    }

#ifdef Q_OS_LINUX
# if defined(__x86_64__) || defined(__aarch64__) || __SIZEOF_POINTER__ == 4
    mDefaultIndex = (int)mList.size() - 1; // x86-64, AArch64 Linux or 32-bit Unix, default to "debug with ASan"
# else
    mDefaultIndex = (int)mList.size() - 2; // other Unix, where ASan can be very slow, default to "debug"
# endif
#else
    mDefaultIndex = (int)mList.size() - 1;
#endif

    return true;

}

bool Settings::CompilerSets::addSets(const QString &folder)
{
    bool found = false;
    if (!directoryExists(folder))
        return found;
    if (fileExists(folder, GCC_PROGRAM)) {
        addSets(folder,GCC_PROGRAM);
        found=true;
    }
    if (fileExists(folder, CLANG_PROGRAM)) {
        addSets(folder,CLANG_PROGRAM);
        found=true;
    }
#ifdef ENABLE_SDCC
    //qDebug()<<folder;
    if (fileExists(folder, SDCC_PROGRAM)) {
        addSets(folder,SDCC_PROGRAM);
        found=true;
    }
#endif
    return found;
}

Settings::CompilerSetList Settings::CompilerSets::clearSets()
{
    CompilerSetList persisted;
    for (size_t i=0;i<mList.size();i++) {
        mSettings->mSettings.beginGroup(QString(SETTING_COMPILTER_SET).arg(i));
        mSettings->mSettings.remove("");
        mSettings->mSettings.endGroup();
        if (mList[i]->persistInAutoFind())
            persisted.push_back(std::move(mList[i]));
    }
    mList.clear();
    mDefaultIndex = -1;
    return persisted;
}

void Settings::CompilerSets::findSets()
{
    CompilerSetList persisted = clearSets();
    // canonical paths that has been searched.
    // use canonical paths here to resolve symbolic links.
    QSet<QString> searched;

#ifdef ENABLE_LUA_ADDON
    QJsonObject compilerHint;
    if (
        QFile scriptFile(pSettings->dirs().appLibexecDir() + "/compiler_hint.lua");
        scriptFile.exists() && scriptFile.open(QFile::ReadOnly)
    ) {
        QByteArray script = scriptFile.readAll();
        try {
            compilerHint = AddOn::CompilerHintExecutor{}(script);
        } catch (const AddOn::LuaError &e) {
            QMessageBox::critical(nullptr,
                                  QObject::tr("Error executing platform compiler hint add-on"),
                                  e.reason());
        }
        if (!compilerHint.empty()) {
            QJsonArray compilerList = compilerHint["compilerList"].toArray();
            for (const QJsonValue &value : compilerList) {
                addSet(value.toObject());
            }
            QJsonArray noSearch = compilerHint["noSearch"].toArray();
            QString canonicalPath;
            for (const QJsonValue &value : noSearch) {
                canonicalPath = QDir(value.toString()).canonicalPath();
                if (!canonicalPath.isEmpty())
                    searched.insert(canonicalPath);
            }
        }
    }
#endif

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList pathList = path.split(PATH_SEPARATOR);
#ifdef Q_OS_WIN
    pathList = QStringList{
        mSettings->dirs().appDir() + "/clang64/bin",
        mSettings->dirs().appDir() + "/mingw64/bin",
        mSettings->dirs().appDir() + "/mingw32/bin",
    } + pathList;
#endif
    QString folder, canonicalFolder;
    for (int i=pathList.count()-1;i>=0;i--) {
        folder = QDir(pathList[i]).absolutePath();
        canonicalFolder = QDir(pathList[i]).canonicalPath();
        if (canonicalFolder.isEmpty())
            continue;
        if (searched.contains(canonicalFolder))
            continue;
        searched.insert(canonicalFolder);
        // but use absolute path to search so compiler set can survive system upgrades.
        // during search:
        //   /opt/gcc-13 -> /opt/gcc-13.1.0
        // after upgrade:
        //   /opt/gcc-13 -> /opt/gcc-13.2.0
        addSets(folder);
    }

#ifdef ENABLE_LUA_ADDON
    if (
        // note that array index starts from 1 in Lua
        int preferCompilerInLua = compilerHint["preferCompiler"].toInt();
        preferCompilerInLua >= 1 && preferCompilerInLua <= (int)mList.size()
    ) {
        mDefaultIndex = preferCompilerInLua - 1;
    }
#endif

    for (PCompilerSet &set: persisted)
        addSet(set);
}

void Settings::CompilerSets::saveSets()
{
    for (size_t i=0;i<mList.size();i++) {
        saveSet(i);
    }
    if (mDefaultIndex>=(int)mList.size()) {
        setDefaultIndex( mList.size()-1 );
    }
    mSettings->mSettings.beginGroup(SETTING_COMPILTER_SETS);
    mSettings->mSettings.setValue(SETTING_COMPILTER_SETS_DEFAULT_INDEX,mDefaultIndex);
    mSettings->mSettings.setValue(SETTING_COMPILTER_SETS_DEFAULT_INDEX_TIMESTAMP,mDefaultIndexTimeStamp);
    mSettings->mSettings.setValue(SETTING_COMPILTER_SETS_COUNT,(int)mList.size());

    mSettings->mSettings.endGroup();
}

void Settings::CompilerSets::loadSets()
{
    mList.clear();
    mSettings->mSettings.beginGroup(SETTING_COMPILTER_SETS);
    mDefaultIndex = mSettings->mSettings.value(SETTING_COMPILTER_SETS_DEFAULT_INDEX,-1).toInt();
    mDefaultIndexTimeStamp = mSettings->mSettings.value(SETTING_COMPILTER_SETS_DEFAULT_INDEX_TIMESTAMP,0).toLongLong();
    //fix error time
    if (mDefaultIndexTimeStamp > QDateTime::currentMSecsSinceEpoch())
        mDefaultIndexTimeStamp = QDateTime::currentMSecsSinceEpoch();
    int listSize = mSettings->mSettings.value(SETTING_COMPILTER_SETS_COUNT,0).toInt();
    mSettings->mSettings.endGroup();
    bool loadError = false;
    for (int i=0;i<listSize;i++) {
        PCompilerSet pSet=loadSet(i);
        if (!pSet) {
            loadError = true;
            break;
        }
        mList.push_back(pSet);
    }
    if (loadError) {
        mList.clear();
        setDefaultIndex(-1);
    }
    PCompilerSet pCurrentSet = defaultSet();
    if (pCurrentSet) {
        QString msg;
//        if (!pCurrentSet->dirsValid(msg)) {
//            if (QMessageBox::warning(nullptr,QObject::tr("Confirm"),
//                       QObject::tr("The following problems were found during validation of compiler set \"%1\":")
//                                     .arg(pCurrentSet->name())
//                                     +"<br /><br />"
//                                     +msg
//                                     +"<br /><br />"
//                                     +QObject::tr("Leaving those directories will lead to problems during compilation.")
//                                     +"<br /><br />"
//                                     +QObject::tr("Would you like Red Panda C++ to remove them for you and add the default paths to the valid paths?")
//                                     ,
//                                     QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
//                return;
//            }
//            findSets();
//            if ( (int)mList.size() <= mDefaultIndex)
//                mDefaultIndex =  mList.size()-1;
//            pCurrentSet = defaultSet();
//            if (!pCurrentSet) {
//                mList.clear();
//                mDefaultIndex = -1;
//                saveSets();
//                return;
//            }
//            saveSets();
//            pCurrentSet->setProperties(pCurrentSet->CCompiler());
//        } else {
//            return;
//        }
        return;
    } else {
#ifdef Q_OS_WIN
        QString msg = QObject::tr("Compiler set not configuared.")
                +"<br /><br />"
                +QObject::tr("Would you like Red Panda C++ to search for compilers in the following locations: <BR />'%1'<BR />'%2'? ")
                .arg(includeTrailingPathDelimiter(pSettings->dirs().appDir()) + "mingw32")
                .arg(includeTrailingPathDelimiter(pSettings->dirs().appDir()) + "mingw64");
#else
        QString msg = QObject::tr("Compiler set not configuared.")
                +"<br /><br />"
                +QObject::tr("Would you like Red Panda C++ to search for compilers in PATH?");
#endif
        if (QMessageBox::warning(nullptr,QObject::tr("Confirm"),
                   msg,
                                 QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        findSets();
        pCurrentSet = defaultSet();
        if (!pCurrentSet) {
            mList.clear();
            setDefaultIndex(-1);
            saveSets();
            return;
        }
        saveSets();
    }

}

void Settings::CompilerSets::saveDefaultIndex()
{
    mSettings->mSettings.beginGroup(SETTING_COMPILTER_SETS);
    mSettings->mSettings.setValue(SETTING_COMPILTER_SETS_DEFAULT_INDEX,mDefaultIndex);
    mSettings->mSettings.setValue(SETTING_COMPILTER_SETS_DEFAULT_INDEX_TIMESTAMP,mDefaultIndexTimeStamp);
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
    saveSets();
}

size_t Settings::CompilerSets::size() const
{
    return mList.size();
}

int Settings::CompilerSets::defaultIndex() const
{
    return mDefaultIndex;
}

qint64 Settings::CompilerSets::defaultIndexTimestamp() const
{
    return mDefaultIndexTimeStamp;
}

void Settings::CompilerSets::setDefaultIndex(int value)
{
    mDefaultIndex = value;
    mDefaultIndexTimeStamp = QDateTime::currentMSecsSinceEpoch();
}

Settings::PCompilerSet Settings::CompilerSets::defaultSet()
{
    return getSet(mDefaultIndex);
}

Settings::PCompilerSet Settings::CompilerSets::getSet(int index)
{
    if (index>=0 && index<(int)mList.size()) {
        return mList[index];
    }
    return PCompilerSet();
}

void Settings::CompilerSets::savePath(const QString& name, const QString& path) {
    if (!isGreenEdition()) {
        mSettings->mSettings.setValue(name, path);
        return;
    }

    QString s;
    QString prefix1 = excludeTrailingPathDelimiter(mSettings->mDirs.appDir()) + "/";
    QString prefix2 = excludeTrailingPathDelimiter(mSettings->mDirs.appDir()) + QDir::separator();
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
    if (!isGreenEdition()) {
        mSettings->mSettings.setValue(name, pathList);
        return;
    }

    QStringList sl;
    for (const QString& path: pathList) {
        QString s;
        QString prefix1 = excludeTrailingPathDelimiter(mSettings->mDirs.appDir()) + "/";
        QString prefix2 = excludeTrailingPathDelimiter(mSettings->mDirs.appDir()) + QDir::separator();
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
    savePath("debug_server", pSet->debugServer());
    savePath("make", pSet->make());
    savePath("windres", pSet->resourceCompiler());

    mSettings->mSettings.remove("Options");
    foreach(const PCompilerOption& option, CompilerInfoManager::getInstance()->getCompilerOptions(pSet->compilerType())) {
        mSettings->mSettings.remove(option->key);
    }
    // Save option string
    for (const QString& optionKey : pSet->compileOptions().keys()) {
        mSettings->mSettings.setValue(optionKey, pSet->compileOptions().value(optionKey));
    }

    // Save extra 'general' options
    mSettings->mSettings.setValue("useCustomCompileParams", pSet->useCustomCompileParams());
    mSettings->mSettings.setValue("customCompileParams", pSet->customCompileParams());
    mSettings->mSettings.setValue("useCustomLinkParams", pSet->useCustomLinkParams());
    mSettings->mSettings.setValue("customLinkParams", pSet->customLinkParams());
    mSettings->mSettings.setValue("AddCharset", pSet->autoAddCharsetParams());
    mSettings->mSettings.setValue("StaticLink", pSet->staticLink());
    mSettings->mSettings.setValue("ExecCharset", pSet->execCharset());
    mSettings->mSettings.setValue("PersistInAutoFind", pSet->persistInAutoFind());
    mSettings->mSettings.setValue("forceEnglishOutput", pSet->forceEnglishOutput());

    mSettings->mSettings.setValue("preprocessingSuffix", pSet->preprocessingSuffix());
    mSettings->mSettings.setValue("compilationProperSuffix", pSet->compilationProperSuffix());
    mSettings->mSettings.setValue("assemblingSuffix", pSet->assemblingSuffix());
    mSettings->mSettings.setValue("executableSuffix", pSet->executableSuffix());
    mSettings->mSettings.setValue("compilationStage", (int)pSet->compilationStage());

    // Misc. properties
    mSettings->mSettings.setValue("DumpMachine", pSet->dumpMachine());
    mSettings->mSettings.setValue("Version", pSet->version());
    mSettings->mSettings.setValue("Name", pSet->name());
    mSettings->mSettings.setValue("Target", pSet->target());
    mSettings->mSettings.setValue("CompilerType", (int)pSet->compilerType());

    // Paths
    savePathList("Bins",pSet->binDirs());
    savePathList("C",pSet->CIncludeDirs());
    savePathList("Cpp",pSet->CppIncludeDirs());
    savePathList("Libs",pSet->libDirs());

    mSettings->mSettings.endGroup();
}

QString Settings::CompilerSets::loadPath(const QString &name)
{
    // always do substitution for backward compatibility
    QString s =  mSettings->mSettings.value(name).toString();
    QString prefix = "%AppPath%/";
    if (s.startsWith(prefix)) {
        s = includeTrailingPathDelimiter(mSettings->mDirs.appDir()) + s.mid(prefix.length());
    }
    return QFileInfo(s).absoluteFilePath();
}

void Settings::CompilerSets::loadPathList(const QString &name, QStringList& list)
{
    // always do substitution for backward compatibility
    list.clear();
    QStringList sl = mSettings->mSettings.value(name).toStringList();
    QString prefix = "%AppPath%/";
    for (QString& s:sl) {
        if (s.startsWith(prefix)) {
            s = includeTrailingPathDelimiter(mSettings->mDirs.appDir()) + s.mid(prefix.length());
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
    pSet->setDebugServer(loadPath("debug_server"));
    pSet->setMake(loadPath("make"));
    pSet->setResourceCompiler(loadPath("windres"));

    pSet->setDumpMachine(mSettings->mSettings.value("DumpMachine").toString());
    pSet->setVersion(mSettings->mSettings.value("Version").toString());
    pSet->setName(mSettings->mSettings.value("Name").toString());
    pSet->setTarget(mSettings->mSettings.value("Target").toString());
    //compatibility
    QString temp = mSettings->mSettings.value("CompilerType").toString();
    if (temp==COMPILER_CLANG) {
        pSet->setCompilerType(CompilerType::Clang);
    } else if (temp==COMPILER_GCC) {
        pSet->setCompilerType(CompilerType::GCC);
    } else if (temp==COMPILER_GCC_UTF8) {
        pSet->setCompilerType(CompilerType::GCC_UTF8);
#ifdef ENABLE_SDCC
    } else if (temp==COMPILER_SDCC) {
        pSet->setCompilerType(CompilerType::SDCC);
#endif
    } else {
        pSet->setCompilerType((CompilerType)mSettings->mSettings.value("CompilerType").toInt());
    }

    // Load extra 'general' options
    pSet->setUseCustomCompileParams(mSettings->mSettings.value("useCustomCompileParams", false).toBool());
    pSet->setCustomCompileParams(mSettings->mSettings.value("customCompileParams").toString());
    pSet->setUseCustomLinkParams(mSettings->mSettings.value("useCustomLinkParams", false).toBool());
    pSet->setCustomLinkParams(mSettings->mSettings.value("customLinkParams").toString());
    pSet->setAutoAddCharsetParams(mSettings->mSettings.value("AddCharset", true).toBool());
    pSet->setStaticLink(mSettings->mSettings.value("StaticLink", false).toBool());
    pSet->setPersistInAutoFind(mSettings->mSettings.value("PersistInAutoFind", false).toBool());
    bool forceEnglishOutput=QLocale::system().name().startsWith("zh")?false:true;
    pSet->setForceEnglishOutput(mSettings->mSettings.value("forceEnglishOutput", forceEnglishOutput).toBool());

    pSet->setExecCharset(mSettings->mSettings.value("ExecCharset", ENCODING_SYSTEM_DEFAULT).toString());
    if (pSet->execCharset().isEmpty()) {
        pSet->setExecCharset(ENCODING_SYSTEM_DEFAULT);
    }
    pSet->setPreprocessingSuffix(mSettings->mSettings.value("preprocessingSuffix", DEFAULT_PREPROCESSING_SUFFIX).toString());
    pSet->setCompilationProperSuffix(mSettings->mSettings.value("compilationProperSuffix",DEFAULT_COMPILATION_SUFFIX).toString());
    pSet->setAssemblingSuffix(mSettings->mSettings.value("assemblingSuffix", DEFAULT_ASSEMBLING_SUFFIX).toString());
    pSet->setExecutableSuffix(mSettings->mSettings.value("executableSuffix", DEFAULT_EXECUTABLE_SUFFIX).toString());
    pSet->setCompilationStage((Settings::CompilerSet::CompilationStage)mSettings->mSettings.value(
                                  "compilationStage",
                                  (int)Settings::CompilerSet::CompilationStage::GenerateExecutable).toInt());

    // Load options
    QByteArray iniOptions = mSettings->mSettings.value("Options","").toByteArray();
    if (!iniOptions.isEmpty())
        pSet->setIniOptions(iniOptions);
    else {
        foreach (const QString &optionKey, mSettings->mSettings.allKeys()) {
            if (CompilerInfoManager::hasCompilerOption(pSet->compilerType(),optionKey)) {
                pSet->setCompileOption(optionKey, mSettings->mSettings.value(optionKey).toString());
            }
        }
    }

    // Paths
    loadPathList("Bins",pSet->binDirs());
    loadPathList("C",pSet->CIncludeDirs());
    loadPathList("Cpp",pSet->CppIncludeDirs());
    loadPathList("Libs",pSet->libDirs());

    mSettings->mSettings.endGroup();

//    if (pSet->binDirs().isEmpty())
//        return PCompilerSet();

    return pSet;
}

void Settings::CompilerSets::prepareCompatibleIndex()
{

    //old settings compatibility, don't reorder, add or remove items
    mCompilerCompatibleIndex.append(CC_CMD_OPT_ANSI);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_NO_ASM);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_TRADITIONAL_CPP);

    mCompilerCompatibleIndex.append(CC_CMD_OPT_ARCH);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_TUNE);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_INSTRUCTION);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_OPTIMIZE);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_POINTER_SIZE);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_STD);

    mCompilerCompatibleIndex.append(CC_CMD_OPT_INHIBIT_ALL_WARNING);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_WARNING_ALL);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_WARNING_EXTRA);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_CHECK_ISO_CONFORMANCE);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_SYNTAX_ONLY);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_WARNING_AS_ERROR);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_ABORT_ON_ERROR);

    mCompilerCompatibleIndex.append(CC_CMD_OPT_PROFILE_INFO);

    mCompilerCompatibleIndex.append(LINK_CMD_OPT_LINK_OBJC);
    mCompilerCompatibleIndex.append(LINK_CMD_OPT_NO_LINK_STDLIB);
    mCompilerCompatibleIndex.append(LINK_CMD_OPT_NO_CONSOLE);
    mCompilerCompatibleIndex.append(LINK_CMD_OPT_STRIP_EXE);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_DEBUG_INFO);

    mCompilerCompatibleIndex.append(CC_CMD_OPT_VERBOSE_ASM);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_ONLY_GEN_ASM_CODE);
    mCompilerCompatibleIndex.append(CC_CMD_OPT_USE_PIPE);
}

QString Settings::CompilerSets::getKeyFromCompilerCompatibleIndex(int idx) const
{
    if (idx<0 || idx >= mCompilerCompatibleIndex.length())
        return QString();
    return mCompilerCompatibleIndex[idx];
}

bool Settings::CompilerSets::isTarget64Bit(const QString &target)
{
    /* Fetched from LLVM 15.0.6's arch parser,
     *   `Triple::ArchType parseArch(StringRef ArchName)`
     *   in `llvm/lib/Support/Triple.cpp`.
     * The following non-CPU targets are not included:
     *   nvptx64, le64, amdil64, hsail64, spir64, spirv64, renderscript64.
     */
    QSet<QString> targets {
        // x86_64
        "amd64", "x86_64", "x86_64h",
        // ppc64
        "powerpc64", "ppu", "ppc64",
        // ppc64le
        "powerpc64le", "ppc64le",
        // aarch64
        "aarch64", "arm64", "arm64e",
        // aarch64_be
        "aarch64_be",
        // aarch64_32
        "aarch64_32", "arm64_32",
        // mips64
        "mips64", "mips64eb", "mipsn32", "mipsisa64r6", "mips64r6", "mipsn32r6",
        // mips64el
        "mips64el", "mipsn32el", "mipsisa64r6el", "mips64r6el", "mipsn32r6el",
        // riscv64
        "riscv64",
        // systemz
        "s390x", "systemz",
        // sparcv9
        "sparcv9", "sparc64",
        // wasm64
        "wasm64",
        // loongarch64
        "loongarch64",
    };
    return targets.contains(target);
}

Settings::Environment::Environment(Settings *settings):_Base(settings, SETTING_ENVIRONMENT)
{

}

void Settings::Environment::doLoad()
{
    //Appearance
    mTheme = stringValue("theme","dark");
    mInterfaceFont = stringValue("interface_font", defaultUiFont());
    mInterfaceFontSize = intValue("interface_font_size",11);
    mIconZoomFactor = doubleValue("icon_zoom_factor",1.0);
    mLanguage = stringValue("language", QLocale::system().name());
    mIconSet = stringValue("icon_set","contrast");
    mUseCustomIconSet = boolValue("use_custom_icon_set", false);

    mCurrentFolder = stringValue("current_folder",QDir::currentPath());
    if (!fileExists(mCurrentFolder)) {
        mCurrentFolder = QDir::currentPath();
    }
    mDefaultOpenFolder = stringValue("default_open_folder",QDir::currentPath());
    if (!fileExists(mDefaultOpenFolder)) {
        mDefaultOpenFolder = QDir::currentPath();
    }

#ifdef Q_OS_WINDOWS
# ifdef WINDOWS_PREFER_OPENCONSOLE
    // prefer UTF-8 compatible OpenConsole.exe
    mUseCustomTerminal = boolValue("use_custom_terminal", true);
# else
    mUseCustomTerminal = boolValue("use_custom_terminal", false);
# endif
#else // UNIX
    mUseCustomTerminal = true;
#endif

    // check saved terminal path
    mTerminalPath = stringValue("terminal_path", "");
    // always do substitution for backward compatibility
    mTerminalPath = replacePrefix(mTerminalPath, "%*APP_DIR*%", pSettings->dirs().appDir());
    mTerminalArgumentsPattern = stringValue("terminal_arguments_pattern", "");

    checkAndSetTerminal();

    mAStylePath = stringValue("astyle_path","");
    if (mAStylePath.isEmpty()
            /* compatibily for old configuration */
            || ( mAStylePath == includeTrailingPathDelimiter(pSettings->dirs().appLibexecDir())+"astyle")
            ) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString path = env.value("PATH");
        QStringList pathList = path.split(PATH_SEPARATOR);
        pathList = QStringList{
            mSettings->dirs().appDir(),
            mSettings->dirs().appLibexecDir(),
        } + pathList;

        foreach (const QString& folder, pathList) {
            QDir dir{folder};
            QFileInfo fileInfo{dir.absoluteFilePath(ASTYLE_PROGRAM)};
            if (fileInfo.exists()) {
                mAStylePath = fileInfo.absoluteFilePath();
                break;
            }
        }
        mAStylePath = replacePrefix(mAStylePath, pSettings->dirs().appDir() , "%*APP_DIR*%");
    }

    mHideNonSupportFilesInFileView=boolValue("hide_non_support_files_file_view",true);
    mOpenFilesInSingleInstance = boolValue("open_files_in_single_instance",false);
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

const QString &Settings::Environment::defaultOpenFolder() const
{
    return mDefaultOpenFolder;
}

void Settings::Environment::setDefaultOpenFolder(const QString &newDefaultOpenFolder)
{
    mDefaultOpenFolder = newDefaultOpenFolder;
}

const QString &Settings::Environment::iconSet() const
{
    return mIconSet;
}

void Settings::Environment::setIconSet(const QString &newIconSet)
{
    mIconSet = newIconSet;
}

QString Settings::Environment::terminalPath() const
{
    return mTerminalPath;
}

void Settings::Environment::setTerminalPath(const QString &terminalPath)
{
    mTerminalPath = terminalPath;
}

QString Settings::Environment::AStylePath() const
{
    QString path = mAStylePath;
    if (path.isEmpty())
        path = includeTrailingPathDelimiter(pSettings->dirs().appLibexecDir())+ASTYLE_PROGRAM;
    else
        path = replacePrefix(path, "%*APP_DIR*%", pSettings->dirs().appDir());
    return path;
}

void Settings::Environment::setAStylePath(const QString &aStylePath)
{
    mAStylePath = aStylePath;
    mAStylePath = replacePrefix(mAStylePath, pSettings->dirs().appDir() , "%*APP_DIR*%");
}

QString Settings::Environment::terminalArgumentsPattern() const
{
    return mTerminalArgumentsPattern;
}

void Settings::Environment::setTerminalArgumentsPattern(const QString &argsPattern)
{
    mTerminalArgumentsPattern = argsPattern;
}

bool Settings::Environment::useCustomIconSet() const
{
    return mUseCustomIconSet;
}

void Settings::Environment::setUseCustomIconSet(bool newUseCustomIconSet)
{
    mUseCustomIconSet = newUseCustomIconSet;
}

bool Settings::Environment::hideNonSupportFilesInFileView() const
{
    return mHideNonSupportFilesInFileView;
}

void Settings::Environment::setHideNonSupportFilesInFileView(bool newHideNonSupportFilesInFileView)
{
    mHideNonSupportFilesInFileView = newHideNonSupportFilesInFileView;
}

bool Settings::Environment::openFilesInSingleInstance() const
{
    return mOpenFilesInSingleInstance;
}

void Settings::Environment::setOpenFilesInSingleInstance(bool newOpenFilesInSingleInstance)
{
    mOpenFilesInSingleInstance = newOpenFilesInSingleInstance;
}

double Settings::Environment::iconZoomFactor() const
{
    return mIconZoomFactor;
}

void Settings::Environment::setIconZoomFactor(double newIconZoomFactor)
{
    mIconZoomFactor = newIconZoomFactor;
}

QString Settings::Environment::queryPredefinedTerminalArgumentsPattern(const QString &executable) const
{
    QString execName = extractFileName(executable);
    for (const TerminalItem& item: loadTerminalList()) {
        QString termName = extractFileName(item.terminal);
        if (termName.compare(execName,PATH_SENSITIVITY)==0) return item.param;
    }
    return QString();
}

bool Settings::Environment::useCustomTerminal() const
{
    return mUseCustomTerminal;
}

void Settings::Environment::setUseCustomTerminal(bool newUseCustomTerminal)
{
    mUseCustomTerminal = newUseCustomTerminal;
}

void Settings::Environment::checkAndSetTerminal()
{
    if (isTerminalValid()) return;

    QStringList pathList = getExecutableSearchPaths();
    QList<TerminalItem> terminalList = loadTerminalList();
    for (const TerminalItem& termItem:terminalList) {
        QString term=termItem.terminal;
        term = replacePrefix(term, "%*APP_DIR*%", pSettings->dirs().appDir());
        QFileInfo info{term};
        QString absoluteTerminalPath;
        if (info.isAbsolute()) {
            absoluteTerminalPath = info.absoluteFilePath();
            if(fileExists(absoluteTerminalPath)) {
                mTerminalPath = absoluteTerminalPath;
                mTerminalArgumentsPattern = termItem.param;
                return;
            }
        } else {
            for (const QString &dirPath: pathList) {
                QDir dir{dirPath};
                absoluteTerminalPath = dir.absoluteFilePath(termItem.terminal);
                if(fileExists(absoluteTerminalPath)) {
                    mTerminalPath = absoluteTerminalPath;
                    mTerminalArgumentsPattern = termItem.param;
                    return;
                }
            }
        }
    }
    //Can't Find a term
    QMessageBox::critical(
                nullptr,
                QCoreApplication::translate("Settings","Error"),
        QCoreApplication::translate("Settings","Can't find terminal program!"));
}

QMap<QString, QString> Settings::Environment::terminalArgsPatternMagicVariables()
{
    return mTerminalArgsPatternMagicVariables;
}

QList<Settings::Environment::TerminalItem> Settings::Environment::loadTerminalList() const
{
#ifdef Q_OS_WINDOWS
    QString terminalListFilename(":/config/terminal-windows.json");
#else // UNIX
    QString terminalListFilename(":/config/terminal-unix.json");
#endif
    QFile terminalListFile(terminalListFilename);
    if (!terminalListFile.open(QFile::ReadOnly))
        throw FileError(QObject::tr("Can't open file '%1' for read.")
                            .arg(terminalListFilename));
    QByteArray terminalListContent = terminalListFile.readAll();
    QJsonDocument terminalListDocument(QJsonDocument::fromJson(terminalListContent));

    QList<Settings::Environment::TerminalItem> result;
    // determing terminal (if not set yet) and build predefined arguments pattern map from our list
    for (const auto &terminalGroup: terminalListDocument.array()) {
        const QJsonArray &terminals = terminalGroup.toObject()["terminals"].toArray();
        for (const auto &terminal_: terminals) {
            const QJsonObject& terminal = terminal_.toObject();
            QString path = terminal["path"].toString();
            QString termExecutable = QFileInfo(path).fileName();
            QString pattern = terminal["argsPattern"].toString();
            Settings::Environment::TerminalItem terminalItem;
            path = replacePrefix(path, "%*APP_DIR*%", pSettings->dirs().appDir());
            terminalItem.terminal = path;
            terminalItem.param = pattern;
            result.append(terminalItem);
        }
    }
    return result;
}

bool Settings::Environment::isTerminalValid()
{
    // don't use custom terminal
    if (!mUseCustomTerminal) return true;
    // terminal patter is empty
    if (mTerminalArgumentsPattern.isEmpty()) return false;

    QStringList patternItems = parseArguments(mTerminalArgumentsPattern, mTerminalArgsPatternMagicVariables, false);

    if (!(patternItems.contains("$argv")
          || patternItems.contains("$command")
          || patternItems.contains("$tmpfile"))) {
        // program not referenced
        return false;
    }
    QFileInfo termPathInfo{mTerminalPath};
    if (termPathInfo.isAbsolute()) {
        return termPathInfo.exists();
    } else {
        QStringList pathList = getExecutableSearchPaths();
        for (const QString &dirName: pathList) {
            QDir dir{dirName};
            QString absoluteTerminalPath = dir.absoluteFilePath(mTerminalPath);
            QFileInfo absTermPathInfo(absoluteTerminalPath);
            if (absTermPathInfo.exists()) return true;
        }
    }
    return false;
}

void Settings::Environment::doSave()
{
    //Appearance
    saveValue("theme", mTheme);
    saveValue("interface_font", mInterfaceFont);
    saveValue("interface_font_size", mInterfaceFontSize);
    saveValue("icon_zoom_factor",mIconZoomFactor);
    saveValue("language", mLanguage);
    saveValue("icon_set",mIconSet);
    saveValue("use_custom_icon_set", mUseCustomIconSet);
    saveValue("use_custom_theme", mUseCustomTheme);

    saveValue("current_folder",mCurrentFolder);
    saveValue("default_open_folder",mDefaultOpenFolder);
    QString terminalPath = mTerminalPath;
    if (isGreenEdition())
    {
        // APP_DIR trick for windows portable app
        // For non-portable app (other platform or Windows installer), multiple instances
        // share the same configuration and thus the trick may break terminal path
        terminalPath = replacePrefix(terminalPath, pSettings->dirs().appDir(), "%*APP_DIR*%");
    }

    saveValue("terminal_path",terminalPath);
    saveValue("terminal_arguments_pattern",mTerminalArgumentsPattern);
#ifdef Q_OS_WINDOWS
    saveValue("use_custom_terminal",mUseCustomTerminal);
#endif
    saveValue("astyle_path",mAStylePath);

    saveValue("hide_non_support_files_file_view",mHideNonSupportFilesInFileView);
    saveValue("open_files_in_single_instance",mOpenFilesInSingleInstance);
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

const QMap<QString, QString> Settings::Environment::mTerminalArgsPatternMagicVariables = {
    {"term", "$term"},
    {"integrated_term", "$integrated_term"},
    {"argv", "$argv"},
    {"command", "$command"},
    {"unix_command", "$unix_command"},
    {"dos_command", "$dos_command"},
    {"lpCommandLine", "$lpCommandLine"},
    {"tmpfile", "$tmpfile"},
    {"sequential_app_id", "$sequential_app_id"},
};

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

bool Settings::Executor::caseEditorFontOnlyMonospaced() const
{
    return mCaseEditorFontOnlyMonospaced;
}

void Settings::Executor::setCaseEditorFontOnlyMonospaced(bool newCaseEditorFontOnlyMonospaced)
{
    mCaseEditorFontOnlyMonospaced = newCaseEditorFontOnlyMonospaced;
}

size_t Settings::Executor::caseTimeout() const
{
    return mCaseTimeout;
}

void Settings::Executor::setCaseTimeout(size_t newCaseTimeout)
{
    mCaseTimeout = newCaseTimeout;
}

size_t Settings::Executor::caseMemoryLimit() const
{
    return mCaseMemoryLimit;
}

void Settings::Executor::setCaseMemoryLimit(size_t newCaseMemoryLimit)
{
    mCaseMemoryLimit = newCaseMemoryLimit;
}

bool Settings::Executor::convertHTMLToTextForExpected() const
{
    return mConvertHTMLToTextForExpected;
}

void Settings::Executor::setConvertHTMLToTextForExpected(bool newConvertHTMLToTextForExpected)
{
    mConvertHTMLToTextForExpected = newConvertHTMLToTextForExpected;
}

bool Settings::Executor::redirectStderrToToolLog() const
{
    return mRedirectStderrToToolLog;
}

void Settings::Executor::setRedirectStderrToToolLog(bool newRedirectStderrToToolLog)
{
    mRedirectStderrToToolLog = newRedirectStderrToToolLog;
}

ProblemCaseValidateType Settings::Executor::problemCaseValidateType() const
{
    return mProblemCaseValidateType;
}

void Settings::Executor::setProblemCaseValidateType(ProblemCaseValidateType newProblemCaseValidateType)
{
    mProblemCaseValidateType = newProblemCaseValidateType;
}

bool Settings::Executor::enableVirualTerminalSequence() const
{
    return mEnableVirualTerminalSequence;
}

void Settings::Executor::setEnableVirualTerminalSequence(bool newEnableVirualTerminalSequence)
{
    mEnableVirualTerminalSequence = newEnableVirualTerminalSequence;
}

bool Settings::Executor::convertHTMLToTextForInput() const
{
    return mConvertHTMLToTextForInput;
}

void Settings::Executor::setConvertHTMLToTextForInput(bool newConvertHTMLToTextForInput)
{
    mConvertHTMLToTextForInput = newConvertHTMLToTextForInput;
}

bool Settings::Executor::enableCaseLimit() const
{
    return mEnableCaseLimit;
}

void Settings::Executor::setEnableCaseLimit(bool newValue)
{
    mEnableCaseLimit = newValue;
}

int Settings::Executor::caseEditorFontSize() const
{
    return mCaseEditorFontSize;
}

void Settings::Executor::setCaseEditorFontSize(int newCaseEditorFontSize)
{
    mCaseEditorFontSize = newCaseEditorFontSize;
}

const QString &Settings::Executor::caseEditorFontName() const
{
    return mCaseEditorFontName;
}

void Settings::Executor::setCaseEditorFontName(const QString &newCaseEditorFontName)
{
    mCaseEditorFontName = newCaseEditorFontName;
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
#ifdef Q_OS_WIN
    saveValue("enable_virtual_terminal_sequence", mEnableVirualTerminalSequence);
#endif
    saveValue("minimize_on_run", mMinimizeOnRun);
    saveValue("use_params",mUseParams);
    saveValue("params",mParams);
    saveValue("redirect_input",mRedirectInput);
    saveValue("input_filename",mInputFilename);
    //problem set
    saveValue("enable_proble_set", mEnableProblemSet);
    saveValue("enable_competivie_companion", mEnableCompetitiveCompanion);
    saveValue("competitive_companion_port", mCompetivieCompanionPort);
    saveValue("input_convert_html", mConvertHTMLToTextForInput);
    saveValue("expected_convert_html", mConvertHTMLToTextForExpected);
    saveValue("problem_case_validate_type", (int)mProblemCaseValidateType);
    saveValue("redirect_stderr_to_toollog", mRedirectStderrToToolLog);
    saveValue("case_editor_font_name",mCaseEditorFontName);
    saveValue("case_editor_font_size",mCaseEditorFontSize);
    saveValue("case_editor_font_only_monospaced",mCaseEditorFontOnlyMonospaced);
    saveValue("case_timeout_ms", mCaseTimeout);
    saveValue("case_memory_limit",mCaseMemoryLimit);
    remove("case_timeout");
    saveValue("enable_case_limit", mEnableCaseLimit);
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
#ifdef Q_OS_WIN
    mEnableVirualTerminalSequence = boolValue("enable_virtual_terminal_sequence", true);
#endif
    mMinimizeOnRun = boolValue("minimize_on_run",false);
    mUseParams = boolValue("use_params",false);
    mParams = stringValue("params", "");
    mRedirectInput = boolValue("redirect_input",false);
    mInputFilename = stringValue("input_filename","");

    mEnableProblemSet = boolValue("enable_proble_set",true);
    mEnableCompetitiveCompanion = boolValue("enable_competivie_companion",true);
    mCompetivieCompanionPort = intValue("competitive_companion_port",10045);
    mConvertHTMLToTextForInput = boolValue("input_convert_html", false);
    mConvertHTMLToTextForExpected = boolValue("expected_convert_html", false);
    mProblemCaseValidateType =(ProblemCaseValidateType)intValue("problem_case_validate_type", (int)ProblemCaseValidateType::Exact);
    mRedirectStderrToToolLog = boolValue("redirect_stderr_to_toollog", false);

    mCaseEditorFontName = stringValue("case_editor_font_name", defaultMonoFont());
    mCaseEditorFontSize = intValue("case_editor_font_size",11);
    mCaseEditorFontOnlyMonospaced = boolValue("case_editor_font_only_monospaced",true);
    int case_timeout = intValue("case_timeout", -1);
    if (case_timeout>0)
        mCaseTimeout = case_timeout*1000;
    else
        mCaseTimeout = uintValue("case_timeout_ms", 2000); //2000ms
    mCaseMemoryLimit = uintValue("case_memory_limit",0); // kb

    mEnableCaseLimit = boolValue("enable_case_limit", true);
    //compatibility
    if (boolValue("enable_time_limit", true)) {
        mEnableCaseLimit=true;
    }
}


Settings::Debugger::Debugger(Settings *settings):_Base(settings, SETTING_DEBUGGER)
{

}

bool Settings::Debugger::enableDebugConsole() const
{
    return mEnableDebugConsole;
}

void Settings::Debugger::setEnableDebugConsole(bool showCommandLog)
{
    mEnableDebugConsole = showCommandLog;
}

bool Settings::Debugger::showDetailLog() const
{
    return mShowDetailLog;
}

void Settings::Debugger::setShowDetailLog(bool showAnnotations)
{
    mShowDetailLog = showAnnotations;
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

bool Settings::Debugger::openCPUInfoWhenSignaled() const
{
    return mOpenCPUInfoWhenSignaled;
}

void Settings::Debugger::setOpenCPUInfoWhenSignaled(bool newOpenCPUInfoWhenSignaled)
{
    mOpenCPUInfoWhenSignaled = newOpenCPUInfoWhenSignaled;
}

bool Settings::Debugger::useGDBServer() const
{
    return mUseGDBServer;
}

void Settings::Debugger::setUseGDBServer(bool newUseGDBServer)
{
    mUseGDBServer = newUseGDBServer;
}

int Settings::Debugger::GDBServerPort() const
{
    return mGDBServerPort;
}

void Settings::Debugger::setGDBServerPort(int newGDBServerPort)
{
    mGDBServerPort = newGDBServerPort;
}

int Settings::Debugger::memoryViewRows() const
{
    return mMemoryViewRows;
}

void Settings::Debugger::setMemoryViewRows(int newMemoryViewRows)
{
    mMemoryViewRows = newMemoryViewRows;
}

int Settings::Debugger::memoryViewColumns() const
{
    return mMemoryViewColumns;
}

void Settings::Debugger::setMemoryViewColumns(int newMemoryViewColumns)
{
    mMemoryViewColumns = newMemoryViewColumns;
}

bool Settings::Debugger::autosave() const
{
    return mAutosave;
}

void Settings::Debugger::setAutosave(bool newAutosave)
{
    mAutosave = newAutosave;
}

int Settings::Debugger::arrayElements() const
{
    return mArrayElements;
}

void Settings::Debugger::setArrayElements(int newArrayElements)
{
    mArrayElements = newArrayElements;
}

int Settings::Debugger::characters() const
{
    return mCharacters;
}

void Settings::Debugger::setCharacters(int newCharacters)
{
    mCharacters = newCharacters;
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
    saveValue("enable_debug_console", mEnableDebugConsole);
    saveValue("show_detail_log", mShowDetailLog);
    saveValue("font_name",mFontName);
    saveValue("only_show_mono",mOnlyShowMono);
    saveValue("font_size",mFontSize);
    saveValue("use_intel_style",mUseIntelStyle);
    saveValue("blend_mode",mBlendMode);
    saveValue("skip_system_lib", mSkipSystemLibraries);
    saveValue("skip_project_lib", mSkipProjectLibraries);
    saveValue("skip_custom_lib", mSkipCustomLibraries);
    saveValue("autosave",mAutosave);
    saveValue("open_cpu_info_when_signaled",mOpenCPUInfoWhenSignaled);
    saveValue("use_gdb_server", mUseGDBServer);
    saveValue("gdb_server_port",mGDBServerPort);
    saveValue("memory_view_rows",mMemoryViewRows);
    saveValue("memory_view_columns",mMemoryViewColumns);
    saveValue("array_elements",mArrayElements);
    saveValue("string_characters",mCharacters);
}

void Settings::Debugger::doLoad()
{
    mEnableDebugConsole = boolValue("enable_debug_console",true);
    mShowDetailLog = boolValue("show_detail_log",false);
    mFontName = stringValue("font_name", defaultMonoFont());
    mOnlyShowMono = boolValue("only_show_mono",true);
    mFontSize = intValue("font_size",14);
    mUseIntelStyle = boolValue("use_intel_style",false);
    mBlendMode = boolValue("blend_mode",true);
    mSkipSystemLibraries = boolValue("skip_system_lib",true);
    mSkipProjectLibraries = boolValue("skip_project_lib",true);
    mSkipCustomLibraries = boolValue("skip_custom_lib",false);
    mAutosave = boolValue("autosave",true);
    mOpenCPUInfoWhenSignaled = boolValue("open_cpu_info_when_signaled",true);
#ifdef Q_OS_WIN
    mUseGDBServer = boolValue("use_gdb_server", false);
#else
    mUseGDBServer = true;
#endif
    mGDBServerPort = intValue("gdb_server_port",41234);
    mMemoryViewRows = intValue("memory_view_rows",16);
    mMemoryViewColumns = intValue("memory_view_columns",16);
    mArrayElements = intValue("array_elements",100);
    mCharacters = intValue("string_characters",300);
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

//bool Settings::CodeCompletion::clearWhenEditorHidden()
//{
//    if (!mShareParser) {
//#ifdef Q_OS_WIN
//        MEMORYSTATUSEX statex;
//        statex.dwLength = sizeof (statex);
//        GlobalMemoryStatusEx (&statex);

//        if (statex.ullAvailPhys < (long long int)2*1024*1024*1024) {
//            return true;
//        }
//#elif defined(Q_OS_LINUX)
//        struct sysinfo si;
//        sysinfo(&si);
//        if (si.freeram < (long long int)2*1024*1024*1024) {
//            return true;
//        }
//#endif
//    }
//    return mClearWhenEditorHidden;
//}

//void Settings::CodeCompletion::setClearWhenEditorHidden(bool newClearWhenEditorHidden)
//{
//    mClearWhenEditorHidden = newClearWhenEditorHidden;
//}

int Settings::CodeCompletion::minCharRequired() const
{
    return mMinCharRequired;
}

void Settings::CodeCompletion::setMinCharRequired(int newMinCharRequired)
{
    mMinCharRequired = newMinCharRequired;
}

bool Settings::CodeCompletion::hideSymbolsStartsWithTwoUnderLine() const
{
    return mHideSymbolsStartsWithTwoUnderLine;
}

void Settings::CodeCompletion::setHideSymbolsStartsWithTwoUnderLine(bool newHideSymbolsStartsWithTwoUnderLine)
{
    mHideSymbolsStartsWithTwoUnderLine = newHideSymbolsStartsWithTwoUnderLine;
}

bool Settings::CodeCompletion::shareParser()
{
    return mShareParser;
}

void Settings::CodeCompletion::setShareParser(bool newShareParser)
{
    mShareParser = newShareParser;
}

bool Settings::CodeCompletion::hideSymbolsStartsWithUnderLine() const
{
    return mHideSymbolsStartsWithUnderLine;
}

void Settings::CodeCompletion::setHideSymbolsStartsWithUnderLine(bool newHideSymbolsStartsWithOneUnderLine)
{
    mHideSymbolsStartsWithUnderLine = newHideSymbolsStartsWithOneUnderLine;
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

int Settings::CodeCompletion::heightInLines() const
{
    return mHeightInLines;
}

void Settings::CodeCompletion::setHeightInLines(int newHeight)
{
    mHeightInLines = newHeight;
}

int Settings::CodeCompletion::widthInColumns() const
{
    return mWidthInColumns;
}

void Settings::CodeCompletion::setWidthInColumns(int newWidth)
{
    mWidthInColumns = newWidth;
}

void Settings::CodeCompletion::doSave()
{
    saveValue("widthInColumns",mWidthInColumns);
    saveValue("heightInLines",mHeightInLines);
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
    //saveValue("clear_when_editor_hidden",mClearWhenEditorHidden);
    saveValue("min_char_required",mMinCharRequired);
    saveValue("hide_symbols_start_with_two_underline", mHideSymbolsStartsWithTwoUnderLine);
    saveValue("hide_symbols_start_with_underline", mHideSymbolsStartsWithUnderLine);
    saveValue("share_parser",mShareParser);
}


void Settings::CodeCompletion::doLoad()
{
    //Appearance
    mWidthInColumns = intValue("widthInColumns",30);
    mHeightInLines = intValue("heightInLines",8);
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
    mMinCharRequired = intValue("min_char_required",1);
    mHideSymbolsStartsWithTwoUnderLine = boolValue("hide_symbols_start_with_two_underline", true);
    mHideSymbolsStartsWithUnderLine = boolValue("hide_symbols_start_with_underline", true);

    bool shouldShare= true;
//    bool doClear = false;

//#ifdef Q_OS_WIN
//    MEMORYSTATUSEX statex;

//    statex.dwLength = sizeof (statex);

//    GlobalMemoryStatusEx (&statex);

//    if (statex.ullAvailPhys > (long long int)32*1024*1024*1024) {
//        shouldShare = false;
//    }

////    if (shouldShare) {
////        SYSTEM_INFO info;
////        GetSystemInfo(&info);
////        if (info.dwNumberOfProcessors>8 && info.dwProcessorType) {
////            doClear = true;
////        }
////    }
//#elif defined(Q_OS_LINUX)
//    struct sysinfo si;
//    sysinfo(&si);
//    if (si.freeram > (long long int)24*1024*1024*1024) {
//        shouldShare = false;
//    }
//#endif
    //mClearWhenEditorHidden = boolValue("clear_when_editor_hidden",doClear);
    mShareParser = boolValue("share_parser",shouldShare);
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
    else if (mBreakBlocksAll)
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
    if (mFillEmptyLines)
        result.append("--fill-empty-lines");
    if (mSqueezeLines)
        result.append(QString("--squeeze-lines=%1").arg(mSqueezeLinesNumber));
    if (mSqueezeWhitespace)
        result.append(QString("--squeeze-ws").arg(mSqueezeLinesNumber));
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

bool Settings::CodeFormatter::squeezeWhitespace() const
{
    return mSqueezeWhitespace;
}

void Settings::CodeFormatter::setSqueezeWhitespace(bool newSqueezeWhitespace)
{
    mSqueezeWhitespace = newSqueezeWhitespace;
}

int Settings::CodeFormatter::squeezeLinesNumber() const
{
    return mSqueezeLinesNumber;
}

void Settings::CodeFormatter::setSqueezeLinesNumber(int newSqueezeLinesNumber)
{
    mSqueezeLinesNumber = newSqueezeLinesNumber;
}

bool Settings::CodeFormatter::squeezeLines() const
{
    return mSqueezeLines;
}

void Settings::CodeFormatter::setSqueezeLines(bool newSqueezeLines)
{
    mSqueezeLines = newSqueezeLines;
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
    saveValue("fill_empty_lines",mFillEmptyLines);
    saveValue("squeeze_lines", mSqueezeLines);
    saveValue("squeeze_line_number", mSqueezeLinesNumber);
    saveValue("squeeze_whitespace", mSqueezeWhitespace);

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
    mIndentClasses = boolValue("indent_classes",false);
    mIndentModifiers = boolValue("indent_modifiers",false);
    mIndentSwitches = boolValue("indent_switches",true);
    mIndentCases = boolValue("indent_cases",false);
    mIndentNamespaces = boolValue("indent_namespaces",true);
    mIndentAfterParens = boolValue("indent_after_parents",false);
    mIndentContinuation = intValue("indent_continuation",1);
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
    mFillEmptyLines = boolValue("fill_empty_lines",false);

    mSqueezeLines = boolValue("squeeze_lines", false);
    mSqueezeLinesNumber = intValue("squeeze_line_number", 1);
    mSqueezeWhitespace = boolValue("squeeze_whitespace", false);
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

int Settings::UI::bottomPanelIndex() const
{
    return mBottomPanelIndex;
}

void Settings::UI::setBottomPanelIndex(int newBottomPanelIndex)
{
    mBottomPanelIndex = newBottomPanelIndex;
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

bool Settings::UI::showProblem() const
{
    return mShowProblem;
}

void Settings::UI::setShowProblem(bool newShowProblem)
{
    mShowProblem = newShowProblem;
}

int Settings::UI::settingsDialogSplitterPos() const
{
    return mSettingsDialogSplitterPos;
}

void Settings::UI::setSettingsDialogSplitterPos(int newSettingsDialogSplitterPos)
{
    mSettingsDialogSplitterPos = newSettingsDialogSplitterPos;
}

int Settings::UI::newProjectDialogWidth() const
{
    return mNewProjectDialogWidth;
}

void Settings::UI::setNewProjectDialogWidth(int newNewProjectDialogWidth)
{
    mNewProjectDialogWidth = newNewProjectDialogWidth;
}

int Settings::UI::newProjectDialogHeight() const
{
    return mNewProjectDialogHeight;
}

void Settings::UI::setNewProjectDialogHeight(int newNewProjectDialogHeight)
{
    mNewProjectDialogHeight = newNewProjectDialogHeight;
}

int Settings::UI::newClassDialogWidth() const
{
    return mNewClassDialogWidth;
}

void Settings::UI::setNewClassDialogWidth(int newNewClassDialogWidth)
{
    mNewClassDialogWidth = newNewClassDialogWidth;
}

int Settings::UI::newClassDialogHeight() const
{
    return mNewClassDialogHeight;
}

void Settings::UI::setNewClassDialogHeight(int newNewClassDialogHeight)
{
    mNewClassDialogHeight = newNewClassDialogHeight;
}

int  Settings::UI::newHeaderDialogHeight() const
{
    return mNewHeaderDialogHeight;
}

void  Settings::UI::setNewHeaderDialogHeight(int newNewFileDialogHeight)
{
    mNewHeaderDialogHeight = newNewFileDialogHeight;
}

const QSize &Settings::UI::messagesTabsSize() const
{
    return mMessagesTabsSize;
}

void Settings::UI::setMessagesTabsSize(const QSize &newMessagesTabsSize)
{
    mMessagesTabsSize = newMessagesTabsSize;
}

int Settings::UI::debugPanelIndex() const
{
    return mDebugPanelIndex;
}

void Settings::UI::setDebugPanelIndex(int newDebugPanelIndex)
{
    mDebugPanelIndex = newDebugPanelIndex;
}

int Settings::UI::problemOrder() const
{
    return mProblemOrder;
}

void Settings::UI::setProblemOrder(int newProblemOrder)
{
    mProblemOrder = newProblemOrder;
}

bool Settings::UI::openEditorsWhenReplace() const
{
    return mOpenEditorsWhenReplace;
}

void Settings::UI::setOpenEditorsWhenReplace(bool newOpenEditorsWhenReplace)
{
    mOpenEditorsWhenReplace = newOpenEditorsWhenReplace;
}

int Settings::UI::bookmarkOrder() const
{
    return mBookmarkOrder;
}

void Settings::UI::setBookmarkOrder(int newBookmarkOrder)
{
    mBookmarkOrder = newBookmarkOrder;
}

int Settings::UI::TODOOrder() const
{
    return mTODOOrder;
}

void Settings::UI::setTODOOrder(int newTODOOrder)
{
    mTODOOrder = newTODOOrder;
}

int Settings::UI::searchOrder() const
{
    return mSearchOrder;
}

void Settings::UI::setSearchOrder(int newSearchOrder)
{
    mSearchOrder = newSearchOrder;
}

int Settings::UI::debugOrder() const
{
    return mDebugOrder;
}

void Settings::UI::setDebugOrder(int newDebugOrder)
{
    mDebugOrder = newDebugOrder;
}

int Settings::UI::compileLogOrder() const
{
    return mCompileLogOrder;
}

void Settings::UI::setCompileLogOrder(int newCompileLogOrder)
{
    mCompileLogOrder = newCompileLogOrder;
}

int Settings::UI::issuesOrder() const
{
    return mIssuesOrder;
}

void Settings::UI::setIssuesOrder(int newIssuesOrder)
{
    mIssuesOrder = newIssuesOrder;
}

int Settings::UI::problemSetOrder() const
{
    return mProblemSetOrder;
}

void Settings::UI::setProblemSetOrder(int newProblemSetOrder)
{
    mProblemSetOrder = newProblemSetOrder;
}

int Settings::UI::filesOrder() const
{
    return mFilesOrder;
}

void Settings::UI::setFilesOrder(int newFilesOrder)
{
    mFilesOrder = newFilesOrder;
}

int Settings::UI::structureOrder() const
{
    return mStructureOrder;
}

void Settings::UI::setStructureOrder(int newStructureOrder)
{
    mStructureOrder = newStructureOrder;
}

int Settings::UI::watchOrder() const
{
    return mWatchOrder;
}

void Settings::UI::setWatchOrder(int newWatchOrder)
{
    mWatchOrder = newWatchOrder;
}

int Settings::UI::projectOrder() const
{
    return mProjectOrder;
}

void Settings::UI::setProjectOrder(int newProjectOrder)
{
    mProjectOrder = newProjectOrder;
}

const QSize &Settings::UI::explorerTabsSize() const
{
    return mExplorerTabsSize;
}

void Settings::UI::setExplorerTabsSize(const QSize &newExplorerTabsSize)
{
    mExplorerTabsSize = newExplorerTabsSize;
}

bool Settings::UI::shrinkMessagesTabs() const
{
    return mShrinkMessagesTabs;
}

void Settings::UI::setShrinkMessagesTabs(bool newShrinkMessagesTabs)
{
    mShrinkMessagesTabs = newShrinkMessagesTabs;
}

bool Settings::UI::shrinkExplorerTabs() const
{
    return mShrinkExplorerTabs;
}

void Settings::UI::setShrinkExplorerTabs(bool newShrinkExplorerTabs)
{
    mShrinkExplorerTabs = newShrinkExplorerTabs;
}

int  Settings::UI::newHeaderDialogWidth() const
{
    return mNewHeaderDialogWidth;
}

void  Settings::UI::setNewHeaderDialogWidth(int newNewFileDialogWidth)
{
    mNewHeaderDialogWidth = newNewFileDialogWidth;
}

int Settings::UI::settingsDialogHeight() const
{
    return mSettingsDialogHeight;
}

void Settings::UI::setSettingsDialogHeight(int newSettingsDialogHeight)
{
    mSettingsDialogHeight = newSettingsDialogHeight;
}

int Settings::UI::settingsDialogWidth() const
{
    return mSettingsDialogWidth;
}

void Settings::UI::setSettingsDialogWidth(int newSettingsDialogWidth)
{
    mSettingsDialogWidth = newSettingsDialogWidth;
}

int Settings::UI::CPUDialogSplitterPos() const
{
    return mCPUDialogSplitterPos;
}

void Settings::UI::setCPUDialogSplitterPos(int newCPUDialogSplitterPos)
{
    mCPUDialogSplitterPos = newCPUDialogSplitterPos;
}

int Settings::UI::CPUDialogHeight() const
{
    return mCPUDialogHeight;
}

void Settings::UI::setCPUDialogHeight(int newCPUDialogHeight)
{
    mCPUDialogHeight = newCPUDialogHeight;
}

int Settings::UI::CPUDialogWidth() const
{
    return mCPUDialogWidth;
}

void Settings::UI::setCPUDialogWidth(int newCPUDialogWidth)
{
    mCPUDialogWidth = newCPUDialogWidth;
}

bool Settings::UI::showBookmark() const
{
    return mShowBookmark;
}

void Settings::UI::setShowBookmark(bool newShowBookmark)
{
    mShowBookmark = newShowBookmark;
}

bool Settings::UI::showTODO() const
{
    return mShowTODO;
}

void Settings::UI::setShowTODO(bool newShowTODO)
{
    mShowTODO = newShowTODO;
}

bool Settings::UI::showSearch() const
{
    return mShowSearch;
}

void Settings::UI::setShowSearch(bool newShowSearch)
{
    mShowSearch = newShowSearch;
}

bool Settings::UI::showDebug() const
{
    return mShowDebug;
}

void Settings::UI::setShowDebug(bool newShowDebug)
{
    mShowDebug = newShowDebug;
}

bool Settings::UI::showCompileLog() const
{
    return mShowCompileLog;
}

void Settings::UI::setShowCompileLog(bool newShowCompileLog)
{
    mShowCompileLog = newShowCompileLog;
}

bool Settings::UI::showIssues() const
{
    return mShowIssues;
}

void Settings::UI::setShowIssues(bool newShowIssues)
{
    mShowIssues = newShowIssues;
}

bool Settings::UI::showProblemSet() const
{
    return mShowProblemSet;
}

void Settings::UI::setShowProblemSet(bool newShowProblemSet)
{
    mShowProblemSet = newShowProblemSet;
}

bool Settings::UI::showFiles() const
{
    return mShowFiles;
}

void Settings::UI::setShowFiles(bool newShowFiles)
{
    mShowFiles = newShowFiles;
}

bool Settings::UI::showStructure() const
{
    return mShowStructure;
}

void Settings::UI::setShowStructure(bool newShowStructure)
{
    mShowStructure = newShowStructure;
}

bool Settings::UI::showWatch() const
{
    return mShowWatch;
}

void Settings::UI::setShowWatch(bool newShowWatch)
{
    mShowWatch = newShowWatch;
}

bool Settings::UI::showProject() const
{
    return mShowProject;
}

void Settings::UI::setShowProject(bool newShowProject)
{
    mShowProject = newShowProject;
}

bool Settings::UI::showToolWindowBars() const
{
    return mShowToolWindowBars;
}

void Settings::UI::setShowToolWindowBars(bool newShowToolWindowBars)
{
    mShowToolWindowBars = newShowToolWindowBars;
}

bool Settings::UI::showStatusBar() const
{
    return mShowStatusBar;
}

void Settings::UI::setShowStatusBar(bool newShowStatusBar)
{
    mShowStatusBar = newShowStatusBar;
}

bool Settings::UI::showToolbar() const
{
    return mShowToolbar;
}

void Settings::UI::setShowToolbar(bool newShowToolbar)
{
    mShowToolbar = newShowToolbar;
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
    saveValue("open_editor_when_batch_replace",mOpenEditorsWhenReplace);

    saveValue("main_window_state",mMainWindowState);
    saveValue("main_window_geometry",mMainWindowGeometry);
    saveValue("bottom_panel_index",mBottomPanelIndex);
    saveValue("left_panel_index",mLeftPanelIndex);
    saveValue("debug_panel_index",mDebugPanelIndex);
    saveValue("class_browser_sort_alphabetically",mClassBrowserSortAlpha);
    saveValue("class_browser_sort_by_type",mClassBrowserSortType);
    saveValue("class_browser_show_inherited",mClassBrowserShowInherited);

    saveValue("shrink_explorer_tabs",mShrinkExplorerTabs);
    saveValue("shrink_messages_tabs",mShrinkMessagesTabs);
    saveValue("explorer_tabs_size", mExplorerTabsSize);
    saveValue("messages_tabs_size",mMessagesTabsSize);

    //view
    saveValue("show_toolbar", mShowToolbar);
    saveValue("show_statusbar", mShowStatusBar);
    saveValue("show_tool_windowbars", mShowToolWindowBars);

    saveValue("show_project", mShowProject);
    saveValue("show_watch", mShowWatch);
    saveValue("show_structure", mShowStructure);
    saveValue("show_file", mShowFiles);
    saveValue("show_problem_set", mShowProblemSet);

    saveValue("show_issues", mShowIssues);
    saveValue("show_compile_log", mShowCompileLog);
    saveValue("show_debug", mShowDebug);
    saveValue("show_search", mShowSearch);
    saveValue("show_todo", mShowTODO);
    saveValue("show_bookmark", mShowBookmark);
    saveValue("show_problem", mShowProblem);

    saveValue("project_order", mProjectOrder);
    saveValue("watch_order", mWatchOrder);
    saveValue("structure_order", mStructureOrder);
    saveValue("files_order", mFilesOrder);
    saveValue("problemset_order", mProblemSetOrder);
    saveValue("issues_order", mIssuesOrder);
    saveValue("compilelog_order", mCompileLogOrder);
    saveValue("debug_order", mDebugOrder);
    saveValue("search_order", mSearchOrder);
    saveValue("todo_order", mTODOOrder);
    saveValue("bookmark_order", mBookmarkOrder);
    saveValue("problem_order", mProblemOrder);

    //dialogs
    saveValue("cpu_dialog_width", mCPUDialogWidth);
    saveValue("cpu_dialog_height", mCPUDialogHeight);
    saveValue("cpu_dialog_splitter", mCPUDialogSplitterPos);
    saveValue("settings_dialog_width", mSettingsDialogWidth);
    saveValue("settings_dialog_height", mSettingsDialogHeight);
    saveValue("settings_dialog_splitter", mSettingsDialogSplitterPos);
    saveValue("new_project_dialog_width", mNewProjectDialogWidth);
    saveValue("new_project_dialog_height", mNewProjectDialogHeight);
    saveValue("new_class_dialog_width", mNewClassDialogWidth);
    saveValue("new_class_dialog_height", mNewClassDialogHeight);
    saveValue("new_header_dialog_width", mNewHeaderDialogWidth);
    saveValue("new_header_dialog_height", mNewHeaderDialogHeight);
}

void Settings::UI::doLoad()
{
    mOpenEditorsWhenReplace=boolValue("open_editor_when_batch_replace",true);

    mMainWindowState = value("main_window_state",QByteArray()).toByteArray();
    mMainWindowGeometry = value("main_window_geometry",QByteArray()).toByteArray();
    mBottomPanelIndex = intValue("bottom_panel_index",0);
    mLeftPanelIndex = intValue("left_panel_index",0);
    mDebugPanelIndex = intValue("debug_panel_index",0);

    mClassBrowserSortAlpha = boolValue("class_browser_sort_alphabetically",true);
    mClassBrowserSortType = boolValue("class_browser_sort_by_type",true);
    mClassBrowserShowInherited = boolValue("class_browser_show_inherited",true);

    mShrinkExplorerTabs = boolValue("shrink_explorer_tabs",false);
    mShrinkMessagesTabs = boolValue("shrink_messages_tabs",false);
    mExplorerTabsSize = sizeValue("explorer_tabs_size",QSize(300,600));
    mMessagesTabsSize = sizeValue("messages_tabs_size",QSize(450,150));

    //view
    mShowToolbar = boolValue("show_toolbar",true);
    mShowStatusBar = boolValue("show_statusbar",true);
    mShowToolWindowBars = boolValue("show_tool_windowbars",true);

    mShowProject = boolValue("show_project",true);
    mShowWatch = boolValue("show_watch",true);
    mShowStructure = boolValue("show_structure",true);
    mShowFiles = boolValue("show_file",true);
    mShowProblemSet = boolValue("show_problem_set",true);

    mShowIssues = boolValue("show_issues",true);
    mShowCompileLog = boolValue("show_compile_log",true);
    mShowDebug = boolValue("show_debug",true);
    mShowSearch = boolValue("show_search",true);
    mShowTODO = boolValue("show_todo",true);
    mShowBookmark = boolValue("show_bookmark",true);
    mShowProblem = boolValue("show_problem",true);

    mProjectOrder = intValue("project_order",1);
    mWatchOrder = intValue("watch_order",2);
    mStructureOrder = intValue("structure_order",3);
    mFilesOrder = intValue("files_order",0);
    mProblemSetOrder = intValue("problemset_order",4);

    mIssuesOrder = intValue("issues_order",0);
    mCompileLogOrder = intValue("compilelog_order",1);
    mDebugOrder = intValue("debug_order",2);
    mSearchOrder = intValue("search_order",3);
    mTODOOrder = intValue("todo_order",4);
    mBookmarkOrder = intValue("bookmark_order",5);
    mProblemOrder = intValue("problem_order",6);

    //dialogs
    QRect geometry = qApp->primaryScreen()->geometry();
    int width = geometry.width();
    int height = geometry.height();

    mCPUDialogWidth = intValue("cpu_dialog_width", 977 * width / 1920);
    mCPUDialogHeight = intValue("cpu_dialog_height", 622 * height / 1080);
    mCPUDialogSplitterPos = intValue("cpu_dialog_splitter", 500 * width / 1920);
    mSettingsDialogWidth = intValue("settings_dialog_width", 977 * width / 1920);
    mSettingsDialogHeight = intValue("settings_dialog_height", 622 * height / 1080);
    mSettingsDialogSplitterPos = intValue("settings_dialog_splitter", 300 * width / 1920);

    mNewProjectDialogWidth = intValue("new_project_dialog_width", 900 * width / 1920);
    mNewProjectDialogHeight = intValue("new_project_dialog_height", 600 * height / 1080);
    mNewClassDialogWidth = intValue("new_class_dialog_width", 642 * width / 1920);
    mNewClassDialogHeight = intValue("new_class_dialog_height", 300 * height / 1080);
    mNewHeaderDialogWidth = intValue("new_header_dialog_width", 642 * width / 1920);
    mNewHeaderDialogHeight = intValue("new_header_dialog_height", 300 * height / 1080);
}

#ifdef ENABLE_VCS
Settings::VCS::VCS(Settings *settings):_Base(settings,SETTING_VCS),
    mGitOk(false)
{
}

void Settings::VCS::doSave()
{
    saveValue("git_path",mGitPath);
}

void Settings::VCS::doLoad()
{
    setGitPath(stringValue("git_path", ""));
}

const QString &Settings::VCS::gitPath() const
{
    return mGitPath;
}

void Settings::VCS::setGitPath(const QString &newGitPath)
{
    if (mGitPath!=newGitPath) {
        mGitPath = newGitPath;
        validateGit();
    }
}

void Settings::VCS::validateGit()
{
    mGitOk = false;
    QFileInfo fileInfo(mGitPath);
    if (!fileInfo.exists()) {
        return;
    }
    mGitOk=true;
//    QStringList args;
//    args.append("--version");
//    QString output = runAndGetOutput(
//                fileInfo.fileName(),
//                fileInfo.absolutePath(),
//                args);
//    mGitOk = output.startsWith("git version");
}

bool Settings::VCS::gitOk() const
{
    return mGitOk;
}

void Settings::VCS::detectGitInPath()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList pathList = path.split(PATH_SEPARATOR);
    QSet<QString> searched;
    foreach (const QString& s, pathList){
        if (searched.contains(s))
            continue;;
        searched.insert(s);
        QDir dir(s);
        if (dir.exists(GIT_PROGRAM)) {
            QString oldPath = mGitPath;
            setGitPath(dir.filePath(GIT_PROGRAM));
            validateGit();
            if (mGitOk) {
                save();
                return;
            } else {
                mGitPath = oldPath;
            }
        }

    }
}
#endif

Settings::Languages::Languages(Settings *settings):
    _Base(settings,SETTING_LANGUAGES)
{
}

Settings::Languages::X86ASMDialect Settings::Languages::x86DialectOfASMGenerated() const
{
    return mX86DialectOfASMGenerated;
}

void Settings::Languages::setX86DialectOfASMGenerated(X86ASMDialect newX86DialectOfASMGenerated)
{
    mX86DialectOfASMGenerated = newX86DialectOfASMGenerated;
}

bool Settings::Languages::indentCSwitchCaseKeywords() const
{
    return mIndentCSwitchCaseKeywords;
}

void Settings::Languages::setIndentCSwitchCaseKeywords(bool newIndentCSwitchCaseKeywords)
{
    mIndentCSwitchCaseKeywords = newIndentCSwitchCaseKeywords;
}

bool Settings::Languages::indentCClassMemberVisibilityKeywords() const
{
    return mIndentCClassMemberVisibilityKeywords;
}

void Settings::Languages::setIndentCClassMemberVisibilityKeywords(bool newIndentCClassMemberVisibilityKeywords)
{
    mIndentCClassMemberVisibilityKeywords = newIndentCClassMemberVisibilityKeywords;
}

void Settings::Languages::doSave()
{
    //ASM
    saveValue("no_debug_directives_when_generate_asm",mNoDebugDirectivesWhenGenerateASM);
    saveValue("no_seh_directives_when_generate_asm",mNoSEHDirectivesWhenGenerateASM);
    saveValue("x86_dialect_of_asm_generated",(int)mX86DialectOfASMGenerated);

    //C/C++
//    saveValue("ident_c_class_member_visibility_keywords",mIndentCClassMemberVisibilityKeywords);
//    saveValue("ident_c_switch_case_keywords",mIndentCSwitchCaseKeywords);
}

void Settings::Languages::doLoad()
{
    mNoDebugDirectivesWhenGenerateASM = boolValue("no_debug_directives_when_generate_asm",true);
    mNoSEHDirectivesWhenGenerateASM = boolValue("no_seh_directives_when_generate_asm",true);
    mX86DialectOfASMGenerated = (X86ASMDialect)intValue("x86_dialect_of_asm_generated",(int)X86ASMDialect::ATT);


    //C/C++
//    mIndentCClassMemberVisibilityKeywords = boolValue("ident_c_class_member_visibility_keywords",false);
//    mIndentCSwitchCaseKeywords = boolValue("ident_c_switch_case_keywords",false);
}

bool Settings::Languages::noSEHDirectivesWhenGenerateASM() const
{
    return mNoSEHDirectivesWhenGenerateASM;
}

void Settings::Languages::setNoSEHDirectivesWhenGenerateASM(bool newNoSEHDirectivesWhenGenerateASM)
{
    mNoSEHDirectivesWhenGenerateASM = newNoSEHDirectivesWhenGenerateASM;
}

bool Settings::Languages::noDebugDirectivesWhenGenerateASM() const
{
    return mNoDebugDirectivesWhenGenerateASM;
}

void Settings::Languages::setNoDebugDirectivesWhenGenerateASM(bool newNoDebugDirectivesWhenGenerateASM)
{
    mNoDebugDirectivesWhenGenerateASM = newNoDebugDirectivesWhenGenerateASM;
}
