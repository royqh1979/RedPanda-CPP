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
#include "makefile.h"
#include "../constants.h"
#include <qt_utils/utils.h>
//#include <QDebug>

namespace QSynedit {
const QSet<QString> MakefileSyntaxer::Directives {
    "abspath",
    "addprefix",
    "addsuffix",
    "and",
    "AR",
    "ARFLAGS",
    "AS",
    "ASFLAGS",
    "basename",
    "bindir",

    "call",
    "CC",
    "CFLAGS",
    "CO",
    "COFLAGS",
    "COMSPEC",
    "CPP",
    "CPPFLAGS",
    "CTANGLE",
    "CURDIR",
    "CWEAVE",
    "CXX",
    "CXXFLAGS",

    "define",
    "DESTDIR",
    "dir",

    "else",
    "endef",
    "endif",
    "error",
    "eval",
    "exec_prefix",
    "export",

    "FC",
    "FFLAGS",
    "file",
    "filter",
    "filter-out",
    "findstring",
    "firstword",
    "flavor",
    "foreach",

    "GET",
    "GFLAGS",
    "gmk-eval",
    "gmk-expand",
    "gmk_add_function",
    "gmk_alloc",
    "gmk_eval",
    "gmk_expand",
    "gmk_free",
    "gmk_func_ptr",
    "GNUmakefile",
    "GPATH",
    "guile",

    "if",
    "if",
    "ifdef",
    "ifeq",
    "ifndef",
    "ifneq",
    "include",
    "info",
    "intcmp",

    "join",

    "lastword",
    "LDFLAGS",
    "LDLIBS",
    "let",
    "LEX",
    "LFLAGS",
    "libexecdir",
    "LINT",
    "LINTFLAGS",
    "load",
    "LOADLIBES",

    "M2C",
    "MAKE",
    "MAKE",
    "MAKECMDGOALS",
    "Makefile",
    "makefile",
    "MAKEFILES",
    "MAKEFILES",
    "MAKEFILE_LIST",
    "MAKEFLAGS",
    "MAKEINFO",
    "MAKELEVEL",
    "MAKELEVEL",
    "MAKEOVERRIDES",
    "MAKESHELL"
    "MAKE_HOST",
    "MAKE_RESTARTS"
    "MAKE_TERMERR"
    "MAKE_TERMOUT"
    "MAKE_VERSION",
    "MFLAGS",

    "notdir",

    "or",
    "origin",
    "OUTPUT_OPTION",
    "override",

    "patsubst",
    "patsubst",
    "PC",
    "PFLAGS",
    "prefix",
    "private",

    "realpath",
    "RFLAGS",
    "RM",

    "sbindir",
    "SHELL",
    "shell",
    "sort",
    "strip",
    "subst",
    "subst",
    "suffix",
    "SUFFIXES",

    "TANGLE",
    "TEX",
    "TEXI2DVI",

    "undefine",
    "unexport",

    "value",
    "VPATH",
    "VPATH",
    "vpath",
    "vpath",

    "warning",
    "WEAVE",
    "wildcard",
    "wildcard",
    "word",
    "wordlist",
    "words",

    "YACC",
    "YFLAGS",
};

MakefileSyntaxer::MakefileSyntaxer()
{
    mTargetAttribute = std::make_shared<TokenAttribute>(SYNS_AttrClass, TokenType::Identifier);
    addAttribute(mTargetAttribute);
    mCommandAttribute = std::make_shared<TokenAttribute>(SYNS_AttrGlobalVariable, TokenType::Identifier);
    addAttribute(mCommandAttribute);
    mCommandParamAttribute = std::make_shared<TokenAttribute>(SYNS_AttrPreprocessor, TokenType::Identifier);
    addAttribute(mCommandParamAttribute);
    mNumberAttribute = std::make_shared<TokenAttribute>(SYNS_AttrNumber, TokenType::Number);
    addAttribute(mNumberAttribute);
    mVariableAttribute = std::make_shared<TokenAttribute>(SYNS_AttrLocalVariable, TokenType::Identifier);
    addAttribute(mVariableAttribute);
    mExpressionAttribute = std::make_shared<TokenAttribute>(SYNS_AttrFunction, TokenType::Identifier);
    addAttribute(mExpressionAttribute);
}

void MakefileSyntaxer::procSpace()
{
    mTokenID = TokenId::Space;
    while (mLine[mRun]!=0 && mLine[mRun] <= ' ')
        mRun++;
    if (mRun>=mStringLen)
        mHasTrailingSpaces = true;
}

void MakefileSyntaxer::procNumber()
{
    while (isNumberChar(mLine[mRun]))
        mRun++;
    mTokenID = TokenId::Number;
}

void MakefileSyntaxer::procNull()
{
    mTokenID = TokenId::Null;
    mState = RangeState::Unknown;
}

void MakefileSyntaxer::procString(bool inExpression )
{
    mTokenID = TokenId::String;
    while (mLine[mRun] != 0) {
        if (mState==RangeState::DQString && mLine[mRun] == '\"') {
            mRun++;
            popState();
            break;
        } else if (mState==RangeState::SQString && mLine[mRun] == '\'') {
                mRun++;
                popState();
                break;
        } else if (!inExpression && mLine[mRun] == '$') {
            break;
        } else if (isSpaceChar(mLine[mRun])) {
            break;
        } else
            mRun++;
    }

}

void MakefileSyntaxer::procStringStart(StringStartType type,bool inExpression )
{
    mRun++;
    pushState();
    switch(type) {
    case StringStartType::SingleQuoted:
        mState = RangeState::SQString;
        break;
    case StringStartType::DoubleQuoted:
        mState = RangeState::DQString;
        break;
    }
    procString(inExpression);
}

void MakefileSyntaxer::procExpressionStart(ExpressionStartType type)
{
    mRun+=2; //skip '$(' or '${'
    pushState();
    switch(type) {
    case ExpressionStartType::Brace:
        mState = RangeState::BraceExpression;
        break;
    case ExpressionStartType::Parenthesis:
        mState = RangeState::ParenthesisExpression;
        break;
    }
    mTokenID = TokenId::Expression;
}

void MakefileSyntaxer::procExpressionEnd()
{
    mTokenID = TokenId::Expression;
    mRun+=1;
    popState();
}

void MakefileSyntaxer::procSymbol()
{
    mTokenID = TokenId::Symbol;
    mRun+=1;
}

void MakefileSyntaxer::procVariableExpression()
{
    mRun+=1; //skip $
    while (isIdentStartChar(mLine[mRun]))
        mRun++;
    mTokenID = TokenId::Variable;
}

void MakefileSyntaxer::procAutoVariable()
{
    mRun+=1; //skip $
    switch(mLine[mRun].unicode()) {
    case '@':
    case '%':
    case '<':
    case '?':
    case '^':
    case '+':
    case '|':
    case '*':
    case '$':
        mRun+=1;
        mTokenID=TokenId::Expression;
        break;
    default:
        mTokenID=TokenId::Symbol;
        break;
    }
}

void MakefileSyntaxer::procAssignment()
{
    mTokenID = TokenId::Symbol;
    mRun++;
    mState = RangeState::Assignment;
}

void MakefileSyntaxer::procDollar()
{
    if (mLine[mRun+1]=='(') {
        procExpressionStart(ExpressionStartType::Parenthesis);
    } else if (mLine[mRun+1]=='{') {
        procExpressionStart(ExpressionStartType::Brace);
    } else if (isIdentStartChar(mLine[mRun+1])) {
        procVariableExpression();
    } else {
        procAutoVariable();
    }
}

void MakefileSyntaxer::procComment()
{
    mRun++; //skip #
    mRun = mLineString.length();
    mTokenID = TokenId::Comment;
}

void MakefileSyntaxer::procIdentifier()
{
    int start = mRun;
    while (isIdentChar(mLine[mRun])) {
        mRun++;
    }
    QString s = mLineString.mid(start,mRun-start).toLower();
    if (Directives.contains(s)) {
        mTokenID = TokenId::Directive;
    } else {
        switch(mState) {
        case RangeState::Assignment:
        case RangeState::BraceExpression:
        case RangeState::ParenthesisExpression:
            mTokenID = TokenId::Variable;
            break;
        case RangeState::CommandParameters:
            mTokenID = TokenId::CommandParam;
            break;
        case RangeState::Command:
            mTokenID = TokenId::Command;
            mState = RangeState::CommandParameters;
            break;
        case RangeState::Prequisitions:
        case RangeState::Unknown:
            mTokenID = TokenId::Target;
            break;
        case RangeState::DQString:
        case RangeState::SQString:
            mTokenID = TokenId::String;
            break;
        }
    }
}


void MakefileSyntaxer::pushState()
{
    mStates.push_back(mState);
}

void MakefileSyntaxer::popState()
{
    if (!mStates.empty()) {
        mState = mStates.back();
        mStates.pop_back();
    }
}

bool MakefileSyntaxer::isIdentChar(const QChar &ch) const
{
    if ((ch>='0') && (ch <= '9')) {
        return true;
    }
    if ((ch>='a') && (ch <= 'z')) {
        return true;
    }
    if ((ch>='A') && (ch <= 'Z')) {
        return true;
    }
    switch(ch.unicode()) {
    case '_':
    case '%':
    case '.':
    case '*':
    case '-':
    case '+':
    case '/':
        return true;
    }
    return false;
}

bool MakefileSyntaxer::eol() const
{
    return mTokenID == TokenId::Null;
}

QString MakefileSyntaxer::languageName()
{
    return "makefile";
}

ProgrammingLanguage MakefileSyntaxer::language()
{
    return ProgrammingLanguage::Makefile;
}

QString MakefileSyntaxer::getToken() const
{
    return mLineString.mid(mTokenPos,mRun-mTokenPos);
}

const PTokenAttribute &MakefileSyntaxer::getTokenAttribute() const
{
    /*
        Directive,
        Unknown
    */
    switch(mTokenID) {
    case TokenId::Comment:
        return mCommentAttribute;
    case TokenId::Target:
        return mTargetAttribute;
    case TokenId::Command:
        return mCommandAttribute;
    case TokenId::CommandParam:
        return mCommandParamAttribute;
    case TokenId::Number:
        return mNumberAttribute;
    case TokenId::Space:
        return mWhitespaceAttribute;
    case TokenId::String:
        return mStringAttribute;
    case TokenId::Identifier:
        return mIdentifierAttribute;
    case TokenId::Variable:
        return mVariableAttribute;
    case TokenId::Expression:
        return mExpressionAttribute;
    case TokenId::Symbol:
        return mSymbolAttribute;
    case TokenId::Directive:
        return mKeywordAttribute;
    default:
        return mSymbolAttribute;
    }
}

int MakefileSyntaxer::getTokenPos()
{
    return mTokenPos;
}

void MakefileSyntaxer::next()
{
    mTokenPos = mRun;
    if (mLine[mRun].unicode()==0) {
        procNull();
        return;
    } else if (mRun==0 && mLine[mRun]=='\t') {
        mState = RangeState::Command;
        procSpace();
        return;
    } else if (isSpaceChar(mLine[mRun].unicode())) {        
        procSpace();
        return;
    }
    switch(mState) {
    case RangeState::DQString:
    case RangeState::SQString:
        if (mLine[mRun] == '$')
            procDollar();
        else
            procString(false);
        break;
    case RangeState::Command:
    case RangeState::CommandParameters:
    case RangeState::Prequisitions:
    case RangeState::Assignment:
        switch(mLine[mRun].unicode()) {
        case '$':
            procDollar();
            break;
        case '\"':
            procStringStart(StringStartType::DoubleQuoted,false);
            break;
        case '\'':
            procStringStart(StringStartType::SingleQuoted,false);
            break;
        case '#':
            procComment();
            break;
        case '-':
            if (mState == RangeState::Command)
                procSymbol();
            else
                procIdentifier();
            break;
        default:
            if (mLine[mRun]>='0' && mLine[mRun]<='9') {
                procNumber();
            } else if (isIdentStartChar(mLine[mRun])) {
                procIdentifier();
            } else {
                procSymbol();
            }
        }
        break;
    case RangeState::ParenthesisExpression:
    case RangeState::BraceExpression:
        switch(mLine[mRun].unicode()) {
        case '$':
            procDollar();
            break;
        case ')':
            if (mState == RangeState::ParenthesisExpression)
                procExpressionEnd();
            else
                procSymbol();
            break;
        case '\"':
            procStringStart(StringStartType::DoubleQuoted,true);
            break;
        case '\'':
            procStringStart(StringStartType::SingleQuoted,true);
            break;
        case '}':
            if (mState == RangeState::BraceExpression)
                procExpressionEnd();
            else
                procSymbol();
            break;
        case '#':
            procComment();
            break;
        case '@':
        case '+':
        case '*':
        case '^':
        case '<':
        case '?':
            if (mLine[mRun]=='D' || mLine[mRun]=='F') {
                //auto variable
                mRun+=2;
                mTokenID = TokenId::Variable;
            } else
                procSymbol();
            break;
        case '%':
            if (mLine[mRun]=='D' || mLine[mRun]=='F') {
                //auto variable
                mRun+=2;
                mTokenID = TokenId::Variable;
            } else
                procIdentifier();
            break;
        default:
            if (mLine[mRun]>='0' && mLine[mRun]<='9') {
                procNumber();
            } else if (isIdentStartChar(mLine[mRun])) {
                procIdentifier();
            } else {
                procSymbol();
            }
        }
        break;
    case RangeState::Unknown:
        switch(mLine[mRun].unicode()) {
        case '$':
            procDollar();
            break;
        case '#':
            procComment();
            break;
        case '\"':
            procStringStart(StringStartType::DoubleQuoted,false);
            break;
        case '\'':
            procStringStart(StringStartType::SingleQuoted,false);
            break;
        case '?':
        case '+':
            if (mLine[mRun+1]=='=') {
                mRun++;
                procAssignment();
            } else {
                procSymbol();
            }
            break;
        case ':':
            if (mLine[mRun+1]=='=') {
                mRun++;
                procAssignment();
            } else if (mLine[mRun+1]==':') {
                mRun+=2;
                mTokenID = TokenId::Target;
                mState = RangeState::Prequisitions;
            } else {
                mRun++;
                mTokenID = TokenId::Target;
                mState = RangeState::Prequisitions;
            }
            break;
        case '=':
            procAssignment();
            break;
        default:
            if (mLine[mRun]>='0' && mLine[mRun]<='9') {
                procNumber();
            } else if (isIdentStartChar(mLine[mRun])) {
                procIdentifier();
            } else {
                procSymbol();
            }
        }

    }
}

void MakefileSyntaxer::setLine(const QString &newLine, int lineNumber)
{
    mLineString = newLine;
    mLine = getNullTerminatedStringData(mLineString);
    mLineNumber = lineNumber;
    mRun = 0;
    next();
}

bool MakefileSyntaxer::isCommentNotFinished(int /*state*/) const
{
    return false;
}

bool MakefileSyntaxer::isStringNotFinished(int /*state*/) const
{
    return false;
}

SyntaxState MakefileSyntaxer::getState() const
{
    SyntaxState state;
    state.state = (int)mState;
    state.hasTrailingSpaces = mHasTrailingSpaces;
    return state;
}

void MakefileSyntaxer::setState(const SyntaxState & rangeState)
{
    mState = (RangeState)rangeState.state;
    mStates.clear();
    mHasTrailingSpaces = false;
}

void MakefileSyntaxer::resetState()
{
    mState = RangeState::Unknown;
    mStates.clear();
    mHasTrailingSpaces = false;
}

QSet<QString> MakefileSyntaxer::keywords()
{
    return Directives;
}

QString MakefileSyntaxer::commentSymbol()
{
    return "#";
}

bool MakefileSyntaxer::supportFolding()
{
    return false;
}

bool MakefileSyntaxer::needsLineState()
{
    return true;
}

}
