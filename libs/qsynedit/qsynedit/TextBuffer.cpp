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

Document::Document(const QFont& font, const QFont& nonAsciiFont, QObject *parent):
      QObject(parent),
      mFontMetrics(font),
      mNonAsciiFontMetrics(nonAsciiFont),
      mTabWidth(4),
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      mMutex()
#else
      mMutex(QMutex::Recursive)
#endif
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



int Document::parenthesisLevels(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange.parenthesisLevel;
    } else
        return 0;
}

int Document::bracketLevels(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange.bracketLevel;
    } else
        return 0;
}

int Document::braceLevels(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange.braceLevel;
    } else
        return 0;
}

int Document::lineColumns(int Index)
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

int Document::leftBraces(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange.leftBraces;
    } else
        return 0;
}

int Document::rightBraces(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange.rightBraces;
    } else
        return 0;
}

int Document::lengthOfLongestLine() {
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

QString Document::lineBreak() const
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

HighlighterState Document::ranges(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mLines.size()) {
        return mLines[Index]->fRange;
    } else {
         ListIndexOutOfBounds(Index);
    }
    return HighlighterState();
}

void Document::insertItem(int Index, const QString &s)
{
    beginUpdate();
    PDocumentLine line = std::make_shared<DocumentLine>();
    line->fString = s;
    mIndexOfLongestLine = -1;
    mLines.insert(Index,line);
    endUpdate();
}

void Document::addItem(const QString &s)
{
    beginUpdate();
    PDocumentLine line = std::make_shared<DocumentLine>();
    line->fString = s;
    mIndexOfLongestLine = -1;
    mLines.append(line);
    endUpdate();
}

bool Document::getAppendNewLineAtEOF()
{
    QMutexLocker locker(&mMutex);
    return mAppendNewLineAtEOF;
}

void Document::setAppendNewLineAtEOF(bool appendNewLineAtEOF)
{
    QMutexLocker locker(&mMutex);
    mAppendNewLineAtEOF = appendNewLineAtEOF;
}

void Document::setRange(int Index, const HighlighterState& ARange)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>=mLines.count()) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    mLines[Index]->fRange = ARange;
    endUpdate();
}

QString Document::getString(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>=mLines.count()) {
        return QString();
    }
    return mLines[Index]->fString;
}

int Document::count()
{
    QMutexLocker locker(&mMutex);
    return mLines.count();
}

QString Document::text()
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
        mIndexOfLongestLine = -1;
        int FirstAdded = mLines.count();

        foreach (const QString& s,text) {
            addItem(s);
        }
        emit inserted(FirstAdded,text.count());
    }
}

QStringList Document::contents()
{
    QMutexLocker locker(&mMutex);
    QStringList Result;
    DocumentLines list = mLines;
    foreach (const PDocumentLine& line, list) {
        Result.append(line->fString);
    }
    return Result;
}

void Document::beginUpdate()
{
    if (mUpdateCount == 0) {
        setUpdateState(true);
    }
    mUpdateCount++;
}

void Document::endUpdate()
{
    mUpdateCount--;
    if (mUpdateCount == 0) {
        setUpdateState(false);
    }
}


int Document::add(const QString &s)
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    int Result = mLines.count();
    insertItem(Result, s);
    emit inserted(Result,1);
    endUpdate();
    return Result;
}

void Document::addStrings(const QStringList &Strings)
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

