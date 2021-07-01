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
    explicit SynEditASMHighlighter()
    {
        mCommentAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrComment);
        mCommentAttribute->setStyles(SynFontStyle::fsItalic);
        addAttribute(mCommentAttribute);
        mIdentifierAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrIdentifier);
        addAttribute(mIdentifierAttribute);
        mKeywordAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrReservedWord);
        mKeywordAttribute->setStyles(SynFontStyle::fsBold);
        addAttribute(mKeywordAttribute);
        mNumberAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrNumber);
        addAttribute(mNumberAttribute);
        mWhitespaceAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrSpace);
        addAttribute(mWhitespaceAttribute);
        mStringAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrString);
        addAttribute(mStringAttribute);
        mSymbolAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrSymbol);
        addAttribute(mSymbolAttribute);
    }

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
};

#endif // SYNEDITASMHIGHLIGHTER_H
