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

static const QSet<QString> GLSLStatementKeyWords {
    "if",
    "for",
    "try",
    "catch",
    "else",
    "while"
};

const QSet<QString> SynEditGLSLHighlighter::Keywords {
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

SynEditGLSLHighlighter::SynEditGLSLHighlighter(): SynHighlighter()
{
    mAsmAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrAssembler);
    addAttribute(mAsmAttribute);
    mCharAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrCharacter);
    addAttribute(mCharAttribute);
    mCommentAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrComment);
    addAttribute(mCommentAttribute);
    mClassAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrClass);
    addAttribute(mClassAttribute);
    mFloatAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrFloat);
    addAttribute(mFloatAttribute);
    mFunctionAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrFunction);
    addAttribute(mFunctionAttribute);
    mGlobalVarAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrGlobalVariable);
    addAttribute(mGlobalVarAttribute);
    mHexAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrHexadecimal);
    addAttribute(mHexAttribute);
    mIdentifierAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrIdentifier);
    addAttribute(mIdentifierAttribute);
    mInvalidAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrIllegalChar);
    addAttribute(mInvalidAttribute);
    mLocalVarAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrLocalVariable);
    addAttribute(mLocalVarAttribute);
    mNumberAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrNumber);
    addAttribute(mNumberAttribute);
    mOctAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrOctal);
    addAttribute(mOctAttribute);
    mPreprocessorAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrPreprocessor);
    addAttribute(mPreprocessorAttribute);
    mKeywordAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrReservedWord);
    addAttribute(mKeywordAttribute);
    mWhitespaceAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrSpace);
    addAttribute(mWhitespaceAttribute);
    mStringAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrString);
    addAttribute(mStringAttribute);
    mStringEscapeSequenceAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrStringEscapeSequences);
    addAttribute(mStringEscapeSequenceAttribute);
    mSymbolAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrSymbol);
    addAttribute(mSymbolAttribute);
    mVariableAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrVariable);
    addAttribute(mVariableAttribute);

    resetState();
}

