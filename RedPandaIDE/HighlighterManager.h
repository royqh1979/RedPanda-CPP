#ifndef HIGHLIGHTERMANAGER_H
#define HIGHLIGHTERMANAGER_H
#include "qsynedit/highlighter/base.h"

class HighlighterManager
{
public:
    HighlighterManager();

    PSynHighlighter getHighlighter(const QString& filename);
    PSynHighlighter copyHighlighter(PSynHighlighter highlighter);
    PSynHighlighter getCppHighlighter();
    PSynHighlighter getAsmHighlighter();
    void applyColorScheme(PSynHighlighter highlighter, const QString& schemeName);
};

extern HighlighterManager highlighterManager;

#endif // HIGHLIGHTERMANAGER_H
