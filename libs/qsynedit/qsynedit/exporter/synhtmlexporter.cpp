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
#include "synhtmlexporter.h"
#include "../MiscProcs.h"
#include <functional>

namespace QSynedit {

SynHTMLExporter::SynHTMLExporter(int tabSize,const QByteArray charset):
    SynExporter(charset)
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

bool SynHTMLExporter::createHTMLFragment() const
{
    return mCreateHTMLFragment;
}

void SynHTMLExporter::setCreateHTMLFragment(bool createHTMLFragment)
{
    mCreateHTMLFragment = createHTMLFragment;
}

QString SynHTMLExporter::AttriToCSS(PHighlighterAttribute Attri, const QString &UniqueAttriName)
{
    QString StyleName = MakeValidName(UniqueAttriName);

    QString Result = "." + StyleName + " { ";
    if (mUseBackground && Attri->background().isValid())
        Result += "background-color: " + ColorToHTML(Attri->background()) + "; ";
    if (Attri->foreground().isValid())
        Result += "color: " + ColorToHTML(Attri->foreground()) + "; ";

    if (Attri->styles().testFlag(FontStyle::fsBold))
        Result += "font-weight: bold; ";
    if (Attri->styles().testFlag(FontStyle::fsItalic))
        Result += "font-style: italic; ";
    if (Attri->styles().testFlag(FontStyle::fsUnderline))
        Result += "text-decoration: underline; ";
    if (Attri->styles().testFlag(FontStyle::fsStrikeOut))
        Result += "text-decoration: line-through; ";
    Result += "}";
    return Result;
}

bool SynHTMLExporter::AttriToCSSCallback(PHighlighter , PHighlighterAttribute Attri, const QString& UniqueAttriName, QList<void *> params)
{
    QString& styles = *static_cast<QString *>(params[0]);
    styles.append(AttriToCSS(Attri,UniqueAttriName) + lineBreak());
    return true;
}

QString SynHTMLExporter::ColorToHTML(const QColor &AColor)
{
    return AColor.name();
}

QString SynHTMLExporter::GetStyleName(PHighlighter Highlighter, PHighlighterAttribute Attri)
{
    QString result;
    enumHighlighterAttributes(Highlighter,false,
                          std::bind(
                              &SynHTMLExporter::StyleNameCallback,this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3, std::placeholders::_4),
                          {&Attri,&result});
    return result;
}

QString SynHTMLExporter::MakeValidName(const QString &Name)
{
    QString Result;
    for (QChar ch:Name) {
        ch = ch.toLower();
        if (ch == '.' || ch =='_')
            Result += '-';
        else if ((ch >='a' && ch <= 'z') || (ch>='0' && ch<='9') || (ch == '-'))
            Result += ch;
    }
    return Result;
}

bool SynHTMLExporter::StyleNameCallback(PHighlighter /*Highlighter*/, PHighlighterAttribute Attri, const QString& UniqueAttriName, QList<void *> params)
{
    PHighlighterAttribute& AttriToFind = *static_cast<PHighlighterAttribute*>(params[0]);
    QString& StyleName = *static_cast<QString *>(params[1]);

    if (Attri == AttriToFind) {
        StyleName.clear();
        StyleName.append(MakeValidName(UniqueAttriName));
        return false;
    }
    return true;
}

void SynHTMLExporter::FormatAttributeDone(bool , bool , FontStyles )
{
    AddData("</span>");
}

void SynHTMLExporter::FormatAttributeInit(bool , bool , FontStyles )
{
    QString StyleName = GetStyleName(mHighlighter, mLastAttri);
    AddData(QString("<span class=\"%1\">").arg(StyleName));
}

void SynHTMLExporter::FormatAfterLastAttribute()
{
    AddData("</span>");
}

void SynHTMLExporter::FormatBeforeFirstAttribute(bool, bool, FontStyles)
{
    QString StyleName = GetStyleName(mHighlighter, mLastAttri);
    AddData(QString("<span class=\"%1\">").arg(StyleName));
}

void SynHTMLExporter::FormatNewLine()
{
    AddData("<br />");
    AddNewLine();
}

QString SynHTMLExporter::GetFooter()
{
    QString Result = "";
    Result = "</span>" + lineBreak();
    if (mCreateHTMLFragment)
        Result += "<!--EndFragment-->";
    Result += "</body>"+lineBreak()+ "</html>";
    return Result;
}

QString SynHTMLExporter::GetFormatName()
{
    return "HTML";
}

QString SynHTMLExporter::GetHeader()
{
    using namespace std::placeholders;
    QString Styles;
    enumHighlighterAttributes(mHighlighter, true,
                          std::bind(&SynHTMLExporter::AttriToCSSCallback,
                                    this, _1, _2, _3, _4),
                          {&Styles});

    QString HTMLAsTextHeader = "<?xml version=\"1.0\" encoding=\"%2\"?>"+lineBreak() +
            "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" + lineBreak() +
            "<html xmlns=\"http://www.w3.org/1999/xhtml\">" + lineBreak() +
            "<head>"+ lineBreak() +
            "<title>%1</title>" + lineBreak() +
            "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%2\" />" + lineBreak() +
            "<meta name=\"generator\" content=\"SynEdit HTML exporter\" />" + lineBreak() +
            "<style type=\"text/css\">"+ lineBreak() +
            "<!--" + lineBreak() +
            "body { color: %3; background-color: %4; }"+ lineBreak() +
            "%5" +
            "-->" + lineBreak() +
            "</style>" + lineBreak() +
            "</head>" + lineBreak() +
            "<body>" + lineBreak();
    QString Header = HTMLAsTextHeader
            .arg(mTitle)
            .arg(QString(mCharset))
            .arg(ColorToHTML(mForegroundColor))
            .arg(ColorToHTML(mBackgroundColor))
            .arg(Styles);
    if (mCreateHTMLFragment) {
        HTMLAsTextHeader = "<?xml version=\"1.0\" encoding=\"%2\"?>"+lineBreak() +
                    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" + lineBreak() +
                    "<html xmlns=\"http://www.w3.org/1999/xhtml\">" + lineBreak() +
                    "<head>"+ lineBreak() +
                    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%1\" />" + lineBreak() +
                    "<meta name=\"generator\" content=\"SynEdit HTML exporter\" />" + lineBreak() +
                    "<style type=\"text/css\">"+ lineBreak() +
                    "<!--" + lineBreak() +
                    "body { color: %2; background-color: %3; }"+ lineBreak() +
                    "%4" +
                    "-->" + lineBreak() +
                    "</style>" + lineBreak() +
                    "</head>" + lineBreak() +
                    "<body>" + lineBreak();
        Header = HTMLAsTextHeader
                    .arg(QString(mCharset))
                    .arg(ColorToHTML(mForegroundColor))
                    .arg(ColorToHTML(mBackgroundColor))
                    .arg(Styles);
    }
    QString Result = Header;
    if (mCreateHTMLFragment) {
        Result += "<!--StartFragment-->";
    }
    Result += QString("<span style=\"font: %1pt %2;\">")
            .arg(pixelToPoint(mFont.pixelSize()))
            .arg(mFont.family());

    return Result;
}

void SynHTMLExporter::SetTokenAttribute(PHighlighterAttribute Attri)
{
    mLastAttri = Attri;
    SynExporter::SetTokenAttribute(Attri);
}
}
