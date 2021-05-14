#include "TextBuffer.h"
#include <QDataStream>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <stdexcept>
#include "../utils.h"

SynEditStringList::SynEditStringList(QObject* parent):
    QObject(parent)
{
    mAppendNewLineAtEOF = true;
    mFileEndingType = FileEndingType::Windows;
    mIndexOfLongestLine = -1;
    mUpdateCount = 0;
    setTabWidth(8);
}

static void ListIndexOutOfBounds(int index) {
    throw IndexOutOfRange(index);
}

int SynEditStringList::parenthesisLevels(int Index)
{
    if (Index>=0 && Index < mList.size()) {
        return mList[Index]->fParenthesisLevel;
    } else
        return 0;
}

int SynEditStringList::bracketLevels(int Index)
{
    if (Index>=0 && Index < mList.size()) {
        return mList[Index]->fBracketLevel;
    } else
        return 0;
}

int SynEditStringList::braceLevels(int Index)
{
    if (Index>=0 && Index < mList.size()) {
        return mList[Index]->fBraceLevel;
    } else
        return 0;
}

QString SynEditStringList::expandedStrings(int Index)
{
    if (Index>=0 && Index < mList.size()) {
        if (mList[Index]->fFlags & SynEditStringFlag::sfHasNoTabs)
            return mList[Index]->fString;
        else
            return ExpandString(Index);
    } else
        return QString();
}

int SynEditStringList::expandedStringLength(int Index)
{
    if (Index>=0 && Index < mList.size()) {
        if (mList[Index]->fFlags & sfExpandedLengthUnknown)
            return ExpandString(Index).length();
        else
            return mList[Index]->fExpandedLength;
    } else
        return 0;
}

int SynEditStringList::lengthOfLongestLine()
{
    if (mIndexOfLongestLine < 0) {
        int MaxLen = -1;
        mIndexOfLongestLine = -1;
        if (mList.count() > 0 ) {
            for (int i=0;i<mList.size();i++) {
                int len = expandedStringLength(i);
                if (len > MaxLen) {
                    MaxLen = len;
                    mIndexOfLongestLine = i;
                }
            }
        }
    }
    if (mIndexOfLongestLine >= 0)
      return mList[mIndexOfLongestLine]->fExpandedLength;
    else
        return 0;
}

SynRangeState SynEditStringList::ranges(int Index)
{
    if (Index>=0 && Index < mList.size()) {
        return mList[Index]->fRange;
    } else
        return {0,0};
}

void SynEditStringList::InsertItem(int Index, const QString &s)
{
    beginUpdate();
    PSynEditStringRec line = std::make_shared<SynEditStringRec>();
    line->fString = s;
    mIndexOfLongestLine = -1;
    mList.insert(Index,line);
    endUpdate();
}

ConvertTabsProcEx SynEditStringList::getConvertTabsProc() const
{
    return mConvertTabsProc;
}

bool SynEditStringList::getAppendNewLineAtEOF() const
{
    return mAppendNewLineAtEOF;
}

void SynEditStringList::setAppendNewLineAtEOF(bool appendNewLineAtEOF)
{
    mAppendNewLineAtEOF = appendNewLineAtEOF;
}

void SynEditStringList::setRange(int Index, SynRangeState ARange)
{
    if (Index<0 || Index>=mList.count()) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    mList[Index]->fRange = ARange;
    endUpdate();
}

void SynEditStringList::setParenthesisLevel(int Index, int level)
{
    if (Index<0 || Index>=mList.count()) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    mList[Index]->fParenthesisLevel = level;
    endUpdate();
}

void SynEditStringList::setBracketLevel(int Index, int level)
{
    if (Index<0 || Index>=mList.count()) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    mList[Index]->fBracketLevel = level;
    endUpdate();
}

void SynEditStringList::setBraceLevel(int Index, int level)
{
    if (Index<0 || Index>=mList.count()) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    mList[Index]->fBraceLevel = level;
    endUpdate();
}

QString SynEditStringList::getString(int Index)
{
    if (Index<0 || Index>=mList.count()) {
        return QString();
    }
    return mList[Index]->fString;
}

int SynEditStringList::count()
{
    return mList.count();
}

void *SynEditStringList::getObject(int Index)
{
    if (Index<0 || Index>=mList.count()) {
        return nullptr;
    }
    return mList[Index]->fObject;
}

