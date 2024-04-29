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
#include "document.h"
#include "qt_utils/utils.h"
#include <QDataStream>
#include <QFile>
#include <QTextStream>
#include <QMutexLocker>
#include <stdexcept>
#include <QMessageBox>
#include <cmath>
#include <optional>
#include "qt_utils/charsetinfo.h"
#include <QDateTime>
#include <QDebug>

namespace QSynedit {

Document::Document(const QFont& font, QObject *parent):
    QObject{parent},
    mSetLineWidthLockCount{0},
    mMaxLineChangedInSetLinesWidth{false},
    mMutex{},
    mGlyphCalculator{font}
{
    mAppendNewLineAtEOF = true;
    mNewlineType = NewlineType::Windows;
    mIndexOfLongestLine = -1;
    mUpdateCount = 0;
    mUpdateDocumentLineWidthFunc = std::bind(&GlyphCalculator::calcLineWidth,
        &mGlyphCalculator,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3);
}

static void listIndexOutOfBounds(int index) {
    throw IndexOutOfRange(index);
}



int Document::parenthesisLevel(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        return mLines[line]->syntaxState().parenthesisLevel;
    } else
        return 0;
}

int Document::bracketLevel(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        return mLines[line]->syntaxState().bracketLevel;
    } else
        return 0;
}

int Document::braceLevel(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        return mLines[line]->syntaxState().braceLevel;
    } else
        return 0;
}

int Document::lineWidth(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        int width = mLines[line]->width();
        //updateLongestLineWidth(width);
        return width;
    } else
        return 0;
}

int Document::lineWidth(int line, const QString &newText) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line >= mLines.size())
        return 0;
    QString lineText = mLines[line]->lineText();
    if (lineText==newText) {
        return mLines[line]->width();
    } else {
        return mGlyphCalculator.stringWidth(newText,0);
    }
}

int Document::blockLevel(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        return mLines[line]->syntaxState().blockLevel;
    } else
        return 0;
}

int Document::blockStarted(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        return mLines[line]->syntaxState().blockStarted;
    } else
        return 0;
}

int Document::blockEnded(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        int result = mLines[line]->syntaxState().blockEnded;
//        if (index+1 < mLines.size())
//            result += mLines[index+1]->syntaxState.blockEndedLastLine;
        return result;
    } else
        return 0;
}

int Document::maxLineWidth() const {
    QMutexLocker locker(&mMutex);
    if (mIndexOfLongestLine >= 0) {
        return mLines[mIndexOfLongestLine]->width();
    } else if (mLines.isEmpty()) {
        return 0;
    } else
        return -1;
}

QString Document::lineBreak() const
{
    switch(mNewlineType) {
    case NewlineType::Unix:
        return "\n";
    case NewlineType::Windows:
        return "\r\n";
    case NewlineType::MacOld:
        return "\r";
    }
    return "\n";
}

SyntaxState Document::getSyntaxState(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        return mLines[line]->syntaxState();
    } else {
         listIndexOutOfBounds(line);
    }
    return SyntaxState();
}

void Document::insertItem(int line, const QString &s)
{
    beginUpdate();
    PDocumentLine documentLine = std::make_shared<DocumentLine>(
                mUpdateDocumentLineWidthFunc);
    documentLine->setLineText(s);
    mLines.insert(line,documentLine);
    mIndexOfLongestLine = -1;
    endUpdate();
}

void Document::addItem(const QString &s)
{
    beginUpdate();
    PDocumentLine line = std::make_shared<DocumentLine>(mUpdateDocumentLineWidthFunc);
    line->setLineText(s);
    mLines.append(line);
    endUpdate();
}

bool Document::getAppendNewLineAtEOF() const
{
    QMutexLocker locker(&mMutex);
    return mAppendNewLineAtEOF;
}

void Document::setAppendNewLineAtEOF(bool appendNewLineAtEOF)
{
    QMutexLocker locker(&mMutex);
    mAppendNewLineAtEOF = appendNewLineAtEOF;
}

void Document::setSyntaxState(int line, const SyntaxState& state)
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=mLines.count()) {
        listIndexOutOfBounds(line);
    }
    mLines[line]->setSyntaxState(state);
}

QString Document::getLine(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=mLines.count()) {
        return QString();
    }
    return mLines[line]->lineText();
}

int Document::getLineGlyphsCount(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=mLines.count()) {
        return 0;
    }
    return mLines[line]->glyphsCount();
}

// QList<int> Document::getGlyphPositions(int index)
// {
//     QMutexLocker locker(&mMutex);
//     if (index<0 || index>=mLines.count()) {
//         return QList<int>{};
//     }
//     return mLines[index]->glyphStartCharList();
// }

int Document::count() const
{
    QMutexLocker locker(&mMutex);
    return mLines.count();
}

QString Document::text() const
{
    QMutexLocker locker(&mMutex);
    return getTextStr();
}

void Document::setText(const QString &text)
{
    QMutexLocker locker(&mMutex);
    putTextStr(text);
}

void Document::setContents(const QStringList &text)
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    internalClear();
    if (text.count() > 0) {
        int FirstAdded = mLines.count();

        foreach (const QString& s,text) {
            addItem(s);
        }
        mIndexOfLongestLine = -1;
        emit inserted(FirstAdded,text.count());
    }
}

QStringList Document::contents() const
{
    QMutexLocker locker(&mMutex);
    QStringList result;
    DocumentLines list = mLines;
    foreach (const PDocumentLine& line, list) {
        result.append(line->lineText());
    }
    return result;
}

void Document::beginUpdate()
{
    if (mUpdateCount == 0) {
        setUpdateState(true);
        beginSetLinesWidth();
    }
    mUpdateCount++;
}

void Document::endUpdate()
{
    mUpdateCount--;
    if (mUpdateCount == 0) {
        setUpdateState(false);
        endSetLinesWidth();
    }
}


int Document::addLine(const QString &s)
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    int Result = mLines.count();
    insertItem(Result, s);
    emit inserted(Result,1);
    endUpdate();
    return Result;
}

