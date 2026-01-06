#ifndef LANGUAGE_SETTINGS_H
#define LANGUAGE_SETTINGS_H
#include "basesettings.h"

#define SETTING_LANGUAGES "Languages"

class LanguageSettings: public BaseSettings {
public:
    enum class X86ASMDialect {
        ATT,
        Intel
    };
    explicit LanguageSettings(SettingsPersistor *persistor);
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



#endif
//LANGUAGE_SETTINGS_H
