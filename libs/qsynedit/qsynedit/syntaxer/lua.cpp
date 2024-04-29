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
#include "lua.h"
#include "../constants.h"
#include "qt_utils/utils.h"

#include <QFont>
#include <QDebug>

namespace QSynedit {

const QSet<QString> LuaSyntaxer::Keywords {
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "goto",
    "if", "in", "local", "nil", "not", "or",
    "repeat", "return", "then", "true", "until",
    "while"
};

const QSet<QString> LuaSyntaxer::StdLibFunctions {
    "assert", "collectgarbage","dofile","error",
    "_G","getmetaobject","ipairs","load","loadfile",
    "next","pairs","pcall","print","rawequal",
    "rawget","rawlen","rawset","select","setmetatable",
    "tonumber","tostring","type","_VERSION","warn",
    "xpcall",
    "require"
};

const QMap<QString,QSet<QString>> LuaSyntaxer::StdLibTables {
    {"coroutine",{"close","create","isyieldable","resume","running",
        "status","wrap","yield"}},
    {"package",{"config","cpath","loaded","loadlib","path",
        "preload","searchers","searchpath"}},
    {"string",{"byte","char","dump","find","format","gmatch",
        "gsub","len","lower","match","pack","packsize","rep",
        "reverse","sub","unpack","upper"}},
    {"utf8",{"char","charpattern","codes","codepoint","len",
        "offset"}},
    {"table",{"concat","insert","move","pack","remove","sort",
        "unpack"}},
    {"math",{"abs","acos","asin","atan","ceil","cos","deg","exp",
        "floor","fmod","huge","log","max","maxinteger","min","mininteger",
        "modf","pi","rad","random","randomseed","sin","sqrt","tan",
        "tointeger","type","ult"}},
    {"io",{"close","flush","input","lines","open","output","popen",
        "read","tmpfile","type","write"}},
    {"file",{"close","flush","lines","read","seek","setvbuf","write"}},
    {"os",{"clock","date","difftime","execute","exit","getenv","remove",
        "rename","setlocale","time","tmpname"}},
    {"debug",{"debug","gethook","getinfo","getlocal","getmetatable","getregistry","getupvalue",
        "getuservalue","sethook","setlocale","setmetatable","setupvalue",
        "setuservalue","traceback","upvalueid","upvaluejoin"}},

};

const QSet<QString> LuaSyntaxer::XMakeLibFunctions {
    "is_os", "is_arch", "is_plat", "is_host", "is_mode", "is_kind",
    "is_config", "has_config", "has_package",

    "includes","set_project","set_version",
    "set_xmakever","add_moduledirs","add_plugindirs",
    "get_config","set_config","add_requires","add_requireconfs",
    "add_repositories","set_defaultplat","set_defaultarchs"
    "set_defaultmode","set_allowedplats","set_allowedarchs",
    "set_allowedmodes",
    "configvar_check_links","configvar_check_ctypes",
    "configvar_check_cxxtypes","configvar_check_cfuncs",
    "configvar_check_cxxfuncs","configvar_check_cincludes",
    "configvar_check_cxxincludes","configvar_check_csnippets",
    "configvar_check_cxxsnippets","configvar_check_features",
    "check_macros","configvar_check_macros",

    "target","target_end","set_kind","set_strip",
    "set_enabled","set_enabled","set_default",
    "set_options","set_symbols","set_basename",
    "set_filename","set_prefixname","set_suffixname",
    "set_extension","set_warnings","set_optimize",
    "set_languages","set_fpmodels","set_targetdir",
    "set_objectdir","set_dependir","add_imports",
    "add_rules","on_load","on_config","on_link",
    "on_build","on_build_file","on_build_files",
    "on_clean","on_package","on_install",
    "on_uninstall","on_run","before_link",
    "before_build","before_build_file",
    "before_build_files","before_clean",
    "before_package","before_install",
    "before_uninstall","before_run",
    "after_link","after_build","after_build_file",
    "after_build_files","after_clean","after_package",
    "after_install","after_uninstall","after_run",
    "set_pcheader","set_pcxxheader","add_deps",
    "add_links","add_syslinks","add_files",
    "remove_files","remove_headerfiles",
    "add_linkdirs","add_rpathdirs",
    "add_includedirs","add_includedirs",
    "add_defines","add_undefines",
    "add_cflags","add_cxflags",
    "add_cxxflags","add_mflags","add_mxflags",
    "add_mxxflags","add_scflags","add_asflags",
    "add_gcflags","add_dcflags","add_rcflags",
    "add_fcflags","add_zcflags","add_cuflags",
    "add_culdflags","add_cugencodes","add_ldflags",
    "add_arflags","add_shflags","add_options",
    "add_packages","add_languages","add_vectorexts",
    "add_frameworks","add_frameworkdirs","set_toolset",
    "set_toolchains","set_plat","set_arch",
    "set_values","add_values","set_rundir",
    "set_runargs","add_runenvs","set_runenv",
    "set_installdir","add_installfiles",
    "add_headerfiles","set_configdir","set_configvar",
    "add_configfiles","set_policy","set_runtimes",
    "set_group","add_filegroups","set_exceptions",

    "option","option_end","add_deps","before_check",
    "on_check","after_check","set_values","set_default",
    "set_showmenu","set_category","set_description",
    "add_links","add_linkdirs","add_rpathdirs",
    "add_cincludes","add_cxxincludes","add_ctypes",
    "add_cxxtypes","add_csnippets","add_cxxsnippets",
    "add_cfuncs","add_cxxfuncs",

    "task","task_end","set_menu","set_category","on_run",

    "rule","rule_end","add_deps","add_imports","set_extensions",
    "on_load","on_config","on_link","on_build",
    "on_clean","on_package","on_install","on_uninstall",
    "on_build_file","on_buildcmd_file","on_build_files",
    "on_buildcmd_files","before_link","before_build",
    "before_clean","before_package","before_install",
    "before_uninstall","before_build_file",
    "before_buildcmd_file","before_build_files",
    "before_buildcmd_files","after_link",
    "after_build","after_clean","after_package",
    "after_install","after_uninstall","after_build_file",
    "after_buildcmd_file","after_build_files",
    "after_buildcmd_files",

    "toolchain","toolchain_end","set_kind","set_toolset",
    "set_sdkdir","set_bindir","on_check","on_load",

};


LuaSyntaxer::LuaSyntaxer(): Syntaxer()
{
    mUseXMakeLibs=false;
    mCharAttribute = std::make_shared<TokenAttribute>(SYNS_AttrCharacter,
                                                            TokenType::Character);
    addAttribute(mCharAttribute);

    mFloatAttribute = std::make_shared<TokenAttribute>(SYNS_AttrFloat,
                                                             TokenType::Number);
    addAttribute(mFloatAttribute);
    mHexAttribute = std::make_shared<TokenAttribute>(SYNS_AttrHexadecimal,
                                                           TokenType::Number);
    addAttribute(mHexAttribute);
    mInvalidAttribute = std::make_shared<TokenAttribute>(SYNS_AttrIllegalChar,
                                                               TokenType::Error);
    addAttribute(mInvalidAttribute);
    mNumberAttribute = std::make_shared<TokenAttribute>(SYNS_AttrNumber,
                                                              TokenType::Number);
    addAttribute(mNumberAttribute);

    mStringEscapeSequenceAttribute = std::make_shared<TokenAttribute>(SYNS_AttrStringEscapeSequences,
                                                                            TokenType::String);
    addAttribute(mStringEscapeSequenceAttribute);
    resetState();
}

const PTokenAttribute &LuaSyntaxer::invalidAttribute() const
{
    return mInvalidAttribute;
}

const PTokenAttribute &LuaSyntaxer::numberAttribute() const
{
    return mNumberAttribute;
}

const PTokenAttribute &LuaSyntaxer::floatAttribute() const
{
    return mFloatAttribute;
}

const PTokenAttribute &LuaSyntaxer::hexAttribute() const
{
    return mHexAttribute;
}

const PTokenAttribute &LuaSyntaxer::stringEscapeSequenceAttribute() const
{
    return mStringEscapeSequenceAttribute;
}

const PTokenAttribute &LuaSyntaxer::charAttribute() const
{
    return mCharAttribute;
}
LuaSyntaxer::TokenId LuaSyntaxer::getTokenId()
{
    return mTokenId;
}

void LuaSyntaxer::commentProc()
{
    mTokenId = TokenId::Comment;
    if (mRun>=mLineSize) {
        nullProc();
        return;
    }
    while (mRun<mLineSize) {
        if (isSpaceChar(mLine[mRun]))
            break;
        mRun++;
    }
    if (mRun<mLineSize) {
        mRange.state = RangeState::rsComment;
    } else
        mRange.state = RangeState::rsUnknown;
}

void LuaSyntaxer::longCommentProc()
{
    mTokenId = TokenId::Comment;
    mRange.state=RangeState::rsLongComment;
    if (mRun>=mLineSize) {
        nullProc();
        return;
    }
    while (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case ' ':
        case '\t':
            return;
        case ']':
            mRun ++;
            mRange.state = RangeState::rsUnknown;
            return;
        default:
            mRun+=1;
        }
    }
}

