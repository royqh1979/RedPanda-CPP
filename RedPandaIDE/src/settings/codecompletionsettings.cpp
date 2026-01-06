#include "codecompletionsettings.h"

CodeCompletionSettings::CodeCompletionSettings(SettingsPersistor *persistor):
    BaseSettings{persistor, SETTING_CODE_COMPLETION}
{

}

bool CodeCompletionSettings::showCodeIns() const
{
    return mShowCodeIns;
}

void CodeCompletionSettings::setShowCodeIns(bool newShowCodeIns)
{
    mShowCodeIns = newShowCodeIns;
}

bool CodeCompletionSettings::clearWhenEditorHidden()
{
    return mClearWhenEditorHidden;
}

void CodeCompletionSettings::setClearWhenEditorHidden(bool newClearWhenEditorHidden)
{
    mClearWhenEditorHidden = newClearWhenEditorHidden;
}

int CodeCompletionSettings::minCharRequired() const
{
    return mMinCharRequired;
}

void CodeCompletionSettings::setMinCharRequired(int newMinCharRequired)
{
    mMinCharRequired = newMinCharRequired;
}

bool CodeCompletionSettings::hideSymbolsStartsWithTwoUnderLine() const
{
    return mHideSymbolsStartsWithTwoUnderLine;
}

void CodeCompletionSettings::setHideSymbolsStartsWithTwoUnderLine(bool newHideSymbolsStartsWithTwoUnderLine)
{
    mHideSymbolsStartsWithTwoUnderLine = newHideSymbolsStartsWithTwoUnderLine;
}

bool CodeCompletionSettings::shareParser() const
{
    return mShareParser;
}

void CodeCompletionSettings::setShareParser(bool newShareParser)
{
    mShareParser = newShareParser;
}

bool CodeCompletionSettings::hideSymbolsStartsWithUnderLine() const
{
    return mHideSymbolsStartsWithUnderLine;
}

void CodeCompletionSettings::setHideSymbolsStartsWithUnderLine(bool newHideSymbolsStartsWithOneUnderLine)
{
    mHideSymbolsStartsWithUnderLine = newHideSymbolsStartsWithOneUnderLine;
}

bool CodeCompletionSettings::appendFunc() const
{
    return mAppendFunc;
}

void CodeCompletionSettings::setAppendFunc(bool newAppendFunc)
{
    mAppendFunc = newAppendFunc;
}

bool CodeCompletionSettings::ignoreCase() const
{
    return mIgnoreCase;
}

void CodeCompletionSettings::setIgnoreCase(bool newIgnoreCase)
{
    mIgnoreCase = newIgnoreCase;
}

bool CodeCompletionSettings::showKeywords() const
{
    return mShowKeywords;
}

void CodeCompletionSettings::setShowKeywords(bool newShowKeywords)
{
    mShowKeywords = newShowKeywords;
}

bool CodeCompletionSettings::sortByScope() const
{
    return mSortByScope;
}

void CodeCompletionSettings::setSortByScope(bool newSortByScope)
{
    mSortByScope = newSortByScope;
}

bool CodeCompletionSettings::recordUsage() const
{
    return mRecordUsage;
}

void CodeCompletionSettings::setRecordUsage(bool newRecordUsage)
{
    mRecordUsage = newRecordUsage;
}

bool CodeCompletionSettings::showCompletionWhileInput() const
{
    return mShowCompletionWhileInput;
}

void CodeCompletionSettings::setShowCompletionWhileInput(bool newShowCompletionWhileInput)
{
    mShowCompletionWhileInput = newShowCompletionWhileInput;
}

bool CodeCompletionSettings::parseGlobalHeaders() const
{
    return mParseGlobalHeaders;
}

void CodeCompletionSettings::setParseGlobalHeaders(bool newParseGlobalHeaders)
{
    mParseGlobalHeaders = newParseGlobalHeaders;
}

bool CodeCompletionSettings::parseLocalHeaders() const
{
    return mParseLocalHeaders;
}

void CodeCompletionSettings::setParseLocalHeaders(bool newParseLocalHeaders)
{
    mParseLocalHeaders = newParseLocalHeaders;
}

bool CodeCompletionSettings::enabled() const
{
    return mEnabled;
}

void CodeCompletionSettings::setEnabled(bool newEnabled)
{
    mEnabled = newEnabled;
}

int CodeCompletionSettings::heightInLines() const
{
    return mHeightInLines;
}

void CodeCompletionSettings::setHeightInLines(int newHeight)
{
    mHeightInLines = newHeight;
}

int CodeCompletionSettings::widthInColumns() const
{
    return mWidthInColumns;
}

void CodeCompletionSettings::setWidthInColumns(int newWidth)
{
    mWidthInColumns = newWidth;
}

void CodeCompletionSettings::doSave()
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
    saveValue("clear_when_editor_hidden",mClearWhenEditorHidden);
    saveValue("min_char_required",mMinCharRequired);
    saveValue("hide_symbols_start_with_two_underline", mHideSymbolsStartsWithTwoUnderLine);
    saveValue("hide_symbols_start_with_underline", mHideSymbolsStartsWithUnderLine);
    saveValue("share_parser",mShareParser);
}


void CodeCompletionSettings::doLoad()
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
    mShareParser = boolValue("share_parser",shouldShare);
    mClearWhenEditorHidden = boolValue("clear_when_editor_hidden", mShareParser);
}