void Document::addLines(const QStringList &strings)
{
    QMutexLocker locker(&mMutex);
    if (strings.count() > 0) {
        beginUpdate();
        auto action = finally([this]{
            endUpdate();
        });
        int FirstAdded = mLines.count();
        for (const QString& s:strings) {
            addItem(s);
        }
        mIndexOfLongestLine = -1;
        emit inserted(FirstAdded,strings.count());
    }
}

int Document::getTextLength() const
{
    QMutexLocker locker(&mMutex);
    int Result = 0;
    foreach (const PDocumentLine& line, mLines ) {
        Result += line->glyphsCount();
        if (mNewlineType == NewlineType::Windows) {
            Result += 2;
        } else {
            Result += 1;
        }
    }
    return Result;
}

void Document::clear()
{
    QMutexLocker locker(&mMutex);
    internalClear();
}

void Document::deleteLines(int index, int numLines)
{
    QMutexLocker locker(&mMutex);
    if (numLines<=0)
        return;
    if ((index < 0) || (index >= mLines.count())) {
        listIndexOutOfBounds(index);
    }
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    if (mIndexOfLongestLine>=index) {
        if (mIndexOfLongestLine <index+numLines) {
            mIndexOfLongestLine = -1;
        } else {
            mIndexOfLongestLine -= numLines;
        }
    }
    int LinesAfter = mLines.count() - (index + numLines);
    if (LinesAfter < 0) {
       numLines = mLines.count() - index;
    }
    mLines.remove(index,numLines);
    emit deleted(index,numLines);
}

void Document::exchange(int index1, int index2)
{
    QMutexLocker locker(&mMutex);
    if ((index1 < 0) || (index1 >= mLines.count())) {
        listIndexOutOfBounds(index1);
    }
    if ((index2 < 0) || (index2 >= mLines.count())) {
        listIndexOutOfBounds(index2);
    }
    beginUpdate();
    PDocumentLine temp = mLines[index1];
    mLines[index1]=mLines[index2];
    mLines[index2]=temp;
    //mList.swapItemsAt(Index1,Index2);
    if (mIndexOfLongestLine == index1) {
        mIndexOfLongestLine = index2;
    } else if (mIndexOfLongestLine == index2) {
        mIndexOfLongestLine = index1;
    }
    endUpdate();
}

void Document::insertLine(int index, const QString &s)
{
    QMutexLocker locker(&mMutex);
    if ((index < 0) || (index > mLines.count())) {
        listIndexOutOfBounds(index);
    }
    beginUpdate();
    insertItem(index, s);
    emit inserted(index,1);
    endUpdate();
}

void Document::deleteAt(int index)
{
    QMutexLocker locker(&mMutex);
    if ((index < 0) || (index >= mLines.count())) {
        listIndexOutOfBounds(index);
    }
    beginUpdate();
    if (mIndexOfLongestLine == index)
        mIndexOfLongestLine = -1;
    else if (mIndexOfLongestLine>index)
        mIndexOfLongestLine -= 1;
    mLines.removeAt(index);
    emit deleted(index,1);
    endUpdate();
}

QString Document::getTextStr() const
{
    QString result;
    for (int i=0;i<mLines.count()-1;i++) {
        const PDocumentLine& line = mLines[i];
        result.append(line->lineText());
        result.append(lineBreak());
    }
    if (mLines.length()>0) {
        result.append(mLines.back()->lineText());
    }
    return result;
}

void Document::putLine(int index, const QString &s, bool notify) {
    QMutexLocker locker(&mMutex);
    if (index == mLines.count()) {
        addLine(s);
    } else {
        if (index<0 || index>=mLines.count()) {
            listIndexOutOfBounds(index);
        }
        if (notify)
            beginUpdate();
        mLines[index]->setLineText(s);
        if (mIndexOfLongestLine == index) {
            // width is invalidated, so we must recalculate longest line
            mIndexOfLongestLine = -1;
        }
        if (notify)
            emit putted(index);
        if (notify)
            endUpdate();
    }
}

void Document::setUpdateState(bool Updating)
{
    if (Updating)
        emit changing();
    else
        emit changed();
}

void Document::insertLines(int index, int numLines)
{
    QMutexLocker locker(&mMutex);
    if (index<0 || index>mLines.count()) {
        listIndexOutOfBounds(index);
    }
    if (numLines<=0)
        return;
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    PDocumentLine line;
    mLines.insert(index,numLines,line);
    for (int i=index;i<index+numLines;i++) {
        line = std::make_shared<DocumentLine>(mUpdateDocumentLineWidthFunc);
        mLines[i]=line;
    }
    mIndexOfLongestLine = -1;
    emit inserted(index,numLines);
}


bool Document::tryLoadFileByEncoding(QByteArray encodingName, QFile& file) {
    TextDecoder decoder(encodingName);
    if (!decoder.isValid())
        return false;
    file.reset();
    internalClear();
    while (true) {
        if (file.atEnd()){
            break;
        }
        QByteArray line = file.readLine();
        if (line.endsWith("\r\n")) {
            line.remove(line.length()-2,2);
        } else if (line.endsWith("\r")) {
            line.remove(line.length()-1,1);
        } else if (line.endsWith("\n")){
            line.remove(line.length()-1,1);
        }
        auto [ok, newLine] = decoder.decode(line);
        if (!ok) {
            return false;
        }
        addItem(newLine);
    }
    return true;
}

void Document::loadUTF16BOMFile(QFile &file)
{
    TextDecoder decoder = TextDecoder::decoderForUtf16();
    if (!decoder.isValid())
        return;
    file.reset();
    internalClear();
    QByteArray buf = file.readAll();
    if (buf.length()<2)
        return;
    QString text = decoder.decodeUnchecked(buf.mid(2));
    this->setText(text);
}

void Document::loadUTF32BOMFile(QFile &file)
{
    TextDecoder decoder = TextDecoder::decoderForUtf32();
    if (!decoder.isValid())
        return;
    file.reset();
    internalClear();
    QByteArray buf = file.readAll();
    if (buf.length()<4)
        return;
    QString text = decoder.decodeUnchecked(buf.mid(4));
    this->setText(text);
}