void LuaSyntaxer::braceCloseProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;

    mRange.braceLevel -= 1;
    if (mRange.braceLevel<0) {
        mRange.braceLevel = 0;
    }
}

void LuaSyntaxer::braceOpenProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;

    mRange.braceLevel += 1;
}

void LuaSyntaxer::colonProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]==':') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void LuaSyntaxer::equalProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1] == '=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void LuaSyntaxer::greaterProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize) {
        switch (mLine[mRun+1].unicode()) {
        case '=':
        case '>':
            mRun += 2;
            return;
        }
    }
    mRun+=1;
}

void LuaSyntaxer::identProc()
{
    int wordEnd = mRun;
    while (wordEnd<mLineSize && isIdentChar(mLine[wordEnd])) {
        wordEnd+=1;
    }
    QString word = mLine.mid(mRun,wordEnd-mRun);
    mRun=wordEnd;
    if (isKeyword(word)) {
        mTokenId = TokenId::Key;
        if (word == "then" || word == "do" || word == "repeat" || word == "function") {
            mRange.blockLevel += 1;
            mRange.blockStarted++;
            pushIndents(IndentType::Block);
        } else if (word == "end" || word =="until") {
            mRange.blockLevel -= 1;
            if (mRange.blockLevel<0) {
                mRange.blockLevel = 0;
            }
            if (mRange.blockStarted>0) {
                mRange.blockStarted--;
            } else {
                mRange.blockEnded++ ;
            }
            popIndents(IndentType::Block);
        }
    } else {
        mTokenId = TokenId::Identifier;
    }
}

