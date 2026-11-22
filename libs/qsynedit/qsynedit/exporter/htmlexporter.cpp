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
    mReplaceReserved['\''] = "&apos;";
    mReplaceReserved['\t'] = QString(" ").repeated(tabSize);
    mCreateHTMLFragment = false;
}

void HTMLExporter::clear()
{
    Exporter::clear();
    mCssNames.clear();
    mStyleTokens.clear();
    mLastToken = PaintHighlightToken{};
}

bool HTMLExporter::createHTMLFragment() const
{
    return mCreateHTMLFragment;
}

void HTMLExporter::setCreateHTMLFragment(bool createHTMLFragment)
{
    mCreateHTMLFragment = createHTMLFragment;
}

QString HTMLExporter::attrToCSS(const PaintHighlightToken &token, const QString &styleName)
{
    QString result = "." + styleName + " { ";
    if (mUseBackground && token.background.isValid())
        result += "background-color: " + colorToHTML(token.background) + "; ";
    if (token.foreground.isValid())
        result += "color: " + colorToHTML(token.foreground) + "; ";
    if (token.style.testFlag(FontStyle::fsBold))
        result += "font-weight: bold; ";
    if (token.style.testFlag(FontStyle::fsItalic))
        result += "font-style: italic; ";
    QStringList decorations;
    if (token.style.testFlag(FontStyle::fsUnderline))
        decorations.append("underline");
    if (token.style.testFlag(FontStyle::fsStrikeOut))
        decorations.append("line-through");
    if (!decorations.isEmpty())
        result += "text-decoration: " + decorations.join(' ') + "; ";
    result += "}";
    return result;
}

QString HTMLExporter::colorToHTML(const QColor &color) const
{
    return color.name();
}

QString HTMLExporter::getStyleName(PSyntaxer syntaxer, const PaintHighlightToken &token)
{
    QString attributeName = token.attr ? token.attr->name() : QString();
    QString name = QString("%1-%2-%3-%4-%5-%6-%7")
            .arg(syntaxer->languageName(), attributeName)
            .arg(token.foreground.isValid())
            .arg(token.foreground.rgba())
            .arg(token.background.isValid())
            .arg(token.background.rgba())
            .arg(int(token.style));
    QString cssName = mCssNames.value(name);
    if (cssName.isEmpty()) {
        //cssName = makeValidName(name);
        cssName = QString("c%1").arg(mCssNames.count());
        mCssNames.insert(name,cssName);
        mStyleTokens.insert(name, token);
    }
    return cssName;
}

//QString HTMLExporter::makeValidName(const QString &name)
//{
//    QString result;
//    for (QChar ch:name) {
//        ch = ch.toLower();
//        if (ch == '.' || ch =='_')
//            result += '-';
//        else if ((ch >='a' && ch <= 'z') || (ch>='0' && ch<='9') || (ch == '-'))
//            result += ch;
//    }
//    return result;
//}

void HTMLExporter::formatAttributeDone(bool , bool , FontStyles )
{
    addData("</span>");
}

void HTMLExporter::formatAttributeInit(bool , bool , FontStyles )
{
    QString styleName = getStyleName(mSyntaxer, mLastToken);
    addData(QString("<span class=\"%1\">").arg(styleName));
}

void HTMLExporter::formatAfterLastAttribute()
{
    addData("</span>");
}

void HTMLExporter::formatBeforeFirstAttribute(bool, bool, FontStyles)
{
    QString styleName = getStyleName(mSyntaxer, mLastToken);
    addData(QString("<span class=\"%1\">").arg(styleName));
}

void HTMLExporter::formatNewLine()
{
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
    QString styles;
    foreach (const PTokenAttribute &pAttr, mSyntaxer->attributes()){
        PaintHighlightToken token{};
        token.attr = pAttr;
        token.foreground = pAttr->foreground();
        token.background = pAttr->background();
        token.style = pAttr->styles();
        getStyleName(mSyntaxer, token);
    }
    for (auto it = mStyleTokens.constBegin(); it != mStyleTokens.constEnd(); ++it) {
        styles.append(attrToCSS(it.value(), mCssNames.value(it.key())) + lineBreak());
    }

    QString title;
    if (!mCreateHTMLFragment)
        title = "<title>" + replaceReservedChars(mTitle) + "</title>" + lineBreak();
    QString result = "<!DOCTYPE html>" + lineBreak()
            + "<html>" + lineBreak()
            + "<head>" + lineBreak()
            + title
            + "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%1\" />" + lineBreak()
            + "<meta name=\"generator\" content=\"QSynedit HTML exporter\" />" + lineBreak()
            + "<style type=\"text/css\">" + lineBreak()
            + "<!--" + lineBreak()
            + "body { color: %2; background-color: %3; }" + lineBreak()
            + "%4"
            + "-->" + lineBreak()
            + "</style>" + lineBreak()
            + "</head>" + lineBreak()
            + "<body>" + lineBreak();
    result = result.arg(QString(mCharset),
                        colorToHTML(mForegroundColor),
                        colorToHTML(mBackgroundColor),
                        styles);
    if (mCreateHTMLFragment)
        result += "<!--StartFragment-->";
    result += QString("<div style=\"font-size: %1pt; font-family: '%2'; color: %3; background-color: %4; white-space: pre;\">")
            .arg(fontPointSize())
            .arg(replaceReservedChars(mFont.family()),
                 colorToHTML(mForegroundColor),
                 colorToHTML(mBackgroundColor));
    return result;
}

void HTMLExporter::setTokenAttribute(const PaintHighlightToken &token)
{
    mLastToken = token;
    Exporter::setTokenAttribute(token);
}

QString HTMLExporter::getStartLineNumberString(int startLine, int endLine)
{
    int maxLineNumberWidth = (QString("%1").arg(endLine).length()+1) * fontPointSize();
    QString result =
            QString("<table style='width:100%; border:1px; cellspacing:1px;background-color: %1; '><tr><td style=\"width: %2pt; font: %3pt '%4'; color: %5; text-align: right; padding-right: 0.5em; \">")
            .arg(colorToHTML(mBackgroundColor))
            .arg(maxLineNumberWidth)
            .arg(fontPointSize())
            .arg(replaceReservedChars(mFont.family()),
                 colorToHTML(mLineNumberColor))
            +lineBreak();
    for (int i=startLine;i<=endLine;i++)
        result+=QString("<span>%1</span><br/>").arg(i)+lineBreak();
    result+=QString("</td><td style=\"font: %1pt '%2';\">")
            .arg(fontPointSize())
            .arg(replaceReservedChars(mFont.family()))
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