PSynHighlighterAttribute SynEditGLSLHighlighter::asmAttribute() const
{
    return mAsmAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::preprocessorAttribute() const
{
    return mPreprocessorAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::invalidAttribute() const
{
    return mInvalidAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::numberAttribute() const
{
    return mNumberAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::floatAttribute() const
{
    return mFloatAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::hexAttribute() const
{
    return mHexAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::octAttribute() const
{
    return mOctAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::stringEscapeSequenceAttribute() const
{
    return mStringEscapeSequenceAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::charAttribute() const
{
    return mCharAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::variableAttribute() const
{
    return mVariableAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::functionAttribute() const
{
    return mFunctionAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::classAttribute() const
{
    return mClassAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::globalVarAttribute() const
{
    return mGlobalVarAttribute;
}

PSynHighlighterAttribute SynEditGLSLHighlighter::localVarAttribute() const
{
    return mLocalVarAttribute;
}

SynEditGLSLHighlighter::ExtTokenKind SynEditGLSLHighlighter::getExtTokenId()
{
    return mExtTokenId;
}

SynTokenKind SynEditGLSLHighlighter::getTokenId()
{
    if ((mRange.state == RangeState::rsAsm || mRange.state == RangeState::rsAsmBlock)
            && !mAsmStart && !(mTokenId == TokenKind::Comment || mTokenId == TokenKind::Space
                               || mTokenId == TokenKind::Null)) {
        return TokenKind::Asm;
    } else {
        return mTokenId;
    }
}

void SynEditGLSLHighlighter::andSymbolProc()
{
    mTokenId = TokenKind::Symbol;
    switch (mLine[mRun+1].unicode()) {
    case '=':
        mRun+=2;
        mExtTokenId = ExtTokenKind::AndAssign;
        break;
    case '&':
        mRun+=2;
        mExtTokenId = ExtTokenKind::LogAnd;
        break;
    default:
        mRun+=1;
        mExtTokenId = ExtTokenKind::And;
    }
}

void SynEditGLSLHighlighter::ansiCppProc()
{
    mTokenId = TokenKind::Comment;
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

void SynEditGLSLHighlighter::ansiCProc()
{
    bool finishProcess = false;
    mTokenId = TokenKind::Comment;
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

void SynEditGLSLHighlighter::asciiCharProc()
{
    mTokenId = TokenKind::Char;
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

void SynEditGLSLHighlighter::atSymbolProc()
{
    mTokenId = TokenKind::Unknown;
    mRun+=1;
}

void SynEditGLSLHighlighter::braceCloseProc()
{
    mRun += 1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::BraceClose;
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

void SynEditGLSLHighlighter::braceOpenProc()
{
    mRun += 1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::BraceOpen;
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

void SynEditGLSLHighlighter::colonProc()
{
    mTokenId = TokenKind::Symbol;
    if (mLine[mRun+1]==':') {
        mRun+=2;
        mExtTokenId = ExtTokenKind::ScopeResolution;
    } else {
        mRun+=1;
        mExtTokenId = ExtTokenKind::Colon;
    }
}

void SynEditGLSLHighlighter::commaProc()
{
    mRun+=1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::Comma;
}

void SynEditGLSLHighlighter::directiveProc()
{
    QString preContents = mLineString.left(mRun).trimmed();
    if (!preContents.isEmpty()) { // '#' is not first non-space char on the line, treat it as an invalid char
       mTokenId = TokenKind::Unknown;
       mRun+=1;
       return;
    }
    mTokenId = TokenKind::Directive;
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

void SynEditGLSLHighlighter::directiveEndProc()
{
    mTokenId = TokenKind::Directive;
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

void SynEditGLSLHighlighter::equalProc()
{
    mTokenId = TokenKind::Symbol;
    if (mLine[mRun+1] == '=') {
        mRun += 2;
        mExtTokenId = ExtTokenKind::LogEqual;
    } else {
        mRun += 1;
        mExtTokenId = ExtTokenKind::Assign;
    }
}

void SynEditGLSLHighlighter::greaterProc()
{
    mTokenId = TokenKind::Symbol;
    switch (mLine[mRun + 1].unicode()) {
    case '=':
        mRun += 2;
        mExtTokenId = ExtTokenKind::GreaterThanEqual;
        break;
    case '>':
        if (mLine[mRun+2] == '=') {
            mRun+=3;
            mExtTokenId = ExtTokenKind::ShiftRightAssign;
        } else {
            mRun += 2;
            mExtTokenId = ExtTokenKind::ShiftRight;
        }
        break;
    default:
        mRun+=1;
        mExtTokenId = ExtTokenKind::GreaterThan;
    }
}

void SynEditGLSLHighlighter::identProc()
{
    int wordEnd = mRun;
    while (isIdentChar(mLine[wordEnd])) {
        wordEnd+=1;
    }
    QString word = mLineString.mid(mRun,wordEnd-mRun);
    mRun=wordEnd;
    if (isKeyword(word)) {
        mTokenId = TokenKind::Key;
        if (GLSLStatementKeyWords.contains(word)) {
            pushIndents(sitStatement);
        }
    } else {
        mTokenId = TokenKind::Identifier;
    }
}

void SynEditGLSLHighlighter::lowerProc()
{
    mTokenId = TokenKind::Symbol;
    switch(mLine[mRun+1].unicode()) {
    case '=':
        mRun+=2;
        mExtTokenId = ExtTokenKind::LessThanEqual;
        break;
    case '<':
        if (mLine[mRun+2] == '=') {
            mRun+=3;
            mExtTokenId = ExtTokenKind::ShiftLeftAssign;
        } else {
            mRun+=2;
            mExtTokenId = ExtTokenKind::ShiftLeft;
        }
        break;
    default:
        mRun+=1;
        mExtTokenId = ExtTokenKind::LessThan;
    }
}

void SynEditGLSLHighlighter::minusProc()
{
    mTokenId = TokenKind::Symbol;
    switch(mLine[mRun+1].unicode()) {
    case '=':
        mRun += 2;
        mExtTokenId = ExtTokenKind::SubtractAssign;
        break;
    case '-':
        mRun += 2;
        mExtTokenId = ExtTokenKind::Decrement;
        break;
    case '>':
        if (mLine[mRun+2]=='*') {
            mRun += 3;
            mExtTokenId = ExtTokenKind::PointerToMemberOfPointer;
        } else {
            mRun += 2;
            mExtTokenId = ExtTokenKind::Arrow;
        }
        break;
    default:
        mRun += 1;
        mExtTokenId = ExtTokenKind::Subtract;
    }
}

void SynEditGLSLHighlighter::modSymbolProc()
{
    mTokenId = TokenKind::Symbol;
    switch(mLine[mRun + 1].unicode()) {
    case '=':
        mRun += 2;
        mExtTokenId = ExtTokenKind::ModAssign;
        break;
    default:
        mRun += 1;
        mExtTokenId = ExtTokenKind::Mod;
    }
}

void SynEditGLSLHighlighter::notSymbolProc()
{
    mTokenId = TokenKind::Symbol;
    switch(mLine[mRun + 1].unicode()) {
    case '=':
        mRun+=2;
        mExtTokenId = ExtTokenKind::NotEqual;
        break;
    default:
        mRun+=1;
        mExtTokenId = ExtTokenKind::LogComplement;
    }
}

void SynEditGLSLHighlighter::nullProc()
{
    if ((mRun-1>=0) && isSpaceChar(mLine[mRun-1]) &&
    (mRange.state == RangeState::rsCppComment
     || mRange.state == RangeState::rsDirective
     || mRange.state == RangeState::rsString
     || mRange.state == RangeState::rsMultiLineString
     || mRange.state == RangeState::rsMultiLineDirective) ) {
        mRange.state = RangeState::rsUnknown;
    } else
        mTokenId = TokenKind::Null;
}

void SynEditGLSLHighlighter::numberProc()
{
    int idx1; // token[1]
    idx1 = mRun;
    mRun+=1;
    mTokenId = TokenKind::Number;
    bool shouldExit = false;
    while (mLine[mRun]!=0) {
        switch(mLine[mRun].unicode()) {
        case '\'':
            if (mTokenId != TokenKind::Number) {
                mTokenId = TokenKind::Symbol;
                return;
            }
            break;
        case '.':
            if (mLine[mRun+1] == '.') {
                mRun+=2;
                mTokenId = TokenKind::Unknown;
                return;
            } else if (mTokenId != TokenKind::Hex) {
                mTokenId = TokenKind::Float;
            } else {
                mTokenId = TokenKind::Unknown;
                return;
            }
            break;
        case '-':
        case '+':
            if (mTokenId != TokenKind::Float) // number <> float. an arithmetic operator
                return;
            if (mLine[mRun-1]!= 'e' && mLine[mRun-1]!='E')  // number = float, but no exponent. an arithmetic operator
                return;
            if (mLine[mRun+1]<'0' || mLine[mRun+1]>'9')  {// invalid
                mRun+=1;
                mTokenId = TokenKind::Unknown;
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
                mTokenId = TokenKind::Octal;
            }
            break;
        case '8':
        case '9':
            if ( (mLine[idx1]=='0') && (mTokenId != TokenKind::Hex)  && (mTokenId != TokenKind::Float) ) // invalid octal char
                mTokenId = TokenKind::Unknown; // we must continue parse, it may be an float number
            break;
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
            if (mTokenId!=TokenKind::Hex) { //invalid
                mTokenId = TokenKind::Unknown;
                return;
            }
            break;
        case 'e':
        case 'E':
            if (mTokenId!=TokenKind::Hex) {
                if (mLine[mRun-1]>='0' || mLine[mRun-1]<='9' ) {//exponent
                    for (int i=idx1;i<mRun;i++) {
                        if (mLine[i] == 'e' || mLine[i]=='E') { // too many exponents
                            mRun+=1;
                            mTokenId = TokenKind::Unknown;
                            return;
                        }
                    }
                    if (mLine[mRun+1]!='+' && mLine[mRun+1]!='-' && !(mLine[mRun+1]>='0' && mLine[mRun+1]<='9')) {
                        return;
                    } else {
                        mTokenId = TokenKind::Float;
                    }
                } else {
                    mRun+=1;
                    mTokenId = TokenKind::Unknown;
                    return;
                }
            }
            break;
        case 'f':
        case 'F':
            if (mTokenId!=TokenKind::Hex) {
                for (int i=idx1;i<mRun;i++) {
                    if (mLine[i] == 'f' || mLine[i]=='F') {
                        mRun+=1;
                        mTokenId = TokenKind::Unknown;
                        return;
                    }
                }
                if (mTokenId == TokenKind::Float) {
                    if (mLine[mRun-1]=='l' || mLine[mRun-1]=='L') {
                        mRun+=1;
                        mTokenId = TokenKind::Unknown;
                        return;
                    }
                } else {
                    mTokenId = TokenKind::Float;
                }
            }
            break;
        case 'l':
        case 'L':
            for (int i=idx1;i<=mRun-2;i++) {
                if (mLine[i] == 'l' && mLine[i]=='L') {
                    mRun+=1;
                    mTokenId = TokenKind::Unknown;
                    return;
                }
            }
            if (mTokenId == TokenKind::Float && (mLine[mRun-1]=='f' || mLine[mRun-1]=='F')) {
                mRun+=1;
                mTokenId = TokenKind::Unknown;
                return;
            }
            break;
        case 'u':
        case 'U':
            if (mTokenId == TokenKind::Float) {
                mRun+=1;
                mTokenId = TokenKind::Unknown;
                return;
            } else {
                for (int i=idx1;i<mRun;i++) {
                    if (mLine[i] == 'u' || mLine[i]=='U') {
                        mRun+=1;
                        mTokenId = TokenKind::Unknown;
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
                mTokenId = TokenKind::Hex;
            } else {
                mRun+=1;
                mTokenId = TokenKind::Unknown;
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
        mTokenId = TokenKind::Unknown;
    }
}

void SynEditGLSLHighlighter::orSymbolProc()
{
    mTokenId = TokenKind::Symbol;
    switch ( mLine[mRun+1].unicode()) {
    case '=':
        mRun+=2;
        mExtTokenId = ExtTokenKind::IncOrAssign;
        break;
    case '|':
        mRun+=2;
        mExtTokenId = ExtTokenKind::LogOr;
        break;
    default:
        mRun+=1;
        mExtTokenId = ExtTokenKind::IncOr;
    }
}

void SynEditGLSLHighlighter::plusProc()
{
    mTokenId = TokenKind::Symbol;
    switch(mLine[mRun+1].unicode()){
    case '=':
        mRun+=2;
        mExtTokenId = ExtTokenKind::AddAssign;
        break;
    case '+':
        mRun+=2;
        mExtTokenId = ExtTokenKind::Increment;
        break;
    default:
        mRun+=1;
        mExtTokenId = ExtTokenKind::Add;
    }
}

void SynEditGLSLHighlighter::pointProc()
{
    mTokenId = TokenKind::Symbol;
    if (mLine[mRun+1] == '*' ) {
        mRun+=2;
        mExtTokenId = ExtTokenKind::PointerToMemberOfObject;
    } else if (mLine[mRun+1] == '.' && mLine[mRun+2] == '.') {
        mRun+=3;
        mExtTokenId = ExtTokenKind::Ellipse;
    } else if (mLine[mRun+1]>='0' && mLine[mRun+1]<='9') {
        numberProc();
    } else {
        mRun+=1;
        mExtTokenId = ExtTokenKind::Point;
    }
}

void SynEditGLSLHighlighter::questionProc()
{
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::Question;
    mRun+=1;
}

void SynEditGLSLHighlighter::rawStringProc()
{
    bool noEscaping = false;
    if (mRange.state == RangeState::rsRawStringNotEscaping)
        noEscaping = true;
    mTokenId = TokenKind::RawString;
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

void SynEditGLSLHighlighter::roundCloseProc()
{
    mRun += 1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::RoundClose;
    mRange.parenthesisLevel--;
    if (mRange.parenthesisLevel<0)
        mRange.parenthesisLevel=0;
    popIndents(sitParenthesis);
}

void SynEditGLSLHighlighter::roundOpenProc()
{
    mRun += 1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::RoundOpen;
    mRange.parenthesisLevel++;
    pushIndents(sitParenthesis);
}

void SynEditGLSLHighlighter::semiColonProc()
{
    mRun += 1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::SemiColon;
    if (mRange.state == RangeState::rsAsm)
        mRange.state = RangeState::rsUnknown;
    while (mRange.getLastIndent() == sitStatement) {
        popIndents(sitStatement);
    }
}

void SynEditGLSLHighlighter::slashProc()
{
    switch(mLine[mRun+1].unicode()) {
    case '/': // Cpp style comment
        mTokenId = TokenKind::Comment;
        mRun+=2;
        mRange.state = RangeState::rsCppComment;
        return;
    case '*': // C style comment
        mTokenId = TokenKind::Comment;
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
        mTokenId = TokenKind::Symbol;
        mExtTokenId = ExtTokenKind::DivideAssign;
        break;
    default:
        mRun += 1;
        mTokenId = TokenKind::Symbol;
        mExtTokenId = ExtTokenKind::Divide;
    }
}

void SynEditGLSLHighlighter::spaceProc()
{
    mRun += 1;
    mTokenId = TokenKind::Space;
    while (mLine[mRun]>=1 && mLine[mRun]<=32)
        mRun+=1;
    mRange.state = RangeState::rsUnknown;
}

void SynEditGLSLHighlighter::squareCloseProc()
{
    mRun+=1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::SquareClose;
    mRange.bracketLevel--;
    if (mRange.bracketLevel<0)
        mRange.bracketLevel=0;
    popIndents(sitBracket);
}

void SynEditGLSLHighlighter::squareOpenProc()
{
    mRun+=1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::SquareOpen;
    mRange.bracketLevel++;
    pushIndents(sitBracket);
}

void SynEditGLSLHighlighter::starProc()
{
    mTokenId = TokenKind::Symbol;
    if (mLine[mRun+1] == '=') {
        mRun += 2;
        mExtTokenId = ExtTokenKind::MultiplyAssign;
    } else {
        mRun += 1;
        mExtTokenId = ExtTokenKind::Star;
    }
}

void SynEditGLSLHighlighter::stringEndProc()
{
    mTokenId = TokenKind::String;
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

void SynEditGLSLHighlighter::stringEscapeSeqProc()
{
    mTokenId = TokenKind::StringEscapeSeq;
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
        mTokenId = TokenKind::Unknown;
        mRun+=1;
        break;
    case 'x':
        mRun+=1;
        if ( !(
                 (mLine[mRun]>='0' && mLine[mRun]<='9')
               ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
               ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                )) {
            mTokenId = TokenKind::Unknown;
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
                mTokenId = TokenKind::Unknown;
                return;
            }
            mRun+=1;
        }
        break;
    case 'U':
        mRun+=1;
        for (int i=0;i<8;i++) {
            if (mLine[mRun]<'0' || mLine[mRun]>'7') {
                mTokenId = TokenKind::Unknown;
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

void SynEditGLSLHighlighter::stringProc()
{
    if (mLine[mRun] == 0) {
        mRange.state = RangeState::rsUnknown;
        return;
    }
    mTokenId = TokenKind::String;
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

void SynEditGLSLHighlighter::stringStartProc()
{
    mTokenId = TokenKind::String;
    mRun += 1;
    if (mLine[mRun]==0) {
        mRange.state = RangeState::rsUnknown;
        return;
    }
    stringProc();
}

void SynEditGLSLHighlighter::tildeProc()
{
    mRun+=1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::BitComplement;
}

void SynEditGLSLHighlighter::unknownProc()
{
    mRun+=1;
    mTokenId = TokenKind::Unknown;
}

void SynEditGLSLHighlighter::xorSymbolProc()
{
    mTokenId = TokenKind::Symbol;
    if (mLine[mRun+1]=='=') {
        mRun+=2;
        mExtTokenId = ExtTokenKind::XorAssign;
    } else {
        mRun+=1;
        mExtTokenId = ExtTokenKind::Xor;
    }
}

void SynEditGLSLHighlighter::processChar()
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

void SynEditGLSLHighlighter::popIndents(int indentType)
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

void SynEditGLSLHighlighter::pushIndents(int indentType)
{
    int idx = mRange.indents.length();
    if (idx<mRange.firstIndentThisLine)
        mRange.firstIndentThisLine = idx;
    mRange.indents.push_back(indentType);
}

bool SynEditGLSLHighlighter::getTokenFinished() const
{
    if (mTokenId == TokenKind::Comment
            || mTokenId == TokenKind::String
            || mTokenId == TokenKind::RawString) {
        return mRange.state == RangeState::rsUnknown;
    }
    return true;
}

bool SynEditGLSLHighlighter::isLastLineCommentNotFinished(int state) const
{
    return (state == RangeState::rsAnsiC ||
            state == RangeState::rsAnsiCAsm ||
            state == RangeState::rsAnsiCAsmBlock ||
            state == RangeState::rsDirectiveComment||
            state == RangeState::rsCppComment);
}

bool SynEditGLSLHighlighter::isLastLineStringNotFinished(int state) const
{
    return state == RangeState::rsMultiLineString;
}

bool SynEditGLSLHighlighter::eol() const
{
    return mTokenId == TokenKind::Null;
}

QString SynEditGLSLHighlighter::getToken() const
{
    return mLineString.mid(mTokenPos,mRun-mTokenPos);
}

PSynHighlighterAttribute SynEditGLSLHighlighter::getTokenAttribute() const
{
    switch (mTokenId) {
    case TokenKind::Asm:
        return mAsmAttribute;
    case TokenKind::Comment:
        return mCommentAttribute;
    case TokenKind::Directive:
        return mPreprocessorAttribute;
    case TokenKind::Identifier:
        return mIdentifierAttribute;
    case TokenKind::Key:
        return mKeywordAttribute;
    case TokenKind::Number:
        return mNumberAttribute;
    case TokenKind::Float:
    case TokenKind::HexFloat:
        return mFloatAttribute;
    case TokenKind::Hex:
        return mHexAttribute;
    case TokenKind::Octal:
        return mOctAttribute;
    case TokenKind::Space:
        return mWhitespaceAttribute;
    case TokenKind::String:
        return mStringAttribute;
    case TokenKind::StringEscapeSeq:
        return mStringEscapeSequenceAttribute;
    case TokenKind::RawString:
        return mStringAttribute;
    case TokenKind::Char:
        return mCharAttribute;
    case TokenKind::Symbol:
        return mSymbolAttribute;
    case TokenKind::Unknown:
        return mInvalidAttribute;
    default:
        return mInvalidAttribute;
    }
}

SynTokenKind SynEditGLSLHighlighter::getTokenKind()
{
    return mTokenId;
}

int SynEditGLSLHighlighter::getTokenPos()
{
    return mTokenPos;
}

void SynEditGLSLHighlighter::next()
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
                mTokenId = TokenKind::Char;
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
    } while (mTokenId!=TokenKind::Null && mRun<=mTokenPos);
}

void SynEditGLSLHighlighter::setLine(const QString &newLine, int lineNumber)
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

bool SynEditGLSLHighlighter::isKeyword(const QString &word)
{
    return Keywords.contains(word);
}

SynHighlighterTokenType SynEditGLSLHighlighter::getTokenType()
{
    switch(mTokenId) {
    case TokenKind::Comment:
        return SynHighlighterTokenType::Comment;
    case TokenKind::Directive:
        return SynHighlighterTokenType::PreprocessDirective;
    case TokenKind::Identifier:
        return SynHighlighterTokenType::Identifier;
    case TokenKind::Key:
        return SynHighlighterTokenType::Keyword;
    case TokenKind::Space:
        switch (mRange.state) {
        case RangeState::rsAnsiC:
        case RangeState::rsAnsiCAsm:
        case RangeState::rsAnsiCAsmBlock:
        case RangeState::rsAsm:
        case RangeState::rsAsmBlock:
        case RangeState::rsDirectiveComment:
        case RangeState::rsCppComment:
            return SynHighlighterTokenType::Comment;
        case RangeState::rsDirective:
        case RangeState::rsMultiLineDirective:
            return SynHighlighterTokenType::PreprocessDirective;
        case RangeState::rsString:
        case RangeState::rsMultiLineString:
        case RangeState::rsStringEscapeSeq:
        case RangeState::rsMultiLineStringEscapeSeq:
        case RangeState::rsRawString:
            return SynHighlighterTokenType::String;
        case RangeState::rsChar :
            return SynHighlighterTokenType::Character;
        default:
            return SynHighlighterTokenType::Space;
        }
    case TokenKind::String:
        return SynHighlighterTokenType::String;
    case TokenKind::StringEscapeSeq:
        return SynHighlighterTokenType::StringEscapeSequence;
    case TokenKind::RawString:
        return SynHighlighterTokenType::String;
    case TokenKind::Char:
        return SynHighlighterTokenType::Character;
    case TokenKind::Symbol:
        return SynHighlighterTokenType::Symbol;
    case TokenKind::Number:
        return SynHighlighterTokenType::Number;
    default:
        return SynHighlighterTokenType::Default;
    }
}

void SynEditGLSLHighlighter::setState(const SynRangeState& rangeState)
{
    mRange = rangeState;
    // current line's left / right parenthesis count should be reset before parsing each line
    mRange.leftBraces = 0;
    mRange.rightBraces = 0;
    mRange.firstIndentThisLine = mRange.indents.length();
    mRange.matchingIndents.clear();
}

void SynEditGLSLHighlighter::resetState()
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

SynHighlighterClass SynEditGLSLHighlighter::getClass() const
{
    return SynHighlighterClass::GLSLHighlighter;
}

QString SynEditGLSLHighlighter::getName() const
{
    return SYN_HIGHLIGHTER_CPP;
}

QString SynEditGLSLHighlighter::languageName()
{
    return "glsl";
}

SynHighlighterLanguage SynEditGLSLHighlighter::language()
{
    return SynHighlighterLanguage::GLSL;
}

SynRangeState SynEditGLSLHighlighter::getRangeState() const
{
    return mRange;
}

bool SynEditGLSLHighlighter::isIdentChar(const QChar &ch) const
{
    return ch=='_' || (ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || (ch>='0' && ch<='9');
}

QSet<QString> SynEditGLSLHighlighter::keywords() const
{
    return Keywords;
}
