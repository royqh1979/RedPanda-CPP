#ifndef SYNHTMLEXPORTER_H
#define SYNHTMLEXPORTER_H

#include "synexporter.h"

class SynHTMLExporter : public SynExporter
{
public:
    SynHTMLExporter();
    bool createHTMLFragment() const;
    void setCreateHTMLFragment(bool createHTMLFragment);

protected:
    bool mCreateHTMLFragment;
private:
    PSynHighlighterAttribute mLastAttri;
    QString AttriToCSS(PSynHighlighterAttribute Attri, const QString& UniqueAttriName);
    bool AttriToCSSCallback(PSynHighlighter Highlighter, PSynHighlighterAttribute  Attri,
                            const QString& UniqueAttriName,  QList<void *> params);
    QString ColorToHTML(const QColor &AColor);
    QString GetStyleName(PSynHighlighter Highlighter,
                         PSynHighlighterAttribute Attri);
    QString MakeValidName(const QString &Name);
    bool StyleNameCallback(PSynHighlighter Highlighter, PSynHighlighterAttribute  Attri,
                           const QString& UniqueAttriName,  QList<void *> params);

    // SynExporter interface
protected:
    void FormatAttributeDone(bool BackgroundChanged, bool ForegroundChanged, SynFontStyles FontStylesChanged);
    void FormatAttributeInit(bool BackgroundChanged, bool ForegroundChanged, SynFontStyles FontStylesChanged);
    void FormatAfterLastAttribute();
    void FormatBeforeFirstAttribute(bool BackgroundChanged, bool ForegroundChanged, SynFontStyles FontStylesChanged);
    void FormatNewLine();
    QString GetFooter();
    QString GetFormatName();
    QString GetHeader();
    void SetTokenAttribute(PSynHighlighterAttribute Attri);
};

#endif // SYNHTMLEXPORTER_H
