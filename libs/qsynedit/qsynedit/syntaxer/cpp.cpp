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
#include "cpp.h"
#include "../constants.h"
#include "qt_utils/utils.h"

#include <QFont>
#include <QDebug>

#define DATA_KEY_INITIAL_DCHAR_SEQ "initialDCharSeq"
#define DATA_KEY_IN_ATTRIBUTE "inAttribute"

namespace QSynedit {

static const QSet<QString> CppStatementKeyWords {
    "if",
    "for",
    "try",
    "catch",
    "else",
    "while",
    "do"
};
const QSet<QString> CppSyntaxer::ValidIntegerSuffixes {
    "u",
    "ll",
    "z",
    "l",
    "uz",
    "zu",
    "ull",
    "llu",
    "lu",
    "ul"
};


const QSet<QString> CppSyntaxer::Keywords {
    "and",
    "and_eq",
    "bitand",
    "bitor",
    "break",
    "compl",
    "constexpr",
    "const_cast",
    "continue",
    "dynamic_cast",
    "else",
    "explicit",
    "export",
    "extern",
    "false",
    "for",
    "mutable",
    "noexcept",
    "not",
    "not_eq",
    "nullptr",
    "or",
    "or_eq",
    "register",
    "reinterpret_cast",
    "static_assert",
    "static_cast",
    "template",
    "this",
    "thread_local",
    "true",
    "typename",
    "virtual",
    "volatile",
    "xor",
    "xor_eq",
    "delete",
    "delete[]",
    "goto",
    "new",
    "return",
    "throw",
    "using",
    "case",
    "default",

    "alignas",
    "alignof",
    "decltype",
    "if",
    "sizeof",
    "switch",
    "typeid",
    "while",

    "asm",
    "catch",
    "do",
    "namespace",
    "try",

    "atomic_cancel",
    "atomic_commit",
    "atomic_noexcept",
    "concept",
    "consteval",
    "constinit",
    "co_await",
    "co_return",
    "co_yield",
    "reflexpr",
    "requires",

    "auto",
    "bool",
    "char",
    "char8_t",
    "char16_t",
    "char32_t",
    "double",
    "float",
    "int",
    "long",
    "short",
    "signed",
    "unsigned",
    "void",
    "wchar_t",

    "const",
    "inline",

    "class",
    "enum",
    "friend",
    "operator",
    "private",
    "protected",
    "public",
    "static",
    "struct",
    "typedef",
    "union",

    "nullptr",
    "override",
    "final",
    "import",
    "module",
};

const QSet<QString> CppSyntaxer::StandardAttributes {
    "noreturn",
    "carries_dependency",
    "deprecated",
    "fallthrough",
    "nodiscard",
    "maybe_unused",
    "likely",
    "unlikely",
    "no_unique_address",
    "assume",
    "optimize_for_synchronized"
};

CppSyntaxer::CppSyntaxer(): Syntaxer()
{
    mCharAttribute = std::make_shared<TokenAttribute>(SYNS_AttrCharacter,
                                                            TokenType::Character);
    addAttribute(mCharAttribute);

    mClassAttribute = std::make_shared<TokenAttribute>(SYNS_AttrClass,
                                                             TokenType::Identifier);
    addAttribute(mClassAttribute);
    mFloatAttribute = std::make_shared<TokenAttribute>(SYNS_AttrFloat,
                                                             TokenType::Number);
    addAttribute(mFloatAttribute);
    mFunctionAttribute = std::make_shared<TokenAttribute>(SYNS_AttrFunction,
                                                                TokenType::Identifier);
    addAttribute(mFunctionAttribute);
    mGlobalVarAttribute = std::make_shared<TokenAttribute>(SYNS_AttrGlobalVariable,
                                                                 TokenType::Identifier);
    addAttribute(mGlobalVarAttribute);
    mHexAttribute = std::make_shared<TokenAttribute>(SYNS_AttrHexadecimal,
                                                           TokenType::Number);
    addAttribute(mHexAttribute);
    mInvalidAttribute = std::make_shared<TokenAttribute>(SYNS_AttrIllegalChar,
                                                               TokenType::Error);
    addAttribute(mInvalidAttribute);
    mLocalVarAttribute = std::make_shared<TokenAttribute>(SYNS_AttrLocalVariable,
                                                                TokenType::Identifier);
    addAttribute(mLocalVarAttribute);
    mNumberAttribute = std::make_shared<TokenAttribute>(SYNS_AttrNumber,
                                                              TokenType::Number);
    addAttribute(mNumberAttribute);
    mOctAttribute = std::make_shared<TokenAttribute>(SYNS_AttrOctal,
                                                           TokenType::Number);
    addAttribute(mOctAttribute);
    mPreprocessorAttribute = std::make_shared<TokenAttribute>(SYNS_AttrPreprocessor,
                                                                    TokenType::Preprocessor);
    addAttribute(mPreprocessorAttribute);

    mStringEscapeSequenceAttribute = std::make_shared<TokenAttribute>(SYNS_AttrStringEscapeSequences,
                                                                            TokenType::String);
    addAttribute(mStringEscapeSequenceAttribute);
    mVariableAttribute = std::make_shared<TokenAttribute>(SYNS_AttrVariable,
                                                                TokenType::Identifier);
    addAttribute(mVariableAttribute);
    resetState();
}

bool CppSyntaxer::isInAttribute(const SyntaxState &state)
{
    return state.extraData.contains(DATA_KEY_IN_ATTRIBUTE)
           && state.extraData[DATA_KEY_IN_ATTRIBUTE].toBool();
}

void CppSyntaxer::procAndSymbol()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize) {
        switch (mLine[mRun+1].unicode()) {
        case '=':
            mRun+=2;
            return;
        case '&':
            mRun+=2;
            return;
        }
    }
    mRun+=1;
}

