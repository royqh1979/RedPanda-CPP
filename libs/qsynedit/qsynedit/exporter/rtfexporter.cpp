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
#include "rtfexporter.h"

#include <QFontMetrics>
namespace QSynedit {

RTFExporter::RTFExporter(int tabSize, const QByteArray charset):Exporter(tabSize,charset)
{
    mDefaultFilter = "Rich Text Format Documents (*.rtf)|*.rtf";
    mClipboardFormat = "application/x-qt-windows-mime;value=\"Rich Format Text\"";
    mReplaceReserved['\\'] = "\\\\";
    mReplaceReserved['{'] = "\\{";
    mReplaceReserved['}'] = "\\}";
    mReplaceReserved['\t'] = "\\tab";
}

void RTFExporter::clear()
{
    Exporter::clear();
    mListColors.clear();
    mColorIdx.clear();
    mAttributesChanged = false;
}

QString RTFExporter::colorToRTF(const QColor &color) const
{
    return QString("\\red%1\\green%2\\blue%3;")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue());
}

int RTFExporter::getColorIndex(const QColor &color)
{
    QRgb rgb = color.rgb();
    int index = mColorIdx.value(rgb,-1);
    if (index<0) {
        index = mListColors.count();
        mListColors.append(color);
        mColorIdx.insert(rgb,index);
    }
    return index;
}

QString RTFExporter::getFontTable()
{
    return "{\\fonttbl{\\f0\\fmodern\\fcharset0 "
            + encodeText(replaceReservedChars(mFont.family())) + ";}}" + lineBreak();
}

QString RTFExporter::encodeText(const QString &text) const
{
    QString result;
    result.reserve(text.size());
    for (QChar ch : text) {
        if (ch.unicode() <= 127) {
            result += ch;
        } else {
            result += QString("\\u%1?").arg(static_cast<short>(ch.unicode()));
        }
    }
    return result;
}

void RTFExporter::formatToken(const QString &token)
{
    for (QChar ch : token) {
        if (ch.unicode() > 127) {
            addData(encodeText(token));
            return;
        }
    }
    addData(token);
}

void RTFExporter::formatAttributeDone(bool , bool , FontStyles fontStyles)
{
    if (fontStyles.testFlag(FontStyle::fsBold)) {
        mAttributesChanged = true;
        addData("\\b0");
    }
    if (fontStyles.testFlag(FontStyle::fsItalic)) {
        mAttributesChanged = true;
        addData("\\i0");
    }
    if (fontStyles.testFlag(FontStyle::fsUnderline)) {
        mAttributesChanged = true;
        addData("\\ul0");
    }
    if (fontStyles.testFlag(FontStyle::fsStrikeOut)) {
        mAttributesChanged = true;
        addData("\\strike0");
    }
}

void RTFExporter::formatAttributeInit(bool backgroundChanged, bool foregroundChanged, FontStyles fontStyles)
{
    mAttributesChanged = false;
    if (backgroundChanged) {
        addData(QString("\\chshdng0\\chcbpat%1\\cb%2\\highlight%3 ")
              .arg(getColorIndex(mLastBG))
              .arg(getColorIndex(mLastBG))
              .arg(getColorIndex(mLastBG)));
        mAttributesChanged = true;
    }
    if (foregroundChanged) {
        addData(QString("\\cf%1").arg(getColorIndex(mLastFG)));
        mAttributesChanged = true;
    }
    if (fontStyles.testFlag(FontStyle::fsBold)) {
        mAttributesChanged = true;
        addData("\\b");
    }
    if (fontStyles.testFlag(FontStyle::fsItalic)) {
        mAttributesChanged = true;
        addData("\\i");
    }
    if (fontStyles.testFlag(FontStyle::fsUnderline)) {
        mAttributesChanged = true;
        addData("\\ul");
    }
    if (fontStyles.testFlag(FontStyle::fsStrikeOut)) {
        mAttributesChanged = true;
        addData("\\strike");
    }
    if (mAttributesChanged) {
        addData(" ");
    }
}

void RTFExporter::formatAfterLastAttribute()
{
}

void RTFExporter::formatBeforeFirstAttribute(bool backgroundChanged, bool foregroundChanged, FontStyles fontStyles)
{
    formatAttributeInit(backgroundChanged, foregroundChanged, fontStyles);
}

void RTFExporter::formatNewLine()
{
    addNewLine();
    addData("\\par ");
}

QString RTFExporter::getFooter()
{
    return "}";
}

QString RTFExporter::getFormatName()
{
    return "RTF";
}

QString RTFExporter::getHeader()
{
    QFontMetrics fm(mFont);
    int tabWidth = mTabSize * fm.horizontalAdvance("M")*72*20/fm.fontDpi();

    QString result = QString("{\\rtf1\\ansi\\uc1\\deff0\\deftab%1").arg(tabWidth) + getFontTable();
    getColorIndex(mBackgroundColor);
    result = result + "{\\colortbl";
    for (const QColor &color : mListColors)
        result += colorToRTF(color);
    result = result + "}" + lineBreak();
    result = result + "{\\info{\\comment Generated by the QSynEdit RTF "
        "exporter}" + lineBreak();
    result = result + "{\\title " + encodeText(replaceReservedChars(mTitle)) + "}}" + lineBreak();
    result = result + QString("\\pard\\plain\\shading10000\\cfpat%1\\cbpat%1\\f0\\fs%2")
            .arg(getColorIndex(mBackgroundColor))
            .arg(static_cast<int>(2 * fontPointSize()))
            + lineBreak();
    return result;
}
}