int Document::getTextLength()
{
    QMutexLocker locker(&mMutex);
    int Result = 0;
    foreach (const PDocumentLine& line, mLines ) {
        Result += line->fString.length();
        if (mFileEndingType == FileEndingType::Windows) {
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

void Document::deleteLines(int Index, int NumLines)
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

void Document::exchange(int Index1, int Index2)
{
    QMutexLocker locker(&mMutex);
    if ((Index1 < 0) || (Index1 >= mLines.count())) {
        ListIndexOutOfBounds(Index1);
    }
    if ((Index2 < 0) || (Index2 >= mLines.count())) {
        ListIndexOutOfBounds(Index2);
    }
    beginUpdate();
    PDocumentLine temp = mLines[Index1];
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

void Document::insert(int Index, const QString &s)
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

void Document::deleteAt(int Index)
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

QString Document::getTextStr() const
{
    QString result;
    for (int i=0;i<mLines.count()-1;i++) {
        const PDocumentLine& line = mLines[i];
        result.append(line->fString);
        result.append(lineBreak());
    }
    if (mLines.length()>0) {
        result.append(mLines.back()->fString);
    }
    return result;
}

void Document::putString(int Index, const QString &s, bool notify) {
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

void Document::setUpdateState(bool Updating)
{
    if (Updating)
        emit changing();
    else
        emit changed();
}

int Document::calculateLineColumns(int Index)
{
    PDocumentLine line = mLines[Index];

    line->fColumns = stringColumns(line->fString,0);
    return line->fColumns;
}

void Document::insertLines(int Index, int NumLines)
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
    PDocumentLine line;
    mLines.insert(Index,NumLines,line);
    for (int i=Index;i<Index+NumLines;i++) {
        line = std::make_shared<DocumentLine>();
        mLines[i]=line;
    }
    emit inserted(Index,NumLines);
}


bool Document::tryLoadFileByEncoding(QByteArray encodingName, QFile& file) {
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

const QFontMetrics &Document::fontMetrics() const
{
    return mFontMetrics;
}

void Document::setFontMetrics(const QFont &newFont, const QFont& newNonAsciiFont)
{
    mFontMetrics = QFontMetrics(newFont);
    mCharWidth =  mFontMetrics.horizontalAdvance("M");
    mNonAsciiFontMetrics = QFontMetrics(newNonAsciiFont);
}

void Document::setTabWidth(int newTabWidth)
{
    if (mTabWidth!=newTabWidth) {
        mTabWidth = newTabWidth;
        resetColumns();
    }
}
void Document::loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding)
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



void Document::saveToFile(QFile &file, const QByteArray& encoding,
                                   const QByteArray& defaultEncoding, QByteArray& realEncoding)
{
    QMutexLocker locker(&mMutex);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        throw FileError(tr("Can't open file '%1' for save!").arg(file.fileName()));
    if (mLines.isEmpty())
        return;
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
    bool allAscii = true;
    for (PDocumentLine& line:mLines) {
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
    if (allAscii) {
        realEncoding = ENCODING_ASCII;
    } else if (encoding == ENCODING_AUTO_DETECT) {
        if (codec->name().compare("System",Qt::CaseInsensitive)==0) {
            realEncoding = pCharsetInfoManager->getDefaultSystemEncoding();
        } else {
            realEncoding = codec->name();
        }
    }
}

int Document::stringColumns(const QString &line, int colsBefore) const
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

int Document::charColumns(QChar ch) const
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
        add(text.mid(start,pos-start));
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
        mIndexOfLongestLine = -1;
        mLines.clear();
        emit deleted(0,oldCount);
        endUpdate();
    }
}

FileEndingType Document::getFileEndingType()
{
    QMutexLocker locker(&mMutex);
    return mFileEndingType;
}

void Document::setFileEndingType(const FileEndingType &fileEndingType)
{
    QMutexLocker locker(&mMutex);
    mFileEndingType = fileEndingType;
}

bool Document::empty()
{
    QMutexLocker locker(&mMutex);
    return mLines.count()==0;
}

void Document::resetColumns()
{
    QMutexLocker locker(&mMutex);
    mIndexOfLongestLine = -1;
    if (mLines.count() > 0 ) {
        for (int i=0;i<mLines.size();i++) {
            mLines[i]->fColumns = -1;
        }
    }
}

void Document::invalidAllLineColumns()
{
    QMutexLocker locker(&mMutex);
    mIndexOfLongestLine = -1;
    for (PDocumentLine& line:mLines) {
        line->fColumns = -1;
    }
}

DocumentLine::DocumentLine():
    fString(),
    fRange(),
    fColumns(-1)
{
}


UndoList::UndoList():QObject()
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
//    qDebug()<<"add change"<<changeNumber<<(int)reason;
    mItems.append(newItem);
    addMemoryUsage(newItem);
    ensureMaxEntries();

    if (reason!=ChangeReason::GroupBreak && !inBlock()) {
        mBlockCount++;
//        qDebug()<<"add"<<mBlockCount;
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
    mBlockCount=0;
    mBlockLock=0;
    mMemoryUsage=0;
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
                mBlockCount++;
//                qDebug()<<"end block"<<mBlockCount;
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

void UndoList::addMemoryUsage(PUndoItem item)
{
    if (!item)
        return;
    mMemoryUsage += item->memoryUsage();
}

void UndoList::reduceMemoryUsage(PUndoItem item)
{
    if (!item)
        return;
    mMemoryUsage -= item->memoryUsage();
}

int UndoList::maxMemoryUsage() const
{
    return mMaxMemoryUsage;
}

void UndoList::setMaxMemoryUsage(int newMaxMemoryUsage)
{
    mMaxMemoryUsage = newMaxMemoryUsage;
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
        if (mLastPoppedItemChangeNumber!=item->changeNumber() && item->changeReason()!=ChangeReason::GroupBreak) {
            mBlockCount--;
            Q_ASSERT(mBlockCount>=0);
//            qDebug()<<"pop"<<mBlockCount;
            if (mBlockCount<0) {
                mBlockCount=0;
            }
        }
        mLastPoppedItemChangeNumber =  item->changeNumber();
        reduceMemoryUsage(item);
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

int UndoList::maxUndoActions() const
{
    return mMaxUndoActions;
}

void UndoList::setMaxUndoActions(int maxUndoActions)
{
    if (maxUndoActions!=mMaxUndoActions) {
        mMaxUndoActions = maxUndoActions;
        ensureMaxEntries();
    }
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

void UndoList::ensureMaxEntries()
{
    if (mItems.isEmpty())
        return;
//    qDebug()<<QString("-- List Memory: %1 %2").arg(mMemoryUsage).arg(mMaxMemoryUsage);
    if ((mMaxUndoActions >0 && mBlockCount > mMaxUndoActions)
         || (mMaxMemoryUsage>0 && mMemoryUsage>mMaxMemoryUsage)){
        PUndoItem lastItem = mItems.back();
        mFullUndoImposible = true;
        while (((mMaxUndoActions >0 && mBlockCount > mMaxUndoActions)
               || (mMaxMemoryUsage>0 && mMemoryUsage>mMaxMemoryUsage))
               && !mItems.isEmpty()) {
            //remove all undo item in block
            PUndoItem item = mItems.front();
            size_t changeNumber = item->changeNumber();
            //we shouldn't drop the newest changes;
            if (changeNumber == lastItem->changeNumber())
                break;
            while (mItems.count()>0) {
                item = mItems.front();
                if (item->changeNumber()!=changeNumber)
                    break;
                reduceMemoryUsage(item);
                mItems.removeFirst();
            }
            if (item->changeReason()!=ChangeReason::GroupBreak)
                mBlockCount--;
      }
    }
//    qDebug()<<QString("++ List Memory: %1").arg(mMemoryUsage);
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

unsigned int UndoItem::memoryUsage() const
{
    return mMemoryUsage;
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
    mMemoryUsage =  length * sizeof(QChar) + text.length() * sizeof(QString)
            + sizeof(UndoItem);
//    qDebug()<<mMemoryUsage;
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

}