void LuaSyntaxer::lowerProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize) {
        switch(mLine[mRun+1].unicode()) {
        case '=':
        case '<':
            mRun+=2;
            return;
        }
    }
    mRun+=1;
}

void LuaSyntaxer::minusProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]=='-') {
        if (mRun+2<mLineSize && mLine[mRun+2]=='[') {
            mRun+=3;
            mTokenId = TokenId::Comment;
            mRange.state=RangeState::rsLongComment;
        } else {
            mRun+=2;
            mTokenId=TokenId::Comment;
            mRange.state = RangeState::rsComment;
        }
    } else
        mRun++;
}

void LuaSyntaxer::nullProc()
{
    mTokenId = TokenId::Null;
}

void LuaSyntaxer::numberProc()
{
    int idx1; // token[1]
    idx1 = mRun;
    mRun+=1;
    mTokenId = TokenId::Number;
    bool shouldExit = false;
    while (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case '\'':
            if (mTokenId != TokenId::Number) {
                mTokenId = TokenId::Symbol;
                return;
            }
            break;
        case '.':
            if (mRun+1<mLineSize && mLine[mRun+1] == '.') {
                mRun+=2;
                mTokenId = TokenId::Unknown;
                return;
            } else if (mTokenId != TokenId::Hex) {
                mTokenId = TokenId::Float;
            } else {
                mTokenId = TokenId::Unknown;
                return;
            }
            break;
        case '-':
        case '+':
            if (mTokenId != TokenId::Float) // number <> float. an arithmetic operator
                return;
            if (mRun-1>=0 && mLine[mRun-1]!= 'e' && mLine[mRun-1]!='E')  // number = float, but no exponent. an arithmetic operator
                return;
            if (mRun+1<mLineSize && (mLine[mRun+1]<'0' || mLine[mRun+1]>'9'))  {// invalid
                mRun+=1;
                mTokenId = TokenId::Unknown;
                return;
            }
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if ( (mLine[idx1]=='0') && (mTokenId != TokenId::Hex)  && (mTokenId != TokenId::Float) ) // invalid octal char
                mTokenId = TokenId::Unknown; // we must continue parse, it may be an float number
            break;
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
            if (mTokenId!=TokenId::Hex) { //invalid
                mTokenId = TokenId::Unknown;
                return;
            }
            break;
        case 'e':
        case 'E':
            if (mTokenId!=TokenId::Hex) {
                if (mRun-1>=0 && (mLine[mRun-1]>='0' || mLine[mRun-1]<='9') ) {//exponent
                    for (int i=idx1;i<mRun;i++) {
                        if (mLine[i] == 'e' || mLine[i]=='E') { // too many exponents
                            mRun+=1;
                            mTokenId = TokenId::Unknown;
                            return;
                        }
                    }
                    if (mRun+1<mLineSize && mLine[mRun+1]!='+' && mLine[mRun+1]!='-' && !(mLine[mRun+1]>='0' && mLine[mRun+1]<='9')) {
                        return;
                    } else {
                        mTokenId = TokenId::Float;
                    }
                } else {
                    mRun+=1;
                    mTokenId = TokenId::Unknown;
                    return;
                }
            }
            break;
        case 'f':
        case 'F':
            if (mTokenId!=TokenId::Hex) {
                for (int i=idx1;i<mRun;i++) {
                    if (mLine[i] == 'f' || mLine[i]=='F') {
                        mRun+=1;
                        mTokenId = TokenId::Unknown;
                        return;
                    }
                }
                if (mTokenId == TokenId::Float) {
                    if (mRun-1>=0 && (mLine[mRun-1]=='l' || mLine[mRun-1]=='L')) {
                        mRun+=1;
                        mTokenId = TokenId::Unknown;
                        return;
                    }
                } else {
                    mTokenId = TokenId::Float;
                }
            }
            break;
        case 'l':
        case 'L':
            for (int i=idx1;i<=mRun-2;i++) {
                if (mLine[i] == 'l' && mLine[i]=='L') {
                    mRun+=1;
                    mTokenId = TokenId::Unknown;
                    return;
                }
            }
            if (mTokenId == TokenId::Float && (mLine[mRun-1]=='f' || mLine[mRun-1]=='F')) {
                mRun+=1;
                mTokenId = TokenId::Unknown;
                return;
            }
            break;
        case 'u':
        case 'U':
            if (mTokenId == TokenId::Float) {
                mRun+=1;
                mTokenId = TokenId::Unknown;
                return;
            } else {
                for (int i=idx1;i<mRun;i++) {
                    if (mLine[i] == 'u' || mLine[i]=='U') {
                        mRun+=1;
                        mTokenId = TokenId::Unknown;
                        return;
                    }
                }
            }
            break;
        case 'x':
        case 'X':
            if ((mRun == idx1+1) && (mLine[idx1]=='0') &&
                    mRun+1<mLineSize &&
                    ((mLine[mRun+1]>='0' && mLine[mRun+1]<='9')
                     || (mLine[mRun+1]>='a' && mLine[mRun+1]<='f')
                     || (mLine[mRun+1]>='A' && mLine[mRun+1]<='F')) ) {
                mTokenId = TokenId::Hex;
            } else {
                mRun+=1;
                mTokenId = TokenId::Unknown;
                return;
            }
            break;
        default:
            shouldExit=true;
        }
        if (shouldExit) {
            break;
        }
        mRun+=1;        
    }
    if (mRun-1>=0 && mLine[mRun-1] == '\'') {
        mTokenId = TokenId::Unknown;
    }
}

