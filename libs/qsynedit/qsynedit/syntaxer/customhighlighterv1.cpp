#include "customhighlighterv1.h"

namespace QSynedit {
CustomHighlighterV1::CustomHighlighterV1()
{

}

void CustomHighlighterV1::resetState()
{
    mRange.state = RangeState::rsUnknown;
    mRange.braceLevel = 0;
    mRange.bracketLevel = 0;
    mRange.parenthesisLevel = 0;
    mRange.indents.clear();
    mRange.lastUnindent=IndentInfo{IndentType::None,0};
    mRange.hasTrailingSpaces=false;
}

QString CustomHighlighterV1::languageName()
{
    return mLanguageName;
}

ProgrammingLanguage CustomHighlighterV1::language()
{
    return ProgrammingLanguage::Custom;
}
}
