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
#include "../Constants.h"

#include <QFont>
#include <QDebug>

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



const QSet<QString> CppHighlighter::Keywords {
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
    "co_wait",
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
};
CppHighlighter::CppHighlighter(): Highlighter()
{
    mAsmAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrAssembler,
                                                           TokenType::Embeded);
    addAttribute(mAsmAttribute);
    mCharAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrCharacter,
                                                            TokenType::Character);
    addAttribute(mCharAttribute);

    mClassAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrClass,
                                                             TokenType::Identifier);
    addAttribute(mClassAttribute);
    mFloatAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrFloat,
                                                             TokenType::Number);
    addAttribute(mFloatAttribute);
    mFunctionAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrFunction,
                                                                TokenType::Identifier);
    addAttribute(mFunctionAttribute);
    mGlobalVarAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrGlobalVariable,
                                                                 TokenType::Identifier);
    addAttribute(mGlobalVarAttribute);
    mHexAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrHexadecimal,
                                                           TokenType::Number);
    addAttribute(mHexAttribute);
    mInvalidAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrIllegalChar,
                                                               TokenType::Error);
    addAttribute(mInvalidAttribute);
    mLocalVarAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrLocalVariable,
                                                                TokenType::Identifier);
    addAttribute(mLocalVarAttribute);
    mNumberAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrNumber,
                                                              TokenType::Number);
    addAttribute(mNumberAttribute);
    mOctAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrOctal,
                                                           TokenType::Number);
    addAttribute(mOctAttribute);
    mPreprocessorAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrPreprocessor,
                                                                    TokenType::Preprocessor);
    addAttribute(mPreprocessorAttribute);

    mStringEscapeSequenceAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrStringEscapeSequences,
                                                                            TokenType::String);
    addAttribute(mStringEscapeSequenceAttribute);
    mVariableAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrVariable,
                                                                TokenType::Identifier);
    addAttribute(mVariableAttribute);
    resetState();
}

const PHighlighterAttribute &CppHighlighter::asmAttribute() const
{
    return mAsmAttribute;
}

const PHighlighterAttribute &CppHighlighter::preprocessorAttribute() const
{
    return mPreprocessorAttribute;
}

const PHighlighterAttribute &CppHighlighter::invalidAttribute() const
{
    return mInvalidAttribute;
}

const PHighlighterAttribute &CppHighlighter::numberAttribute() const
{
    return mNumberAttribute;
}

const PHighlighterAttribute &CppHighlighter::floatAttribute() const
{
    return mFloatAttribute;
}

const PHighlighterAttribute &CppHighlighter::hexAttribute() const
{
    return mHexAttribute;
}

const PHighlighterAttribute &CppHighlighter::octAttribute() const
{
    return mOctAttribute;
}

const PHighlighterAttribute &CppHighlighter::stringEscapeSequenceAttribute() const
{
    return mStringEscapeSequenceAttribute;
}

const PHighlighterAttribute &CppHighlighter::charAttribute() const
{
    return mCharAttribute;
}

const PHighlighterAttribute &CppHighlighter::variableAttribute() const
{
    return mVariableAttribute;
}

const PHighlighterAttribute &CppHighlighter::functionAttribute() const
{
    return mFunctionAttribute;
}

const PHighlighterAttribute &CppHighlighter::classAttribute() const
{
    return mClassAttribute;
}

const PHighlighterAttribute &CppHighlighter::globalVarAttribute() const
{
    return mGlobalVarAttribute;
}

const PHighlighterAttribute &CppHighlighter::localVarAttribute() const
{
    return mLocalVarAttribute;
}

CppHighlighter::TokenId CppHighlighter::getTokenId()
{
    if ((mRange.state == RangeState::rsAsm || mRange.state == RangeState::rsAsmBlock)
            && !mAsmStart && !(mTokenId == TokenId::Comment || mTokenId == TokenId::Space
                               || mTokenId == TokenId::Null)) {
        return TokenId::Asm;
    } else {
        return mTokenId;
    }
}

void CppHighlighter::andSymbolProc()
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

