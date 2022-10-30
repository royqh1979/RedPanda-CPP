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
#include "synexporter.h"

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QMimeData>
#include <QTextCodec>

namespace QSynedit {

SynExporter::SynExporter(const QByteArray charset):mCharset(charset)
{
    mClipboardFormat = "text/plain";
    mFont = QGuiApplication::font();
    mBackgroundColor = QGuiApplication::palette().color(QPalette::Base);
    mForegroundColor = QGuiApplication::palette().color(QPalette::Text);
    mUseBackground = false;
    mExportAsText = false;
    mFileEndingType = FileEndingType::Windows;
    clear();
    setTitle("");
}

void SynExporter::clear()
{
    mBuffer.clear();
    mLastStyle = FontStyle::fsNone;
    mLastBG = QGuiApplication::palette().color(QPalette::Base);
    mLastFG = QGuiApplication::palette().color(QPalette::Text);
}

void SynExporter::CopyToClipboard()
{
    if (mExportAsText) {
        CopyToClipboardFormat("text/plain");
    } else
        CopyToClipboardFormat(clipboardFormat());
}

void SynExporter::ExportAll(PDocument ALines)
{
    ExportRange(ALines, BufferCoord{1, 1}, BufferCoord{INT_MAX, INT_MAX});
}

void SynExporter::ExportRange(PDocument ALines, BufferCoord Start, BufferCoord Stop)
{
    // abort if not all necessary conditions are met
    if (!ALines || !mHighlighter || (ALines->count() == 0))
        return;
    Stop.line = std::max(1, std::min(Stop.line, ALines->count()));
    Stop.ch = std::max(1, std::min(Stop.ch, ALines->getString(Stop.line - 1).length() + 1));
    Start.line = std::max(1, std::min(Start.line, ALines->count()));
    Start.ch = std::max(1, std::min(Start.ch, ALines->getString(Start.line - 1).length() + 1));
    if ( (Start.line > ALines->count()) || (Start.line > Stop.line) )
        return;
    if ((Start.line == Stop.line) && (Start.ch >= Stop.ch))
        return;
    // initialization
    mBuffer.clear();
    // export all the lines into fBuffer
    mFirstAttribute = true;

    if (Start.line == 1)
        mHighlighter->resetState();
    else
        mHighlighter->setState(ALines->ranges(Start.line-2));
    for (int i = Start.line; i<=Stop.line; i++) {
        QString Line = ALines->getString(i-1);
        // order is important, since Start.Y might be equal to Stop.Y
//        if (i == Stop.Line)
//            Line.remove(Stop.Char-1, INT_MAX);
//        if ( (i = Start.Line) && (Start.Char > 1))
//            Line.remove(0, Start.Char - 1);
        // export the line
        mHighlighter->setLine(Line, i);
        while (!mHighlighter->eol()) {
            PHighlighterAttribute attri = mHighlighter->getTokenAttribute();
            int startPos = mHighlighter->getTokenPos();
            QString token = mHighlighter->getToken();
            if (i==Start.line && (startPos+token.length() < Start.ch)) {
                mHighlighter->next();
                continue;
            }
            if (i==Stop.line && (startPos >= Stop.ch-1)) {
                mHighlighter->next();
                continue;
            }
            if (i==Stop.line && (startPos+token.length() > Stop.ch)) {
                token = token.remove(Stop.ch - startPos - 1);
            }
            if (i==Start.line && startPos < Start.ch-1) {
                token = token.mid(Start.ch-1-startPos);
            }

            QString Token = ReplaceReservedChars(token);
            if (mOnFormatToken)
                mOnFormatToken(mHighlighter, i, mHighlighter->getTokenPos()+1, mHighlighter->getToken(),attri);
            SetTokenAttribute(attri);
            FormatToken(Token);
            mHighlighter->next();
        }
        if (i!=Stop.line)
            FormatNewLine();
    }
    if (!mFirstAttribute)
        FormatAfterLastAttribute();
    // insert header
    InsertData(0, GetHeader());
    // add footer
    AddData(GetFooter());
}

void SynExporter::SaveToFile(const QString &AFileName)
{
    QFile file(AFileName);
    if (file.open(QIODevice::WriteOnly)) {
        SaveToStream(file);
    } else {
        throw FileError(QObject::tr("Can't open file '%1' to write!").arg(AFileName));
    }
}

void SynExporter::SaveToStream(QIODevice &AStream)
{
    if (AStream.write(mBuffer)<0) {
        throw FileError(QObject::tr("Failed to write data."));
    }
}

bool SynExporter::exportAsText() const
{
    return mExportAsText;
}

void SynExporter::setExportAsText(bool Value)
{
    if (mExportAsText != Value) {
        mExportAsText = Value;
        clear();
    }
}

QFont SynExporter::font() const
{
    return mFont;
}

void SynExporter::setFont(const QFont &font)
{
    mFont = font;
}

PHighlighter SynExporter::highlighter() const
{
    return mHighlighter;
}

void SynExporter::setHighlighter(PHighlighter Value)
{
    if (mHighlighter != Value) {
        mHighlighter = Value;
        clear();
        if ((mHighlighter) && (mHighlighter->whitespaceAttribute()) && mUseBackground)
            mBackgroundColor = mHighlighter->whitespaceAttribute()->background();
    }
}

QString SynExporter::title() const
{
    return mTitle;
}

void SynExporter::setTitle(const QString &Value)
{
    if (mTitle != Value) {
      if (!Value.isEmpty())
          mTitle = Value;
      else
          mTitle = "Untitled";
    }
}

bool SynExporter::useBackground() const
{
    return mUseBackground;
}

void SynExporter::setUseBackground(bool Value)
{
    if (mUseBackground != Value) {
        mUseBackground = Value;
        clear();
        if ((mHighlighter) && (mHighlighter->whitespaceAttribute()) && mUseBackground)
            mBackgroundColor = mHighlighter->whitespaceAttribute()->background();
    }
}

FileEndingType SynExporter::fileEndingType() const
{
    return mFileEndingType;
}

void SynExporter::setFileEndingType(const FileEndingType &fileEndingType)
{
    mFileEndingType = fileEndingType;
}

QColor SynExporter::foregroundColor() const
{
    return mForegroundColor;
}

void SynExporter::setForegroundColor(const QColor &value)
{
    if (mForegroundColor != value) {
        mForegroundColor = value;
    }
}

QColor SynExporter::backgroundColor() const
{
    return mBackgroundColor;
}

void SynExporter::setBackgroundColor(const QColor &value)
{
    if (mBackgroundColor != value) {
        mBackgroundColor = value;
    }
}

QByteArray SynExporter::charset() const
{
    return mCharset;
}

void SynExporter::setCharset(const QByteArray &charset)
{
    mCharset = charset;
}

QString SynExporter::defaultFilter() const
{
    return mDefaultFilter;
}

void SynExporter::setDefaultFilter(const QString &defaultFilter)
{
    mDefaultFilter = defaultFilter;
}

void SynExporter::AddData(const QString &AText)
{
    if (!AText.isEmpty()) {
        QTextCodec* codec = getCodec();
        mBuffer.append(codec->fromUnicode(AText));
    }
}

void SynExporter::AddDataNewLine(const QString &AText)
{
    AddData(AText);
    AddNewLine();
}

void SynExporter::AddNewLine()
{
    AddData(lineBreak());
}

void SynExporter::CopyToClipboardFormat(QByteArray AFormat)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    QMimeData * mimeData = new QMimeData();
    mimeData->setData(AFormat,mBuffer);
    clipboard->clear();
    clipboard->setMimeData(mimeData);
}