void Document::saveUTF16File(QFile &file, TextEncoder &encoder)
{
    if (!encoder.isValid())
        return;
    QString text=getTextStr();
    file.write(encoder.encodeUnchecked(text));
}

void Document::saveUTF32File(QFile &file, TextEncoder &encoder)
{
    if (!encoder.isValid())
        return;
    QString text=getTextStr();
    file.write(encoder.encodeUnchecked(text));
}

void Document::setTabSize(int newTabSize)
{
    if (tabSize()!=newTabSize) {
        mGlyphCalculator.setTabSize(newTabSize);
        invalidateAllLineWidth();
    }
}

void Document::setForceMonospace(bool newForceMonospace)
{
    int oldValue = forceMonospace();
    mGlyphCalculator.setForceMonospace(newForceMonospace);
    if (oldValue != newForceMonospace)
        invalidateAllLineWidth();
}

void Document::loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding)
{
    QMutexLocker locker(&mMutex);
    QFile file(filename);
    if (!file.open(QFile::ReadOnly))
        throw FileError(tr("Can't open file '%1' for read!").arg(file.fileName()));
    beginUpdate();
    internalClear();
    auto action = finally([this]{
        if (mLines.count()>0)
            emit inserted(0,mLines.count());
        endUpdate();
    });
    //test for utf8 / utf 8 bom
    if (encoding == ENCODING_AUTO_DETECT) {
        if (file.atEnd()) {
            realEncoding = ENCODING_ASCII;
            return;
        }
        QByteArray line = file.readLine();
        std::optional<TextDecoder> decoder;
        bool needReread = false;
        bool allAscii = true;
        //test for BOM
        if ((line.length()>=3) && ((unsigned char)line[0]==0xEF) && ((unsigned char)line[1]==0xBB) && ((unsigned char)line[2]==0xBF) ) {
            realEncoding = ENCODING_UTF8_BOM;
            line = line.mid(3);
            decoder = TextDecoder::decoderForUtf8();
        } else if ((line.length()>=4) && ((unsigned char)line[0]==0xFF) && ((unsigned char)line[1]==0xFE)
                   && ((unsigned char)line[2]==0x00)
                   && ((unsigned char)line[3]==0x00)) {
            realEncoding = ENCODING_UTF32_BOM;
            loadUTF32BOMFile(file);
            return;
        } else if ((line.length()>=2) && ((unsigned char)line[0]==0xFF) && ((unsigned char)line[1]==0xFE)) {
            realEncoding = ENCODING_UTF16_BOM;
            loadUTF16BOMFile(file);
            return;
        } else {
            realEncoding = ENCODING_UTF8;
            decoder = TextDecoder::decoderForUtf8();
        }
        if (!decoder.has_value())
            throw FileError(tr("Can't load codec '%1'!").arg(QString(realEncoding)));
        if (line.endsWith("\r\n")) {
            mNewlineType = NewlineType::Windows;
        } else if (line.endsWith("\n")) {
            mNewlineType = NewlineType::Unix;
        } else if (line.endsWith("\r")) {
            mNewlineType = NewlineType::MacOld;
        }

        internalClear();
        while (true) {
            if (line.endsWith("\r\n")) {
                line.remove(line.length()-2,2);
            } else if (line.endsWith("\r")) {
                line.remove(line.length()-1,1);
            } else if (line.endsWith("\n")){
                line.remove(line.length()-1,1);
            }
            if (isBinaryContent(line))
                throw BinaryFileError(tr("'%1' is a binaray File!").arg(filename));
            if (allAscii) {
                allAscii = isTextAllAscii(line);
            }
            if (allAscii) {
                addItem(QString::fromLatin1(line));
            } else {
                auto [ok, newLine] = decoder->decode(line);
                if (!ok) {
                    needReread = true;
                    break;
                }
                addItem(newLine);
            }
            if (file.atEnd()){
                break;
            }
            line = file.readLine();
        }
        if (!needReread) {
            if (allAscii)
                realEncoding = ENCODING_ASCII;
            return;
        }
        realEncoding = pCharsetInfoManager->getDefaultSystemEncoding();
        if (tryLoadFileByEncoding(realEncoding,file)) {
            return;
        }
        QList<PCharsetInfo> charsets = pCharsetInfoManager->findCharsetByLocale(pCharsetInfoManager->localeName());
        if (!charsets.isEmpty()) {

            QSet<QByteArray> encodingSet;
            for (int i=0;i<charsets.size();i++) {
                encodingSet.insert(charsets[i]->name);
            }
            encodingSet.remove(realEncoding);
            foreach (const QByteArray& encodingName,encodingSet) {
                if (encodingName == ENCODING_UTF8)
                    continue;
                if (tryLoadFileByEncoding(encodingName,file)) {
                    //qDebug()<<encodingName;
                    realEncoding = encodingName;
                    return;
                }
            }
        }
    } else {
        realEncoding = encoding;
    }

    if (realEncoding == ENCODING_SYSTEM_DEFAULT) {
        realEncoding = pCharsetInfoManager->getDefaultSystemEncoding();
    }
    file.reset();
    QByteArray data = file.readAll();
    QString text;
    QTextStream textStream(&text);
    if (realEncoding == ENCODING_UTF8_BOM) {
        textStream.setAutoDetectUnicode(true);
        text = QString::fromUtf8(data);
    } else {
        textStream.setAutoDetectUnicode(false);
        TextDecoder decoder(realEncoding);
        text = decoder.decodeUnchecked(data);
    }
    QString line;
    internalClear();
    while (textStream.readLineInto(&line)) {
        if (line.endsWith("\r\n")) {
            line.remove(line.length()-2,2);
        } else if (line.endsWith("\r")) {
            line.remove(line.length()-1,1);
        } else if (line.endsWith("\n")){
            line.remove(line.length()-1,1);
        }
        addItem(line);
    }
}



