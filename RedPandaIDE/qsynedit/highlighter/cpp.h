#ifndef SYNEDITCPPHIGHLIGHTER_H
#define SYNEDITCPPHIGHLIGHTER_H
#include "base.h"
#include <QSet>

class SynEditCppHighlighter: public SynHighlighter
{
    Q_OBJECT

    enum TokenKind {
        Asm = 1,
        Comment,
        Directive,
        Identifier,
        Key,
        Null,
        Number,
        Space,
        String,
        StringEscapeSeq,
        Symbol,
        Unknown,
        Char,
        Float,
        Hex,
        HexFloat,
        Octal,
        RawString
    };

    enum class ExtTokenKind {
        Add, AddAssign, And, AndAssign, Arrow, Assign,
        BitComplement, BraceClose, BraceOpen, Colon, Comma,
        Decrement, Divide, DivideAssign, Ellipse, GreaterThan,
        GreaterThanEqual, IncOr, IncOrAssign, Increment, LessThan,
        LessThanEqual, LogAnd, LogComplement, LogEqual, LogOr,
        Mod, ModAssign, MultiplyAssign, NotEqual, Point, Question,
        RoundClose, RoundOpen, ScopeResolution, SemiColon, ShiftLeft,
        ShiftLeftAssign, ShiftRight, ShiftRightAssign, SquareClose,
        SquareOpen, Star, Subtract, SubtractAssign, Xor,
        XorAssign
    };

    enum RangeState {
        rsUnknown, rsAnsiC, rsAnsiCAsm, rsAnsiCAsmBlock, rsAsm,
        rsAsmBlock, rsDirective, rsDirectiveComment, rsString,
        rsMultiLineString, rsMultiLineDirective, rsCppComment,
        rsStringEscapeSeq, rsMultiLineStringEscapeSeq,
        rsRawString, rsSpace,rsRawStringEscaping,rsRawStringNotEscaping,rsChar
    };

public:
    explicit SynEditCppHighlighter(QObject* parent = nullptr);

    PSynHighlighterAttribute asmAttribute() const;

    PSynHighlighterAttribute direcAttribute() const;

    PSynHighlighterAttribute invalidAttribute() const;

    PSynHighlighterAttribute keyAttribute() const;

    PSynHighlighterAttribute numberAttribute() const;

    PSynHighlighterAttribute floatAttribute() const;

    PSynHighlighterAttribute hexAttribute() const;

    PSynHighlighterAttribute octAttribute() const;

    PSynHighlighterAttribute stringEscapeSequenceAttribute() const;

    PSynHighlighterAttribute charAttribute() const;

    PSynHighlighterAttribute variableAttribute() const;

    PSynHighlighterAttribute functionAttribute() const;

    PSynHighlighterAttribute classAttribute() const;

    PSynHighlighterAttribute globalVarAttribute() const;

    PSynHighlighterAttribute localVarAttribute() const;

    static const QSet<QString> Keywords;

    ExtTokenKind getExtTokenId();
    SynTokenKind getTokenId();
private:
    void andSymbolProc();
    void ansiCppProc();
    void ansiCProc();
    void asciiCharProc();
    void atSymbolProc();
    void braceCloseProc();
    void braceOpenProc();
    void colonProc();
    void commaProc();
    void directiveProc();
    void directiveEndProc();
    void equalProc();
    void greaterProc();
    void identProc();
    void lowerProc();
    void minusProc();
    void modSymbolProc();
    void notSymbolProc();
    void nullProc();
    void numberProc();
    void orSymbolProc();
    void plusProc();
    void pointProc();
    void questionProc();
    void rawStringProc();
    void roundCloseProc();
    void roundOpenProc();
    void semiColonProc();
    void slashProc();
    void spaceProc();
    void squareCloseProc();
    void squareOpenProc();
    void starProc();
    void stringEndProc();
    void stringEscapeSeqProc();
    void stringProc();
    void stringStartProc();
    void tildeProc();
    void unknownProc();
    void xorSymbolProc();
    void processChar();

private:
    bool mAsmStart;
    SynRangeState mRange;
//    SynRangeState mSpaceRange;
    int mParenthesisLevel;
    int mBracketLevel;
    int mBraceLevel;
    QString mLineString;
    QChar* mLine;
    int mLineSize;
    int mRun;
    int mStringLen;
    int mToIdent;
    int mTokenPos;
    int mTokenId;
    ExtTokenKind mExtTokenId;
    int mLineNumber;

    PSynHighlighterAttribute mAsmAttribute;
    PSynHighlighterAttribute mDirecAttribute;
    PSynHighlighterAttribute mInvalidAttribute;
    PSynHighlighterAttribute mKeyAttribute;
    PSynHighlighterAttribute mNumberAttribute;
    PSynHighlighterAttribute mFloatAttribute;
    PSynHighlighterAttribute mHexAttribute;
    PSynHighlighterAttribute mOctAttribute;
    PSynHighlighterAttribute mStringEscapeSequenceAttribute;
    PSynHighlighterAttribute mCharAttribute;
    PSynHighlighterAttribute mVariableAttribute;
    PSynHighlighterAttribute mFunctionAttribute;
    PSynHighlighterAttribute mClassAttribute;
    PSynHighlighterAttribute mGlobalVarAttribute;
    PSynHighlighterAttribute mLocalVarAttribute;



    // SynHighligterBase interface
public:
    bool getTokenFinished() const override;
    bool isLastLineCommentNotFinished(int state) const override;
    bool isLastLineStringNotFinished(int state) const override;
    bool eol() const override;
    QString getToken() const override;
    PSynHighlighterAttribute getTokenAttribute() const override;
    SynTokenKind getTokenKind() override;
    int getTokenPos() override;
    void next() override;
    void setLine(const QString &newLine, int lineNumber) override;

    // SynHighligterBase interface
public:
    int getBraceLevel() const override;
    int getBracketLevel() const override;
    int getParenthesisLevel() const override;

    // SynHighligterBase interface
public:
    bool isKeyword(const QString &word) override;

    // SynHighligterBase interface
public:
    SynHighlighterTokenType getTokenType() override;

    // SynHighligterBase interface
public:
    void setState(SynRangeState rangeState, int braceLevel, int bracketLevel, int parenthesisLevel) override;

    // SynHighligterBase interface
public:
    void resetState() override;

    // SynHighligterBase interface
public:
    SynHighlighterClass getClass() const override;

    // SynHighlighter interface
public:
    QString getName() const override;
};

#endif // SYNEDITCPPHIGHLIGHTER_H
