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
    QSet<QString> mKeywords;
    QSet<QString> mTypeKeywords;
    QSet<QString> mCallableKeywords;
    QString mLanguageName;
    QSet<QString> mSuffixes;
};

}


#endif // CUSTOMHIGHLIGHTERV1_H
