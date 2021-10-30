#include "cpp.h"
#include "../Constants.h"

#include <QFont>

static const QSet<QString> StatementKeyWords {
    "if",
    "for",
    "try",
    "catch",
    "else"
};



const QSet<QString> SynEditCppHighlighter::Keywords {
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
SynEditCppHighlighter::SynEditCppHighlighter(): SynHighlighter()
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

PSynHighlighterAttribute SynEditCppHighlighter::asmAttribute() const
{
    return mAsmAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::preprocessorAttribute() const
{
    return mPreprocessorAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::invalidAttribute() const
{
    return mInvalidAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::numberAttribute() const
{
    return mNumberAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::floatAttribute() const
{
    return mFloatAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::hexAttribute() const
{
    return mHexAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::octAttribute() const
{
    return mOctAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::stringEscapeSequenceAttribute() const
{
    return mStringEscapeSequenceAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::charAttribute() const
{
    return mCharAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::variableAttribute() const
{
    return mVariableAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::functionAttribute() const
{
    return mFunctionAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::classAttribute() const
{
    return mClassAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::globalVarAttribute() const
{
    return mGlobalVarAttribute;
}

PSynHighlighterAttribute SynEditCppHighlighter::localVarAttribute() const
{
    return mLocalVarAttribute;
}

SynEditCppHighlighter::ExtTokenKind SynEditCppHighlighter::getExtTokenId()
{
    return mExtTokenId;
}

SynTokenKind SynEditCppHighlighter::getTokenId()
{
    if ((mRange.state == RangeState::rsAsm || mRange.state == RangeState::rsAsmBlock)
            && !mAsmStart && !(mTokenId == TokenKind::Comment || mTokenId == TokenKind::Space
                               || mTokenId == TokenKind::Null)) {
        return TokenKind::Asm;
    } else {
        return mTokenId;
    }
}

void SynEditCppHighlighter::andSymbolProc()
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

void SynEditCppHighlighter::ansiCppProc()
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
        if ( isSpaceChar(mLine[mRun]) ) {
            mRange.spaceState = mRange.state;
            mRange.state = RangeState::rsSpace;
            return;
        }
        mRun+=1;
    }
    mRange.state = RangeState::rsCppCommentEnded;
    if (mLine[mRun-1] == '\\' && mLine[mRun]==0) { // continues on next line
        mRange.state = RangeState::rsCppComment;
    }
}

void SynEditCppHighlighter::ansiCProc()
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
            } else
                mRun+=1;
            break;
        case 9:
        case 32:
            mRange.spaceState = mRange.state;
            mRange.state = RangeState::rsSpace;
            finishProcess = true;
            break;
        default:
            mRun+=1;
        }
        if (finishProcess)
            break;
    }
}

void SynEditCppHighlighter::asciiCharProc()
{
    mTokenId = TokenKind::Char;
    do {
        if (isSpaceChar(mLine[mRun])) {
            mRange.spaceState = RangeState::rsChar;
            mRange.state = RangeState::rsSpace;
            return;
        }
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

void SynEditCppHighlighter::atSymbolProc()
{
    mTokenId = TokenKind::Unknown;
    mRun+=1;
}

void SynEditCppHighlighter::braceCloseProc()
{
    mRun += 1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::BraceClose;
    if (mRange.state == RangeState::rsAsmBlock) {
        mRange.state = rsUnknown;
    }

    mRange.braceLevel -= 1;
    if (mRange.leftBraces>0) {
        mRange.leftBraces--;
    } else {
        mRange.rightBraces++ ;
    }
    popIndents(sitBrace);
}

void SynEditCppHighlighter::braceOpenProc()
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

void SynEditCppHighlighter::colonProc()
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

void SynEditCppHighlighter::commaProc()
{
    mRun+=1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::Comma;
}

void SynEditCppHighlighter::directiveProc()
{
    QString preContents = mLineString.left(mRun).trimmed();
    if (!preContents.isEmpty()) { // '#' is not first non-space char on the line, treat it as an invalid char
       mTokenId = TokenKind::Unknown;
       mRun+=1;
       return;
    }
    mTokenId = TokenKind::Directive;
    do {
        if (isSpaceChar(mLine[mRun])) {
            mRange.spaceState = RangeState::rsMultiLineDirective;
            mRange.state = RangeState::rsSpace;
            return;
        }
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

void SynEditCppHighlighter::directiveEndProc()
{
    mTokenId = TokenKind::Directive;
    if (mLine[mRun] == 0) {
        nullProc();
        return;
    }
    mRange.state = RangeState::rsUnknown;
    do {
        if (isSpaceChar(mLine[mRun])) {
            mRange.spaceState = RangeState::rsMultiLineDirective;
            mRange.state = RangeState::rsSpace;
            return;
        }
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

void SynEditCppHighlighter::equalProc()
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

void SynEditCppHighlighter::greaterProc()
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

void SynEditCppHighlighter::identProc()
{
    int wordEnd = mRun;
    while (isIdentChar(mLine[wordEnd])) {
        wordEnd+=1;
    }
    QString word = mLineString.mid(mRun,wordEnd-mRun);
    mRun=wordEnd;
    if (isKeyword(word)) {
        mTokenId = TokenKind::Key;
        if (StatementKeyWords.contains(word)) {
            pushIndents(sitStatement);
        }
    } else {
        mTokenId = TokenKind::Identifier;
    }
}

void SynEditCppHighlighter::lowerProc()
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

void SynEditCppHighlighter::minusProc()
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
        mRun += 2;
        mExtTokenId = ExtTokenKind::Arrow;
        break;
    default:
        mRun += 1;
        mExtTokenId = ExtTokenKind::Subtract;
    }
}

void SynEditCppHighlighter::modSymbolProc()
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

void SynEditCppHighlighter::notSymbolProc()
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

void SynEditCppHighlighter::nullProc()
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

void SynEditCppHighlighter::numberProc()
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

void SynEditCppHighlighter::orSymbolProc()
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

void SynEditCppHighlighter::plusProc()
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

void SynEditCppHighlighter::pointProc()
{
    mTokenId = TokenKind::Symbol;
    if (mLine[mRun+1] == '.' && mLine[mRun+2] == '.') {
        mRun+=3;
        mExtTokenId = ExtTokenKind::Ellipse;
    } else if (mLine[mRun+1]>='0' && mLine[mRun+1]<='9') {
        numberProc();
    } else {
        mRun+=1;
        mExtTokenId = ExtTokenKind::Point;
    }
}

void SynEditCppHighlighter::questionProc()
{
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::Question;
    mRun+=1;
}

void SynEditCppHighlighter::rawStringProc()
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
        case ' ':
        case '\t':
            mRange.state = rsSpace;
            if (noEscaping) {
                mRange.spaceState = RangeState::rsRawStringNotEscaping;
            } else {
                mRange.spaceState = RangeState::rsRawStringEscaping;
            }
            return;
            break;
        }
        mRun+=1;
    }
    mRange.state = RangeState::rsUnknown;
}

void SynEditCppHighlighter::roundCloseProc()
{
    mRun += 1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::RoundClose;
    mRange.parenthesisLevel--;
    popIndents(sitParenthesis);
}

void SynEditCppHighlighter::roundOpenProc()
{
    mRun += 1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::RoundOpen;
    mRange.parenthesisLevel++;
    pushIndents(sitParenthesis);
}

void SynEditCppHighlighter::semiColonProc()
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

void SynEditCppHighlighter::slashProc()
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

void SynEditCppHighlighter::spaceProc()
{
    mRun += 1;
    mTokenId = TokenKind::Space;
    while (mLine[mRun]>=1 && mLine[mRun]<=32)
        mRun+=1;
    mRange.state = mRange.spaceState;
    mRange.spaceState = RangeState::rsUnknown;
}

void SynEditCppHighlighter::squareCloseProc()
{
    mRun+=1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::SquareClose;
    mRange.bracketLevel--;
    popIndents(sitBracket);
}

void SynEditCppHighlighter::squareOpenProc()
{
    mRun+=1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::SquareOpen;
    mRange.bracketLevel++;
    pushIndents(sitBracket);
}

void SynEditCppHighlighter::starProc()
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

void SynEditCppHighlighter::stringEndProc()
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
        if (isSpaceChar(mLine[mRun])) {
            mRange.spaceState = RangeState::rsMultiLineString;
            mRange.state = RangeState::rsSpace;
            return;
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

void SynEditCppHighlighter::stringEscapeSeqProc()
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

void SynEditCppHighlighter::stringProc()
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
        if (isSpaceChar(mLine[mRun])) {
            mRange.spaceState = RangeState::rsString;
            mRange.state = RangeState::rsSpace;
            return;
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

void SynEditCppHighlighter::stringStartProc()
{
    mTokenId = TokenKind::String;
    mRun += 1;
    if (mLine[mRun]==0) {
        mRange.state = RangeState::rsUnknown;
        return;
    }
    stringProc();
}

void SynEditCppHighlighter::tildeProc()
{
    mRun+=1;
    mTokenId = TokenKind::Symbol;
    mExtTokenId = ExtTokenKind::BitComplement;
}

void SynEditCppHighlighter::unknownProc()
{
    mRun+=1;
    mTokenId = TokenKind::Unknown;
}

void SynEditCppHighlighter::xorSymbolProc()
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

void SynEditCppHighlighter::processChar()
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

void SynEditCppHighlighter::popIndents(int indentType)
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

void SynEditCppHighlighter::pushIndents(int indentType)
{
    int idx = mRange.indents.length();
    if (idx<mRange.firstIndentThisLine)
        mRange.firstIndentThisLine = idx;
    mRange.indents.push_back(indentType);
}

bool SynEditCppHighlighter::getTokenFinished() const
{
    if (mTokenId == TokenKind::Comment
            || mTokenId == TokenKind::String
            || mTokenId == TokenKind::RawString) {
        return mRange.state == RangeState::rsUnknown;
    }
    return true;
}

bool SynEditCppHighlighter::isLastLineCommentNotFinished(int state) const
{
    return (state == RangeState::rsAnsiC ||
            state == RangeState::rsAnsiCAsm ||
            state == RangeState::rsAnsiCAsmBlock ||
            state == RangeState::rsDirectiveComment||
            state == RangeState::rsCppComment);
}

bool SynEditCppHighlighter::isLastLineStringNotFinished(int state) const
{
    return state == RangeState::rsMultiLineString;
}

bool SynEditCppHighlighter::eol() const
{
    return mTokenId == TokenKind::Null;
}

QString SynEditCppHighlighter::getToken() const
{
    return mLineString.mid(mTokenPos,mRun-mTokenPos);
}

PSynHighlighterAttribute SynEditCppHighlighter::getTokenAttribute() const
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

SynTokenKind SynEditCppHighlighter::getTokenKind()
{
    return mTokenId;
}

int SynEditCppHighlighter::getTokenPos()
{
    return mTokenPos;
}

void SynEditCppHighlighter::next()
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
        case RangeState::rsSpace:
            spaceProc();
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

void SynEditCppHighlighter::setLine(const QString &newLine, int lineNumber)
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

bool SynEditCppHighlighter::isKeyword(const QString &word)
{
    return Keywords.contains(word);
}

SynHighlighterTokenType SynEditCppHighlighter::getTokenType()
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

void SynEditCppHighlighter::setState(const SynRangeState& rangeState)
{
    mRange = rangeState;
    // current line's left / right parenthesis count should be reset before parsing each line
    mRange.leftBraces = 0;
    mRange.rightBraces = 0;
    mRange.firstIndentThisLine = mRange.indents.length();
    mRange.matchingIndents.clear();
}

void SynEditCppHighlighter::resetState()
{
    mRange.state = RangeState::rsUnknown;
    mRange.spaceState = RangeState::rsUnknown;
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

SynHighlighterClass SynEditCppHighlighter::getClass() const
{
    return SynHighlighterClass::CppHighlighter;
}

QString SynEditCppHighlighter::getName() const
{
    return SYN_HIGHLIGHTER_CPP;
}

QString SynEditCppHighlighter::languageName()
{
    return "cpp";
}

SynHighlighterLanguage SynEditCppHighlighter::language()
{
    return SynHighlighterLanguage::Cpp;
}

SynRangeState SynEditCppHighlighter::getRangeState() const
{
    return mRange;
}

bool SynEditCppHighlighter::isIdentChar(const QChar &ch) const
{
    return ch=='_' || ch.isDigit() || ch.isLetter();
}