void LuaSyntaxer::pointProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1] == '.' ) {
        if (mRun+2<mLineSize && mLine[mRun+2] == '.')
                mRun+=3;
        else
            mRun+=2;
    } else {
        mRun+=1;
    }
}

void LuaSyntaxer::roundCloseProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel--;
    if (mRange.parenthesisLevel<0)
        mRange.parenthesisLevel=0;
    popIndents(IndentType::Parenthesis);
}

void LuaSyntaxer::roundOpenProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel++;
    pushIndents(IndentType::Parenthesis);
}

void LuaSyntaxer::slashProc()
{
    if (mRun+1<mLineSize && mLine[mRun+1]=='/')
        mRun += 2;
    else
        mRun++;
    mTokenId = TokenId::Symbol;
}

void LuaSyntaxer::spaceProc()
{
    mRun += 1;
    mTokenId = TokenId::Space;
    while (mRun<mLineSize && isLexicalSpace(mLine[mRun]))
        mRun+=1;
    if (mRun>=mLineSize) {
        mRange.hasTrailingSpaces = true;
        if (mRange.state==RangeState::rsComment)
            mRange.state = RangeState::rsUnknown;
    }
}

void LuaSyntaxer::squareCloseProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
    mRange.bracketLevel--;
    if (mRange.bracketLevel<0)
        mRange.bracketLevel=0;
    popIndents(IndentType::Bracket);
}

