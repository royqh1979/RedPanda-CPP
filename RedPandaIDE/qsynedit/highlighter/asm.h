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
    SynEditASMHighlighter();

    static const QSet<QString> Keywords;
private:
    QChar* mLine;
    int mLineNumber;
    int mRun;
    int mStringLen;
    QChar mToIdent;
    int mTokenPos;
    SynTokenKind mTokenID;
    SynHighlighterAttribute mCommentAttri;
    SynHighlighterAttribute mIdentifierAttri;
    SynHighlighterAttribute mKeyAttri;
    SynHighlighterAttribute mNumberAttri;
    SynHighlighterAttribute mSpaceAttri;
    SynHighlighterAttribute mStringAttri;
    SynHighlighterAttribute mSymbolAttri;
};

#endif // SYNEDITASMHIGHLIGHTER_H