void Document::saveToFile(QFile &file, const QByteArray& encoding,
                                   const QByteArray& defaultEncoding, QByteArray& realEncoding)
{
    QMutexLocker locker(&mMutex);
    std::optional<TextEncoder> encoder;
    realEncoding = encoding;
    QString codecName = realEncoding;
    if (realEncoding == ENCODING_UTF16_BOM || realEncoding == ENCODING_UTF16) {
        encoder = TextEncoder::encoderForUtf16();
        codecName = ENCODING_UTF16;
    } else if (realEncoding == ENCODING_UTF32_BOM || realEncoding == ENCODING_UTF32) {
        encoder = TextEncoder::encoderForUtf32();
        codecName = ENCODING_UTF32;
    } else if (realEncoding == ENCODING_UTF8_BOM) {
        encoder = TextEncoder::encoderForUtf8();
        codecName = ENCODING_UTF8;
    } else if (realEncoding == ENCODING_SYSTEM_DEFAULT) {
        encoder = TextEncoder::encoderForSystem();
        codecName = realEncoding;
    } else if (realEncoding == ENCODING_AUTO_DETECT) {
        encoder = TextEncoder(defaultEncoding);
        codecName = defaultEncoding;
    } else {
        encoder = TextEncoder(realEncoding);
    }
    if (!encoder.has_value() || !encoder->isValid())
        throw FileError(tr("Can't load codec '%1'!").arg(codecName));

    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        throw FileError(tr("Can't open file '%1' for save!").arg(file.fileName()));
    if (mLines.isEmpty())
        return;
    if (realEncoding == ENCODING_UTF16) {
        saveUTF16File(file, encoder.value());
        return;
    } else if (realEncoding == ENCODING_UTF32) {
        saveUTF32File(file, encoder.value());
        return;
    } if (realEncoding == ENCODING_UTF8_BOM) {
        file.putChar(0xEF);
        file.putChar(0xBB);
        file.putChar(0xBF);
    }
    bool allAscii = true;
    QByteArray data;
    for (PDocumentLine& line:mLines) {
        QString text = line->lineText()+lineBreak();
        data = encoder->encodeUnchecked(text);
        if (allAscii) {
            allAscii = (data==text.toLatin1());
        }
        if (file.write(data)!=data.size())
            throw FileError(tr("Data not correctly writed to file '%1'.").arg(file.fileName()));
    }
    if (allAscii) {
        realEncoding = ENCODING_ASCII;
    } else if (realEncoding == ENCODING_SYSTEM_DEFAULT) {
        if (encoder->name().compare("System",Qt::CaseInsensitive)==0) {
            realEncoding = pCharsetInfoManager->getDefaultSystemEncoding();
        } else {
            realEncoding = encoder->name();
        }
    }
}

QString Document::glyph(int line, int glyphIdx) const
{
    QMutexLocker locker(&mMutex);
    return mLines[line]->glyph(glyphIdx);
}

QString Document::glyphAt(int line, int charPos) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return QString();
    QList<int> glyphStartCharList = mLines[line]->glyphStartCharList();
    int glyphIdx = charToGlyphIndex(mLines[line]->lineText(), glyphStartCharList, charPos);
    return mLines[line]->glyph(glyphIdx);
}

int Document::charToGlyphStartChar(int line, int charPos) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    QList<int> glyphStartCharList = mLines[line]->glyphStartCharList();
    int glyphIdx = charToGlyphIndex(mLines[line]->lineText(), glyphStartCharList, charPos);
    return mLines[line]->glyphStartChar(glyphIdx);
}

// int Document::columnToGlyphStartColumn(int line, int charPos)
// {
//     QMutexLocker locker(&mMutex);
//     QList<int> glyphColumnsList = mLines[line]->glyphColumnsList();
//     int glyphIdx = columnToGlyphIndex(glyphColumnsList, charPos);
//     return mLines[line]->glyphStartColumn(glyphIdx);
// }

QList<int> calcGlyphStartCharList(const QString &text)
{
    QList<int> glyphStartCharList;
    //parse mGlyphs
    int i=0;
    bool consecutive = false;
    while (i<text.length()) {
        QChar ch = text[i];
        if (ch.isHighSurrogate() && i+1<text.length() && QChar::isLowSurrogate(text[i+1].unicode())) {
            //character that larger than 0xffff
            uint ucs4 = QChar::surrogateToUcs4(ch, text[i+1]);
            if (QChar::combiningClass(ucs4)!=0 && !glyphStartCharList.isEmpty()) {
                //a Combining character
            } else if (ucs4>=0xE0100 && ucs4 <= 0xE01EF) {
                //variation selector
            } else if (ucs4>=0x1F3FB && ucs4 <= 0x1F3FF) {
                //Emoji modifiers for skin tone
            } else if (!consecutive) {
                glyphStartCharList.append(i);
                if (ucs4>=0x1F1E6 && ucs4<=0x1F1FF) // National Flags emoji
                    consecutive = true;
                else
                    consecutive = false;
            } else {
                consecutive = false;
            }
            i+=2;
            continue;
        } else if (ch.unicode() == 0x200D || ch.unicode() == 0x200C ) {
            //ZWJ && ZWNJ
            consecutive = true;
        } else if (ch.combiningClass()!=0 && !glyphStartCharList.isEmpty()) {
            //a Combining character
        } else if (ch.unicode()>=0xFE00 && ch.unicode()<=0xFE0F) {
            //variation selector
        } else {
            //qDebug("%x %d", ch.unicode(), ch.combiningClass());
            if (!consecutive)
                glyphStartCharList.append(i);
            consecutive = false;
        }
        i++;
    }
    //qDebug()<<text<<glyphStartCharList;
    // if (text=="...}")
    //     qDebug()<<"where it comes?";
    return glyphStartCharList;
}

int GlyphCalculator::stringWidth(const QString &str, int left) const
{
    QList<int> glyphStartCharList = calcGlyphStartCharList(str);
    int right;
    calcGlyphPositionList(str, glyphStartCharList, left, right);
    return right - left;
}

int GlyphCalculator::stringWidth(const QString &str, int left, const QFontMetrics &fontMetrics)
{
    QList<int> glyphStartCharList = calcGlyphStartCharList(str);
    int right;
    calcGlyphPositionList(str, glyphStartCharList, fontMetrics, left, right);
    return right - left;
}

