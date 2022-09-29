/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SYNHTMLEXPORTER_H
#define SYNHTMLEXPORTER_H

#include "synexporter.h"

namespace QSynedit {
class SynHTMLExporter : public SynExporter
{
public:
    SynHTMLExporter(int tabSize,const QByteArray charset);
    bool createHTMLFragment() const;
    void setCreateHTMLFragment(bool createHTMLFragment);

protected:
    bool mCreateHTMLFragment;
private:
    PHighlighterAttribute mLastAttri;
    QString AttriToCSS(PHighlighterAttribute Attri, const QString& UniqueAttriName);
    bool AttriToCSSCallback(PHighlighter Highlighter, PHighlighterAttribute  Attri,
                            const QString& UniqueAttriName,  QList<void *> params);
    QString ColorToHTML(const QColor &AColor);
    QString GetStyleName(PHighlighter Highlighter,
                         PHighlighterAttribute Attri);
    QString MakeValidName(const QString &Name);
    bool StyleNameCallback(PHighlighter Highlighter, PHighlighterAttribute  Attri,
                           const QString& UniqueAttriName,  QList<void *> params);

    // SynExporter interface
protected:
    void FormatAttributeDone(bool BackgroundChanged, bool ForegroundChanged, FontStyles FontStylesChanged);
    void FormatAttributeInit(bool BackgroundChanged, bool ForegroundChanged, FontStyles FontStylesChanged);
    void FormatAfterLastAttribute();
    void FormatBeforeFirstAttribute(bool BackgroundChanged, bool ForegroundChanged, FontStyles FontStylesChanged);
    void FormatNewLine();
    QString GetFooter();
    QString GetFormatName();
    QString GetHeader();
    void SetTokenAttribute(PHighlighterAttribute Attri);
};
}
#endif // SYNHTMLEXPORTER_H