void CppSyntaxer::procCppStyleComment()
{
    if (mRun>=mLineSize) {
        procNull();
        return;
    }
    mTokenId = TokenId::Comment;
    bool isWord = isIdentChar(mLine[mRun]);
    while (mRun<mLineSize) {
        if (isSpaceChar(mLine[mRun])) {
            break;
        } else {
            if (isWord) {
                if (!isIdentChar(mLine[mRun]))
                    break;
            } else {
                if (isIdentChar(mLine[mRun]))
                    break;
            }
        }
        mRun++;
    }
    if (mRun<mLineSize) {
        mRange.state = RangeState::rsCppComment;
    } else if (mRun-1>=0 && mLine[mRun-1] == '\\' ) { // continues on next line
        mRange.state = RangeState::rsCppComment;
    } else
        mRange.state = RangeState::rsUnknown;
}

void CppSyntaxer::procDocstring()
{
    mTokenId = TokenId::Comment;
    if (mRun>=mLineSize) {
        procNull();
        return;
    }
    bool isWord = isIdentChar(mLine[mRun]);
    while (mRun<mLineSize) {
        if(isSpaceChar(mLine[mRun]))
            break;
        else if (mLine[mRun] == '*') {
            if (mRun+1<mLineSize && mLine[mRun+1] == '/') {
                if (isWord)
                    break;
                mRun += 2;
                mRange.state = RangeState::rsUnknown;
                break;
            }
        } else {
            if (isWord) {
                if (!isIdentChar(mLine[mRun]))
                    break;
            } else {
                if (isIdentChar(mLine[mRun]))
                    break;
            }
        }
        mRun++;
    }
}

void CppSyntaxer::procAnsiCStyleComment()
{
    mTokenId = TokenId::Comment;
    if (mRun>=mLineSize) {
        procNull();
        return;
    }
    bool isWord = isIdentChar(mLine[mRun]);
    while (mRun<mLineSize) {
        if(isSpaceChar(mLine[mRun])) {
            break;
        } else if (mLine[mRun] == '*') {
            if (mRun+1<mLineSize && mLine[mRun+1] == '/') {
                if (isWord)
                    break;
                mRun += 2;
                if (mRange.state == RangeState::rsDirectiveComment &&
                            mRun<mLineSize && mLine[mRun]!='\r' && mLine[mRun]!='\n') {
                    mRange.state = RangeState::rsMultiLineDirective;
                } else {
                    mRange.state = RangeState::rsUnknown;
                }
                break;
            }
        } else {
            if (isWord) {
                if (!isIdentChar(mLine[mRun]))
                    break;
            } else {
                if (isIdentChar(mLine[mRun]))
                    break;
            }
        }
        mRun++;
    }
}

void CppSyntaxer::procAsciiChar()
{
    mTokenId = TokenId::Char;
    while (mRun < mLineSize) {
        if (mLine[mRun] =='\'') {
            mRun++;
            mRange.state = RangeState::rsUnknown;
            return;
        } if (mLine[mRun] == '\\') {
            if (mRun+1>=mLineSize) {
                mRun++;
                mRange.state = RangeState::rsCharEscaping;
                return;
            } else if (mLine[mRun+1] == '\'' || mLine[mRun+1] == '\\') {
                mRun+=1;
            }
        } else if (isSpaceChar(mLine[mRun])) {
            return;
        }
        mRun+=1;
    }
}

void CppSyntaxer::procBraceClose()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    if (mRange.state == RangeState::rsDefineRemaining)
        return;

    mRange.braceLevel -= 1;
    mRange.blockLevel -= 1;
    if (mRange.braceLevel<0) {
        mRange.braceLevel = 0;
        mRange.blockLevel = 0;
    }
    if (mRange.blockStarted>0) {
        mRange.blockStarted--;
    } else {
        mRange.blockEnded++ ;
    }
    popIndents(IndentType::Block);
}