int Document::glyphCount(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    return mLines[line]->glyphsCount();
}

int Document::glyphStartChar(int line, int glyphIdx) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    return mLines[line]->glyphStartChar(glyphIdx);
}

int Document::glyphLength(int line, int glyphIdx) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    return mLines[line]->glyphLength(glyphIdx);
}

int Document::glyphStartPostion(int line, int glyphIdx) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    return mLines[line]->glyphStartPosition(glyphIdx);
}

int Document::glyphWidth(int line, int glyphIdx) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    return mLines[line]->glyphWidth(glyphIdx);
}

int Document::charToGlyphIndex(int line, int charIdx) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    QList<int> glyphStartCharList = mLines[line]->glyphStartCharList();
    return charToGlyphIndex(mLines[line]->lineText(), glyphStartCharList, charIdx);
}

int Document::charToGlyphIndex(const QString& str, QList<int> glyphStartCharList, int charIdx) const
{
    Q_ASSERT(charIdx>=0);
    return searchForSegmentIdx(glyphStartCharList, 0, str.length(), charIdx);
    // if (charIdx>=str.length())
    //     return glyphStartCharList.length();
    // for (int i=0;i<glyphStartCharList.length();i++) {
    //     if (glyphStartCharList[i]>charIdx) {
    //         Q_ASSERT(i-1>=0);
    //         return i-1;
    //     }
    // }
    // Q_ASSERT(glyphStartCharList.length()-1>=0);
    // return glyphStartCharList.length()-1;
}

QList<int> GlyphCalculator::calcGlyphPositionList(const QString &lineText, const QList<int> &glyphStartCharList, const QFontMetrics &fontMetrics, int left, int &right) const
{
    right = std::max(0,left);
    int start,end;
    QList<int> glyphPostionList;
    for (int i=0;i<glyphStartCharList.length();i++) {
        start = glyphStartCharList[i];
        if (i+1<glyphStartCharList.length()) {
            end = glyphStartCharList[i+1];
        } else {
            end = lineText.length();
        }
        QString glyph = lineText.mid(start,end-start);
        int gWidth = glyphWidth(glyph, right, fontMetrics, mForceMonospace);
        glyphPostionList.append(right);
        right += gWidth;
    }
    return glyphPostionList;
}

int Document::xposToGlyphIndex(int line, int xpos) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    QList<int> glyphPositionList = mLines[line]->glyphStartPositionList();
    return xposToGlyphIndex(mLines[line]->width(), glyphPositionList, xpos);
}

int Document::xposToGlyphIndex(int strWidth, QList<int> glyphPositionList, int xpos) const
{
    return searchForSegmentIdx(glyphPositionList,0,strWidth,xpos);
    // if (xpos>=strWidth)
    //     return glyphPositionList.length();
    // for (int i=0;i<glyphPositionList.length();i++) {
    //     if (glyphPositionList[i]>xpos) {
    //         return i-1;
    //     }
    // }
    // return glyphPositionList.length()-1;
}

int Document::charToGlyphStartPosition(int line, int charPos) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    QList<int> glyphStartCharList = mLines[line]->glyphStartCharList();
    int glyphIdx = charToGlyphIndex(mLines[line]->lineText(), glyphStartCharList, charPos);
    return mLines[line]->glyphStartPosition(glyphIdx);
}

int Document::xposToGlyphStartChar(int line, int xpos) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    QList<int> glyphPositionList = mLines[line]->glyphStartPositionList();
    int glyphIdx = xposToGlyphIndex(mLines[line]->width(), glyphPositionList, xpos);
    return mLines[line]->glyphStartChar(glyphIdx);
}

int Document::charToGlyphStartPosition(int line, const QString newStr, int charPos) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line<count() && mLines[line]->lineText() == newStr) {
        return charToGlyphStartPosition(line,charPos);
    } else {
        QList<int> glyphStartCharList = calcGlyphStartCharList(newStr);
        int glyphIdx = charToGlyphIndex(newStr, glyphStartCharList, charPos);
        int width;
        QList<int> glyphStartPositionList = mGlyphCalculator.calcGlyphPositionList(newStr, width);
        if (glyphIdx<glyphStartCharList.length())
            return glyphStartPositionList[glyphIdx];
        else
            return width;
    }
}

int Document::xposToGlyphStartChar(int line, const QString newStr, int xpos) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    QList<int> glyphPositionList;
    int width;
    if (mLines[line]->lineText() == newStr) {
        glyphPositionList = mLines[line]->glyphStartPositionList();
        width = mLines[line]->width();
    } else {
        glyphPositionList = mGlyphCalculator.calcGlyphPositionList(mLines[line]->lineText(), width);
    }
    int glyphIdx = xposToGlyphIndex(width, glyphPositionList, xpos);
    return mLines[line]->glyphStartChar(glyphIdx);
}

// int Document::charToColumn(int line, int charPos)
// {
//     QMutexLocker locker(&mMutex);
//     QList<int> glyphPositions = mLines[line]->glyphPositions();

// }

// int Document::charToColumn(const QString &lineText, int charPos) const
// {
//     QMutexLocker locker(&mMutex);
//     QList<int> glyphPositions = calcGlyphPositions(lineText);
//     return charToColumn(lineText, glyphPositions, charPos);
// }

// int Document::charToColumn(const QString &lineText, const QList<int> &glyphPositions, int charPos) const
// {
//     Q_ASSERT(charPos<lineText.length() && charPos>=0);
//     int glyphIdx;
//     for (glyphIdx=0;glyphIdx<glyphPositions.length();glyphIdx++) {
//         if (charPos<glyphPositions[glyphIdx])
//             break;
//     }
//     int cols=0;
// }

// int Document::columnToChar(const QString &lineText, int column) const
// {
//     QList<int> glyphPositions = calcGlyphPositions(lineText);
//     return columnToChar(lineText, glyphPositions, column);
// }

// int Document::columnToChar(const QString &lineText, const QList<int> &glyphPositions, int column) const
// {

