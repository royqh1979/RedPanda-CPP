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
#include "glsl.h"
#include "../constants.h"
#include <qt_utils/utils.h>

#include <QFont>

namespace QSynedit {
static const QSet<QString> GLSLStatementKeyWords {
    "if",
    "for",
    "try",
    "catch",
    "else",
    "while"
};

const QSet<QString> GLSLSyntaxer::Keywords {
    "const", "uniform", "buffer", "shared", "attribute", "varying",
    "coherent", "volatile", "restrict", "readonly", "writeonly",
    "atomic_uint",
    "layout",
    "centroid", "flat", "smooth", "noperspective",
    "patch", "sample",
    "invariant", "precise",
    "break", "continue", "do", "for", "while", "switch", "case", "default",
    "if", "else",
    "subroutine",
    "in", "out", "inout",
    "int", "void", "bool", "true", "false", "float", "double",
    "discard", "return",
    "vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4", "bvec2", "bvec3", "bvec4",
    "uint", "uvec2", "uvec3", "uvec4",
    "dvec2", "dvec3", "dvec4",
    "mat2", "mat3", "mat4",
    "mat2x2", "mat2x3", "mat2x4",
    "mat3x2", "mat3x3", "mat3x4",
    "mat4x2", "mat4x3", "mat4x4",
    "dmat2", "dmat3", "dmat4",
    "dmat2x2", "dmat2x3", "dmat2x4",
    "dmat3x2", "dmat3x3", "dmat3x4",
    "dmat4x2", "dmat4x3", "dmat4x4",
    "lowp", "mediump", "highp", "precision",
    "sampler1D", "sampler1DShadow", "sampler1DArray", "sampler1DArrayShadow",
    "isampler1D", "isampler1DArray", "usampler1D", "usampler1DArray",
    "sampler2D", "sampler2DShadow", "sampler2DArray", "sampler2DArrayShadow",
    "isampler2D", "isampler2DArray", "usampler2D", "usampler2DArray",
    "sampler2DRect", "sampler2DRectShadow", "isampler2DRect", "usampler2DRect",
    "sampler2DMS", "isampler2DMS", "usampler2DMS",
    "sampler2DMSArray", "isampler2DMSArray", "usampler2DMSArray",
    "sampler3D", "isampler3D", "usampler3D",
    "samplerCube", "samplerCubeShadow", "isamplerCube", "usamplerCube",
    "samplerCubeArray", "samplerCubeArrayShadow",
    "isamplerCubeArray", "usamplerCubeArray",
    "samplerBuffer", "isamplerBuffer", "usamplerBuffer",
    "image1D", "iimage1D", "uimage1D",
    "image1DArray", "iimage1DArray", "uimage1DArray",
    "image2D", "iimage2D", "uimage2D",
    "image2DArray", "iimage2DArray", "uimage2DArray",
    "image2DRect", "iimage2DRect", "uimage2DRect",
    "image2DMS", "iimage2DMS", "uimage2DMS",
    "image2DMSArray", "iimage2DMSArray", "uimage2DMSArray",
    "image3D", "iimage3D", "uimage3D",
    "imageCube", "iimageCube", "uimageCube",
    "imageCubeArray", "iimageCubeArray", "uimageCubeArray",
    "imageBuffer", "iimageBuffer", "uimageBuffer",
    "struct"
};

GLSLSyntaxer::GLSLSyntaxer(): Syntaxer()
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

const PTokenAttribute &GLSLSyntaxer::preprocessorAttribute() const
{
    return mPreprocessorAttribute;
}

const PTokenAttribute &GLSLSyntaxer::invalidAttribute() const
{
    return mInvalidAttribute;
}

const PTokenAttribute &GLSLSyntaxer::numberAttribute() const
{
    return mNumberAttribute;
}

const PTokenAttribute &GLSLSyntaxer::floatAttribute() const
{
    return mFloatAttribute;
}

const PTokenAttribute &GLSLSyntaxer::hexAttribute() const
{
    return mHexAttribute;
}

const PTokenAttribute &GLSLSyntaxer::octAttribute() const
{
    return mOctAttribute;
}

const PTokenAttribute &GLSLSyntaxer::stringEscapeSequenceAttribute() const
{
    return mStringEscapeSequenceAttribute;
}

const PTokenAttribute &GLSLSyntaxer::charAttribute() const
{
    return mCharAttribute;
}

const PTokenAttribute &GLSLSyntaxer::variableAttribute() const
{
    return mVariableAttribute;
}

const PTokenAttribute &GLSLSyntaxer::functionAttribute() const
{
    return mFunctionAttribute;
}

const PTokenAttribute &GLSLSyntaxer::classAttribute() const
{
    return mClassAttribute;
}

const PTokenAttribute &GLSLSyntaxer::globalVarAttribute() const
{
    return mGlobalVarAttribute;
}

const PTokenAttribute &GLSLSyntaxer::localVarAttribute() const
{
    return mLocalVarAttribute;
}

GLSLSyntaxer::TokenId GLSLSyntaxer::getTokenId()
{
    return mTokenId;
}

void GLSLSyntaxer::andSymbolProc()
{
    mTokenId = TokenId::Symbol;
    switch (mLine[mRun+1].unicode()) {
    case '=':
        mRun+=2;
        break;
    case '&':
        mRun+=2;
        break;
    default:
        mRun+=1;
    }
}

void GLSLSyntaxer::ansiCppProc()
{
    mTokenId = TokenId::Comment;
    if (mLine[mRun]==0) {
        nullProc();
        if  ( (mRun<1)  || (mLine[mRun-1]!='\\')) {
            mRange.state = RangeState::rsUnknown;
            return;
        }
    }
    while (mLine[mRun]!=0) {
        mRun+=1;
    }
    mRange.state = RangeState::rsCppCommentEnded;
    if (mLine[mRun-1] == '\\' && mLine[mRun]==0) { // continues on next line
        mRange.state = RangeState::rsCppComment;
    }
}

void GLSLSyntaxer::ansiCProc()
{
    bool finishProcess = false;
    mTokenId = TokenId::Comment;
    if (mLine[mRun].unicode() == 0) {
        nullProc();
        return;
    }
    while (mLine[mRun]!=0) {
        switch(mLine[mRun].unicode()) {
        case '*':
            if (mLine[mRun+1] == '/') {
                mRun += 2;
                if (mRange.state == RangeState::rsDirectiveComment &&
                           mLine[mRun] != 0 && mLine[mRun]!='\r' && mLine[mRun]!='\n') {
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

void GLSLSyntaxer::asciiCharProc()
{
    mTokenId = TokenId::Char;
    do {
        if (mLine[mRun] == '\\') {
            if (mLine[mRun+1] == '\'' || mLine[mRun+1] == '\\') {
                mRun+=1;
            }
        }
        mRun+=1;
    } while (mLine[mRun]!=0 && mLine[mRun]!='\'');
    if (mLine[mRun] == '\'')
        mRun+=1;
    mRange.state = RangeState::rsUnknown;
}

void GLSLSyntaxer::atSymbolProc()
{
    mTokenId = TokenId::Unknown;
    mRun+=1;
}

void GLSLSyntaxer::braceCloseProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
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

void GLSLSyntaxer::braceOpenProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.braceLevel += 1;
    mRange.blockLevel += 1;
    mRange.blockStarted += 1;
    if (mRange.getLastIndentType() == IndentType::Statement) {
        // if last indent is started by 'if' 'for' etc
        // just replace it
        int lastLine=-1;
        while (mRange.getLastIndentType() == IndentType::Statement) {
            popIndents(IndentType::Statement);
            lastLine = mRange.lastUnindent.line;
        }
        pushIndents(IndentType::Block, lastLine);
    } else
        pushIndents(IndentType::Block);
}

void GLSLSyntaxer::colonProc()
{
    mTokenId = TokenId::Symbol;
    if (mLine[mRun+1]==':') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void GLSLSyntaxer::commaProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
}

void GLSLSyntaxer::directiveProc()
{
    QString preContents = mLineString.left(mRun).trimmed();
    if (!preContents.isEmpty()) { // '#' is not first non-space char on the line, treat it as an invalid char
       mTokenId = TokenId::Unknown;
       mRun+=1;
       return;
    }
    mTokenId = TokenId::Directive;
    mRun+=1;
    //skip spaces
    while (mLine[mRun]!=0 && isSpaceChar(mLine[mRun])) {
        mRun+=1;
    }

    while (mLine[mRun]!=0 && isIdentChar(mLine[mRun])) {
        mRun+=1;
    }
    mRange.state = RangeState::rsUnknown;
//    do {
//        switch(mLine[mRun].unicode()) {
//        case '/': //comment?
//            switch (mLine[mRun+1].unicode()) {
//            case '/': // is end of directive as well
//                mRange.state = RangeState::rsUnknown;
//                return;
//            case '*': // might be embeded only
//                mRange.state = RangeState::rsDirectiveComment;
//                return;
//            }
//            break;
//        case '\\': // yet another line?
//            if (mLine[mRun+1] == 0) {
//                mRun+=1;
//                mRange.state = RangeState::rsMultiLineDirective;
//                return;
//            }
//            break;
//        }
//        mRun+=1;
//    } while (mLine[mRun]!=0);
}

void GLSLSyntaxer::directiveEndProc()
{
    mTokenId = TokenId::Directive;
    if (mLine[mRun] == 0) {
        nullProc();
        return;
    }
    mRange.state = RangeState::rsUnknown;
    do {
        switch(mLine[mRun].unicode()) {
        case '/': //comment?
            switch (mLine[mRun+1].unicode()) {
            case '/': // is end of directive as well
                mRange.state = RangeState::rsUnknown;
                return;
            case '*': // might be embeded only
                mRange.state = RangeState::rsDirectiveComment;
                return;
            }
            break;
        case '\\': // yet another line?
              if (mLine[mRun+1] == 0) {
                  mRun+=1;
                  mRange.state = RangeState::rsMultiLineDirective;
                  return;
            }
            break;
        }
        mRun+=1;
    } while (mLine[mRun]!=0);
}

void GLSLSyntaxer::equalProc()
{
    mTokenId = TokenId::Symbol;
    if (mLine[mRun+1] == '=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void GLSLSyntaxer::greaterProc()
{
    mTokenId = TokenId::Symbol;
    switch (mLine[mRun + 1].unicode()) {
    case '=':
        mRun += 2;
        break;
    case '>':
        if (mLine[mRun+2] == '=') {
            mRun+=3;
        } else {
            mRun += 2;
        }
        break;
    default:
        mRun+=1;
    }
}

void GLSLSyntaxer::identProc()
{
    int wordEnd = mRun;
    while (isIdentChar(mLine[wordEnd])) {
        wordEnd+=1;
    }
    QString word = mLineString.mid(mRun,wordEnd-mRun);
    mRun=wordEnd;
    if (isKeyword(word)) {
        mTokenId = TokenId::Key;
        if (GLSLStatementKeyWords.contains(word)) {
            pushIndents(IndentType::Statement);
        }
    } else {
        mTokenId = TokenId::Identifier;
    }
}

void GLSLSyntaxer::lowerProc()
{
    mTokenId = TokenId::Symbol;
    switch(mLine[mRun+1].unicode()) {
    case '=':
        mRun+=2;
        break;
    case '<':
        if (mLine[mRun+2] == '=') {
            mRun+=3;
        } else {
            mRun+=2;
        }
        break;
    default:
        mRun+=1;
    }
}

void GLSLSyntaxer::minusProc()
{
    mTokenId = TokenId::Symbol;
    switch(mLine[mRun+1].unicode()) {
    case '=':
        mRun += 2;
        break;
    case '-':
        mRun += 2;
        break;
    case '>':
        if (mLine[mRun+2]=='*') {
            mRun += 3;
        } else {
            mRun += 2;
        }
        break;
    default:
        mRun += 1;
    }
}

void GLSLSyntaxer::modSymbolProc()
{
    mTokenId = TokenId::Symbol;
    switch(mLine[mRun + 1].unicode()) {
    case '=':
        mRun += 2;
        break;
    default:
        mRun += 1;
    }
}

void GLSLSyntaxer::notSymbolProc()
{
    mTokenId = TokenId::Symbol;
    switch(mLine[mRun + 1].unicode()) {
    case '=':
        mRun+=2;
        break;
    default:
        mRun+=1;
    }
}

void GLSLSyntaxer::nullProc()
{
    if ((mRun-1>=0) && isSpaceChar(mLine[mRun-1]) &&
    (mRange.state == RangeState::rsCppComment
     || mRange.state == RangeState::rsDirective
     || mRange.state == RangeState::rsString
     || mRange.state == RangeState::rsMultiLineString
     || mRange.state == RangeState::rsMultiLineDirective) ) {
        mRange.state = RangeState::rsUnknown;
    } else
        mTokenId = TokenId::Null;
}

void GLSLSyntaxer::numberProc()
{
    int idx1; // token[1]
    idx1 = mRun;
    mRun+=1;
    mTokenId = TokenId::Number;
    bool shouldExit = false;
    while (mLine[mRun]!=0) {
        switch(mLine[mRun].unicode()) {
        case '\'':
            if (mTokenId != TokenId::Number) {
                mTokenId = TokenId::Symbol;
                return;
            }
            break;
        case '.':
            if (mLine[mRun+1] == '.') {
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
            if (mLine[mRun-1]!= 'e' && mLine[mRun-1]!='E')  // number = float, but no exponent. an arithmetic operator
                return;
            if (mLine[mRun+1]<'0' || mLine[mRun+1]>'9')  {// invalid
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
                if (mLine[mRun-1]>='0' || mLine[mRun-1]<='9' ) {//exponent
                    for (int i=idx1;i<mRun;i++) {
                        if (mLine[i] == 'e' || mLine[i]=='E') { // too many exponents
                            mRun+=1;
                            mTokenId = TokenId::Unknown;
                            return;
                        }
                    }
                    if (mLine[mRun+1]!='+' && mLine[mRun+1]!='-' && !(mLine[mRun+1]>='0' && mLine[mRun+1]<='9')) {
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
                    if (mLine[mRun-1]=='l' || mLine[mRun-1]=='L') {
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
    if (mLine[mRun-1] == '\'') {
        mTokenId = TokenId::Unknown;
    }
}

void GLSLSyntaxer::orSymbolProc()
{
    mTokenId = TokenId::Symbol;
    switch ( mLine[mRun+1].unicode()) {
    case '=':
        mRun+=2;
        break;
    case '|':
        mRun+=2;
        break;
    default:
        mRun+=1;
    }
}

void GLSLSyntaxer::plusProc()
{
    mTokenId = TokenId::Symbol;
    switch(mLine[mRun+1].unicode()){
    case '=':
        mRun+=2;
        break;
    case '+':
        mRun+=2;
        break;
    default:
        mRun+=1;
    }
}

void GLSLSyntaxer::pointProc()
{
    mTokenId = TokenId::Symbol;
    if (mLine[mRun+1] == '*' ) {
        mRun+=2;
    } else if (mLine[mRun+1] == '.' && mLine[mRun+2] == '.') {
        mRun+=3;
    } else if (mLine[mRun+1]>='0' && mLine[mRun+1]<='9') {
        numberProc();
    } else {
        mRun+=1;
    }
}

void GLSLSyntaxer::questionProc()
{
    mTokenId = TokenId::Symbol;
    mRun+=1;
}

void GLSLSyntaxer::rawStringProc()
{
    bool noEscaping = false;
    if (mRange.state == RangeState::rsRawStringNotEscaping)
        noEscaping = true;
    mTokenId = TokenId::RawString;
    mRange.state = RangeState::rsRawString;

    while (mLine[mRun]!=0) {
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

void GLSLSyntaxer::roundCloseProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel--;
    if (mRange.parenthesisLevel<0)
        mRange.parenthesisLevel=0;
    popIndents(IndentType::Parenthesis);
}

void GLSLSyntaxer::roundOpenProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel++;
    pushIndents(IndentType::Parenthesis);
}

void GLSLSyntaxer::semiColonProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    while (mRange.getLastIndentType() == IndentType::Statement) {
        popIndents(IndentType::Statement);
    }
}

void GLSLSyntaxer::slashProc()
{
    switch(mLine[mRun+1].unicode()) {
    case '/': // Cpp style comment
        mTokenId = TokenId::Comment;
        mRun+=2;
        mRange.state = RangeState::rsCppComment;
        return;
    case '*': // C style comment
        mTokenId = TokenId::Comment;
        if (mRange.state == RangeState::rsDirective) {
            mRange.state = RangeState::rsDirectiveComment;
        } else {
            mRange.state = RangeState::rsAnsiC;
        }
        mRun += 2;
        if (mLine[mRun]!=0)
            ansiCProc();
        break;
    case '=':
        mRun+=2;
        mTokenId = TokenId::Symbol;
        break;
    default:
        mRun += 1;
        mTokenId = TokenId::Symbol;
    }
}

void GLSLSyntaxer::spaceProc()
{
    mRun += 1;
    mTokenId = TokenId::Space;
    while (isLexicalSpace(mLine[mRun]))
        mRun+=1;
    mRange.state = RangeState::rsUnknown;
    if (mRun>=mLineSize)
        mRange.hasTrailingSpaces=true;
}

void GLSLSyntaxer::squareCloseProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
    mRange.bracketLevel--;
    if (mRange.bracketLevel<0)
        mRange.bracketLevel=0;
    popIndents(IndentType::Bracket);
}

void GLSLSyntaxer::squareOpenProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
    mRange.bracketLevel++;
    pushIndents(IndentType::Bracket);
}

void GLSLSyntaxer::starProc()
{
    mTokenId = TokenId::Symbol;
    if (mLine[mRun+1] == '=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void GLSLSyntaxer::stringEndProc()
{
    mTokenId = TokenId::String;
    if (mLine[mRun]==0) {
        nullProc();
        return;
    }
    mRange.state = RangeState::rsUnknown;

    while (mLine[mRun]!=0) {
        if (mLine[mRun]=='"') {
            mRun += 1;
            break;
        }
        if (mLine[mRun].unicode()=='\\') {
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
            case 0:
                mRun+=1;
                mRange.state = RangeState::rsMultiLineString;
                return;
            }
        }
        mRun += 1;
    }
}

void GLSLSyntaxer::stringEscapeSeqProc()
{
    mTokenId = TokenId::StringEscapeSeq;
    mRun+=1;
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
            if (mLine[mRun]<'0' || mLine[mRun]>'7')
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
        if ( !(
                 (mLine[mRun]>='0' && mLine[mRun]<='9')
               ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
               ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                )) {
            mTokenId = TokenId::Unknown;
        } else {
            while (
                   (mLine[mRun]>='0' && mLine[mRun]<='9')
                 ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
                 ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                   )  {
                mRun+=1;
            }
        }
        break;
    case 'u':
        mRun+=1;
        for (int i=0;i<4;i++) {
            if (mLine[mRun]<'0' || mLine[mRun]>'7') {
                mTokenId = TokenId::Unknown;
                return;
            }
            mRun+=1;
        }
        break;
    case 'U':
        mRun+=1;
        for (int i=0;i<8;i++) {
            if (mLine[mRun]<'0' || mLine[mRun]>'7') {
                mTokenId = TokenId::Unknown;
                return;
            }
            mRun+=1;
        }
        break;
    }
    if (mRange.state == RangeState::rsMultiLineStringEscapeSeq)
        mRange.state = RangeState::rsMultiLineString;
    else
        mRange.state = RangeState::rsString;
}

void GLSLSyntaxer::stringProc()
{
    if (mLine[mRun] == 0) {
        mRange.state = RangeState::rsUnknown;
        return;
    }
    mTokenId = TokenId::String;
    mRange.state = RangeState::rsString;
    while (mLine[mRun]!=0) {
        if (mLine[mRun]=='"') {
            mRun+=1;
            break;
        }
        if (mLine[mRun].unicode()=='\\') {
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
            case 0:
                mRun+=1;
                mRange.state = RangeState::rsMultiLineString;
                return;
            }
        }
        mRun+=1;
    }
    mRange.state = RangeState::rsUnknown;
}

void GLSLSyntaxer::stringStartProc()
{
    mTokenId = TokenId::String;
    mRun += 1;
    if (mLine[mRun]==0) {
        mRange.state = RangeState::rsUnknown;
        return;
    }
    stringProc();
}

void GLSLSyntaxer::tildeProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
}

void GLSLSyntaxer::unknownProc()
{
    mRun+=1;
    mTokenId = TokenId::Unknown;
}

void GLSLSyntaxer::xorSymbolProc()
{
    mTokenId = TokenId::Symbol;
    if (mLine[mRun+1]=='=') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void GLSLSyntaxer::processChar()
{
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
    case 0:
        nullProc();
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

void GLSLSyntaxer::popIndents(IndentType indentType)
{
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

void GLSLSyntaxer::pushIndents(IndentType indentType, int line)
{
    if (line==-1)
        line = mLineNumber;
    mRange.indents.push_back(IndentInfo{indentType,line});
}

bool GLSLSyntaxer::isCommentNotFinished(int state) const
{
    return (state == RangeState::rsAnsiC ||
            state == RangeState::rsDirectiveComment||
            state == RangeState::rsCppComment);
}

bool GLSLSyntaxer::isStringNotFinished(int state) const
{
    return state == RangeState::rsMultiLineString;
}

bool GLSLSyntaxer::eol() const
{
    return mTokenId == TokenId::Null;
}

QString GLSLSyntaxer::getToken() const
{
    return mLineString.mid(mTokenPos,mRun-mTokenPos);
}

const PTokenAttribute &GLSLSyntaxer::getTokenAttribute() const
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

int GLSLSyntaxer::getTokenPos()
{
    return mTokenPos;
}

void GLSLSyntaxer::next()
{
    mTokenPos = mRun;
    do {
        switch (mRange.state) {
        case RangeState::rsAnsiC:
        case RangeState::rsDirectiveComment:
            ansiCProc();
            break;
        case RangeState::rsString:
            stringProc();
            break;
        case RangeState::rsCppComment:
            ansiCppProc();
            break;
        case RangeState::rsMultiLineDirective:
            directiveEndProc();
            break;
        case RangeState::rsMultiLineString:
            stringEndProc();
            break;
        case RangeState::rsRawStringEscaping:
        case RangeState::rsRawStringNotEscaping:
            rawStringProc();
            break;
        case RangeState::rsStringEscapeSeq:
        case RangeState::rsMultiLineStringEscapeSeq:
            stringEscapeSeqProc();
            break;
        case RangeState::rsChar:
            if (mLine[mRun]=='\'') {
                mRange.state = rsUnknown;
                mTokenId = TokenId::Char;
                mRun+=1;
            } else {
                asciiCharProc();
            }
            break;
        default:
            mRange.state = RangeState::rsUnknown;
            if (mLine[mRun] == 'R' && mLine[mRun+1] == '"') {
                mRun+=2;
                rawStringProc();
            } else if ((mLine[mRun] == 'L' || mLine[mRun] == 'u' || mLine[mRun]=='U') && mLine[mRun+1]=='\"') {
                mRun+=1;
                stringStartProc();
            } else if (mLine[mRun] == 'u' && mLine[mRun+1] == '8' && mLine[mRun+2]=='\"') {
                mRun+=2;
                stringStartProc();
            } else
                processChar();
        }
    } while (mTokenId!=TokenId::Null && mRun<=mTokenPos);
}

void GLSLSyntaxer::setLine(const QString &newLine, int lineNumber)
{
    mLineString = newLine;
    mLine = getNullTerminatedStringData(mLineString);
    mLineNumber = lineNumber;
    mRun = 0;
    mRange.blockStarted = 0;
    mRange.blockEnded = 0;
    mRange.blockEndedLastLine = 0;
    mRange.lastUnindent=IndentInfo{IndentType::None,0};
    mRange.hasTrailingSpaces = false;
    next();
}

bool GLSLSyntaxer::isKeyword(const QString &word)
{
    return Keywords.contains(word);
}

void GLSLSyntaxer::setState(const SyntaxState& rangeState)
{
    mRange = rangeState;
    // current line's left / right parenthesis count should be reset before parsing each line
    mRange.lastUnindent=IndentInfo{IndentType::None,0};
    mRange.hasTrailingSpaces = false;
}

void GLSLSyntaxer::resetState()
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

QString GLSLSyntaxer::languageName()
{
    return "glsl";
}

ProgrammingLanguage GLSLSyntaxer::language()
{
    return ProgrammingLanguage::GLSL;
}

SyntaxState GLSLSyntaxer::getState() const
{
    return mRange;
}

QSet<QString> GLSLSyntaxer::keywords()
{
    return Keywords;
}

bool GLSLSyntaxer::supportBraceLevel()
{
    return true;
}

QString GLSLSyntaxer::commentSymbol()
{
    return "//";
}

QString GLSLSyntaxer::blockCommentBeginSymbol()
{
    return "/*";
}

QString GLSLSyntaxer::blockCommentEndSymbol()
{
    return "*/";
}

bool GLSLSyntaxer::supportFolding()
{
    return true;
}

bool GLSLSyntaxer::needsLineState()
{
    return true;
}
}