QString SynEditStringList::text()
{
    return GetTextStr();
}

void SynEditStringList::setText(const QString &text)
{
    PutTextStr(text);
}

void SynEditStringList::beginUpdate()
{
    if (mUpdateCount == 0) {
        SetUpdateState(true);
    }
    mUpdateCount++;
}

void SynEditStringList::endUpdate()
{
    mUpdateCount--;
    if (mUpdateCount == 0) {
        SetUpdateState(false);
    }
}

int SynEditStringList::tabWidth()
{
    return mTabWidth;
}

void SynEditStringList::setTabWidth(int value)
{
    if (value != mTabWidth) {
        mTabWidth = value;
        mConvertTabsProc = GetBestConvertTabsProcEx(mTabWidth);
        mIndexOfLongestLine = -1;
        for (PSynEditStringRec& line:mList) {
            line->fExpandedLength = -1;
            line->fFlags = SynEditStringFlag::sfExpandedLengthUnknown;
        }
    }
}

int SynEditStringList::add(const QString &s)
{
    beginUpdate();
    int Result = mList.count();
    InsertItem(Result, s);
    emit inserted(Result,1);
    endUpdate();
    return Result;
}

int SynEditStringList::addStrings(const QStringList &Strings)
{
    if (Strings.count() > 0) {
        mIndexOfLongestLine = -1;
        beginUpdate();
        auto action = finally([this]{
            endUpdate();
        });
        int FirstAdded = mList.count();

        for (const QString& s:Strings) {
            add(s);
        }
        emit inserted(FirstAdded,Strings.count());
    }
}

int SynEditStringList::getTextLength()
{
    int Result = 0;
    for (const PSynEditStringRec& line: mList ) {
        Result += line->fString.length();
        if (mFileEndingType == FileEndingType::Windows) {
            Result += 2;
        } else {
            Result += 1;
        }
    }
}

void SynEditStringList::clear()
{
    if (!mList.isEmpty()) {
        beginUpdate();
        mIndexOfLongestLine = -1;
        mList.clear();
        endUpdate();
    }
}

void SynEditStringList::deleteLines(int Index, int NumLines)
{
    if (NumLines<=0)
        return;
    if ((Index < 0) || (Index >= mList.count())) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    if (mIndexOfLongestLine>=Index && (mIndexOfLongestLine <Index+NumLines)) {
        mIndexOfLongestLine = - 1;
    }
    int LinesAfter = mList.count() - (Index + NumLines);
    if (LinesAfter < 0) {
       NumLines = mList.count() - Index;
    }
    mList.remove(Index,NumLines);
    emit deleted(Index,NumLines);
}

void SynEditStringList::Exchange(int Index1, int Index2)
{
    if ((Index1 < 0) || (Index1 >= mList.count())) {
        ListIndexOutOfBounds(Index1);
    }
    if ((Index2 < 0) || (Index2 >= mList.count())) {
        ListIndexOutOfBounds(Index2);
    }
    beginUpdate();
    mList.swapItemsAt(Index1,Index2);
    if (mIndexOfLongestLine == Index1) {
        mIndexOfLongestLine = Index2;
    } else if (mIndexOfLongestLine == Index2) {
        mIndexOfLongestLine = Index1;
    }
    endUpdate();
}

void SynEditStringList::Insert(int Index, const QString &s)
{
    if ((Index < 0) || (Index > mList.count())) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    InsertItem(Index, s);
    emit inserted(Index,1);
    endUpdate();
}

void SynEditStringList::deleteAt(int Index)
{
    if ((Index < 0) || (Index >= mList.count())) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    if (mIndexOfLongestLine == Index)
        mIndexOfLongestLine = -1;
    mList.removeAt(Index);
    emit deleted(Index,1);
    endUpdate();
}

QString SynEditStringList::GetTextStr()
{
    QString Result;
    for (PSynEditStringRec& line:mList) {
        Result.append(line->fString);
        switch(mFileEndingType) {
        case FileEndingType::Linux:
            Result.append('\n');
        case FileEndingType::Windows:
            Result.append("\r\n");
        case FileEndingType::Mac:
            Result.append("\r");
        }
    }
    return Result;
}