void CppSyntaxer::procBraceOpen()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    if (mRange.state == RangeState::rsDefineRemaining)
        return;

    mRange.braceLevel += 1;
    mRange.blockLevel += 1;
    mRange.blockStarted++;
    if (mRange.getLastIndentType() == IndentType::Statement) {
        // if last indent is started by 'if' 'for' etc
        // just replace it
        int lastLine=-1;
        if (!mRange.indents.isEmpty()) {
            lastLine=mRange.indents.back().line;
        } else {
            lastLine=0;
        }
        while (mRange.getLastIndentType() == IndentType::Statement) {
            popIndents(IndentType::Statement);
        }
        pushIndents(IndentType::Block, lastLine);
    } else
        pushIndents(IndentType::Block);
}

void CppSyntaxer::procColon()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]==':') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void CppSyntaxer::procComma()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
}

void CppSyntaxer::procDirective()
{
    QString preContents = mLine.left(mRun).trimmed();
    if (!preContents.isEmpty()) { // '#' is not first non-space char on the line, treat it as an invalid char
       mTokenId = TokenId::Unknown;
       mRun+=1;
       return;
    }
    mTokenId = TokenId::Directive;
    mRun+=1;
    //skip spaces
    while (mRun < mLineSize && isSpaceChar(mLine[mRun])) {
        mRun+=1;
    }

    QString directive;
    while (mRun < mLineSize && isIdentChar(mLine[mRun])) {
        directive+=mLine[mRun];
        mRun+=1;
    }
    if (directive == "define") {
        mRange.state = RangeState::rsDefineIdentifier;
    } else {
        mRange.state = RangeState::rsUnknown;
        if (directive=="if"
                ||  directive=="ifdef"
                ||  directive=="ifndef"
                ) {
            mRange.blockLevel++;
            mRange.blockStarted=1;
        } else if (directive=="else"
                   ||  directive=="elif"
                   ||  directive=="elifdef"
                   ||  directive=="elifndef") {
            mRange.blockStarted=1;
            mRange.blockEnded=1;
        } else if (directive=="endif") {
            mRange.blockLevel--;
            mRange.blockEnded=1;
        }
    }
}

void CppSyntaxer::procDefineIdent()
{
    mTokenId = TokenId::Identifier;

    while(mRun < mLineSize && isIdentChar(mLine[mRun]))
        mRun++;

    if (mRun<mLineSize)
        mRange.state = RangeState::rsDefineRemaining;
    else
        mRange.state = RangeState::rsUnknown;
}

void CppSyntaxer::procDefineRemaining()
{
    mTokenId = TokenId::Directive;
    while (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case ' ':
        case '\t':
            return;
        case '/': //comment?
            if (mRun+1<mLineSize) {
                switch (mLine[mRun+1].unicode()) {
                case '/': // is end of directive as well
                    mRange.state = RangeState::rsUnknown;
                    return;
                case '*': // might be embeded only
                    mRange.state = RangeState::rsDirectiveComment;
                    return;
                }
            }
            break;
        case '\\': // yet another line?
            if (mRun == mLineSize-1) {
                mRun+=1;
                mRange.state = RangeState::rsMultiLineDirective;
                return;
            }
            break;
        }
        mRun+=1;
    }
    mRange.state=RangeState::rsUnknown;
}

void CppSyntaxer::procDirectiveEnd()
{
    mTokenId = TokenId::Directive;
    if (mRun >= mLineSize) {
        procNull();
        return;
    }
    mRange.state = RangeState::rsUnknown;
    do {
        switch(mLine[mRun].unicode()) {
        case '/': //comment?
            if (mRun+1<mLineSize) {
                switch (mLine[mRun+1].unicode()) {
                case '/': // is end of directive as well
                    mRange.state = RangeState::rsUnknown;
                    return;
                case '*': // might be embeded only
                    mRange.state = RangeState::rsDirectiveComment;
                    return;
                }
            }
            break;
        case '\\': // yet another line?
              if (mRun == mLineSize-1) {
                  mRun+=1;
                  mRange.state = RangeState::rsMultiLineDirective;
                  return;
            }
            break;
        }
        mRun+=1;
    } while (mRun < mLineSize);
}

void CppSyntaxer::procEqual()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1] == '=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void CppSyntaxer::procGreater()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize) {
        switch (mLine[mRun+1].unicode()) {
        case '=':
            mRun += 2;
            if (mRun<mLineSize && mLine[mRun]=='>') // C++20 <=>
                mRun++;
            return;
        case '>':
            if (mRun+2<mLineSize && mLine[mRun+2] == '=') { // >>=
                mRun+=3;
            } else {
                mRun += 2; // >>
            }
            return;
        }
    }
    mRun+=1;
}