// }

void Document::putTextStr(const QString &text)
{
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    internalClear();
    int pos = 0;
    int start;
    while (pos < text.length()) {
        start = pos;
        while (pos<text.length()) {
            if (text[pos] == '\r' || text[pos] == '\n') {
                break;
            }
            pos++;
        }
        addLine(text.mid(start,pos-start));
        if (pos>=text.length())
            break;
        if (text[pos] == '\r')
            pos++;
        if (text[pos] == '\n')
            pos++;
    }
}

void Document::internalClear()
{
    if (!mLines.isEmpty()) {
        beginUpdate();
        int oldCount = mLines.count();
        mLines.clear();
        mIndexOfLongestLine = -1;
        emit deleted(0,oldCount);
        endUpdate();
    }
}

bool Document::lineWidthValid(int line)
{
    if (line<0 || line>=mLines.count())
        return false;
    return mLines[line]->mWidth>=0 && !mLines[line]->mIsTempWidth;
}

void Document::beginSetLinesWidth()
{
    if (mSetLineWidthLockCount == 0) {
        mMaxLineChangedInSetLinesWidth = false;
    }
    mSetLineWidthLockCount++;
}

void Document::endSetLinesWidth()
{
    mSetLineWidthLockCount--;
    if (mSetLineWidthLockCount == 0) {
        if (mMaxLineChangedInSetLinesWidth)
            updateMaxLineWidthAndNotify();
    }
}

void Document::setLineWidth(int line, int newWidth, const QList<int> glyphStartPositionList)
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return ;
    int oldWidth = mLines[line]->mWidth;
    // qDebug()<<line<<oldWidth<<newWidth;
    mLines[line]->mWidth = newWidth;
    mLines[line]->mIsTempWidth = false;
    mLines[line]->mGlyphStartPositionList = glyphStartPositionList;
    if (mIndexOfLongestLine<0) {
        mIndexOfLongestLine = line;
        updateMaxLineWidthChanged();
    } else if (mIndexOfLongestLine == line) {
        if (oldWidth > newWidth) {
            mIndexOfLongestLine = -1;
            updateMaxLineWidthChanged();
        } else if (oldWidth < newWidth) {
            updateMaxLineWidthChanged();
        }
    } else if (mLines[mIndexOfLongestLine]->mWidth < newWidth) {
        mIndexOfLongestLine = line;
        updateMaxLineWidthChanged();
    }
    Q_ASSERT(mLines[line]->mGlyphStartPositionList.length() == mLines[line]->mGlyphStartCharList.length());
}

void Document::updateMaxLineWidthChanged()
{
    if (mSetLineWidthLockCount>0) {
        mMaxLineChangedInSetLinesWidth = true;
    } else {
        //updateMaxLineWidthAndNotify();
        emit maxLineWidthChanged();
    }
}

void Document::updateMaxLineWidthAndNotify()
{
    int MaxLen = -1;
    mIndexOfLongestLine = -1;
    if (mLines.count() > 0 ) {
        for (int i=0;i<mLines.size();i++) {
            int len = mLines[i]->mWidth;
            if (len > MaxLen) {
                MaxLen = len;
                mIndexOfLongestLine = i;
            }
        }
    }
    if (mIndexOfLongestLine>=0)
        emit maxLineWidthChanged();
}

QList<int> GlyphCalculator::calcGlyphPositionList(const QString &lineText, int &width) const
{
    QList<int> glyphStartCharList = calcGlyphStartCharList(lineText);
    return calcGlyphPositionList(lineText,glyphStartCharList,0,width);
}

QList<int> Document::getGlyphStartCharList(int line, const QString &lineText)
{
    if (line<0 || line>=count() || mLines[line]->lineText()!=lineText)
        return calcGlyphStartCharList(lineText);
    return mLines[line]->glyphStartCharList();
}

QList<int> Document::getGlyphStartCharList(int line)
{
    if (line<0 || line>=count())
        return QList<int>();
    return mLines[line]->glyphStartCharList();
}

QList<int> Document::getGlyphStartPositionList(int line)
{
    return mLines[line]->glyphStartPositionList();
}

int Document::getLineWidth(int line)
{
    return mLines[line]->mWidth;
}

NewlineType Document::getNewlineType() const
{
    QMutexLocker locker(&mMutex);
    return mNewlineType;
}

void Document::setNewlineType(const NewlineType &fileEndingType)
{
    QMutexLocker locker(&mMutex);
    mNewlineType = fileEndingType;
}

bool Document::empty() const
{
    QMutexLocker locker(&mMutex);
    return mLines.count()==0;
}

void Document::invalidateAllLineWidth()
{
    QMutexLocker locker(&mMutex);
    for (PDocumentLine& line:mLines) {
        line->invalidateWidth();
    }
    mIndexOfLongestLine = -1;
}

void Document::invalidateAllNonTempLineWidth()
{
    QMutexLocker locker(&mMutex);
    for (PDocumentLine& line:mLines) {
        if (!line->mIsTempWidth)
            line->mIsTempWidth;
    }
}

DocumentLine::DocumentLine(DocumentLine::UpdateWidthFunc updateWidthFunc):
    mSyntaxState{},
    mWidth{-1},
    mIsTempWidth{true},
    mUpdateWidthFunc{updateWidthFunc}
{
}

int DocumentLine::glyphLength(int i) const
{
    return calcSegmentInterval(mGlyphStartCharList, mLineText.length(), i);
}

QString DocumentLine::glyph(int i) const
{
   if (i<0 || i>=mGlyphStartCharList.length())
       return QString();
   return mLineText.mid(glyphStartChar(i),glyphLength(i));
}

int DocumentLine::glyphStartPosition(int i)
{
    if (i<0)
       return 0;
    if (mWidth<0)
        updateWidth();
    if (i>=mGlyphStartPositionList.length())
       return mWidth;
    return mGlyphStartPositionList[i];
}

int DocumentLine::glyphWidth(int i)
{
    if (mWidth <0)
        updateWidth();
    return calcSegmentInterval(mGlyphStartPositionList, mWidth, i);
}