void CppHighlighter::ansiCppProc()
{
    mTokenId = TokenId::Comment;
    if (mRun>=mLineSize) {
        nullProc();
        if  ( (mRun-1<0)  || (mLine[mRun-1]!='\\')) {
            mRange.state = RangeState::rsUnknown;
        }
        return;
    }
    while (mRun<mLineSize) {
        mRun+=1;
    }
    mRange.state = RangeState::rsCppCommentEnded;
    if (mRun == mLineSize-1 && mLine[mRun-1] == '\\' ) { // continues on next line
        mRange.state = RangeState::rsCppComment;
    }
}

void CppHighlighter::ansiCProc()
{
    bool finishProcess = false;
    mTokenId = TokenId::Comment;
    if (mRun>=mLineSize) {
        nullProc();
        return;
    }
    while (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case '*':
            if (mRun+1<mLineSize && mLine[mRun+1] == '/') {
                mRun += 2;
                if (mRange.state == RangeState::rsAnsiCAsm) {
                    mRange.state = RangeState::rsAsm;
                } else if (mRange.state == RangeState::rsAnsiCAsmBlock){
                    mRange.state = RangeState::rsAsmBlock;
                } else if (mRange.state == RangeState::rsDirectiveComment &&
                            mRun<mLineSize && mLine[mRun]!='\r' && mLine[mRun]!='\n') {
                    mRange.state = RangeState::rsMultiLineDirective;
                } else {
                    mRange.state = RangeState::rsUnknown;
                }
                finishProcess = true;
            } else
                mRun+=1;
            break;
        default:
            mRun+=1;
        }
        if (finishProcess)
            break;
    }
}

void CppHighlighter::asciiCharProc()
{
    mTokenId = TokenId::Char;
    do {
        if (mLine[mRun] == '\\') {
            if (mRun+1<mLineSize && (mLine[mRun+1] == '\'' || mLine[mRun+1] == '\\')) {
                mRun+=1;
            }
        }
        mRun+=1;
    } while (mRun < mLineSize && mLine[mRun]!='\'');
    if (mRun<mLineSize && mLine[mRun] == '\'')
        mRun+=1;
    mRange.state = RangeState::rsUnknown;
}

void CppHighlighter::atSymbolProc()
{
    mTokenId = TokenId::Unknown;
    mRun+=1;
}

void CppHighlighter::braceCloseProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    if (mRange.state == RangeState::rsAsmBlock) {
        mRange.state = rsUnknown;
    }

    mRange.braceLevel -= 1;
    if (mRange.braceLevel<0)
        mRange.braceLevel = 0;
    if (mRange.leftBraces>0) {
        mRange.leftBraces--;
    } else {
        mRange.rightBraces++ ;
    }
    popIndents(sitBrace);
}

void CppHighlighter::braceOpenProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    if (mRange.state == RangeState::rsAsm) {
        mRange.state = RangeState::rsAsmBlock;
        mAsmStart = true;
    }
    mRange.braceLevel += 1;
    mRange.leftBraces++;
    if (mRange.getLastIndent() == sitStatement) {
        // if last indent is started by 'if' 'for' etc
        // just replace it
        while (mRange.getLastIndent() == sitStatement)
            popIndents(sitStatement);
        pushIndents(sitBrace);
//        int idx = mRange.indents.length()-1;
//        if (idx < mRange.firstIndentThisLine) {
//            mRange.firstIndentThisLine = idx;
//        }
//        mRange.indents.replace(idx,1,BraceIndentType);
    } else {
        pushIndents(sitBrace);
    }
}

void CppHighlighter::colonProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]==':') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void CppHighlighter::commaProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
}

void CppHighlighter::directiveProc()
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
        while(mRun < mLineSize && isSpaceChar(mLine[mRun]))
            mRun++;
        mRange.state = RangeState::rsDefineIdentifier;
        return;
    } else
        mRange.state = RangeState::rsUnknown;
}

void CppHighlighter::defineIdentProc()
{
    mTokenId = TokenId::Identifier;
    while(mRun < mLineSize && isIdentChar(mLine[mRun]))
        mRun++;
    mRange.state = RangeState::rsDefineRemaining;
}