void SynEditStringList::putString(int Index, const QString &s) {
    if (Index == mList.count()) {
        add(s);
    } else {
        if (Index<0 || Index>=mList.count()) {
            ListIndexOutOfBounds(Index);
        }
        beginUpdate();
        mIndexOfLongestLine = -1;
        mList[Index]->fFlags = SynEditStringFlag::sfExpandedLengthUnknown;
        mList[Index]->fString = s;
        emit putted(Index,1);
        endUpdate();
    }
}

void SynEditStringList::putObject(int Index, void *AObject)
{
    if (Index<0 || Index>=mList.count()) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    mList[Index]->fObject = AObject;
    endUpdate();
}

void SynEditStringList::SetUpdateState(bool Updating)
{
    if (Updating)
        emit changing();
    else
        emit changed();
}

QString SynEditStringList::ExpandString(int Index)
{
    QString Result("");
    PSynEditStringRec line = mList[Index];
    if (line->fString.isEmpty()) {
        line->fFlags = SynEditStringFlag::sfHasNoTabs;
        line->fExpandedLength = 0;
    } else {
        bool hasTabs;
        Result = mConvertTabsProc(line->fString,mTabWidth,hasTabs);
        line->fExpandedLength = Result.length();
        if (hasTabs) {
            line->fFlags = SynEditStringFlag::sfHasTabs;
        } else {
            line->fFlags = SynEditStringFlag::sfHasNoTabs;
        }
    }
    return Result;
}

void SynEditStringList::InsertLines(int Index, int NumLines)
{
    if (Index<0 || Index>=mList.count()) {
        ListIndexOutOfBounds(Index);
    }
    if (NumLines<=0)
        return;
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    PSynEditStringRec line;
    mList.insert(Index,NumLines,line);
    for (int i=Index;i<Index+NumLines;i++) {
        line = std::make_shared<SynEditStringRec>();
        mList[i]=line;
    }
    emit inserted(Index,NumLines);
}

void SynEditStringList::InsertStrings(int Index, const QStringList &NewStrings)
{
    if (Index<0 || Index>=mList.count()) {
        ListIndexOutOfBounds(Index);
    }
    if (NewStrings.isEmpty())
        return;
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    PSynEditStringRec line;
    mList.insert(Index,NewStrings.length(),line);
    for (int i=0;i<NewStrings.length();i++) {
        line = std::make_shared<SynEditStringRec>();
        line->fString = NewStrings[i];
        mList[i+Index]=line;
    }
    emit inserted(Index,NewStrings.length());
}

void SynEditStringList::InsertText(int Index, const QString &NewText)
{
    if (Index<0 || Index>=mList.count()) {
        ListIndexOutOfBounds(Index);
    }
    if (NewText.isEmpty())
        return;
    QStringList lines = TextToLines(NewText);
    InsertStrings(Index,lines);
}

void SynEditStringList::LoadFromFile(QFile &file, const QByteArray& encoding, QByteArray& realEncoding)
{
    if (!file.open(QFile::ReadOnly | QFile::Text))
        throw FileError(tr("Can't open file '%1' for read!").arg(file.fileName()));
    if (!file.canReadLine())
        throw FileError(tr("Can't read from file '%1'!").arg(file.fileName()));
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });

    //test for utf8 / utf 8 bom
    if (encoding == ENCODING_AUTO_DETECT) {
        QByteArray line = file.readLine();
        QTextCodec* codec;
        QTextCodec::ConverterState * state;
        bool needReread = false;
        bool allAscii = true;
        //test for BOM
        if (line.isEmpty()) {
            realEncoding = ENCODING_ASCII;
            return;
        }
        if ((line.length()>=3) && ((unsigned char)line[0]==0xEF) && ((unsigned char)line[1]==0xBB) && ((unsigned char)line[2]==0xBF) ) {
            realEncoding = ENCODING_UTF8_BOM;
            line = line.mid(3);
            codec = QTextCodec::codecForName(ENCODING_UTF8);
        } else {
            realEncoding = ENCODING_UTF8;
            codec = QTextCodec::codecForName(ENCODING_UTF8);
        }
        clear();
        do {
            if (allAscii) {
                allAscii = isTextAllAscii(line);
            }
            if (allAscii) {
                add(QString::fromLatin1(line));
            } else {
                QString newLine = codec->toUnicode(line.constData(),line.length(),state);
                if (state->invalidChars>0) {
                    needReread = true;
                    break;
                }
                add(newLine);
            }
            line = file.readLine();
        } while (!file.atEnd());
        if (!needReread) {
            if (allAscii)
                realEncoding = ENCODING_ASCII;
            return;
        }
        realEncoding = ENCODING_SYSTEM_DEFAULT;
    } else {
        realEncoding = encoding;
    }

    if (realEncoding == ENCODING_SYSTEM_DEFAULT) {
        realEncoding = QTextCodec::codecForLocale()->name();
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
    clear();
    while (textStream.readLineInto(&line)) {
        add(line);
    }
}



