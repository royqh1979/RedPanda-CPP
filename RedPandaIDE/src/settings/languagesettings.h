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