void LuaSyntaxer::squareOpenProc()
{
    int i=mRun+1;
    while (i<mLineSize && mLine[i]=='=')
        i++;
    if (i<mLineSize && mLine[i]=='[') {
        mRange.state=RangeState::rsLongString+(i-mRun-1);
        mRun=i+1;
        mTokenId=TokenId::String;
    } else {
        mRun++;
        mTokenId = TokenId::Symbol;
        mRange.bracketLevel++;
        pushIndents(IndentType::Bracket);
    }
}

//void LuaSyntaxer::stringEndProc()
//{
//    mTokenId = TokenId::String;
//    if (mRun>=mLineSize) {
//        nullProc();
//        return;
//    }
//    mRange.state = RangeState::rsUnknown;

//    while (mRun<mLineSize) {
//        if (mLine[mRun]=='"') {
//            mRun += 1;
//            break;
//        } else if (isSpaceChar(mLine[mRun])) {
//            mRange.state = RangeState::rsString;
//            return;
//        } else if (mLine[mRun]=='\\') {
//            if (mRun == mLineSize-1) {
//                mRun+=1;
//                mRange.state = RangeState::rsMultiLineString;
//                return;
//            }
//            if (mRun+1<mLineSize) {
//                switch(mLine[mRun+1].unicode()) {
//                case '\'':
//                case '"':
//                case '\\':
//                case '?':
//                case 'a':
//                case 'b':
//                case 'f':
//                case 'n':
//                case 'r':
//                case 't':
//                case 'v':
//                case '0':
//                case '1':
//                case '2':
//                case '3':
//                case '4':
//                case '5':
//                case '6':
//                case '7':
//                case '8':
//                case '9':
//                case 'x':
//                case 'u':
//                case 'U':
//                    mRange.state = RangeState::rsMultiLineStringEscapeSeq;
//                    return;
//                }
//            }
//        }
//        mRun += 1;
//    }
//}

