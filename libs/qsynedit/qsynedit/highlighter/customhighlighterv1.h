#ifndef CUSTOMHIGHLIGHTERV1_H
#define CUSTOMHIGHLIGHTERV1_H
#include "base.h"
namespace QSynedit {

class CustomHighlighterV1:public Highlighter
{
public:
    enum RangeState {
        rsUnknown, rsComment, rsInlineComment,
        rsString, rsMultiLineString,
        rsRawString, rsSpace,
    };

    CustomHighlighterV1();
//    bool getTokenFinished() const override;
//    bool isLastLineCommentNotFinished(int state) const override;
//    bool isLastLineStringNotFinished(int state) const override;
//    bool eol() const override;
//    QString getToken() const override;
//    PHighlighterAttribute getTokenAttribute() const override;
//    int getTokenPos() override;
//    void next() override;
//    void setLine(const QString &newLine, int lineNumber) override;
//    bool isKeyword(const QString &word) override;
//    void setState(const HighlighterState& rangeState) override;
    void resetState() override;

    QString languageName() override;
    HighlighterLanguage language() override;
protected:
    bool mIgnoreCase;
    QString mCommentBlockStart;
    QString mCommentBlockEnd;
    QString mInlineCommentStart;

    QSet<QString> mTypeKeywords;
    QSet<QString> mFunctionKeywords;
    QSet<QString> mKeywords1;
    QSet<QString> mKeywords2;
    QSet<QString> mKeywords3;
    QSet<QString> mKeywords4;
    QSet<QString> mKeywords5;
    QSet<QString> mKeywords6;
    QSet<QString> mOperators;

    QString mLanguageName;
    QSet<QString> mSuffixes;

    HighlighterState mRange;
//    SynRangeState mSpaceRange;
    QString mLine;
    int mLineSize;
    int mRun;
    int mStringLen;
    int mTokenPos;
    int mTokenId;
    int mLineNumber;
    int mLeftBraces;
    int mRightBraces;

    QSet<QString> mCustomTypeKeywords;

    PHighlighterAttribute mInvalidAttribute;
    PHighlighterAttribute mTypeKeywordAttribute;
    PHighlighterAttribute mCallableAttribute;
};

}


#endif // CUSTOMHIGHLIGHTERV1_H