int DocumentLine::width()
{
    if(mWidth<0)
        updateWidth();
    return mWidth;
}

void DocumentLine::setLineText(const QString &newLineText)
{
    mLineText = newLineText;
    mGlyphStartCharList = calcGlyphStartCharList(newLineText);
    invalidateWidth();
}

void DocumentLine::updateWidth()
{
    Q_ASSERT(mUpdateWidthFunc!=nullptr);
    mGlyphStartPositionList = mUpdateWidthFunc(mLineText, mGlyphStartCharList, mWidth);
//    qDebug()<<"Update Width"<<mLineText<<mWidth<<mGlyphPositionList;
}

const QList<int> &DocumentLine::glyphStartPositionList()
{
    if(mWidth<0)
        updateWidth();
    return mGlyphStartPositionList;
}

int DocumentLine::glyphStartChar(int i) const
{
   if (i<0)
       return 0;
   if (i>=mGlyphStartCharList.length())
       return mLineText.length();
   return mGlyphStartCharList[i];
}

UndoList::UndoList():QObject()
{
    mNextChangeNumber = 1;
    mInsideRedo = false;

    mBlockChangeNumber=0;
    mBlockLock=0;
    mFullUndoImposible=false;
    mLastPoppedItemChangeNumber=0;
    mInitialChangeNumber = 0;
    mLastRestoredItemChangeNumber=0;
}

void UndoList::addChange(ChangeReason reason, const BufferCoord &startPos,
                                const BufferCoord &endPos, const QStringList& changeText,
                                SelectionMode selMode)
{
    int changeNumber;
    if (inBlock()) {
        changeNumber = mBlockChangeNumber;
    } else {
        changeNumber = getNextChangeNumber();
    }
    PUndoItem  newItem = std::make_shared<UndoItem>(
                reason,
                selMode,startPos,endPos,changeText,
                changeNumber);
    mItems.append(newItem);

    if (reason!=ChangeReason::GroupBreak && !inBlock()) {
        emit addedUndo();
    }
}

void UndoList::restoreChange(ChangeReason AReason, const BufferCoord &AStart, const BufferCoord &AEnd, const QStringList &ChangeText, SelectionMode SelMode, size_t changeNumber)
{
    PUndoItem  newItem = std::make_shared<UndoItem>(AReason,
                                                                  SelMode,AStart,AEnd,ChangeText,
                                                                  changeNumber);
    restoreChange(newItem);
}

void UndoList::restoreChange(PUndoItem item)
{
    size_t changeNumber = item->changeNumber();
    mItems.append(item);
    if (changeNumber>mNextChangeNumber)
        mNextChangeNumber=changeNumber;
    if (changeNumber!=mLastRestoredItemChangeNumber) {
        emit addedUndo();
    }
    mLastRestoredItemChangeNumber=changeNumber;
}

void UndoList::addGroupBreak()
{
    if (!canUndo())
        return;

    if (lastChangeReason() != ChangeReason::GroupBreak) {
        addChange(ChangeReason::GroupBreak, {0,0}, {0,0}, QStringList(), SelectionMode::Normal);
    }
}

void UndoList::beginBlock()
{
//    qDebug()<<"begin block";
    if (mBlockLock==0)
        mBlockChangeNumber = getNextChangeNumber();
    mBlockLock++;

}

void UndoList::clear()
{
    mItems.clear();
    mFullUndoImposible = false;
    mInitialChangeNumber=0;
    mLastPoppedItemChangeNumber=0;
    mLastRestoredItemChangeNumber=0;
    mBlockLock=0;
}

void UndoList::endBlock()
{
//    qDebug()<<"end block";
    if (mBlockLock > 0) {
        mBlockLock--;
        if (mBlockLock == 0)  {
            size_t iBlockID = mBlockChangeNumber;
            mBlockChangeNumber = 0;
            if (mItems.count() > 0 && peekItem()->changeNumber() == iBlockID) {
                emit addedUndo();
            }
        }
    }
}

bool UndoList::inBlock()
{
    return mBlockLock>0;
}

unsigned int UndoList::getNextChangeNumber()
{
    return mNextChangeNumber++;
}

ChangeReason UndoList::lastChangeReason()
{
    if (mItems.count() == 0)
        return ChangeReason::Nothing;
    else
        return mItems.last()->changeReason();
}

bool UndoList::isEmpty()
{
    return mItems.count()==0;
}

PUndoItem UndoList::peekItem()
{
    if (mItems.count() == 0)
        return PUndoItem();
    else
        return mItems.last();
}

PUndoItem UndoList::popItem()
{
    if (mItems.count() == 0)
        return PUndoItem();
    else {
        PUndoItem item = mItems.last();
//        qDebug()<<"popped"<<item->changeNumber()<<item->changeText()<<(int)item->changeReason()<<mLastPoppedItemChangeNumber;
        mLastPoppedItemChangeNumber =  item->changeNumber();
        mItems.removeLast();
        return item;
    }
}

bool UndoList::canUndo()
{
    return mItems.count()>0;
}

int UndoList::itemCount()
{
    return mItems.count();
}

bool UndoList::initialState()
{
    if (itemCount() == 0) {
        return mInitialChangeNumber==0;
    } else {
        return peekItem()->changeNumber() == mInitialChangeNumber;
    }
}

void UndoList::setInitialState()
{
    if (itemCount() == 0)
        mInitialChangeNumber = 0;
    else
        mInitialChangeNumber = peekItem()->changeNumber();
}

bool UndoList::insideRedo() const
{
    return mInsideRedo;
}

void UndoList::setInsideRedo(bool insideRedo)
{
    mInsideRedo = insideRedo;
}

bool UndoList::fullUndoImposible() const
{
    return mFullUndoImposible;
}

SelectionMode UndoItem::changeSelMode() const
{
    return mChangeSelMode;
}

BufferCoord UndoItem::changeStartPos() const
{
    return mChangeStartPos;
}

BufferCoord UndoItem::changeEndPos() const
{
    return mChangeEndPos;
}