void LuaSyntaxer::stringEscapeSeqProc()
{
    mTokenId = TokenId::StringEscapeSeq;
    mRun+=1;
    if (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case '\'':
        case '"':
        case '?':
        case 'a':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
        case '\\':
            mRun+=1;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            for (int i=0;i<3;i++) {
                if (mRun>=mLineSize || mLine[mRun]<'0' || mLine[mRun]>'7')
                    break;
                mRun+=1;
            }
            break;
        case '8':
        case '9':
            mTokenId = TokenId::Unknown;
            mRun+=1;
            break;
        case 'x':
            mRun+=1;
            if (mRun>=mLineSize || !(
                     (mLine[mRun]>='0' && mLine[mRun]<='9')
                   ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
                   ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                    )) {
                mTokenId = TokenId::Unknown;
            } else {
                while (mRun<mLineSize && (
                       (mLine[mRun]>='0' && mLine[mRun]<='9')
                     ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
                     ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                       ))  {
                    mRun+=1;
                }
            }
            break;
        case 'u':
            mRun+=1;
            for (int i=0;i<4;i++) {
                if (mRun>=mLineSize || !(
                            (mLine[mRun]>='0' && mLine[mRun]<='9')
                          ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
                          ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                           )) {
                    mTokenId = TokenId::Unknown;
                    return;
                }
                mRun+=1;
            }
            break;
        case 'U':
            mRun+=1;
            for (int i=0;i<8;i++) {
                if (mRun>=mLineSize || !(
                            (mLine[mRun]>='0' && mLine[mRun]<='9')
                          ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
                          ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                           )) {
                    mTokenId = TokenId::Unknown;
                    return;
                }
                mRun+=1;
            }
            break;
        }
    }
    mRange.state = RangeState::rsString;
}

void LuaSyntaxer::stringProc()
{
    if (mRun>=mLineSize) {
        nullProc();
        return;
    }
    mTokenId = TokenId::String;
    while (mRun < mLineSize) {
        if (mRange.state==RangeState::rsString && mLine[mRun]=='"') {
            mRun++;
            break;
        } else if (mRange.state>=RangeState::rsLongString && mLine[mRun]==']') {
            int i=mRun+1;
            while (i<mLineSize && mLine[i]=='=')
                i++;
            if (i<mLineSize && mLine[i]==']' && (mRange.state-RangeState::rsLongString == i-mRun-1)) {
                mRun=i+1;
                mRange.state = RangeState::rsUnknown;
                return;
            }
        } else if (mLine[mRun]=='\\') {
            if (mRun+1<mLineSize) {
                switch(mLine[mRun+1].unicode()) {
                case '\'':
                case '"':
                case '\\':
                case '?':
                case 'a':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                case 'v':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case 'x':
                case 'u':
                case 'U':
                    mRange.state = RangeState::rsStringEscapeSeq;
                    return;
                }
            }
        }
        mRun+=1;
    }
    if (mRange.state == RangeState::rsString)
        mRange.state = RangeState::rsUnknown;
}

void LuaSyntaxer::tildeProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]=='=') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void LuaSyntaxer::unknownProc()
{
    mRun+=1;
    mTokenId = TokenId::Unknown;
}

void LuaSyntaxer::processChar()
{
    if (mRun>=mLineSize) {
        nullProc();
    } else {
        switch(mLine[mRun].unicode()) {
        case '-':
            minusProc();
            break;
        case '+':
        case '*':
        case '%':
        case '^':
        case '#':
        case '&':
        case '|':
        case ',':
        case ';':
            mTokenId = TokenId::Symbol;
            mRun+=1;
            break;
        case '~':
            tildeProc();
            break;
        case '<':
            lowerProc();
            break;
        case '>':
            greaterProc();
            break;
        case '=':
            equalProc();
            break;
        case '/':
            slashProc();
            break;
        case ':':
            colonProc();
            break;
        case '.':
            pointProc();
            break;

        case '}':
            braceCloseProc();
            break;
        case '{':
            braceOpenProc();
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            numberProc();
            break;
        case ')':
            roundCloseProc();
            break;
        case '(':
            roundOpenProc();
            break;

        case ']':
            squareCloseProc();
            break;
        case '[':
            squareOpenProc();
            break;
        case '"':
            mRange.state=RangeState::rsString;
            mTokenId=TokenId::String;
            mRun++;
            break;
        default:
            if (isIdentChar(mLine[mRun])) {
                identProc();
            } else {
                unknownProc();
            }
        }
    }
}

