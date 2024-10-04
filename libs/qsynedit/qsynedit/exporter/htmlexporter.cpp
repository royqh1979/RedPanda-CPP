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
#include "htmlexporter.h"
#include "../miscprocs.h"
#include <functional>

namespace QSynedit {

HTMLExporter::HTMLExporter(int tabSize,const QByteArray charset):
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

bool HTMLExporter::createHTMLFragment() const
{
    return mCreateHTMLFragment;
}

void HTMLExporter::setCreateHTMLFragment(bool createHTMLFragment)
{
    mCreateHTMLFragment = createHTMLFragment;
}

QString HTMLExporter::attriToCSS(PTokenAttribute attri, const QString &uniqueAttriName)
{
    QString styleName = makeValidName(uniqueAttriName);

    QString result = "." + styleName + " { ";
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
    result += "}";
    return result;
}

bool HTMLExporter::attriToCSSCallback(PSyntaxer , PTokenAttribute attri, const QString& uniqueAttriName, QList<void *> params)
{
    QString& styles = *static_cast<QString *>(params[0]);
    styles.append(attriToCSS(attri,uniqueAttriName) + lineBreak());
    return true;
}

QString HTMLExporter::colorToHTML(const QColor &color) const
{
    return color.name();
}

QString HTMLExporter::getStyleName(PSyntaxer syntaxer, PTokenAttribute attri)
{
    QString result;
    enumTokenAttributes(syntaxer,false,
                          std::bind(
                              &HTMLExporter::styleNameCallback,this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3, std::placeholders::_4),
                          {&attri,&result});
    return result;
}

QString HTMLExporter::makeValidName(const QString &name)
{
    QString result;
    for (QChar ch:name) {
        ch = ch.toLower();
        if (ch == '.' || ch =='_')
            result += '-';
        else if ((ch >='a' && ch <= 'z') || (ch>='0' && ch<='9') || (ch == '-'))
            result += ch;
    }
    return result;
}

bool HTMLExporter::styleNameCallback(PSyntaxer /*syntaxer*/, PTokenAttribute attri, const QString& uniqueAttriName, QList<void *> params)
{
    PTokenAttribute& attriToFind = *static_cast<PTokenAttribute*>(params[0]);
    QString& styleName = *static_cast<QString *>(params[1]);

    if (attri == attriToFind) {
        styleName.clear();
        styleName.append(makeValidName(uniqueAttriName));
        return false;
    }
    return true;
}

void HTMLExporter::formatAttributeDone(bool , bool , FontStyles )
{
    addData("</span>");
}

void HTMLExporter::formatAttributeInit(bool , bool , FontStyles )
{
    QString styleName = getStyleName(mSyntaxer, mLastAttri);
    addData(QString("<span class=\"%1\">").arg(styleName));
}

void HTMLExporter::formatAfterLastAttribute()
{
    addData("</span>");
}

void HTMLExporter::formatBeforeFirstAttribute(bool, bool, FontStyles)
{
    QString styleName = getStyleName(mSyntaxer, mLastAttri);
    addData(QString("<span class=\"%1\">").arg(styleName));
}

void HTMLExporter::formatNewLine()
{
    addData("<br />");
    addNewLine();
}

QString HTMLExporter::getFooter()
{
    QString result = "";
    result = "</div>" + lineBreak();
    if (mCreateHTMLFragment)
        result += "<!--EndFragment-->";
    result += "</body>"+lineBreak()+ "</html>";
    return result;
}

QString HTMLExporter::getFormatName()
{
    return "HTML";
}

QString HTMLExporter::getHeader()
{
    using namespace std::placeholders;
    QString styles;
    enumTokenAttributes(mSyntaxer, true,
                          std::bind(&HTMLExporter::attriToCSSCallback,
                                    this, _1, _2, _3, _4),
                          {&styles});

    QString HTMLAsTextHeader =
            "<html>" + lineBreak() +
            "<head>"+ lineBreak() +
            "<title>%1</title>" + lineBreak() +
            "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%2\" />" + lineBreak() +
            "<meta name=\"generator\" content=\"QSynedit HTML exporter\" />" + lineBreak() +
            "<style type=\"text/css\">"+ lineBreak() +
            "<!--" + lineBreak() +
            "body { color: %3; background-color: %4; }"+ lineBreak() +
            "%5" +
            "-->" + lineBreak() +
            "</style>" + lineBreak() +
            "</head>" + lineBreak() +
            "<body>" + lineBreak();
    QString header = HTMLAsTextHeader
            .arg(mTitle)
            .arg(QString(mCharset))
            .arg(colorToHTML(mForegroundColor))
            .arg(colorToHTML(mBackgroundColor))
            .arg(styles);
    if (mCreateHTMLFragment) {
        HTMLAsTextHeader =
                    "<html>" + lineBreak() +
                    "<head>"+ lineBreak() +
                    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%1\" />" + lineBreak() +
                    "<meta name=\"generator\" content=\"QSynedit HTML exporter\" />" + lineBreak() +
                    "<style type=\"text/css\">"+ lineBreak() +
                    "<!--" + lineBreak() +
                    "body { color: %2; background-color: %3; }"+ lineBreak() +
                    "%4" +
                    "-->" + lineBreak() +
                    "</style>" + lineBreak() +
                    "</head>" + lineBreak() +
                    "<body>" + lineBreak();
        header = HTMLAsTextHeader
                    .arg(QString(mCharset))
                    .arg(colorToHTML(mForegroundColor))
                    .arg(colorToHTML(mBackgroundColor))
                    .arg(styles);
    }
    QString result = header;
    if (mCreateHTMLFragment) {
        result += "<!--StartFragment-->";
    }
    result += QString("<div style=\"font: %1pt %2;\">")
            .arg(pixelToPoint(mFont.pixelSize()))
            .arg(mFont.family());

    return result;
}

void HTMLExporter::setTokenAttribute(PTokenAttribute attri)
{
    mLastAttri = attri;
    Exporter::setTokenAttribute(attri);
}

QString HTMLExporter::getStartLineNumberString(int startLine, int endLine)
{
    int maxLineNumbeWidth = (QString("%1").arg(endLine ).length()+1) * pixelToPoint(mFont.pixelSize());
    QString result =
            QString("<table style='width:100%; border:1px; cellspacing:1px;'><tr><td style=\"width: %1pt; font: %2pt '%3'; color: %4; background-color: %5; text-align: right; padding-right: 0.5em; \">")
            .arg(maxLineNumbeWidth)
            .arg(pixelToPoint(mFont.pixelSize()))
            .arg(mFont.family())
            .arg(colorToHTML(mLineNumberColor))
            .arg(colorToHTML(mLineNumberBackgroundColor))
            +lineBreak();
    for (int i=startLine;i<=endLine;i++)
        result+=QString("<span>%1</span><br/>").arg(i)+lineBreak();
    result+=QString("</td><td style=\"font: %1pt '%2';\">")
            .arg(pixelToPoint(mFont.pixelSize()))
            .arg(mFont.family())
            +lineBreak();
    return result;
}

QString HTMLExporter::getEndLineNumberString(int startLine, int endLine)
{
    Q_UNUSED(startLine)
    Q_UNUSED(endLine)
    return "</td></tr></table>"+lineBreak();
}
}