void SynExporter::FormatToken(const QString &Token)
{
    AddData(Token);
}

int SynExporter::GetBufferSize()
{
    return mBuffer.size();
}

QTextCodec * SynExporter::getCodec() {
    QTextCodec* codec = QTextCodec::codecForName(mCharset);
    if (codec == nullptr)
        codec = QTextCodec::codecForLocale();
    return codec;
}
void SynExporter::InsertData(int APos, const QString &AText)
{
    if (!AText.isEmpty()) {
        QTextCodec* codec = getCodec();
        mBuffer.insert(APos,codec->fromUnicode(AText));
    }
}

QString SynExporter::ReplaceReservedChars(const QString &AToken)
{
    if (AToken.isEmpty())
        return "";
    QString result;
    for (QChar ch:AToken) {
        if (mReplaceReserved.contains(ch)) {
            result += mReplaceReserved[ch];
        } else {
            result += ch;
        }
    }
    return result;
}

static QColor ValidatedColor(const QColor& color, const QColor& defaultColor) {
    if (color.isValid())
        return color;
    else
        return defaultColor;
}
void SynExporter::SetTokenAttribute(PHighlighterAttribute Attri)
{
    if (mFirstAttribute) {
        mFirstAttribute = false;
        mLastBG = ValidatedColor(Attri->background(), mBackgroundColor);
        mLastFG = ValidatedColor(Attri->foreground(), mForegroundColor);
        mLastStyle = Attri->styles();
        FormatBeforeFirstAttribute(
                    mUseBackground && (mLastBG != mBackgroundColor),
                    mLastFG != mForegroundColor, Attri->styles());
    } else {
        bool ChangedBG = mUseBackground &&
          (mLastBG != ValidatedColor(Attri->background(), mBackgroundColor));
        bool ChangedFG = (mLastFG != ValidatedColor(Attri->foreground(), mForegroundColor));
        if (ChangedBG || ChangedFG ||  (mLastStyle != Attri->styles())) {
            // which font style bits are to reset?
            FontStyles ChangedStyles = mLastStyle & ~(Attri->styles());
            FormatAttributeDone(ChangedBG, ChangedFG, ChangedStyles);
            // which font style bits are to set?
            ChangedStyles = Attri->styles() & ~(mLastStyle);
            mLastBG = ValidatedColor(Attri->background(), mBackgroundColor);
            mLastFG = ValidatedColor(Attri->foreground(), mForegroundColor);
            mLastStyle = Attri->styles();
            FormatAttributeInit(ChangedBG, ChangedFG, ChangedStyles);
        }
    }
}

const QByteArray &SynExporter::buffer() const
{
    return mBuffer;
}

QByteArray SynExporter::clipboardFormat()
{
    return this->mClipboardFormat;
}

FormatTokenHandler SynExporter::onFormatToken() const
{
    return mOnFormatToken;
}

void SynExporter::setOnFormatToken(const FormatTokenHandler &onFormatToken)
{
    mOnFormatToken = onFormatToken;
}

QString SynExporter::lineBreak()
{
    switch(mFileEndingType) {
    case FileEndingType::Linux:
        return "\n";
    case FileEndingType::Windows:
        return "\r\n";
    case FileEndingType::Mac:
        return "\r";
    }
    return "\n";
}

}