void CppHighlighter::defineRemainingProc()
{
    mTokenId = TokenId::Directive;
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
    } while (mRun<mLineSize);
    mRange.state=RangeState::rsUnknown;
}

void CppHighlighter::directiveEndProc()
{
    mTokenId = TokenId::Directive;
    if (mRun >= mLineSize) {
        nullProc();
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

void CppHighlighter::equalProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1] == '=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void CppHighlighter::greaterProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize) {
        switch (mLine[mRun+1].unicode()) {
        case '=':
            mRun += 2;
            return;
        case '>':
            if (mRun+2<mLineSize && mLine[mRun+2] == '=') {
                mRun+=3;
            } else {
                mRun += 2;
            }
            return;
        }
    }
    mRun+=1;
}

void CppHighlighter::identProc()
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
            pushIndents(sitStatement);
        }
    } else {
        mTokenId = TokenId::Identifier;
    }
}

void CppHighlighter::lowerProc()
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

void CppHighlighter::minusProc()
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

void CppHighlighter::modSymbolProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]=='=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void CppHighlighter::notSymbolProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]=='=') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void CppHighlighter::nullProc()
{
    if (
    (mRange.state == RangeState::rsCppComment
     || mRange.state == RangeState::rsDirective
     || mRange.state == RangeState::rsString
     || mRange.state == RangeState::rsMultiLineString
     || mRange.state == RangeState::rsMultiLineDirective)
            && (mRun-1>=0)
            && (mRun-1<mLineSize)
            && isSpaceChar(mLine[mRun-1]) ) {
        mRange.state = RangeState::rsUnknown;
    } else
        mTokenId = TokenId::Null;
}

void CppHighlighter::numberProc()
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
            if ((mRun == idx1+1) && (mLine[idx1] == '0')) { // octal number
                mTokenId = TokenId::Octal;
            }
            break;
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

void CppHighlighter::orSymbolProc()
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

void CppHighlighter::plusProc()
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

void CppHighlighter::pointProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1] == '*' ) {
        mRun+=2;
    } else if (mRun+2<mLineSize && mLine[mRun+1] == '.' && mLine[mRun+2] == '.') {
        mRun+=3;
    } else if (mRun+1<mLineSize && mLine[mRun+1]>='0' && mLine[mRun+1]<='9') {
        numberProc();
    } else {
        mRun+=1;
    }
}

void CppHighlighter::questionProc()
{
    mTokenId = TokenId::Symbol;
    mRun+=1;
}

void CppHighlighter::rawStringProc()
{
    bool noEscaping = false;
    if (mRange.state == RangeState::rsRawStringNotEscaping)
        noEscaping = true;
    mTokenId = TokenId::RawString;
    mRange.state = RangeState::rsRawString;

    while (mRun<mLineSize) {
        if ((!noEscaping) && (mLine[mRun]=='"')) {
            mRun+=1;
            break;
        }
        switch (mLine[mRun].unicode()) {
        case '(':
            noEscaping = true;
            break;
        case ')':
            noEscaping = false;
            break;
        }
        mRun+=1;
    }
    mRange.state = RangeState::rsUnknown;
}

void CppHighlighter::roundCloseProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel--;
    if (mRange.parenthesisLevel<0)
        mRange.parenthesisLevel=0;
    popIndents(sitParenthesis);
}

void CppHighlighter::roundOpenProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel++;
    pushIndents(sitParenthesis);
}

void CppHighlighter::semiColonProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    if (mRange.state == RangeState::rsAsm)
        mRange.state = RangeState::rsUnknown;
    while (mRange.getLastIndent() == sitStatement) {
        popIndents(sitStatement);
    }
}