void SynEditStringList::SaveToFile(QFile &file, const QByteArray& encoding, QByteArray& realEncoding)
{
    if (!file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text))
        throw FileError(tr("Can't open file '%1' for save!").arg(file.fileName()));
    if (mList.isEmpty())
        return;
    bool allAscii = true;

    QTextCodec* codec;
    if (realEncoding == ENCODING_UTF8_BOM) {
        codec = QTextCodec::codecForName(ENCODING_UTF8_BOM);
    } else if (realEncoding == ENCODING_ASCII) {
        codec = QTextCodec::codecForLocale();
    }
    for (PSynEditStringRec& line:mList) {
        if (allAscii) {
            allAscii = isTextAllAscii(line->fString);
        }
        if (!allAscii) {
            file.write(codec->fromUnicode(line->fString));
        } else {
            file.write(line->fString.toLatin1());
        }
        switch (mFileEndingType) {
        case FileEndingType::Windows:
            file.write("\r\n");
            break;
        case FileEndingType::Linux:
            file.write("\n");
            break;
        case FileEndingType::Mac:
            file.write("\r");
            break;
        }
    }
    if (encoding == ENCODING_AUTO_DETECT && allAscii) {
        realEncoding = ENCODING_ASCII;
    }
}


void SynEditStringList::PutTextStr(const QString &text)
{
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    clear();
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

FileEndingType SynEditStringList::getFileEndingType() const
{
    return mFileEndingType;
}

void SynEditStringList::setFileEndingType(const FileEndingType &fileEndingType)
{
    mFileEndingType = fileEndingType;
}

SynEditStringRec::SynEditStringRec():
    fString(),
    fObject(nullptr),
    fRange{0,0},
    fExpandedLength(-1),
    fParenthesisLevel(0),
    fBracketLevel(0),
    fBraceLevel(0),
    fFlags(SynEditStringFlag::sfExpandedLengthUnknown)
{
}


SynEditUndoList::SynEditUndoList():QObject()
{
    mMaxUndoActions = 1024;
    mNextChangeNumber = 1;
    mInsideRedo = false;

    mBlockChangeNumber=0;
    mBlockCount=0;
    mFullUndoImposible=false;
    mLockCount = 0;
    mInitialChangeNumber = 0;
}

void SynEditUndoList::AddChange(SynChangeReason AReason, const BufferCoord &AStart,
                                const BufferCoord &AEnd, const QString &ChangeText,
                                SynSelectionMode SelMode)
{
    if (mLockCount != 0)
        return;
    int changeNumber;
    if (mBlockChangeNumber != 0) {
        changeNumber = mBlockChangeNumber;
    } else {
        changeNumber = mNextChangeNumber;
        if (mBlockCount == 0) {
            mNextChangeNumber++;
            if (mNextChangeNumber == 0) {
                mNextChangeNumber++;
            }
        }
    }
    PSynEditUndoItem  NewItem = std::make_shared<SynEditUndoItem>(AReason,
                                                                  SelMode,AStart,AEnd,ChangeText,
                                                                  changeNumber);
    PushItem(NewItem);
}

void SynEditUndoList::AddGroupBreak()
{
    //Add the GroupBreak even if ItemCount = 0. Since items are stored in
    //reverse order in TCustomSynEdit.fRedoList, a GroupBreak could be lost.
    if (LastChangeReason() != SynChangeReason::crGroupBreak) {
        AddChange(SynChangeReason::crGroupBreak, {0,0}, {0,0}, "", SynSelectionMode::smNormal);
    }
}

void SynEditUndoList::BeginBlock()
{
    mBlockCount++;
    mBlockChangeNumber = mNextChangeNumber;
}

void SynEditUndoList::Clear()
{
    mItems.clear();
    mFullUndoImposible = false;
}

void SynEditUndoList::DeleteItem(int index)
{
    if (index <0 || index>=mItems.count()) {
        ListIndexOutOfBounds(index);
    }
    mItems.removeAt(index);
}

void SynEditUndoList::EndBlock()
{
    if (mBlockCount > 0) {
        mBlockCount--;
        if (mBlockCount == 0)  {
            int iBlockID = mBlockChangeNumber;
            mBlockChangeNumber = 0;
            mNextChangeNumber++;
            if (mNextChangeNumber == 0)
                mNextChangeNumber++;
            if (mItems.count() > 0 && PeekItem()->changeNumber() == iBlockID)
                emit addedUndo();
        }
    }
}

SynChangeReason SynEditUndoList::LastChangeReason()
{
    if (mItems.count() == 0)
        return SynChangeReason::crNothing;
    else
        mItems.last()->changeReason();
}

void SynEditUndoList::Lock()
{
    mLockCount++;
}

PSynEditUndoItem SynEditUndoList::PeekItem()
{
    if (mItems.count() == 0)
        return PSynEditUndoItem();
    else
        return mItems.last();
}

PSynEditUndoItem SynEditUndoList::PopItem()
{
    if (mItems.count() == 0)
        return PSynEditUndoItem();
    else {
        PSynEditUndoItem item = mItems.last();
        mItems.removeLast();
        return item;
    }
}

void SynEditUndoList::PushItem(PSynEditUndoItem Item)
{
    if (!Item)
        return;
    mItems.append(Item);
    EnsureMaxEntries();
    if (Item->changeReason()!= SynChangeReason::crGroupBreak)
        addedUndo();
}

void SynEditUndoList::Unlock()
{
    if (mLockCount > 0)
        mLockCount--;
}

bool SynEditUndoList::CanUndo()
{
    return mItems.count()>0;
}

int SynEditUndoList::ItemCount()
{
    return mItems.count();
}

int SynEditUndoList::maxUndoActions() const
{
    return mMaxUndoActions;
}

void SynEditUndoList::setMaxUndoActions(int maxUndoActions)
{
    mMaxUndoActions = maxUndoActions;
}

bool SynEditUndoList::initialState()
{
    if (ItemCount() == 0) {
        return mInitialChangeNumber == 0;
    } else {
        return PeekItem()->changeNumber() == mInitialChangeNumber;
    }
}

PSynEditUndoItem SynEditUndoList::item(int index)
{
    if (index <0 || index>=mItems.count()) {
        ListIndexOutOfBounds(index);
    }
    return mItems[index];
}

void SynEditUndoList::setInitialState(const bool Value)
{
    if (Value) {
        if (ItemCount() == 0)
            mInitialChangeNumber = 0;
        else
            mInitialChangeNumber = PeekItem()->changeNumber();
    } else if (ItemCount() == 0) {
        if (mInitialChangeNumber == 0) {
            mInitialChangeNumber = -1;
        }
    } else if (PeekItem()->changeNumber() == mInitialChangeNumber) {
        mInitialChangeNumber = -1;
    }
}

void SynEditUndoList::setItem(int index, PSynEditUndoItem Value)
{
    if (index <0 || index>=mItems.count()) {
        ListIndexOutOfBounds(index);
    }
    mItems[index]=Value;
}

int SynEditUndoList::blockChangeNumber() const
{
    return mBlockChangeNumber;
}

void SynEditUndoList::setBlockChangeNumber(int blockChangeNumber)
{
    mBlockChangeNumber = blockChangeNumber;
}

int SynEditUndoList::blockCount() const
{
    return mBlockCount;
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

void SynEditUndoList::EnsureMaxEntries()
{
    if (mItems.count() > mMaxUndoActions){
        mFullUndoImposible = true;
        while (mItems.count() > mMaxUndoActions) {
            mItems.removeFirst();
      }
    }
}

SynSelectionMode SynEditUndoItem::changeSelMode() const
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

QString SynEditUndoItem::changeStr() const
{
    return mChangeStr;
}

int SynEditUndoItem::changeNumber() const
{
    return mChangeNumber;
}

SynEditUndoItem::SynEditUndoItem(SynChangeReason reason, SynSelectionMode selMode,
                                 BufferCoord startPos, BufferCoord endPos,
                                 const QString &str, int number)
{
    mChangeReason = reason;
    mChangeSelMode = selMode;
    mChangeStartPos = startPos;
    mChangeEndPos = endPos;
    mChangeStr = str;
    mChangeNumber = number;
}

SynChangeReason SynEditUndoItem::changeReason() const
{
    return mChangeReason;
}