void CppSyntaxer::procIdentifier()
{
    int wordEnd = mRun;
    while (wordEnd<mLineSize && isIdentChar(mLine[wordEnd])) {
        wordEnd+=1;
    }
    QString word = mLine.mid(mRun,wordEnd-mRun);
    mRun=wordEnd;
    if (isKeyword(word)) {
        mTokenId = TokenId::Key;
        if (CppStatementKeyWords.contains(word)) {
            pushIndents(IndentType::Statement);
        }
    } else if (isInAttribute(mRange) && StandardAttributes.contains(word)) {
        mTokenId = TokenId::Key;
    } else {
        mTokenId = TokenId::Identifier;
    }
}

void CppSyntaxer::procLower()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize) {
        switch(mLine[mRun+1].unicode()) {
        case '=':
            mRun+=2;
            return;
        case '<':
            if (mRun+2<mLineSize && mLine[mRun+2] == '=') {
                mRun+=3;
            } else {
                mRun+=2;
            }
            return;
        }
    }
    mRun+=1;
}

void CppSyntaxer::procMinus()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize) {
        switch(mLine[mRun+1].unicode()) {
        case '=':
            mRun += 2;
            return;
        case '-':
            mRun += 2;
            return;
        case '>':
            if (mRun+2<mLineSize && mLine[mRun+2]=='*') {
                mRun += 3;
            } else {
                mRun += 2;
            }
            return;
        }
    }
    mRun += 1;
}

void CppSyntaxer::procMod()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]=='=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void CppSyntaxer::procNot()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]=='=') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void CppSyntaxer::procNull()
{
    mTokenId = TokenId::Null;
}

void CppSyntaxer::procNumber(bool isFloat)
{
    if (isFloat) {
        mTokenId = TokenId::Float;
        mRun++;
    } else {
        mTokenId = TokenId::Number;
        if (mRun+1<mLineSize && mLine[mRun]=='0') {
            if (mLine[mRun+1]=='x' || mLine[mRun+1]=='X') {
                mTokenId=TokenId::Hex;
                mRun+=2;
            } else if (mLine[mRun+1]=='b' || mLine[mRun+1]=='B') {
                    mTokenId=TokenId::Binary;
                    mRun+=2;
            } else if (mLine[mRun+1]>='0' && mLine[mRun+1]<='7') {
                mTokenId=TokenId::Octal;
                mRun+=2;
            }
        } else
            mRun+=1;
    }
    switch(mTokenId) {
    case TokenId::Hex:
        procHexNumber();
        break;
    case TokenId::Octal:
        procOctNumber();
        break;
    case TokenId::Binary:
        procBinNumber();
        break;
    default:
        procDecNumber();
    }
}

void CppSyntaxer::procDecNumber()
{

    while (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case '.':
            if (mTokenId != TokenId::Number) {
                mTokenId = TokenId::Unknown;
                return;
            }
            mTokenId = TokenId::Float;
            mRun++;
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
        case '\'':
            mRun++;
            break;
        case 'e':
        case 'E':
            mTokenId = TokenId::Float;
            mRun++;
            if (mRun < mLineSize && (mLine[mRun]== '+' || mLine[mRun]== '-'))  // number = float, but no exponent. an arithmetic operator
                mRun++;
            break;
        default:
            procNumberSuffix();
            return;
        }
    }
}

void CppSyntaxer::procHexNumber()
{

    while (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case '.':
            if (mTokenId != TokenId::Hex) {
                mTokenId = TokenId::Unknown;
                return;
            }
            mTokenId = TokenId::HexFloat;
            mRun++;
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
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case '\'':
            mRun++;
            break;
        case 'p':
        case 'P':
            mTokenId = TokenId::HexFloat;
            mRun++;
            if (mRun < mLineSize && (mLine[mRun]== '+' || mLine[mRun]== '-'))  // number = float, but no exponent. an arithmetic operator
                mRun++;
            break;
        default:
            procNumberSuffix();
            return;
        }
    }
}

void CppSyntaxer::procOctNumber()
{

    while (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '\'':
            mRun++;
            break;
        default:
            procIntegerSuffix();
            return;
        }
    }
}

void CppSyntaxer::procBinNumber()
{
    while (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case '0':
        case '1':
        case '\'':
            mRun++;
            break;
        default:
            procIntegerSuffix();
            return;
        }
    }
}

void CppSyntaxer::procNumberSuffix()
{
    if (mTokenId==TokenId::HexFloat
            || mTokenId==TokenId::Float )
        procFloatSuffix();
    else
        procIntegerSuffix();
}