void LuaSyntaxer::popIndents(IndentType indentType)
{
//    qDebug()<<"----";
//    for (IndentInfo info:mRange.indents)
//        qDebug()<<(int)info.type<<info.line;
//    qDebug()<<"****";
    while (!mRange.indents.isEmpty() && mRange.indents.back().type!=indentType) {
        mRange.indents.pop_back();
    }
    if (!mRange.indents.isEmpty()) {
        mRange.lastUnindent=mRange.indents.back();
        mRange.indents.pop_back();
    } else {
        mRange.lastUnindent=IndentInfo{indentType,0};
    }
}

void LuaSyntaxer::pushIndents(IndentType indentType, int line)
{
    if (line==-1)
        line = mLineNumber;
    mRange.indents.push_back(IndentInfo{indentType,line});
}

bool LuaSyntaxer::useXMakeLibs() const
{
    return mUseXMakeLibs;
}

void LuaSyntaxer::setUseXMakeLibs(bool newUseXMakeLibs)
{
    if (mUseXMakeLibs!=newUseXMakeLibs) {
        mKeywordsCache.clear();
        mUseXMakeLibs = newUseXMakeLibs;
    }
}

QString LuaSyntaxer::commentSymbol()
{
    return "--";
}

QString LuaSyntaxer::blockCommentBeginSymbol()
{
    return "--[";
}

QString LuaSyntaxer::blockCommentEndSymbol()
{
    return "]";
}

bool LuaSyntaxer::supportFolding()
{
    return true;
}

bool LuaSyntaxer::needsLineState()
{
    return false;
}

const QSet<QString> &LuaSyntaxer::customTypeKeywords() const
{
    return mCustomTypeKeywords;
}

void LuaSyntaxer::setCustomTypeKeywords(const QSet<QString> &newCustomTypeKeywords)
{
    mCustomTypeKeywords = newCustomTypeKeywords;
    mKeywordsCache.clear();
}

bool LuaSyntaxer::supportBraceLevel()
{
    return true;
}

QMap<QString, QSet<QString> > LuaSyntaxer::scopedKeywords()
{
    return StdLibTables;
}

bool LuaSyntaxer::isCommentNotFinished(int state) const
{
    return (state == RangeState::rsComment ||
            state == RangeState::rsLongComment);
}

bool LuaSyntaxer::isStringNotFinished(int state) const
{
    return state == RangeState::rsString;
}

bool LuaSyntaxer::eol() const
{
    return mTokenId == TokenId::Null;
}

QString LuaSyntaxer::getToken() const
{
    return mLine.mid(mTokenPos,mRun-mTokenPos);
}

const PTokenAttribute &LuaSyntaxer::getTokenAttribute() const
{
    switch (mTokenId) {
    case TokenId::Comment:
        return mCommentAttribute;
    case TokenId::Identifier:
        return mIdentifierAttribute;
    case TokenId::Key:
        return mKeywordAttribute;
    case TokenId::Number:
        return mNumberAttribute;
    case TokenId::Float:
    case TokenId::HexFloat:
        return mFloatAttribute;
    case TokenId::Hex:
        return mHexAttribute;
    case TokenId::Space:
        return mWhitespaceAttribute;
    case TokenId::String:
        return mStringAttribute;
    case TokenId::StringEscapeSeq:
        return mStringEscapeSequenceAttribute;
    case TokenId::Symbol:
        return mSymbolAttribute;
    case TokenId::Unknown:
        return mInvalidAttribute;
    default:
        return mInvalidAttribute;
    }
}

int LuaSyntaxer::getTokenPos()
{
    return mTokenPos;
}

