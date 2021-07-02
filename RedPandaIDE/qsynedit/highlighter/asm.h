#ifndef SYNEDITASMHIGHLIGHTER_H
#define SYNEDITASMHIGHLIGHTER_H

#include "base.h"


class SynEditASMHighlighter : public SynHighlighter
{
    enum TokenKind {
        Comment,
        Identifier,
        Key,
        Null,
        Number,
        Space,
        String,
        Symbol,
        Unknown
    };
public:
    explicit SynEditASMHighlighter();
    PSynHighlighterAttribute numberAttribute();

    static const QSet<QString> Keywords;
private:
    QChar* mLine;
    QString mLineString;
    int mLineNumber;
    int mRun;
    int mStringLen;
    QChar mToIdent;
    int mTokenPos;
    SynTokenKind mTokenID;
    PSynHighlighterAttribute mNumberAttribute;

private:
    void CommentProc();
    void CRProc();
    void GreaterProc();
    void IdentProc();
    void LFProc();
    void LowerProc();
    void NullProc();
    void NumberProc();
    void SingleQuoteStringProc();
    void SlashProc();
    void SpaceProc();
    void StringProc();
    void SymbolProc();
    void UnknownProc();


    // SynHighlighter interface
public:
    bool eol() const override;

    QString languageName() override;
    SynHighlighterLanguage language() override;
    QString getToken() const override;
    PSynHighlighterAttribute getTokenAttribute() const override;
    SynTokenKind getTokenKind() override;
    SynHighlighterTokenType getTokenType() override;
    int getTokenPos() override;
    void next() override;
    void setLine(const QString &newLine, int lineNumber) override;

    // SynHighlighter interface
public:
    SynHighlighterClass getClass() const override;
    QString getName() const override;

    // SynHighlighter interface
public:
    bool getTokenFinished() const override;
    bool isLastLineCommentNotFinished(int state) const override;
    bool isLastLineStringNotFinished(int state) const override;
    SynRangeState getRangeState() const override;
    void setState(SynRangeState rangeState, int braceLevel, int bracketLevel, int parenthesisLevel) override;
    void resetState() override;
};

#endif // SYNEDITASMHIGHLIGHTER_H