void CppSyntaxer::procIntegerSuffix()
{
    if (mRun>=mLineSize)
        return;
    int i=mRun;
    bool shouldExit = false;
    while (i<mLineSize && !shouldExit) {
        switch (mLine[i].unicode()) {
        case 'u':
        case 'U':
        case 'l':
        case 'L':
        case 'z':
        case 'Z':
            i++;
            break;
        default:
            shouldExit=true;
        }
    }
    if (i>mRun) {
        QString s=mLine.mid(mRun,i-mRun).toLower();
        if (ValidIntegerSuffixes.contains(s)) {
            mRun=i;
        }
    }
    return ;
}

void CppSyntaxer::procFloatSuffix()
{
    if (mRun>=mLineSize)
        return;
    switch (mLine[mRun].unicode()) {
    case 'f':
    case 'F':
    case 'l':
    case 'L':
        mRun++;
        break;
    }
    return ;
}

void CppSyntaxer::procOr()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize) {
        switch ( mLine[mRun+1].unicode()) {
        case '=':
            mRun+=2;
            return;
        case '|':
            mRun+=2;
            return;
        }
    }
    mRun+=1;
}

void CppSyntaxer::procPlus()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize) {
        switch(mLine[mRun+1].unicode()){
        case '=':
            mRun+=2;
            return;
        case '+':
            mRun+=2;
            return;
        }
    }
    mRun+=1;
}

void CppSyntaxer::procPoint()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1] == '*' ) {
        mRun+=2;
    } else if (mRun+2<mLineSize && mLine[mRun+1] == '.' && mLine[mRun+2] == '.') {
        mRun+=3;
    } else if (mRun+1<mLineSize && mLine[mRun+1]>='0' && mLine[mRun+1]<='9') {
        procNumber(true);
    } else {
        mRun+=1;
    }
}

void CppSyntaxer::procQuestion()
{
    mTokenId = TokenId::Symbol;
    mRun+=1;
}

void CppSyntaxer::procRawString()
{
    mTokenId = TokenId::RawString;
    QString rawStringInitialDCharSeq;
    if (mRange.state == RangeState::rsRawString)
        mRange.extraData[DATA_KEY_INITIAL_DCHAR_SEQ] = "";
    while (mRun<mLineSize) {
        if (mRange.state!=RangeState::rsRawStringNotEscaping &&
                (mLine[mRun].isSpace()
                 || mLine[mRun].unicode()>127
                 || mLine[mRun].unicode()<=32)) {
            mRange.state = RangeState::rsUnknown;
            mRun+=1;
            return;
        }
        switch (mLine[mRun].unicode()) {
        case ' ':
        case '\t':
            return;
        case '(':
            if (mRange.state==RangeState::rsRawString) {
                mRange.state = RangeState::rsRawStringNotEscaping;
                mRange.extraData[DATA_KEY_INITIAL_DCHAR_SEQ] = rawStringInitialDCharSeq;
            }
            break;
        case ')':
            if (mRange.state == RangeState::rsRawStringNotEscaping) {
                rawStringInitialDCharSeq = mRange.extraData[DATA_KEY_INITIAL_DCHAR_SEQ].toString();
                if ( mLine.mid(mRun+1,rawStringInitialDCharSeq.length()) == rawStringInitialDCharSeq) {
                    mRun = mRun+rawStringInitialDCharSeq.length();
                    mRange.state = RangeState::rsRawStringEnd;
                    mRange.extraData.remove(DATA_KEY_INITIAL_DCHAR_SEQ);
                }
            }
            break;
        case '\"':
            if (mRange.state == RangeState::rsRawStringEnd) {
                mRange.state = RangeState::rsUnknown;
                mRun++;
                return;
            }
            break;
        }
        if (mRange.state == RangeState::rsRawString)
            rawStringInitialDCharSeq += mLine[mRun];
        mRun++;
    }
}

void CppSyntaxer::procRoundClose()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel--;
    if (mRange.parenthesisLevel<0)
        mRange.parenthesisLevel=0;
    popIndents(IndentType::Parenthesis);
}

void CppSyntaxer::procRoundOpen()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel++;
    pushIndents(IndentType::Parenthesis);
}

void CppSyntaxer::procSemiColon()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    while (mRange.getLastIndentType() == IndentType::Statement) {
        popIndents(IndentType::Statement);
    }
}

void CppSyntaxer::procSlash()
{
    if (mRun+1<mLineSize) {
        switch(mLine[mRun+1].unicode()) {
        case '/': // Cpp style comment
            mTokenId = TokenId::Comment;
            mRun+=2;
            if (mRun<mLineSize)
                mRange.state = RangeState::rsCppComment;
            else
                mRange.state = RangeState::rsUnknown;
            return;
        case '*': // C style comment
            mTokenId = TokenId::Comment;
            if (mRange.state == RangeState::rsDirective) {
                mRange.state = RangeState::rsDirectiveComment;
            } else {
                mRange.state = RangeState::rsAnsiC;
            }
            mRun += 2;
            if (mRun < mLineSize) {
                if (mRange.state == RangeState::rsAnsiC && mLine[mRun] == '*' ) {
                    mRange.state = RangeState::rsDocstring;
                }
            }
            return;
        case '=':
            mRun+=2;
            mTokenId = TokenId::Symbol;
            return;
        }
    }
    mRun += 1;
    mTokenId = TokenId::Symbol;
}

