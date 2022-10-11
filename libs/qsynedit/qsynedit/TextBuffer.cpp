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
#include "TextBuffer.h"
#include "qt_utils/utils.h"
#include <QDataStream>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <QMutexLocker>
#include <stdexcept>
#include "SynEdit.h"
#include <QMessageBox>
#include <cmath>
#include "qt_utils/charsetinfo.h"
#include <QDebug>

namespace QSynedit {

SynDocument::SynDocument(const QFont& font, const QFont& nonAsciiFont, QObject *parent):
      QObject(parent),
      mFontMetrics(font),
      mNonAsciiFontMetrics(nonAsciiFont),
      mTabWidth(4),
      mMutex(QMutex::Recursive)
{

    mAppendNewLineAtEOF = true;
    mFileEndingType = FileEndingType::Windows;
    mIndexOfLongestLine = -1;
    mUpdateCount = 0;
    mCharWidth =  mFontMetrics.horizontalAdvance("M");
}

static void ListIndexOutOfBounds(int index) {
    throw IndexOutOfRange(index);
}



int SynDocument::parenthesisLevels(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange.parenthesisLevel;
    } else
        return 0;
}

int SynDocument::bracketLevels(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange.bracketLevel;
    } else
        return 0;
}

int SynDocument::braceLevels(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange.braceLevel;
    } else
        return 0;
}

//QString SynEditStringList::expandedStrings(int Index)
//{
//    if (Index>=0 && Index < mList.size()) {
//        if (mList[Index]->fFlags & SynEditStringFlag::sfHasNoTabs)
//            return mList[Index]->fString;
//        else
//            return ExpandString(Index);
//    } else
//        return QString();
//}

int SynDocument::lineColumns(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        if (mLines[Index]->fColumns == -1) {
            return calculateLineColumns(Index);
        } else
            return mLines[Index]->fColumns;
    } else
        return 0;
}

int SynDocument::leftBraces(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange.leftBraces;
    } else
        return 0;
}

int SynDocument::rightBraces(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange.rightBraces;
    } else
        return 0;
}

int SynDocument::lengthOfLongestLine() {
    QMutexLocker locker(&mMutex);
    if (mIndexOfLongestLine < 0) {
        int MaxLen = -1;
        mIndexOfLongestLine = -1;
        if (mLines.count() > 0 ) {
            for (int i=0;i<mLines.size();i++) {
                int len = lineColumns(i);
                if (len > MaxLen) {
                    MaxLen = len;
                    mIndexOfLongestLine = i;
                }
            }
        }
    }
    if (mIndexOfLongestLine >= 0)
        return mLines[mIndexOfLongestLine]->fColumns;
    else
        return 0;
}

QString SynDocument::lineBreak() const
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

HighlighterState SynDocument::ranges(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange;
    } else {
         ListIndexOutOfBounds(Index);
    }
    return HighlighterState();
}

void SynDocument::insertItem(int Index, const QString &s)
{
    beginUpdate();
    PSynDocumentLine line = std::make_shared<SynDocumentLine>();
    line->fString = s;
    mIndexOfLongestLine = -1;
    mLines.insert(Index,line);
    endUpdate();
}

void SynDocument::addItem(const QString &s)
{
    beginUpdate();
    PSynDocumentLine line = std::make_shared<SynDocumentLine>();
    line->fString = s;
    mIndexOfLongestLine = -1;
    mLines.append(line);
    endUpdate();
}

bool SynDocument::getAppendNewLineAtEOF()
{
    QMutexLocker locker(&mMutex);
    return mAppendNewLineAtEOF;
}

void SynDocument::setAppendNewLineAtEOF(bool appendNewLineAtEOF)
{
    QMutexLocker locker(&mMutex);
    mAppendNewLineAtEOF = appendNewLineAtEOF;
}

void SynDocument::setRange(int Index, const HighlighterState& ARange)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>=mLines.count()) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    mLines[Index]->fRange = ARange;
    endUpdate();
}

QString SynDocument::getString(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>=mLines.count()) {
        return QString();
    }
    return mLines[Index]->fString;
}

int SynDocument::count()
{
    QMutexLocker locker(&mMutex);
    return mLines.count();
}