void CppHighlighter::slashProc()
{
    if (mRun+1<mLineSize) {
        switch(mLine[mRun+1].unicode()) {
        case '/': // Cpp style comment
            mTokenId = TokenId::Comment;
            mRun+=2;
            mRange.state = RangeState::rsCppComment;
            return;
        case '*': // C style comment
            mTokenId = TokenId::Comment;
            if (mRange.state == RangeState::rsAsm) {
                mRange.state = RangeState::rsAnsiCAsm;
            } else if (mRange.state == RangeState::rsAsmBlock) {
                mRange.state = RangeState::rsAnsiCAsmBlock;
            } else if (mRange.state == RangeState::rsDirective) {
                mRange.state = RangeState::rsDirectiveComment;
            } else {
                mRange.state = RangeState::rsAnsiC;
            }
            mRun += 2;
            if (mRun < mLineSize)
                ansiCProc();
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

void CppHighlighter::backSlashProc()
{
    if (mRun+1==mLineSize-1) {
        mTokenId = TokenId::Symbol;
    } else {
        mTokenId = TokenId::Unknown;
    }
    mRun+=1;
}

void CppHighlighter::spaceProc()
{
    mRun += 1;
    mTokenId = TokenId::Space;
    while (mRun<mLineSize && mLine[mRun]>=1 && mLine[mRun]<=32)
        mRun+=1;
    mRange.state = RangeState::rsUnknown;
}

void CppHighlighter::squareCloseProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
    mRange.bracketLevel--;
    if (mRange.bracketLevel<0)
        mRange.bracketLevel=0;
    popIndents(sitBracket);
}

void CppHighlighter::squareOpenProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
    mRange.bracketLevel++;
    pushIndents(sitBracket);
}

void CppHighlighter::starProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1] == '=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void CppHighlighter::stringEndProc()
{
    mTokenId = TokenId::String;
    if (mRun>=mLineSize) {
        nullProc();
        return;
    }
    mRange.state = RangeState::rsUnknown;

    while (mRun<mLineSize) {
        if (mLine[mRun]=='"') {
            mRun += 1;
            break;
        }
        if (mLine[mRun]=='\\') {
            if (mRun == mLineSize-1) {
                mRun+=1;
                mRange.state = RangeState::rsMultiLineString;
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
                    mRange.state = RangeState::rsMultiLineStringEscapeSeq;
                    return;
                }
            }
        }
        mRun += 1;
    }
}

void CppHighlighter::stringEscapeSeqProc()
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
    if (mRange.state == RangeState::rsMultiLineStringEscapeSeq)
        mRange.state = RangeState::rsMultiLineString;
    else
        mRange.state = RangeState::rsString;
}

void CppHighlighter::stringProc()
{
    if (mRun >= mLineSize) {
        mRange.state = RangeState::rsUnknown;
        return;
    }
    mTokenId = TokenId::String;
    mRange.state = RangeState::rsString;
    while (mRun < mLineSize) {
        if (mLine[mRun]=='"') {
            mRun+=1;
            break;
        }
        if (mLine[mRun]=='\\') {
            if (mRun == mLineSize-1) {
                mRun+=1;
                mRange.state = RangeState::rsMultiLineString;
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
        }
        mRun+=1;
    }
    mRange.state = RangeState::rsUnknown;
}

void CppHighlighter::stringStartProc()
{
    mTokenId = TokenId::String;
    mRun += 1;
    if (mRun>=mLineSize) {
        mRange.state = RangeState::rsUnknown;
        return;
    }
    stringProc();
}

void CppHighlighter::tildeProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
}

void CppHighlighter::unknownProc()
{
    mRun+=1;
    mTokenId = TokenId::Unknown;
}

