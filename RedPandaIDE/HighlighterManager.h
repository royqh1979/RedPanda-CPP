#ifndef HIGHLIGHTERMANAGER_H
#define HIGHLIGHTERMANAGER_H
#include "qsynedit/highlighter/base.h"

class HighlighterManager
{
public:
    HighlighterManager();

    PSynHighlighter createHighlighter(const QString& filename);
    PSynHighlighter createCppHighlighter();
};

extern HighlighterManager highlighterManager;

#endif // HIGHLIGHTERMANAGER_H
