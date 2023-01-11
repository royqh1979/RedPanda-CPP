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
#include "qtsupportedhtmlexporter.h"
#include "../miscprocs.h"
#include <functional>

namespace QSynedit {

QtSupportedHtmlExporter::QtSupportedHtmlExporter(int tabSize,const QByteArray charset):
    Exporter(tabSize,charset)
{
    mClipboardFormat = "text/html";
    mDefaultFilter = "HTML Documents (*.htm;*.html)|*.htm;*.html";
    // setup array of chars to be replaced
    mReplaceReserved['&'] = "&amp;";
    mReplaceReserved['<'] = "&lt;";
    mReplaceReserved['>'] = "&gt;";
    mReplaceReserved['"'] = "&quot;";
    mReplaceReserved[' '] = "&nbsp;";
    mReplaceReserved['\t'] = mReplaceReserved[' '].repeated(tabSize);
    mCreateHTMLFragment = false;
}

bool QtSupportedHtmlExporter::createHTMLFragment() const
{
    return mCreateHTMLFragment;
}

void QtSupportedHtmlExporter::setCreateHTMLFragment(bool createHTMLFragment)
{
    mCreateHTMLFragment = createHTMLFragment;
}

QString QtSupportedHtmlExporter::attriToCSS(PTokenAttribute attri)
{
    QString result;
    if (mUseBackground && attri->background().isValid())
        result += "background-color: " + colorToHTML(attri->background()) + "; ";
    if (attri->foreground().isValid())
        result += "color: " + colorToHTML(attri->foreground()) + "; ";

    if (attri->styles().testFlag(FontStyle::fsBold))
        result += "font-weight: bold; ";
    if (attri->styles().testFlag(FontStyle::fsItalic))
        result += "font-style: italic; ";
    if (attri->styles().testFlag(FontStyle::fsUnderline))
        result += "text-decoration: underline; ";
    if (attri->styles().testFlag(FontStyle::fsStrikeOut))
        result += "text-decoration: line-through; ";
    return result;
}

QString QtSupportedHtmlExporter::colorToHTML(const QColor &color)
{
    return color.name();
}

void QtSupportedHtmlExporter::formatAttributeDone(bool , bool , FontStyles )
{
    addData("</span>");
}

void QtSupportedHtmlExporter::formatAttributeInit(bool , bool , FontStyles )
{
    addData(QString("<span style=\"%1\">").arg(attriToCSS(mLastAttri)));
}

void QtSupportedHtmlExporter::formatAfterLastAttribute()
{
    addData("</span>");
}

void QtSupportedHtmlExporter::formatBeforeFirstAttribute(bool, bool, FontStyles)
{
    addData(QString("<span style=\"%1\">").arg(attriToCSS(mLastAttri)));
}

void QtSupportedHtmlExporter::formatNewLine()
{
    addData("<br />");
    addNewLine();
}

QString QtSupportedHtmlExporter::getFooter()
{
    QString result = "";
    result = "</div>" + lineBreak();
    if (mCreateHTMLFragment)
        result += "<!--EndFragment-->";
    result += "</body>"+lineBreak()+ "</html>";
    return result;
}

QString QtSupportedHtmlExporter::getFormatName()
{
    return "HTML";
}

QString QtSupportedHtmlExporter::getHeader()
{
    using namespace std::placeholders;
    QString styles;


    QString HTMLAsTextHeader = "<?xml version=\"1.0\" encoding=\"%2\"?>"+lineBreak() +
            "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" + lineBreak() +
            "<html xmlns=\"http://www.w3.org/1999/xhtml\">" + lineBreak() +
            "<head>"+ lineBreak() +
            "<title>%1</title>" + lineBreak() +
            "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%2\" />" + lineBreak() +
            "<meta name=\"generator\" content=\"QSynEdit HTML exporter\" />" + lineBreak() +
            "</head>" + lineBreak() +
            "<body>" + lineBreak();
    QString header = HTMLAsTextHeader
            .arg(mTitle)
            .arg(QString(mCharset));
    if (mCreateHTMLFragment) {
        HTMLAsTextHeader = "<?xml version=\"1.0\" encoding=\"%2\"?>"+lineBreak() +
                    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" + lineBreak() +
                    "<html xmlns=\"http://www.w3.org/1999/xhtml\">" + lineBreak() +
                    "<head>"+ lineBreak() +
                    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%1\" />" + lineBreak() +
                    "<meta name=\"generator\" content=\"QSynEdit HTML exporter\" />" + lineBreak() +
                    "</head>" + lineBreak() +
                    "<body>" + lineBreak();
        header = HTMLAsTextHeader
                    .arg(QString(mCharset));
    }
    QString result = header;
    if (mCreateHTMLFragment) {
        result += "<!--StartFragment-->"+lineBreak();
    }
    if (mUseBackground) {
        result += QString("<div style=\"color: %1; background-color: %2; font: %3pt %4;\">")
            .arg(colorToHTML(mForegroundColor))
            .arg(colorToHTML(mBackgroundColor))
            .arg(pixelToPoint(mFont.pixelSize()))
            .arg(mFont.family())+lineBreak();
    } else {
        result += QString("<div style=\"color: %1; font: %2pt %3;\">")
            .arg(colorToHTML(mForegroundColor))
            .arg(pixelToPoint(mFont.pixelSize()))
            .arg(mFont.family())+lineBreak();
    }

    return result;
}

void QtSupportedHtmlExporter::setTokenAttribute(PTokenAttribute Attri)
{
    mLastAttri = Attri;
    Exporter::setTokenAttribute(Attri);
}
}