void CppHighlighter::xorSymbolProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]=='=') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void CppHighlighter::processChar()
{
    if (mRun>=mLineSize) {
        nullProc();
    } else {
        switch(mLine[mRun].unicode()) {
        case '&':
            andSymbolProc();
            break;
        case '\'':
            asciiCharProc();
            break;
        case '@':
            atSymbolProc();
            break;
        case '}':
            braceCloseProc();
            break;
        case '{':
            braceOpenProc();
            break;
        case '\r':
        case '\n':
            spaceProc();
            break;
        case ':':
            colonProc();
            break;
        case ',':
            commaProc();
            break;
        case '#':
            directiveProc();
            break;
        case '=':
            equalProc();
            break;
        case '>':
            greaterProc();
            break;
        case '?':
            questionProc();
            break;
        case '<':
            lowerProc();
            break;
        case '-':
            minusProc();
            break;
        case '%':
            modSymbolProc();
            break;
        case '!':
            notSymbolProc();
            break;
        case '\\':
            backSlashProc();
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
        case '|':
            orSymbolProc();
            break;
        case '+':
            plusProc();
            break;
        case '.':
            pointProc();
            break;
        case ')':
            roundCloseProc();
            break;
        case '(':
            roundOpenProc();
            break;
        case ';':
            semiColonProc();
            break;
        case '/':
            slashProc();
            break;
        case ']':
            squareCloseProc();
            break;
        case '[':
            squareOpenProc();
            break;
        case '*':
            starProc();
            break;
        case '"':
            stringStartProc();
            break;
        case '~':
            tildeProc();
            break;
        case '^':
            xorSymbolProc();
            break;
        default:
            if (isIdentChar(mLine[mRun])) {
                identProc();
            } else if (isSpaceChar(mLine[mRun])) {
                spaceProc();
            } else {
                unknownProc();
            }
        }
    }
}

void CppHighlighter::popIndents(int indentType)
{
    while (!mRange.indents.isEmpty() && mRange.indents.back()!=indentType) {
        mRange.indents.pop_back();
    }
    if (!mRange.indents.isEmpty()) {
        int idx = mRange.indents.length()-1;
        if (idx < mRange.firstIndentThisLine) {
            mRange.matchingIndents.append(mRange.indents[idx]);
        }
        mRange.indents.pop_back();
    }
}

void CppHighlighter::pushIndents(int indentType)
{
    int idx = mRange.indents.length();
    if (idx<mRange.firstIndentThisLine)
        mRange.firstIndentThisLine = idx;
    mRange.indents.push_back(indentType);
}

const QSet<QString> &CppHighlighter::customTypeKeywords() const
{
    return mCustomTypeKeywords;
}

void CppHighlighter::setCustomTypeKeywords(const QSet<QString> &newCustomTypeKeywords)
{
    mCustomTypeKeywords = newCustomTypeKeywords;
}

bool CppHighlighter::supportBraceLevel()
{
    return true;
}

bool CppHighlighter::getTokenFinished() const
{
    if (mTokenId == TokenId::Comment
            || mTokenId == TokenId::String
            || mTokenId == TokenId::RawString) {
        return mRange.state == RangeState::rsUnknown;
    }
    return true;
}

bool CppHighlighter::isLastLineCommentNotFinished(int state) const
{
    return (state == RangeState::rsAnsiC ||
            state == RangeState::rsAnsiCAsm ||
            state == RangeState::rsAnsiCAsmBlock ||
            state == RangeState::rsDirectiveComment||
            state == RangeState::rsCppComment);
}

bool CppHighlighter::isLastLineStringNotFinished(int state) const
{
    return state == RangeState::rsMultiLineString;
}

bool CppHighlighter::eol() const
{
    return mTokenId == TokenId::Null;
}

QString CppHighlighter::getToken() const
{
    return mLine.mid(mTokenPos,mRun-mTokenPos);
}

