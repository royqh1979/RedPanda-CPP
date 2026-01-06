#ifndef CODE_COMPLETION_SETTINGS_H
#define CODE_COMPLETION_SETTINGS_H
#include "basesettings.h"

#define SETTING_CODE_COMPLETION "CodeCompletion"

class CodeCompletionSettings: public BaseSettings {
public:
    explicit CodeCompletionSettings(SettingsPersistor *settings);
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

    bool clearWhenEditorHidden();
    void setClearWhenEditorHidden(bool newClearWhenEditorHidden);

    int minCharRequired() const;
    void setMinCharRequired(int newMinCharRequired);

    bool hideSymbolsStartsWithUnderLine() const;
    void setHideSymbolsStartsWithUnderLine(bool newHideSymbolsStartsWithOneUnderLine);

    bool hideSymbolsStartsWithTwoUnderLine() const;
    void setHideSymbolsStartsWithTwoUnderLine(bool newHideSymbolsStartsWithTwoUnderLine);

    bool shareParser() const;
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
    bool mClearWhenEditorHidden;
    bool mShareParser;

    // _Base interface
protected:
    void doSave() override;
    void doLoad() override;

};


#endif
//CODE_COMPLETION_SETTINGS_H