QStringList UndoItem::changeText() const
{
    return mChangeText;
}

size_t UndoItem::changeNumber() const
{
    return mChangeNumber;
}

UndoItem::UndoItem(ChangeReason reason, SelectionMode selMode,
                                 BufferCoord startPos, BufferCoord endPos,
                                 const QStringList& text, int number)
{
    mChangeReason = reason;
    mChangeSelMode = selMode;
    mChangeStartPos = startPos;
    mChangeEndPos = endPos;
    mChangeText = text;
    mChangeNumber = number;
    int length=0;
    foreach (const QString& s, text) {
        length+=s.length();
    }
}

ChangeReason UndoItem::changeReason() const
{
    return mChangeReason;
}

RedoList::RedoList()
{

}

void RedoList::addRedo(ChangeReason AReason, const BufferCoord &AStart, const BufferCoord &AEnd, const QStringList &ChangeText, SelectionMode SelMode, size_t changeNumber)
{
    PUndoItem  newItem = std::make_shared<UndoItem>(
                AReason,
                SelMode,AStart,AEnd,ChangeText,
                changeNumber);
    mItems.append(newItem);
}

void RedoList::addRedo(PUndoItem item)
{
    mItems.append(item);
}

void RedoList::clear()
{
    mItems.clear();
}

ChangeReason RedoList::lastChangeReason()
{
    if (mItems.count() == 0)
        return ChangeReason::Nothing;
    else
        return mItems.last()->changeReason();
}

bool RedoList::isEmpty()
{
    return mItems.isEmpty();
}

PUndoItem RedoList::peekItem()
{
    if (mItems.count() == 0)
        return PUndoItem();
    else
        return mItems.last();
}

PUndoItem RedoList::popItem()
{
    if (mItems.count() == 0)
        return PUndoItem();
    else {
        PUndoItem item = mItems.last();
        mItems.removeLast();
        return item;
    }
}

bool RedoList::canRedo()
{
    return mItems.count()>0;
}

int RedoList::itemCount()
{
    return mItems.count();
}

BinaryFileError::BinaryFileError(const QString& reason):
    FileError(reason)
{

}

int searchForSegmentIdx(const QList<int> &segList, int minVal, int maxVal, int value)
{
    if (value<minVal)
        return 0;
    if (value>=maxVal)
        return segList.length();
    int start = 0;
    int end = segList.length()-1;
    while ( start<=end ) {
        int mid = (start+end) / 2;
        if (segList[mid]==value) {
            //skip zero length segs
            while(mid+1<segList.length() && segList[mid+1]==value)
                mid++;
            return mid;
        } else if (segList[mid]>value) {
            end = mid-1;
        } else if (mid+1>=segList.length() || segList[mid+1]>value ) {
            return mid;
        } else {
            start = mid+1;
        }
    }
    //Not found, should not happen
    Q_ASSERT(false);
    return -1;
}

int GlyphCalculator::updateGlyphStartPositionList(
        const QString &lineText,
        const QList<int> &glyphStartCharList, int startChar, int endChar,
        const QFontMetrics &fontMetrics,
        QList<int> &glyphStartPositionList, int left, int &right, int &startGlyph, int &endGlyph) const
{
    right = std::max(0,left);
    startGlyph = searchForSegmentIdx(glyphStartCharList,0,lineText.length(),startChar);
    endGlyph = searchForSegmentIdx(glyphStartCharList,0,lineText.length(),endChar);
    for (int i=startGlyph;i<endGlyph;i++) {
        int start = glyphStartCharList[i];
        int end;
        if (i+1<glyphStartCharList.length()) {
            end = glyphStartCharList[i+1];
        } else {
            end = lineText.length();
        }
        QString glyph = lineText.mid(start,end-start);
        int gWidth = glyphWidth(glyph, right, fontMetrics, mForceMonospace);
        glyphStartPositionList[i] = right;
        right += gWidth;
    }
    if (endGlyph<glyphStartPositionList.length())
        glyphStartPositionList[endGlyph] = right;
    return right-left;
}

int GlyphCalculator::glyphWidth(const QString &glyph, int left, const QFontMetrics &fontMetrics, bool forceMonospace) const
{
    int glyphWidth;
    if (glyph.length()==0)
        return 0;
    QChar ch = glyph[0];
    if (ch == '\t') {
        glyphWidth = tabWidth() - left % tabWidth();
    } else {
        glyphWidth = fontMetrics.horizontalAdvance(glyph);
        //qDebug()<<glyph<<glyphCols<<width<<mCharWidth;
    }
    if (forceMonospace) {
        int cols = std::ceil(glyphWidth / (double)mCharWidth);
        glyphWidth = cols * mCharWidth;
    }
    return glyphWidth;
}

void expandGlyphStartCharList(const QString &strAdded, int oldStrLen, QList<int> &glyphStartCharList)
{
    QList<int> addedList = calcGlyphStartCharList(strAdded);
    for (int i=0;i<addedList.length();i++) {
        glyphStartCharList.append(addedList[i]+oldStrLen);
    }
}

int calcSegmentInterval(const QList<int> &segList, int maxVal, int idx)
{
    if (idx<0 || idx>=segList.length())
        return 0;
    if (idx == segList.length()-1)
        return maxVal - segList[idx];
    return segList[idx+1]-segList[idx];
}

int segmentIntervalStart(const QList<int> &segList, int minVal, int maxVal, int idx)
{
    if (idx<0)
        return minVal;
    if (idx>=segList.length())
        return maxVal;
    return segList[idx];
}

GlyphCalculator::GlyphCalculator(const QFont &font):
    mFontMetrics{font},
    mTabSize{4},
    mForceMonospace{false}
{
    mCharWidth =  mFontMetrics.horizontalAdvance("M");
    mSpaceWidth = mFontMetrics.horizontalAdvance(" ");
}

void GlyphCalculator::setFont(const QFont &newFont)
{
    mFontMetrics = QFontMetrics(newFont);
    mCharWidth =  mFontMetrics.horizontalAdvance("M");
    mSpaceWidth = mFontMetrics.horizontalAdvance(" ");
}

}