const PHighlighterAttribute &CppHighlighter::getTokenAttribute() const
{
    switch (mTokenId) {
    case TokenId::Asm:
        return mAsmAttribute;
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

int CppHighlighter::getTokenPos()
{
    return mTokenPos;
}

void CppHighlighter::next()
{
    mAsmStart = false;
    mTokenPos = mRun;
    do {
        switch (mRange.state) {
        case RangeState::rsAnsiC:
        case RangeState::rsAnsiCAsm:
        case RangeState::rsAnsiCAsmBlock:
        case RangeState::rsDirectiveComment:
            //qDebug()<<"*0-0-0*";
            ansiCProc();
            break;
        case RangeState::rsString:
            //qDebug()<<"*1-0-0*";
            stringProc();
            break;
        case RangeState::rsCppComment:
            //qDebug()<<"*2-0-0*";
            ansiCppProc();
            break;
        case RangeState::rsMultiLineDirective:
            //qDebug()<<"*3-0-0*";
            directiveEndProc();
            break;
        case RangeState::rsMultiLineString:
            //qDebug()<<"*4-0-0*";
            stringEndProc();
            break;
        case RangeState::rsRawStringEscaping:
        case RangeState::rsRawStringNotEscaping:
            //qDebug()<<"*5-0-0*";
            rawStringProc();
            break;
        case RangeState::rsStringEscapeSeq:
        case RangeState::rsMultiLineStringEscapeSeq:
            //qDebug()<<"*6-0-0*";
            stringEscapeSeqProc();
            break;
        case RangeState::rsChar:
            //qDebug()<<"*7-0-0*";
            if (mRun>=mLineSize) {
                nullProc();
            } else if (mLine[mRun]=='\'') {
                mRange.state = rsUnknown;
                mTokenId = TokenId::Char;
                mRun+=1;
            } else {
                asciiCharProc();
            }
            break;
        case RangeState::rsDefineIdentifier:
            //qDebug()<<"*8-0-0*";
            defineIdentProc();
            break;
        case RangeState::rsDefineRemaining:
            //qDebug()<<"*9-0-0*";
            defineRemainingProc();
            break;
        default:
            //qDebug()<<"*a-0-0*";
            mRange.state = RangeState::rsUnknown;
            if (mRun>=mLineSize) {
                //qDebug()<<"*b-0-0*";
                nullProc();
            } else if (mRun+1<mLineSize && mLine[mRun] == 'R' && mLine[mRun+1] == '"') {
                //qDebug()<<"*c-0-0*";
                mRun+=2;
                rawStringProc();
            } else if (mRun+1<mLineSize && (mLine[mRun] == 'L' || mLine[mRun] == 'u' || mLine[mRun]=='U') && mLine[mRun+1]=='\"') {
                //qDebug()<<"*d-0-0*";
                mRun+=1;
                stringStartProc();
            } else if (mRun+2<mLineSize && mLine[mRun] == 'u' && mLine[mRun+1] == '8' && mLine[mRun+2]=='\"') {
                //qDebug()<<"*e-0-0*";
                mRun+=2;
                stringStartProc();
            } else {
                //qDebug()<<"*f-0-0*";
                processChar();
            }
        }
    } while (mTokenId!=TokenId::Null && mRun<=mTokenPos);
    //qDebug()<<"1-1-1";
}

void CppHighlighter::setLine(const QString &newLine, int lineNumber)
{
    mLine = newLine;
    mLineSize = mLine.size();
    mLineNumber = lineNumber;
    mRun = 0;
    mRange.leftBraces = 0;
    mRange.rightBraces = 0;
    mRange.firstIndentThisLine = mRange.indents.length();
    mRange.matchingIndents.clear();
    next();
}

bool CppHighlighter::isKeyword(const QString &word)
{
    return Keywords.contains(word) || mCustomTypeKeywords.contains(word);
}

void CppHighlighter::setState(const HighlighterState& rangeState)
{
    mRange = rangeState;
    // current line's left / right parenthesis count should be reset before parsing each line
    mRange.leftBraces = 0;
    mRange.rightBraces = 0;
    mRange.firstIndentThisLine = mRange.indents.length();
    mRange.matchingIndents.clear();
}

void CppHighlighter::resetState()
{
    mRange.state = RangeState::rsUnknown;
    mRange.braceLevel = 0;
    mRange.bracketLevel = 0;
    mRange.parenthesisLevel = 0;
    mRange.leftBraces = 0;
    mRange.rightBraces = 0;
    mRange.indents.clear();
    mRange.firstIndentThisLine = 0;
    mRange.matchingIndents.clear();
    mAsmStart = false;
}

QString CppHighlighter::languageName()
{
    return "cpp";
}

HighlighterLanguage CppHighlighter::language()
{
    return HighlighterLanguage::Cpp;
}

HighlighterState CppHighlighter::getState() const
{
    return mRange;
}

bool CppHighlighter::isIdentChar(const QChar &ch) const
{
    return ch=='_' || ch.isDigit() || ch.isLetter();
}

QSet<QString> CppHighlighter::keywords() const
{
    QSet<QString> set=Keywords;
    set.unite(mCustomTypeKeywords);
    return set;
}

QString CppHighlighter::foldString()
{
    return "...}";
}

}