void CppSyntaxer::procBackSlash()
{
    if (mRun+1==mLineSize-1) {
        mTokenId = TokenId::Symbol;
    } else {
        mTokenId = TokenId::Unknown;
    }
    mRun+=1;
}

void CppSyntaxer::procSpace()
{
    mRun += 1;
    mTokenId = TokenId::Space;
    while (mRun < mLineSize && isLexicalSpace(mLine[mRun]))
        mRun+=1;
    if (mRun>=mLineSize) {
        mRange.hasTrailingSpaces = true;
        if (mRange.state==RangeState::rsCppComment
                || mRange.state == RangeState::rsDefineRemaining)
            mRange.state = RangeState::rsUnknown;
    }
}

void CppSyntaxer::procSquareClose()
{
    mRun++;
    if (mRun < mLineSize && mLine[mRun]==']') {
        if (mRange.extraData.contains(DATA_KEY_IN_ATTRIBUTE)
                && mRange.extraData[DATA_KEY_IN_ATTRIBUTE].toBool()) {
            mTokenId = TokenId::Symbol;
            mRun++;
            mRange.extraData.remove(DATA_KEY_IN_ATTRIBUTE);
            return;
        }
    }
    mTokenId = TokenId::Symbol;
    mRange.bracketLevel--;
    if (mRange.bracketLevel<0)
        mRange.bracketLevel=0;
    popIndents(IndentType::Bracket);
}

void CppSyntaxer::procSquareOpen()
{
    mRun++;
    if (mRun < mLineSize && mLine[mRun]=='[') {
        mRun++;
        mTokenId = TokenId::Symbol;
        mRange.extraData[DATA_KEY_IN_ATTRIBUTE]=true;
    } else{
        mTokenId = TokenId::Symbol;
        mRange.bracketLevel++;
        pushIndents(IndentType::Bracket);
    }
}

void CppSyntaxer::procStar()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1] == '=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void CppSyntaxer::procStringEscapeSeq()
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
                if (mRun>=mLineSize ||
                   (mLine[mRun]<'0' || mLine[mRun]>'7')
                   )
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
            while (mRun<mLineSize && (
                   (mLine[mRun]>='0' && mLine[mRun]<='9')
                 ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
                 ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                   ))  {
                mRun+=1;
            }
            break;
        case 'o':
            mRun+=1;
            while (mRun<mLineSize && (
                   (mLine[mRun]>='0' && mLine[mRun]<='7')
                   ))  {
                mRun+=1;
            }
            break;
        case 'u':
            mRun+=5;
            break;
        case 'U':
            mRun+=9;
            break;
        }
    }
    mRange.state = RangeState::rsString;
}

void CppSyntaxer::procString()
{
    if (mRun >= mLineSize) {
        mRange.state = RangeState::rsStringUnfinished;
        return;
    }
    mTokenId = TokenId::String;
    bool isWord = isIdentChar(mLine[mRun]);
    while (mRun < mLineSize) {
        if (mLine[mRun]=='"') {
            if (isWord)
                break;
            mRun++;
            mRange.state = RangeState::rsUnknown;
            return;
        } else if (mLine[mRun]==' ' || mLine[mRun]=='\t') {
            return;
        } else if (mLine[mRun]=='\\') {
            if (mRun == mLineSize-1) {
                mRun++;
                mRange.state = RangeState::rsStringNextLine;
                return;
            }
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
        } else {
            if (isWord) {
                if (!isIdentChar(mLine[mRun])) {
                    break;
                }
            } else {
                if (isIdentChar(mLine[mRun])) {
                    break;
                }
            }
        }
        mRun+=1;
    }
    // here mRun>=mLineSize
    mRange.state = RangeState::rsStringUnfinished;
}

void CppSyntaxer::procStringStart()
{
    mTokenId = TokenId::String;
    mRange.state = RangeState::rsString;
    mRun++; //skip \"
    if (mRun>=mLineSize) {
        mRange.state = RangeState::rsStringUnfinished;
        return;
    }
    //procString();
}

void CppSyntaxer::procTilde()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
}

void CppSyntaxer::procUnknown()
{
    mRun+=1;
    mTokenId = TokenId::Unknown;
}

