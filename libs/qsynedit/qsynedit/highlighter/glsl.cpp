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
#include "../Constants.h"

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

const QSet<QString> GLSLHighlighter::Keywords {
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

GLSLHighlighter::GLSLHighlighter(): Highlighter()
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

const PHighlighterAttribute &GLSLHighlighter::asmAttribute() const
{
    return mAsmAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::preprocessorAttribute() const
{
    return mPreprocessorAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::invalidAttribute() const
{
    return mInvalidAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::numberAttribute() const
{
    return mNumberAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::floatAttribute() const
{
    return mFloatAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::hexAttribute() const
{
    return mHexAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::octAttribute() const
{
    return mOctAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::stringEscapeSequenceAttribute() const
{
    return mStringEscapeSequenceAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::charAttribute() const
{
    return mCharAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::variableAttribute() const
{
    return mVariableAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::functionAttribute() const
{
    return mFunctionAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::classAttribute() const
{
    return mClassAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::globalVarAttribute() const
{
    return mGlobalVarAttribute;
}

const PHighlighterAttribute &GLSLHighlighter::localVarAttribute() const
{
    return mLocalVarAttribute;
}

GLSLHighlighter::TokenId GLSLHighlighter::getTokenId()
{
    if ((mRange.state == RangeState::rsAsm || mRange.state == RangeState::rsAsmBlock)
            && !mAsmStart && !(mTokenId == TokenId::Comment || mTokenId == TokenId::Space
                               || mTokenId == TokenId::Null)) {
        return TokenId::Asm;
    } else {
        return mTokenId;
    }
}

void GLSLHighlighter::andSymbolProc()
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

void GLSLHighlighter::ansiCppProc()
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

void GLSLHighlighter::ansiCProc()
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
                if (mRange.state == RangeState::rsAnsiCAsm) {
                    mRange.state = RangeState::rsAsm;
                } else if (mRange.state == RangeState::rsAnsiCAsmBlock){
                    mRange.state = RangeState::rsAsmBlock;
                } else if (mRange.state == RangeState::rsDirectiveComment &&
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

void GLSLHighlighter::asciiCharProc()
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

void GLSLHighlighter::atSymbolProc()
{
    mTokenId = TokenId::Unknown;
    mRun+=1;
}

void GLSLHighlighter::braceCloseProc()
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

void GLSLHighlighter::braceOpenProc()
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

void GLSLHighlighter::colonProc()
{
    mTokenId = TokenId::Symbol;
    if (mLine[mRun+1]==':') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void GLSLHighlighter::commaProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
}

void GLSLHighlighter::directiveProc()
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

void GLSLHighlighter::directiveEndProc()
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

void GLSLHighlighter::equalProc()
{
    mTokenId = TokenId::Symbol;
    if (mLine[mRun+1] == '=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void GLSLHighlighter::greaterProc()
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

void GLSLHighlighter::identProc()
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
            pushIndents(sitStatement);
        }
    } else {
        mTokenId = TokenId::Identifier;
    }
}

void GLSLHighlighter::lowerProc()
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

void GLSLHighlighter::minusProc()
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

void GLSLHighlighter::modSymbolProc()
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

void GLSLHighlighter::notSymbolProc()
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

void GLSLHighlighter::nullProc()
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

void GLSLHighlighter::numberProc()
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

void GLSLHighlighter::orSymbolProc()
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

void GLSLHighlighter::plusProc()
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

void GLSLHighlighter::pointProc()
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

void GLSLHighlighter::questionProc()
{
    mTokenId = TokenId::Symbol;
    mRun+=1;
}

void GLSLHighlighter::rawStringProc()
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

void GLSLHighlighter::roundCloseProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel--;
    if (mRange.parenthesisLevel<0)
        mRange.parenthesisLevel=0;
    popIndents(sitParenthesis);
}

void GLSLHighlighter::roundOpenProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel++;
    pushIndents(sitParenthesis);
}

void GLSLHighlighter::semiColonProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    if (mRange.state == RangeState::rsAsm)
        mRange.state = RangeState::rsUnknown;
    while (mRange.getLastIndent() == sitStatement) {
        popIndents(sitStatement);
    }
}

void GLSLHighlighter::slashProc()
{
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

void GLSLHighlighter::spaceProc()
{
    mRun += 1;
    mTokenId = TokenId::Space;
    while (mLine[mRun]>=1 && mLine[mRun]<=32)
        mRun+=1;
    mRange.state = RangeState::rsUnknown;
}

void GLSLHighlighter::squareCloseProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
    mRange.bracketLevel--;
    if (mRange.bracketLevel<0)
        mRange.bracketLevel=0;
    popIndents(sitBracket);
}

void GLSLHighlighter::squareOpenProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
    mRange.bracketLevel++;
    pushIndents(sitBracket);
}

void GLSLHighlighter::starProc()
{
    mTokenId = TokenId::Symbol;
    if (mLine[mRun+1] == '=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void GLSLHighlighter::stringEndProc()
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

void GLSLHighlighter::stringEscapeSeqProc()
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

void GLSLHighlighter::stringProc()
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

void GLSLHighlighter::stringStartProc()
{
    mTokenId = TokenId::String;
    mRun += 1;
    if (mLine[mRun]==0) {
        mRange.state = RangeState::rsUnknown;
        return;
    }
    stringProc();
}

void GLSLHighlighter::tildeProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
}

void GLSLHighlighter::unknownProc()
{
    mRun+=1;
    mTokenId = TokenId::Unknown;
}

void GLSLHighlighter::xorSymbolProc()
{
    mTokenId = TokenId::Symbol;
    if (mLine[mRun+1]=='=') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void GLSLHighlighter::processChar()
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

void GLSLHighlighter::popIndents(int indentType)
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

void GLSLHighlighter::pushIndents(int indentType)
{
    int idx = mRange.indents.length();
    if (idx<mRange.firstIndentThisLine)
        mRange.firstIndentThisLine = idx;
    mRange.indents.push_back(indentType);
}

bool GLSLHighlighter::getTokenFinished() const
{
    if (mTokenId == TokenId::Comment
            || mTokenId == TokenId::String
            || mTokenId == TokenId::RawString) {
        return mRange.state == RangeState::rsUnknown;
    }
    return true;
}

bool GLSLHighlighter::isLastLineCommentNotFinished(int state) const
{
    return (state == RangeState::rsAnsiC ||
            state == RangeState::rsAnsiCAsm ||
            state == RangeState::rsAnsiCAsmBlock ||
            state == RangeState::rsDirectiveComment||
            state == RangeState::rsCppComment);
}

bool GLSLHighlighter::isLastLineStringNotFinished(int state) const
{
    return state == RangeState::rsMultiLineString;
}

bool GLSLHighlighter::eol() const
{
    return mTokenId == TokenId::Null;
}

QString GLSLHighlighter::getToken() const
{
    return mLineString.mid(mTokenPos,mRun-mTokenPos);
}

const PHighlighterAttribute &GLSLHighlighter::getTokenAttribute() const
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

int GLSLHighlighter::getTokenPos()
{
    return mTokenPos;
}

void GLSLHighlighter::next()
{
    mAsmStart = false;
    mTokenPos = mRun;
    do {
        switch (mRange.state) {
        case RangeState::rsAnsiC:
        case RangeState::rsAnsiCAsm:
        case RangeState::rsAnsiCAsmBlock:
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

void GLSLHighlighter::setLine(const QString &newLine, int lineNumber)
{
    mLineString = newLine;
    mLine = mLineString.data();
    mLineNumber = lineNumber;
    mRun = 0;
    mRange.leftBraces = 0;
    mRange.rightBraces = 0;
    mRange.firstIndentThisLine = mRange.indents.length();
    mRange.matchingIndents.clear();
    next();
}

bool GLSLHighlighter::isKeyword(const QString &word)
{
    return Keywords.contains(word);
}

void GLSLHighlighter::setState(const HighlighterState& rangeState)
{
    mRange = rangeState;
    // current line's left / right parenthesis count should be reset before parsing each line
    mRange.leftBraces = 0;
    mRange.rightBraces = 0;
    mRange.firstIndentThisLine = mRange.indents.length();
    mRange.matchingIndents.clear();
}

void GLSLHighlighter::resetState()
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

QString GLSLHighlighter::languageName()
{
    return "glsl";
}

HighlighterLanguage GLSLHighlighter::language()
{
    return HighlighterLanguage::GLSL;
}

HighlighterState GLSLHighlighter::getState() const
{
    return mRange;
}

bool GLSLHighlighter::isIdentChar(const QChar &ch) const
{
    return ch=='_' || (ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || (ch>='0' && ch<='9');
}

QSet<QString> GLSLHighlighter::keywords() const
{
    return Keywords;
}

bool GLSLHighlighter::supportBraceLevel()
{
    return true;
}
}