QString SynDocument::text()
{
    QMutexLocker locker(&mMutex);
    return getTextStr();
}

void SynDocument::setText(const QString &text)
{
    QMutexLocker locker(&mMutex);
    putTextStr(text);
}

void SynDocument::setContents(const QStringList &text)
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    internalClear();
    if (text.count() > 0) {
        mIndexOfLongestLine = -1;
        int FirstAdded = mLines.count();

        foreach (const QString& s,text) {
            addItem(s);
        }
        emit inserted(FirstAdded,text.count());
    }
}

QStringList SynDocument::contents()
{
    QMutexLocker locker(&mMutex);
    QStringList Result;
    SynDocumentLines list = mLines;
    foreach (const PSynDocumentLine& line, list) {
        Result.append(line->fString);
    }
    return Result;
}

void SynDocument::beginUpdate()
{
    if (mUpdateCount == 0) {
        setUpdateState(true);
    }
    mUpdateCount++;
}

void SynDocument::endUpdate()
{
    mUpdateCount--;
    if (mUpdateCount == 0) {
        setUpdateState(false);
    }
}


int SynDocument::add(const QString &s)
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    int Result = mLines.count();
    insertItem(Result, s);
    emit inserted(Result,1);
    endUpdate();
    return Result;
}

void SynDocument::addStrings(const QStringList &Strings)
{
    QMutexLocker locker(&mMutex);
    if (Strings.count() > 0) {
        mIndexOfLongestLine = -1;
        beginUpdate();
        auto action = finally([this]{
            endUpdate();
        });
        int FirstAdded = mLines.count();

        for (const QString& s:Strings) {
            addItem(s);
        }
        emit inserted(FirstAdded,Strings.count());
    }
}

int SynDocument::getTextLength()
{
    QMutexLocker locker(&mMutex);
    int Result = 0;
    foreach (const PSynDocumentLine& line, mLines ) {
        Result += line->fString.length();
        if (mFileEndingType == FileEndingType::Windows) {
            Result += 2;
        } else {
            Result += 1;
        }
    }
    return Result;
}

void SynDocument::clear()
{
    QMutexLocker locker(&mMutex);
    internalClear();
}

void SynDocument::deleteLines(int Index, int NumLines)
{
    QMutexLocker locker(&mMutex);
    if (NumLines<=0)
        return;
    if ((Index < 0) || (Index >= mLines.count())) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    if (mIndexOfLongestLine>=Index) {
        if (mIndexOfLongestLine <Index+NumLines) {
            mIndexOfLongestLine = -1;
        } else {
            mIndexOfLongestLine -= NumLines;
        }
    }
    int LinesAfter = mLines.count() - (Index + NumLines);
    if (LinesAfter < 0) {
       NumLines = mLines.count() - Index;
    }
    mLines.remove(Index,NumLines);
    emit deleted(Index,NumLines);
}

void SynDocument::exchange(int Index1, int Index2)
{
    QMutexLocker locker(&mMutex);
    if ((Index1 < 0) || (Index1 >= mLines.count())) {
        ListIndexOutOfBounds(Index1);
    }
    if ((Index2 < 0) || (Index2 >= mLines.count())) {
        ListIndexOutOfBounds(Index2);
    }
    beginUpdate();
    PSynDocumentLine temp = mLines[Index1];
    mLines[Index1]=mLines[Index2];
    mLines[Index2]=temp;
    //mList.swapItemsAt(Index1,Index2);
    if (mIndexOfLongestLine == Index1) {
        mIndexOfLongestLine = Index2;
    } else if (mIndexOfLongestLine == Index2) {
        mIndexOfLongestLine = Index1;
    }
    endUpdate();
}

void SynDocument::insert(int Index, const QString &s)
{
    QMutexLocker locker(&mMutex);
    if ((Index < 0) || (Index > mLines.count())) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    insertItem(Index, s);
    emit inserted(Index,1);
    endUpdate();
}