void CppSyntaxer::procXor()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]=='=') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void CppSyntaxer::processChar()
{
    if (mRun>=mLineSize) {
        procNull();
    } else {
        switch(mLine[mRun].unicode()) {
        case '&':
            procAndSymbol();
            break;
        case '\'':
            mTokenId = TokenId::Char;
            mRange.state = RangeState::rsChar;
            mRun++;
            procAsciiChar();
            break;
        case '}':
            procBraceClose();
            break;
        case '{':
            procBraceOpen();
            break;
        case ':':
            procColon();
            break;
        case ',':
            procComma();
            break;
        case '#':
            procDirective();
            break;
        case '=':
            procEqual();
            break;
        case '>':
            procGreater();
            break;
        case '?':
            procQuestion();
            break;
        case '<':
            procLower();
            break;
        case '-':
            procMinus();
            break;
        case '%':
            procMod();
            break;
        case '!':
            procNot();
            break;
        case '\\':
            procBackSlash();
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
            procNumber();
            break;
        case '|':
            procOr();
            break;
        case '+':
            procPlus();
            break;
        case '.':
            procPoint();
            break;
        case ')':
            procRoundClose();
            break;
        case '(':
            procRoundOpen();
            break;
        case ';':
            procSemiColon();
            break;
        case '/':
            procSlash();
            break;
        case ']':
            procSquareClose();
            break;
        case '[':
            procSquareOpen();
            break;
        case '*':
            procStar();
            break;
        case '"':
            procStringStart();
            break;
        case '~':
            procTilde();
            break;
        case '^':
            procXor();
            break;
        default:
            if (isIdentStartChar(mLine[mRun])) {
                procIdentifier();
            } else {
                procUnknown();
            }
        }
    }
}

void CppSyntaxer::popIndents(IndentType indentType)
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

void CppSyntaxer::pushIndents(IndentType indentType, int line)
{
    if (line==-1)
        line = mLineNumber;
    mRange.indents.push_back(IndentInfo{indentType,line});
}

const QSet<QString> &CppSyntaxer::customTypeKeywords() const
{
    return mCustomTypeKeywords;
}

void CppSyntaxer::setCustomTypeKeywords(const QSet<QString> &newCustomTypeKeywords)
{
    mCustomTypeKeywords = newCustomTypeKeywords;
}

bool CppSyntaxer::supportBraceLevel()
{
    return true;
}

QString CppSyntaxer::commentSymbol()
{
    return "//";
}

QString CppSyntaxer::blockCommentBeginSymbol()
{
    return "/*";
}

QString CppSyntaxer::blockCommentEndSymbol()
{
    return "*/";
}

bool CppSyntaxer::supportFolding()
{
    return true;
}

bool CppSyntaxer::needsLineState()
{
    return true;
}

bool CppSyntaxer::isCommentNotFinished(int state) const
{
    return (state == RangeState::rsAnsiC ||
            state == RangeState::rsDirectiveComment||
            state == RangeState::rsDocstring ||
            state == RangeState::rsCppComment);
}

bool CppSyntaxer::isStringNotFinished(int state) const
{
    return state == RangeState::rsString || state==RangeState::rsStringNextLine || state==RangeState::rsStringUnfinished;
}

bool CppSyntaxer::isDocstringNotFinished(int state) const
{
    return state == RangeState::rsDocstring;
}

bool CppSyntaxer::eol() const
{
    return mTokenId == TokenId::Null;
}

QString CppSyntaxer::getToken() const
{
    return mLine.mid(mTokenPos,mRun-mTokenPos);
}

const PTokenAttribute &CppSyntaxer::getTokenAttribute() const
{
    switch (mTokenId) {
    case TokenId::Comment:
        return mCommentAttribute;
    case TokenId::Directive:
        return mPreprocessorAttribute;
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
    case TokenId::Octal:
        return mOctAttribute;
    case TokenId::Binary:
        return mOctAttribute;
    case TokenId::Space:
        return mWhitespaceAttribute;
    case TokenId::String:
        return mStringAttribute;
    case TokenId::StringEscapeSeq:
        return mStringEscapeSequenceAttribute;
    case TokenId::RawString:
        return mStringAttribute;
    case TokenId::Char:
        return mCharAttribute;
    case TokenId::Symbol:
        return mSymbolAttribute;
    case TokenId::Unknown:
        return mInvalidAttribute;
    default:
        return mInvalidAttribute;
    }
}

int CppSyntaxer::getTokenPos()
{
    return mTokenPos;
}

