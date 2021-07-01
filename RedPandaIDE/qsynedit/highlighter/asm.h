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

    // SynHighlighter interface
public:
    bool eol() const override;

    // SynHighlighter interface
public:
    QString languageName() override;

    // SynHighlighter interface
public:
    SynHighlighterLanguage language() override;

    // SynHighlighter interface
public:
    QString getToken() const override;
};

#endif // SYNEDITASMHIGHLIGHTER_H