void LuaSyntaxer::next()
{
    mAsmStart = false;
    mTokenPos = mRun;
    do {
        if (mRun<mLineSize && isSpaceChar(mLine[mRun])) {
            spaceProc();
            break;
        }
        switch (mRange.state) {
        case RangeState::rsComment:
            //qDebug()<<"*0-0-0*";
            commentProc();
            break;
        case RangeState::rsString:
            //qDebug()<<"*1-0-0*";
            stringProc();
            break;
        case RangeState::rsLongComment:
            //qDebug()<<"*2-0-0*";
            longCommentProc();
            break;
//        case RangeState::rsMultiLineString:
//            //qDebug()<<"*4-0-0*";
//            stringEndProc();
//            break;
        case RangeState::rsStringEscapeSeq:
            //qDebug()<<"*6-0-0*";
            stringEscapeSeqProc();
            break;
        default:
            if (mRange.state>=RangeState::rsLongString) {
                stringProc();
            } else {
                //qDebug()<<"*a-0-0*";
                mRange.state = RangeState::rsUnknown;
                if (mRun>=mLineSize) {
                    //qDebug()<<"*b-0-0*";
                    nullProc();
                } else {
                    //qDebug()<<"*f-0-0*";
                    processChar();
                }
            }
        }
    } while (mTokenId!=TokenId::Null && mRun<=mTokenPos);
    //qDebug()<<"1-1-1";
}

void LuaSyntaxer::setLine(const QString &newLine, int lineNumber)
{
    mLine = newLine;
    mLineSize = mLine.size();
    mLineNumber = lineNumber;
    mRun = 0;
    mRange.blockStarted = 0;
    mRange.blockEnded = 0;
    mRange.blockEndedLastLine = 0;
    mRange.lastUnindent=IndentInfo{IndentType::None,0};
    mRange.hasTrailingSpaces = false;
    next();
}

bool LuaSyntaxer::isKeyword(const QString &word)
{
    return Keywords.contains(word) || mCustomTypeKeywords.contains(word);
}

void LuaSyntaxer::setState(const SyntaxState& rangeState)
{
    mRange = rangeState;
    // current line's left / right parenthesis count should be reset before parsing each line
    mRange.blockStarted = 0;
    mRange.blockEnded = 0;
    mRange.blockEndedLastLine = 0;
    mRange.lastUnindent=IndentInfo{IndentType::None,0};
    mRange.hasTrailingSpaces = false;
}

void LuaSyntaxer::resetState()
{
    mRange.state = RangeState::rsUnknown;
    mRange.braceLevel = 0;
    mRange.bracketLevel = 0;
    mRange.parenthesisLevel = 0;
    mRange.blockLevel = 0;
    mRange.blockStarted = 0;
    mRange.blockEnded = 0;
    mRange.blockEndedLastLine = 0;
    mRange.indents.clear();
    mRange.lastUnindent=IndentInfo{IndentType::None,0};
    mRange.hasTrailingSpaces = false;
    mAsmStart = false;
}

QString LuaSyntaxer::languageName()
{
    return "lua";
}

ProgrammingLanguage LuaSyntaxer::language()
{
    return ProgrammingLanguage::LUA;
}

SyntaxState LuaSyntaxer::getState() const
{
    return mRange;
}

bool LuaSyntaxer::isIdentChar(const QChar &ch) const
{
    return ch=='_' || ch.isDigit() || ch.isLetter();
}

bool LuaSyntaxer::isIdentStartChar(const QChar &ch) const
{
    return ch=='_' || ch.isLetter();
}

QSet<QString> LuaSyntaxer::keywords() {
    if (mKeywordsCache.isEmpty()) {
        mKeywordsCache = Keywords;
        mKeywordsCache.unite(mCustomTypeKeywords);
        mKeywordsCache.unite(StdLibFunctions);
        if (mUseXMakeLibs)
            mKeywordsCache.unite(XMakeLibFunctions);
        foreach(const QString& s, StdLibTables.keys())
            mKeywordsCache.insert(s);
    }
    return mKeywordsCache;
}

QString LuaSyntaxer::foldString(QString endLine)
{
    if (endLine.trimmed().startsWith("#"))
        return "...";
    return "...}";
}

}
