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
#include "exporter.h"
#include "../document.h"
#include "qdebug.h"
#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QMimeData>
#include <QPalette>
#include <qt_utils/utils.h>

namespace QSynedit {

Exporter::Exporter(int tabSize, const QByteArray charset):
    mTabSize{tabSize},
    mCharset{charset},
    mFont{QGuiApplication::font()},
    mBackgroundColor{QGuiApplication::palette().color(QPalette::Base)},
    mForegroundColor{QGuiApplication::palette().color(QPalette::Text)},
    mUseBackground{false},
    mFileEndingType{NewlineType::Windows},
    mExportLineNumber(false),
    mRecalcLineNumber(true),
    mLineNumberStartFromZero(false)
{
    clear();
    setTitle("");
}

void Exporter::clear()
{
    mText.clear();
    mLastStyle = FontStyle::fsNone;
    mLastBG = QGuiApplication::palette().color(QPalette::Base);
    mLastFG = QGuiApplication::palette().color(QPalette::Text);
}

void Exporter::exportAll(const PDocument& doc)
{
    exportRange(doc, BufferCoord{1, 1}, BufferCoord{INT_MAX, INT_MAX});
}

void Exporter::exportRange(const PDocument& doc, BufferCoord start, BufferCoord stop)
{
    // abort if not all necessary conditions are met
    if (!doc || (doc->count() == 0))
        return;
    stop.line = std::max(1, std::min(stop.line, doc->count()));
    stop.ch = std::max(1, std::min(stop.ch, doc->getLine(stop.line - 1).length() + 1));
    start.line = std::max(1, std::min(start.line, doc->count()));
    start.ch = std::max(1, std::min(start.ch, doc->getLine(start.line - 1).length() + 1));
    if ( (start.line > doc->count()) || (start.line > stop.line) )
        return;
    if ((start.line == stop.line) && (start.ch >= stop.ch))
        return;
    // initialization
    mText.clear();
    // export all the lines into fBuffer
    mFirstAttribute = true;

    int baseStartLine = 0;
    if (mRecalcLineNumber)
        baseStartLine = mLineNumberStartFromZero?start.line:start.line-1;
    if (mExportLineNumber)
        addData(getStartLineNumberString(start.line-baseStartLine, stop.line-baseStartLine));
    if (start.line == 1)
        mSyntaxer->resetState();
    else
        mSyntaxer->setState(doc->getSyntaxState(start.line-2));
    for (int i = start.line; i<=stop.line; i++) {
        if (mExportLineNumber)
            addData(getLineNumberString(i-baseStartLine));
        QString Line = doc->getLine(i-1);
        // order is important, since Start.Y might be equal to Stop.Y
//        if (i == Stop.Line)
//            Line.remove(Stop.Char-1, INT_MAX);
//        if ( (i = Start.Line) && (Start.Char > 1))
//            Line.remove(0, Start.Char - 1);
        // export the line
        mSyntaxer->setLine(Line, i);
        while (!mSyntaxer->eol()) {
            PTokenAttribute attri = mSyntaxer->getTokenAttribute();
            int startPos = mSyntaxer->getTokenPos();
            QString token = mSyntaxer->getToken();
            if (i==start.line && (startPos+token.length() < start.ch)) {
                mSyntaxer->next();
                continue;
            }
            if (i==stop.line && (startPos >= stop.ch-1)) {
                mSyntaxer->next();
                continue;
            }
            if (i==stop.line && (startPos+token.length() > stop.ch)) {
                token = token.left(stop.ch - startPos - 1);
            }
            if (i==start.line && startPos < start.ch-1) {
                token = token.mid(start.ch-1-startPos);
            }

            QString Token = replaceReservedChars(token);
            if (mOnFormatToken)
                mOnFormatToken(mSyntaxer, i, mSyntaxer->getTokenPos()+1, mSyntaxer->getToken(),attri);
            setTokenAttribute(attri);
            formatToken(Token);
            mSyntaxer->next();
        }
        if (i!=stop.line)
            formatNewLine();
    }
    if (!mFirstAttribute)
        formatAfterLastAttribute();
    if (mExportLineNumber)
        addData(getEndLineNumberString(start.line-baseStartLine, stop.line-baseStartLine));
    // insert header
    insertData(0, getHeader());
    // add footer
    addData(getFooter());
}

void Exporter::saveToFile(const QString &filename)
{
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        writeToStream(file);
    } else {
        throw FileError(QObject::tr("Can't open file '%1' to write!").arg(filename));
    }
}

