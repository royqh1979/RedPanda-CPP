#ifndef CUSTOMHIGHLIGHTERV1_H
#define CUSTOMHIGHLIGHTERV1_H
#include "base.h"
namespace QSynedit {

class CustomHighlighterV1:public Highlighter
{
public:
    CustomHighlighterV1();

protected:
    bool mIgnoreCase;
    QSet<QString> mTypeKeywords;
    QSet<QString> mCallableKeywords;
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
