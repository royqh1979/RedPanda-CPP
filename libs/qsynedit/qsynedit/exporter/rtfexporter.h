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
#ifndef QSYNEDIT_RTFEXPORTER_H
#define QSYNEDIT_RTFEXPORTER_H

#include "exporter.h"

namespace QSynedit {

class RTFExporter : public Exporter
{
public:
    explicit RTFExporter(int tabSize,const QByteArray charset);
    RTFExporter(const RTFExporter&)=delete;
    RTFExporter& operator=(const RTFExporter&)=delete;
private:
    bool mAttributesChanged;
    QList<QColor> mListColors;
    QString colorToRTF(const QColor& AColor) const;
    int getColorIndex(const QColor& AColor);
    QString getFontTable();

    // SynExporter interface
protected:
    void formatAttributeDone(bool backgroundChanged, bool foregroundChanged, FontStyles fontStyles) override;
    void formatAttributeInit(bool backgroundChanged, bool foregroundChanged, FontStyles fontStyles) override;
    void formatAfterLastAttribute() override;
    void formatBeforeFirstAttribute(bool backgroundChanged, bool foregroundChanged, FontStyles fontStyles) override;
    void formatNewLine() override;
    QString getFooter() override;
    QString getFormatName() override;
    QString getHeader() override;
};

}
#endif // RTFEXPORTER_H