void Exporter::writeToStream(QIODevice &stream)
{
    TextEncoder encoder = getEncoder();
    auto [ok, encoded] = encoder.encode(mText);
    if (ok && stream.write(encoded) < 0) {
        throw FileError(QObject::tr("Failed to write data."));
    }
}

QFont Exporter::font() const
{
    return mFont;
}

void Exporter::setFont(const QFont &font)
{
    mFont = font;
}

PSyntaxer Exporter::syntaxer() const
{
    return mSyntaxer;
}

void Exporter::setSyntaxer(PSyntaxer value)
{
    if (mSyntaxer != value) {
        mSyntaxer = value;
        clear();
        if ((mSyntaxer) && (mSyntaxer->whitespaceAttribute()) && mUseBackground)
            mBackgroundColor = mSyntaxer->whitespaceAttribute()->background();
    }
}

QString Exporter::title() const
{
    return mTitle;
}

void Exporter::setTitle(const QString &Value)
{
    if (mTitle != Value) {
      if (!Value.isEmpty())
          mTitle = Value;
      else
          mTitle = "Untitled";
    }
}

bool Exporter::useBackground() const
{
    return mUseBackground;
}

void Exporter::setUseBackground(bool Value)
{
    if (mUseBackground != Value) {
        mUseBackground = Value;
        clear();
        if ((mSyntaxer) && (mSyntaxer->whitespaceAttribute()) && mUseBackground)
            mBackgroundColor = mSyntaxer->whitespaceAttribute()->background();
    }
}

NewlineType Exporter::fileEndingType() const
{
    return mFileEndingType;
}

void Exporter::setFileEndingType(const NewlineType &fileEndingType)
{
    mFileEndingType = fileEndingType;
}

QColor Exporter::foregroundColor() const
{
    return mForegroundColor;
}

void Exporter::setForegroundColor(const QColor &value)
{
    if (mForegroundColor != value) {
        mForegroundColor = value;
    }
}

QColor Exporter::backgroundColor() const
{
    return mBackgroundColor;
}

void Exporter::setBackgroundColor(const QColor &value)
{
    if (mBackgroundColor != value) {
        mBackgroundColor = value;
    }
}

QByteArray Exporter::charset() const
{
    return mCharset;
}

void Exporter::setCharset(const QByteArray &charset)
{
    mCharset = charset;
}

QString Exporter::defaultFilter() const
{
    return mDefaultFilter;
}

void Exporter::setDefaultFilter(const QString &defaultFilter)
{
    mDefaultFilter = defaultFilter;
}

void Exporter::addData(const QString &text)
{
    if (!text.isEmpty()) {
        mText.append(text);
    }
}

void Exporter::addDataNewLine(const QString &text)
{
    addData(text);
    addNewLine();
}

void Exporter::addNewLine()
{
    addData(lineBreak());
}

void Exporter::formatToken(const QString &token)
{
    addData(token);
}

int Exporter::getBufferSize() const
{
    return mText.size();
}

TextEncoder Exporter::getEncoder() const {
    TextEncoder encoder(mCharset);
    if (encoder.isValid())
        return encoder;
    else
        return TextEncoder::encoderForSystem();
}

void Exporter::insertData(int pos, const QString &text)
{
    if (!text.isEmpty()) {
        mText.insert(pos,text);
    }
}

