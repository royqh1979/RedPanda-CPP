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
#ifndef HTMLEXPORTER_H
#define HTMLEXPORTER_H

#include "exporter.h"

namespace QSynedit {
class HTMLExporter : public Exporter
{
public:
    explicit HTMLExporter(int tabSize,const QByteArray charset);
    HTMLExporter(const HTMLExporter&)=delete;
    const HTMLExporter& operator=(const HTMLExporter&)=delete;

    bool createHTMLFragment() const;
    void setCreateHTMLFragment(bool createHTMLFragment);

protected:
    bool mCreateHTMLFragment;
private:
    PTokenAttribute mLastAttri;
    QString attriToCSS(PTokenAttribute attri, const QString& uniqueAttriName);
    bool attriToCSSCallback(PSyntaxer syntaxer, PTokenAttribute attri,
                            const QString& uniqueAttriName,  QList<void *> params);
    QString colorToHTML(const QColor &color) const;
    QString getStyleName(PSyntaxer syntaxer,
                         PTokenAttribute attri);
    QString makeValidName(const QString &name);
    bool styleNameCallback(PSyntaxer syntaxer, PTokenAttribute  attri,
                           const QString& uniqueAttriName,  QList<void *> params);

    // Exporter interface
protected:
    void formatAttributeDone(bool backgroundChanged, bool foregroundChanged, FontStyles fontStyles);
    void formatAttributeInit(bool backgroundChanged, bool foregroundChanged, FontStyles fontStyles);
    void formatAfterLastAttribute();
    void formatBeforeFirstAttribute(bool backgroundChanged, bool foregroundChanged, FontStyles fontStyles);
    void formatNewLine();
    QString getFooter();
    QString getFormatName();
    QString getHeader();
    void setTokenAttribute(PTokenAttribute Attri);
protected:
    QString getStartLineNumberString(int startLine, int endLine) override;
    QString getEndLineNumberString(int startLine, int endLine) override;
};
}
#endif // HTMLEXPORTER_H