void CppSyntaxer::next()
{
    if (mRun==0) {
        switch(mRange.state) {
        case RangeState::rsString:
        case RangeState::rsChar:
            mRange.state = RangeState::rsUnknown;
        }
    }
    mTokenPos = mRun;
    if (mLineSize == 0 && mRange.state == RangeState::rsString)
        mRange.state=RangeState::rsUnknown;
    do {
        if (mRun>=mLineSize) {
            procNull();
            break;
        }
        if (mRun<mLineSize && isSpaceChar(mLine[mRun])) {
            procSpace();
            break;
        }
        switch (mRange.state) {
        case RangeState::rsAnsiC:
        case RangeState::rsDirectiveComment:
            //qDebug()<<"*0-0-0*";
            procAnsiCStyleComment();
            break;
        case RangeState::rsDocstring:
            procDocstring();
            break;
        case RangeState::rsString:
        case RangeState::rsStringNextLine:
        case RangeState::rsStringUnfinished:
            //qDebug()<<"*1-0-0*";
            procString();
            break;
        case RangeState::rsCppComment:
            //qDebug()<<"*2-0-0*";
            procCppStyleComment();
            break;
        case RangeState::rsMultiLineDirective:
            //qDebug()<<"*3-0-0*";
            procDirectiveEnd();
            break;
//        case RangeState::rsMultiLineString:
//            //qDebug()<<"*4-0-0*";
//            stringEndProc();
//            break;
        case RangeState::rsStringEscapeSeq:
            //qDebug()<<"*6-0-0*";
            procStringEscapeSeq();
            break;
        case RangeState::rsChar:
            procAsciiChar();
            break;
        case RangeState::rsDefineIdentifier:
            //qDebug()<<"*8-0-0*";
            procDefineIdent();
            break;
        case RangeState::rsDefineRemaining:
            //qDebug()<<"*9-0-0*";
            procDefineRemaining();
            break;
        case RangeState::rsRawStringNotEscaping:
        case RangeState::rsRawString:
            //qDebug()<<"*9-0-0*";
            procRawString();
            break;
        default:
            //qDebug()<<"*a-0-0*";
            mRange.state = RangeState::rsUnknown;
            if (mRun>=mLineSize) {
                //qDebug()<<"*b-0-0*";
                procNull();
            } else if (mRun+1<mLineSize && mLine[mRun] == 'R' && mLine[mRun+1] == '"') {
                //qDebug()<<"*c-0-0*";
                mRun+=2;
                mRange.state = RangeState::rsRawString;
                procRawString();
            } else if (mRun+2<mLineSize && (mLine[mRun] == 'L' || mLine[mRun] == 'u' || mLine[mRun]=='U')  && mLine[mRun+1] == 'R' && mLine[mRun+2]=='\"') {
                mRun+=3;
                mRange.state = RangeState::rsRawString;
                procRawString();
            } else if (mRun+3<mLineSize && mLine[mRun] == 'u' && mLine[mRun+1] == '8' && mLine[mRun+2] == 'R' && mLine[mRun+3]=='\"') {
                mRun+=4;
                mRange.state = RangeState::rsRawString;
                procRawString();
            } else if (mRun+1<mLineSize && (mLine[mRun] == 'L' || mLine[mRun] == 'u' || mLine[mRun]=='U') && mLine[mRun+1]=='\"') {
                //qDebug()<<"*d-0-0*";
                mRun+=1;
                procStringStart();
            } else if (mRun+2<mLineSize && mLine[mRun] == 'u' && mLine[mRun+1] == '8' && mLine[mRun+2]=='\"') {
                //qDebug()<<"*e-0-0*";
                mRun+=2;
                procStringStart();
            } else {
                //qDebug()<<"*f-0-0*";
                processChar();
            }
        }
    } while (mTokenId!=TokenId::Null && mRun<=mTokenPos);
    //qDebug()<<"1-1-1";
}

void CppSyntaxer::setLine(const QString &newLine, int lineNumber)
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

bool CppSyntaxer::isKeyword(const QString &word)
{
    return Keywords.contains(word) || mCustomTypeKeywords.contains(word);
}

void CppSyntaxer::setState(const SyntaxState& rangeState)
{
    mRange = rangeState;
    // current line's left / right parenthesis count should be reset before parsing each line
    mRange.blockStarted = 0;
    mRange.blockEnded = 0;
    mRange.blockEndedLastLine = 0;
    mRange.lastUnindent=IndentInfo{IndentType::None,0};
    mRange.hasTrailingSpaces = false;
}

void CppSyntaxer::resetState()
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
}

QString CppSyntaxer::languageName()
{
    return "cpp";
}

ProgrammingLanguage CppSyntaxer::language()
{
    return ProgrammingLanguage::CPP;
}

SyntaxState CppSyntaxer::getState() const
{
    return mRange;
}

bool CppSyntaxer::isIdentChar(const QChar &ch) const
{
    return ch=='_' || ch.isDigit() || ch.isLetter();
}

bool CppSyntaxer::isIdentStartChar(const QChar &ch) const
{
    return ch=='_' || ch.isLetter();
}

QSet<QString> CppSyntaxer::keywords()
{
    QSet<QString> set=Keywords;
    set.unite(mCustomTypeKeywords);
    return set;
}

QString CppSyntaxer::foldString(QString endLine)
{
    if (endLine.trimmed().startsWith("#"))
        return "...";
    return "...}";
}

}