QString Exporter::replaceReservedChars(const QString &token)
{
    if (token.isEmpty())
        return "";
    QString result;
    for (QChar ch:token) {
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

void Exporter::setTokenAttribute(PTokenAttribute attri)
{
    if (mFirstAttribute) {
        mFirstAttribute = false;
        mLastBG = ValidatedColor(attri->background(), mBackgroundColor);
        mLastFG = ValidatedColor(attri->foreground(), mForegroundColor);
        mLastStyle = attri->styles();
        formatBeforeFirstAttribute(
                    mUseBackground && (mLastBG != mBackgroundColor),
                    mLastFG != mForegroundColor, attri->styles());
    } else {
        bool ChangedBG = mUseBackground &&
          (mLastBG != ValidatedColor(attri->background(), mBackgroundColor));
        bool ChangedFG = (mLastFG != ValidatedColor(attri->foreground(), mForegroundColor));
        if (ChangedBG || ChangedFG ||  (mLastStyle != attri->styles())) {
            // which font style bits are to reset?
            FontStyles ChangedStyles = mLastStyle & ~(attri->styles());
            formatAttributeDone(ChangedBG, ChangedFG, ChangedStyles);
            // which font style bits are to set?
            ChangedStyles = attri->styles() & ~(mLastStyle);
            mLastBG = ValidatedColor(attri->background(), mBackgroundColor);
            mLastFG = ValidatedColor(attri->foreground(), mForegroundColor);
            mLastStyle = attri->styles();
            formatAttributeInit(ChangedBG, ChangedFG, ChangedStyles);
        }
    }
}

QString Exporter::getStartLineNumberString(int startLine, int endLine)
{
    return QString();
}

QString Exporter::getLineNumberString(int line)
{
    return QString();
}

QString Exporter::getEndLineNumberString(int startLine, int endLine)
{
    return QString();
}

QByteArray Exporter::buffer() const
{
    TextEncoder encoder = getEncoder();
    auto [_, encoded] = encoder.encode(mText);
    return encoded;
}

const QString &Exporter::text() const
{
    return mText;
}

bool Exporter::exportLineNumber() const
{
    return mExportLineNumber;
}

void Exporter::setExportLineNumber(bool newExportLineNumber)
{
    mExportLineNumber = newExportLineNumber;
}

bool Exporter::recalcLineNumber() const
{
    return mRecalcLineNumber;
}

void Exporter::setRecalcLineNumber(bool newRecalcLineNumber)
{
    mRecalcLineNumber = newRecalcLineNumber;
}

bool Exporter::lineNumberStartFromZero() const
{
    return mLineNumberStartFromZero;
}

void Exporter::setLineNumberStartFromZero(bool newLineNumberStartFromZero)
{
    mLineNumberStartFromZero = newLineNumberStartFromZero;
}

QColor Exporter::lineNumberColor() const
{
    return mLineNumberColor;
}

void Exporter::setLineNumberColor(const QColor &newLineNumberColor)
{
    mLineNumberColor = newLineNumberColor;
}

QColor Exporter::lineNumberBackgroundColor() const
{
    return mLineNumberBackgroundColor;
}

void Exporter::setLineNumberBackgroundColor(const QColor &newLineNumberBackgroundColor)
{
    mLineNumberBackgroundColor = newLineNumberBackgroundColor;
}

QByteArray Exporter::clipboardFormat()
{
    return mClipboardFormat;
}

FormatTokenHandler Exporter::onFormatToken() const
{
    return mOnFormatToken;
}

void Exporter::setOnFormatToken(const FormatTokenHandler &onFormatToken)
{
    mOnFormatToken = onFormatToken;
}

QString Exporter::lineBreak()
{
    switch(mFileEndingType) {
    case NewlineType::Unix:
        return "\n";
    case NewlineType::Windows:
        return "\r\n";
    case NewlineType::MacOld:
        return "\r";
    }
    return "\n";
}

}
