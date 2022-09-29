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
#ifndef SYNRTFEXPORTER_H
#define SYNRTFEXPORTER_H

#include "synexporter.h"

namespace QSynedit {

class SynRTFExporter : public SynExporter
{
public:
    explicit SynRTFExporter(const QByteArray charset);
private:
    bool mAttributesChanged;
    QList<QColor> mListColors;
    QString ColorToRTF(const QColor& AColor) const;
    int GetColorIndex(const QColor& AColor);
    QString GetFontTable();

    // SynExporter interface
protected:
    void FormatAttributeDone(bool BackgroundChanged, bool ForegroundChanged, FontStyles FontStylesChanged) override;
    void FormatAttributeInit(bool BackgroundChanged, bool ForegroundChanged, FontStyles FontStylesChanged) override;
    void FormatAfterLastAttribute() override;
    void FormatBeforeFirstAttribute(bool BackgroundChanged, bool ForegroundChanged, FontStyles FontStylesChanged) override;
    void FormatNewLine() override;
    QString GetFooter() override;
    QString GetFormatName() override;
    QString GetHeader() override;
};

}
#endif // SYNRTFEXPORTER_H