void SynDocument::deleteAt(int Index)
{
    QMutexLocker locker(&mMutex);
    if ((Index < 0) || (Index >= mLines.count())) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    if (mIndexOfLongestLine == Index)
        mIndexOfLongestLine = -1;
    else if (mIndexOfLongestLine>Index)
        mIndexOfLongestLine -= 1;
    mLines.removeAt(Index);
    emit deleted(Index,1);
    endUpdate();
}

QString SynDocument::getTextStr() const
{
    QString result;
    for (int i=0;i<mLines.count()-1;i++) {
        const PSynDocumentLine& line = mLines[i];
        result.append(line->fString);
        result.append(lineBreak());
    }
    if (mLines.length()>0) {
        result.append(mLines.back()->fString);
    }
    return result;
}

void SynDocument::putString(int Index, const QString &s, bool notify) {
    QMutexLocker locker(&mMutex);
    if (Index == mLines.count()) {
        add(s);
    } else {
        if (Index<0 || Index>=mLines.count()) {
            ListIndexOutOfBounds(Index);
        }
        beginUpdate();
        int oldColumns = mLines[Index]->fColumns;
        mLines[Index]->fString = s;
        calculateLineColumns(Index);
        if (mIndexOfLongestLine == Index && oldColumns>mLines[Index]->fColumns )
            mIndexOfLongestLine = -1;
        else if (mIndexOfLongestLine>=0
                 && mIndexOfLongestLine<mLines.count()
                 && mLines[Index]->fColumns > mLines[mIndexOfLongestLine]->fColumns)
            mIndexOfLongestLine = Index;
        if (notify)
            emit putted(Index,1);
        endUpdate();
    }
}

void SynDocument::setUpdateState(bool Updating)
{
    if (Updating)
        emit changing();
    else
        emit changed();
}

int SynDocument::calculateLineColumns(int Index)
{
    PSynDocumentLine line = mLines[Index];

    line->fColumns = stringColumns(line->fString,0);
    return line->fColumns;
}

void SynDocument::insertLines(int Index, int NumLines)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>mLines.count()) {
        ListIndexOutOfBounds(Index);
    }
    if (NumLines<=0)
        return;
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    mIndexOfLongestLine = -1;
    PSynDocumentLine line;
    mLines.insert(Index,NumLines,line);
    for (int i=Index;i<Index+NumLines;i++) {
        line = std::make_shared<SynDocumentLine>();
        mLines[i]=line;
    }
    emit inserted(Index,NumLines);
}


bool SynDocument::tryLoadFileByEncoding(QByteArray encodingName, QFile& file) {
    QTextCodec* codec = QTextCodec::codecForName(encodingName);
    if (!codec)
        return false;
    file.reset();
    internalClear();
    QTextCodec::ConverterState state;
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
        QString newLine = codec->toUnicode(line.constData(),line.length(),&state);
        if (state.invalidChars>0) {
            return false;
            break;
        }
        addItem(newLine);
    }
    return true;
}

const QFontMetrics &SynDocument::fontMetrics() const
{
    return mFontMetrics;
}

void SynDocument::setFontMetrics(const QFont &newFont, const QFont& newNonAsciiFont)
{
    mFontMetrics = QFontMetrics(newFont);
    mCharWidth =  mFontMetrics.horizontalAdvance("M");
    mNonAsciiFontMetrics = QFontMetrics(newNonAsciiFont);
}

