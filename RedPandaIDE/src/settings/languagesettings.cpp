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
#include "languagesettings.h"

LanguageSettings::LanguageSettings(SettingsPersistor *persistor):
    BaseSettings{persistor,SETTING_LANGUAGES}
{
}

LanguageSettings::X86ASMDialect LanguageSettings::x86DialectOfASMGenerated() const
{
    return mX86DialectOfASMGenerated;
}

void LanguageSettings::setX86DialectOfASMGenerated(X86ASMDialect newX86DialectOfASMGenerated)
{
    mX86DialectOfASMGenerated = newX86DialectOfASMGenerated;
}

bool LanguageSettings::indentCSwitchCaseKeywords() const
{
    return mIndentCSwitchCaseKeywords;
}

void LanguageSettings::setIndentCSwitchCaseKeywords(bool newIndentCSwitchCaseKeywords)
{
    mIndentCSwitchCaseKeywords = newIndentCSwitchCaseKeywords;
}

bool LanguageSettings::indentCClassMemberVisibilityKeywords() const
{
    return mIndentCClassMemberVisibilityKeywords;
}

void LanguageSettings::setIndentCClassMemberVisibilityKeywords(bool newIndentCClassMemberVisibilityKeywords)
{
    mIndentCClassMemberVisibilityKeywords = newIndentCClassMemberVisibilityKeywords;
}

void LanguageSettings::doSave()
{
    //ASM
    saveValue("no_debug_directives_when_generate_asm",mNoDebugDirectivesWhenGenerateASM);
    saveValue("no_seh_directives_when_generate_asm",mNoSEHDirectivesWhenGenerateASM);
    saveValue("x86_dialect_of_asm_generated",(int)mX86DialectOfASMGenerated);

    //C/C++
//    saveValue("ident_c_class_member_visibility_keywords",mIndentCClassMemberVisibilityKeywords);
//    saveValue("ident_c_switch_case_keywords",mIndentCSwitchCaseKeywords);
}

void LanguageSettings::doLoad()
{
    mNoDebugDirectivesWhenGenerateASM = boolValue("no_debug_directives_when_generate_asm",true);
    mNoSEHDirectivesWhenGenerateASM = boolValue("no_seh_directives_when_generate_asm",true);
    mX86DialectOfASMGenerated = (X86ASMDialect)intValue("x86_dialect_of_asm_generated",(int)X86ASMDialect::ATT);


    //C/C++
//    mIndentCClassMemberVisibilityKeywords = boolValue("ident_c_class_member_visibility_keywords",false);
//    mIndentCSwitchCaseKeywords = boolValue("ident_c_switch_case_keywords",false);
}

bool LanguageSettings::noSEHDirectivesWhenGenerateASM() const
{
    return mNoSEHDirectivesWhenGenerateASM;
}

void LanguageSettings::setNoSEHDirectivesWhenGenerateASM(bool newNoSEHDirectivesWhenGenerateASM)
{
    mNoSEHDirectivesWhenGenerateASM = newNoSEHDirectivesWhenGenerateASM;
}

bool LanguageSettings::noDebugDirectivesWhenGenerateASM() const
{
    return mNoDebugDirectivesWhenGenerateASM;
}

void LanguageSettings::setNoDebugDirectivesWhenGenerateASM(bool newNoDebugDirectivesWhenGenerateASM)
{
    mNoDebugDirectivesWhenGenerateASM = newNoDebugDirectivesWhenGenerateASM;
}
