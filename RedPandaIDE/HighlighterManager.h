#ifndef HIGHLIGHTERMANAGER_H
#define HIGHLIGHTERMANAGER_H
#include "qsynedit/highlighter/base.h"

class HighlighterManager
{
public:
    HighlighterManager();

    PSynHighlighter getHighlighter(const QString& filename);
    PSynHighlighter getCppHighlighter();
};

extern HighlighterManager highlighterManager;

#endif // HIGHLIGHTERMANAGER_H