void SynDocument::setTabWidth(int newTabWidth)
{
    if (mTabWidth!=newTabWidth) {
        mTabWidth = newTabWidth;
        resetColumns();
    }
}
void SynDocument::loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding)
{
    QMutexLocker locker(&mMutex);
    QFile file(filename);
    if (!file.open(QFile::ReadOnly ))
        throw FileError(tr("Can't open file '%1' for read!").arg(file.fileName()));
    beginUpdate();
    internalClear();
    auto action = finally([this]{
        if (mLines.count()>0)
            emit inserted(0,mLines.count());
        endUpdate();
    });
    mIndexOfLongestLine = -1;
    //test for utf8 / utf 8 bom
    if (encoding == ENCODING_AUTO_DETECT) {
        if (file.atEnd()) {
            realEncoding = ENCODING_ASCII;
            return;
        }
        QByteArray line = file.readLine();
        QTextCodec* codec;
        QTextCodec::ConverterState state;
        bool needReread = false;
        bool allAscii = true;
        //test for BOM
        if ((line.length()>=3) && ((unsigned char)line[0]==0xEF) && ((unsigned char)line[1]==0xBB) && ((unsigned char)line[2]==0xBF) ) {
            realEncoding = ENCODING_UTF8_BOM;
            line = line.mid(3);
            codec = QTextCodec::codecForName(ENCODING_UTF8);
        } else {
            realEncoding = ENCODING_UTF8;
            codec = QTextCodec::codecForName(ENCODING_UTF8);
        }
        if (line.endsWith("\r\n")) {
            mFileEndingType = FileEndingType::Windows;
        } else if (line.endsWith("\n")) {
            mFileEndingType = FileEndingType::Linux;
        } else if (line.endsWith("\r")) {
            mFileEndingType = FileEndingType::Mac;
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
            if (allAscii) {
                allAscii = isTextAllAscii(line);
            }
            if (allAscii) {
                addItem(QString::fromLatin1(line));
            } else {
                QString newLine = codec->toUnicode(line.constData(),line.length(),&state);
                if (state.invalidChars>0) {
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
        QList<PCharsetInfo> charsets = pCharsetInfoManager->findCharsetByLocale(pCharsetInfoManager->localeName());
        if (!charsets.isEmpty()) {
            if (tryLoadFileByEncoding(realEncoding,file)) {
                return;
            }

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
    QTextStream textStream(&file);
    if (realEncoding == ENCODING_UTF8_BOM) {
        textStream.setAutoDetectUnicode(true);
        textStream.setCodec(ENCODING_UTF8);
    } else {
        textStream.setAutoDetectUnicode(false);
        textStream.setCodec(realEncoding);
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



void SynDocument::saveToFile(QFile &file, const QByteArray& encoding,
                                   const QByteArray& defaultEncoding, QByteArray& realEncoding)
{
    QMutexLocker locker(&mMutex);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        throw FileError(tr("Can't open file '%1' for save!").arg(file.fileName()));
    if (mLines.isEmpty())
        return;
    bool allAscii = true;

    QTextCodec* codec;
    realEncoding = encoding;
    if (realEncoding == ENCODING_UTF8_BOM) {
        codec = QTextCodec::codecForName(ENCODING_UTF8);
        file.putChar(0xEF);
        file.putChar(0xBB);
        file.putChar(0xBF);
    } else if (realEncoding == ENCODING_SYSTEM_DEFAULT) {
        codec = QTextCodec::codecForLocale();
    } else if (realEncoding == ENCODING_AUTO_DETECT) {
        codec = QTextCodec::codecForName(defaultEncoding);
        if (!codec)
            codec = QTextCodec::codecForLocale();
    } else {
        codec = QTextCodec::codecForName(realEncoding);
    }
    for (PSynDocumentLine& line:mLines) {
        if (allAscii) {
            allAscii = isTextAllAscii(line->fString);
        }
        if (!allAscii) {
            file.write(codec->fromUnicode(line->fString));
        } else {
            file.write(line->fString.toLatin1());
        }
        file.write(lineBreak().toLatin1());
    }
    if (encoding == ENCODING_AUTO_DETECT) {
        if (allAscii)
            realEncoding = ENCODING_ASCII;
        else if (codec->name() == "System") {
            realEncoding = pCharsetInfoManager->getDefaultSystemEncoding();
        } else {
            realEncoding = codec->name();
        }
    }
}

int SynDocument::stringColumns(const QString &line, int colsBefore) const
{
    int columns = std::max(0,colsBefore);
    int charCols;
    for (int i=0;i<line.length();i++) {
        QChar ch = line[i];
        if (ch == '\t') {
            charCols = mTabWidth - columns % mTabWidth;
        } else {
            charCols = charColumns(ch);
        }
        columns+=charCols;
    }
    return columns-colsBefore;
}

int SynDocument::charColumns(QChar ch) const
{
    if (ch.unicode()<=32)
        return 1;
    int width;
    if (ch.unicode()<0xFF)
        width = mFontMetrics.horizontalAdvance(ch);
    else
        width = mNonAsciiFontMetrics.horizontalAdvance(ch);
    //return std::ceil((int)(fontMetrics().horizontalAdvance(ch) * dpiFactor()) / (double)mCharWidth);
    return std::ceil(width / (double)mCharWidth);
}

void SynDocument::putTextStr(const QString &text)
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
        add(text.mid(start,pos-start));
        if (pos>=text.length())
            break;
        if (text[pos] == '\r')
            pos++;
        if (text[pos] == '\n')
            pos++;
    }
}

void SynDocument::internalClear()
{
    if (!mLines.isEmpty()) {
        beginUpdate();
        int oldCount = mLines.count();
        mIndexOfLongestLine = -1;
        mLines.clear();
        emit deleted(0,oldCount);
        endUpdate();
    }
}

FileEndingType SynDocument::getFileEndingType()
{
    QMutexLocker locker(&mMutex);
    return mFileEndingType;
}

void SynDocument::setFileEndingType(const FileEndingType &fileEndingType)
{
    QMutexLocker locker(&mMutex);
    mFileEndingType = fileEndingType;
}

bool SynDocument::empty()
{
    QMutexLocker locker(&mMutex);
    return mLines.count()==0;
}

void SynDocument::resetColumns()
{
    QMutexLocker locker(&mMutex);
    mIndexOfLongestLine = -1;
    if (mLines.count() > 0 ) {
        for (int i=0;i<mLines.size();i++) {
            mLines[i]->fColumns = -1;
        }
    }
}

void SynDocument::invalidAllLineColumns()
{
    QMutexLocker locker(&mMutex);
    mIndexOfLongestLine = -1;
    for (PSynDocumentLine& line:mLines) {
        line->fColumns = -1;
    }
}

SynDocumentLine::SynDocumentLine():
    fString(),
    fRange(),
    fColumns(-1)
{
}


SynEditUndoList::SynEditUndoList():QObject()
{
    mMaxUndoActions = 1024;
    mMaxMemoryUsage = 50 * 1024 * 1024;
    mNextChangeNumber = 1;
    mInsideRedo = false;

    mBlockChangeNumber=0;
    mBlockLock=0;
    mFullUndoImposible=false;
    mBlockCount=0;
    mMemoryUsage=0;
    mLastPoppedItemChangeNumber=0;
    mInitialChangeNumber = 0;
    mLastRestoredItemChangeNumber=0;
}

void SynEditUndoList::addChange(SynChangeReason reason, const BufferCoord &startPos,
                                const BufferCoord &endPos, const QStringList& changeText,
                                SelectionMode selMode)
{
    int changeNumber;
    if (inBlock()) {
        changeNumber = mBlockChangeNumber;
    } else {
        changeNumber = getNextChangeNumber();
    }
    PSynEditUndoItem  newItem = std::make_shared<SynEditUndoItem>(
                reason,
                selMode,startPos,endPos,changeText,
                changeNumber);
//    qDebug()<<"add change"<<changeNumber<<(int)reason;
    mItems.append(newItem);
    addMemoryUsage(newItem);
    ensureMaxEntries();

    if (reason!=SynChangeReason::GroupBreak && !inBlock()) {
        mBlockCount++;
//        qDebug()<<"add"<<mBlockCount;
        emit addedUndo();
    }
}

void SynEditUndoList::restoreChange(SynChangeReason AReason, const BufferCoord &AStart, const BufferCoord &AEnd, const QStringList &ChangeText, SelectionMode SelMode, size_t changeNumber)
{
    PSynEditUndoItem  newItem = std::make_shared<SynEditUndoItem>(AReason,
                                                                  SelMode,AStart,AEnd,ChangeText,
                                                                  changeNumber);
    restoreChange(newItem);
}

void SynEditUndoList::restoreChange(PSynEditUndoItem item)
{
    size_t changeNumber = item->changeNumber();
    mItems.append(item);
    addMemoryUsage(item);
    ensureMaxEntries();
    if (changeNumber>mNextChangeNumber)
        mNextChangeNumber=changeNumber;
    if (changeNumber!=mLastRestoredItemChangeNumber) {
//        qDebug()<<"restore"<<mBlockCount;
        mBlockCount++;
        emit addedUndo();
    }
    mLastRestoredItemChangeNumber=changeNumber;
}

void SynEditUndoList::addGroupBreak()
{
    if (!canUndo())
        return;

    if (lastChangeReason() != SynChangeReason::GroupBreak) {
        addChange(SynChangeReason::GroupBreak, {0,0}, {0,0}, QStringList(), SelectionMode::Normal);
    }
}

void SynEditUndoList::beginBlock()
{
//    qDebug()<<"begin block";
    if (mBlockLock==0)
        mBlockChangeNumber = getNextChangeNumber();
    mBlockLock++;

}

void SynEditUndoList::clear()
{
    mItems.clear();
    mFullUndoImposible = false;
    mInitialChangeNumber=0;
    mLastPoppedItemChangeNumber=0;
    mLastRestoredItemChangeNumber=0;
    mBlockCount=0;
    mBlockLock=0;
    mMemoryUsage=0;
}

void SynEditUndoList::endBlock()
{
//    qDebug()<<"end block";
    if (mBlockLock > 0) {
        mBlockLock--;
        if (mBlockLock == 0)  {
            size_t iBlockID = mBlockChangeNumber;
            mBlockChangeNumber = 0;
            if (mItems.count() > 0 && peekItem()->changeNumber() == iBlockID) {
                mBlockCount++;
//                qDebug()<<"end block"<<mBlockCount;
                emit addedUndo();
            }
        }
    }
}

bool SynEditUndoList::inBlock()
{
    return mBlockLock>0;
}

unsigned int SynEditUndoList::getNextChangeNumber()
{
    return mNextChangeNumber++;
}

void SynEditUndoList::addMemoryUsage(PSynEditUndoItem item)
{
    if (!item)
        return;
    int length=0;
    foreach (const QString& s, item->changeText()) {
        length+=s.length()+2;
    }
    mMemoryUsage +=  length * sizeof(QChar) ;
}

void SynEditUndoList::reduceMemoryUsage(PSynEditUndoItem item)
{
    if (!item)
        return;
    int length=0;
    foreach (const QString& s, item->changeText()) {
        length+=s.length()+2;
    }
    mMemoryUsage -=  length * sizeof(QChar) ;
}

int SynEditUndoList::maxMemoryUsage() const
{
    return mMaxMemoryUsage;
}

void SynEditUndoList::setMaxMemoryUsage(int newMaxMemoryUsage)
{
    mMaxMemoryUsage = newMaxMemoryUsage;
}

SynChangeReason SynEditUndoList::lastChangeReason()
{
    if (mItems.count() == 0)
        return SynChangeReason::Nothing;
    else
        return mItems.last()->changeReason();
}

bool SynEditUndoList::isEmpty()
{
    return mItems.count()==0;
}

PSynEditUndoItem SynEditUndoList::peekItem()
{
    if (mItems.count() == 0)
        return PSynEditUndoItem();
    else
        return mItems.last();
}

PSynEditUndoItem SynEditUndoList::popItem()
{
    if (mItems.count() == 0)
        return PSynEditUndoItem();
    else {
        PSynEditUndoItem item = mItems.last();
//        qDebug()<<"popped"<<item->changeNumber()<<item->changeText()<<(int)item->changeReason()<<mLastPoppedItemChangeNumber;
        if (mLastPoppedItemChangeNumber!=item->changeNumber() && item->changeReason()!=SynChangeReason::GroupBreak) {
            mBlockCount--;
//            qDebug()<<"pop"<<mBlockCount;
            if (mBlockCount<0) {
                qDebug()<<"block count calculation error";
                mBlockCount=0;
            }
        }
        mLastPoppedItemChangeNumber =  item->changeNumber();
        reduceMemoryUsage(item);
        mItems.removeLast();
        return item;
    }
}

bool SynEditUndoList::canUndo()
{
    return mItems.count()>0;
}

int SynEditUndoList::itemCount()
{
    return mItems.count();
}

int SynEditUndoList::maxUndoActions() const
{
    return mMaxUndoActions;
}

void SynEditUndoList::setMaxUndoActions(int maxUndoActions)
{
    if (maxUndoActions!=mMaxUndoActions) {
        mMaxUndoActions = maxUndoActions;
        ensureMaxEntries();
    }
}

bool SynEditUndoList::initialState()
{
    if (itemCount() == 0) {
        return mInitialChangeNumber==0;
    } else {
        return peekItem()->changeNumber() == mInitialChangeNumber;
    }
}

void SynEditUndoList::setInitialState()
{
    if (itemCount() == 0)
        mInitialChangeNumber = 0;
    else
        mInitialChangeNumber = peekItem()->changeNumber();
}

bool SynEditUndoList::insideRedo() const
{
    return mInsideRedo;
}

void SynEditUndoList::setInsideRedo(bool insideRedo)
{
    mInsideRedo = insideRedo;
}

bool SynEditUndoList::fullUndoImposible() const
{
    return mFullUndoImposible;
}

void SynEditUndoList::ensureMaxEntries()
{
    if (mMaxUndoActions>0 && (mBlockCount > mMaxUndoActions || mMemoryUsage>mMaxMemoryUsage)){
        mFullUndoImposible = true;
        while ((mBlockCount > mMaxUndoActions || mMemoryUsage>mMaxMemoryUsage)
               && !mItems.isEmpty()) {
            //remove all undo item in block
            PSynEditUndoItem item = mItems.front();
            size_t changeNumber = item->changeNumber();
            while (mItems.count()>0) {
                item = mItems.front();
                if (item->changeNumber()!=changeNumber)
                    break;
                reduceMemoryUsage(item);
                mItems.removeFirst();
            }
            if (item->changeReason()!=SynChangeReason::GroupBreak)
                mBlockCount--;
      }
    }
}

SelectionMode SynEditUndoItem::changeSelMode() const
{
    return mChangeSelMode;
}

BufferCoord SynEditUndoItem::changeStartPos() const
{
    return mChangeStartPos;
}

BufferCoord SynEditUndoItem::changeEndPos() const
{
    return mChangeEndPos;
}

QStringList SynEditUndoItem::changeText() const
{
    return mChangeText;
}

size_t SynEditUndoItem::changeNumber() const
{
    return mChangeNumber;
}

SynEditUndoItem::SynEditUndoItem(SynChangeReason reason, SelectionMode selMode,
                                 BufferCoord startPos, BufferCoord endPos,
                                 const QStringList& text, int number)
{
    mChangeReason = reason;
    mChangeSelMode = selMode;
    mChangeStartPos = startPos;
    mChangeEndPos = endPos;
    mChangeText = text;
    mChangeNumber = number;
}

SynChangeReason SynEditUndoItem::changeReason() const
{
    return mChangeReason;
}

SynEditRedoList::SynEditRedoList()
{

}

void SynEditRedoList::addRedo(SynChangeReason AReason, const BufferCoord &AStart, const BufferCoord &AEnd, const QStringList &ChangeText, SelectionMode SelMode, size_t changeNumber)
{
    PSynEditUndoItem  newItem = std::make_shared<SynEditUndoItem>(
                AReason,
                SelMode,AStart,AEnd,ChangeText,
                changeNumber);
    mItems.append(newItem);
}

void SynEditRedoList::addRedo(PSynEditUndoItem item)
{
    mItems.append(item);
}

void SynEditRedoList::clear()
{
    mItems.clear();
}

SynChangeReason SynEditRedoList::lastChangeReason()
{
    if (mItems.count() == 0)
        return SynChangeReason::Nothing;
    else
        return mItems.last()->changeReason();
}

bool SynEditRedoList::isEmpty()
{
    return mItems.isEmpty();
}

PSynEditUndoItem SynEditRedoList::peekItem()
{
    if (mItems.count() == 0)
        return PSynEditUndoItem();
    else
        return mItems.last();
}

PSynEditUndoItem SynEditRedoList::popItem()
{
    if (mItems.count() == 0)
        return PSynEditUndoItem();
    else {
        PSynEditUndoItem item = mItems.last();
        mItems.removeLast();
        return item;
    }
}

bool SynEditRedoList::canRedo()
{
    return mItems.count()>0;
}

int SynEditRedoList::itemCount()
{
    return mItems.count();
}

}
