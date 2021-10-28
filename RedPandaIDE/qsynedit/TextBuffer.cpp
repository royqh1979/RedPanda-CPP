#include "TextBuffer.h"
#include <QDataStream>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <QMutexLocker>
#include <stdexcept>
#include "SynEdit.h"
#include "../utils.h"
#include "../platform.h"
#include <QMessageBox>

SynEditStringList::SynEditStringList(SynEdit *pEdit, QObject *parent):
      QObject(parent),
      mEdit(pEdit)
{
    mAppendNewLineAtEOF = true;
    mFileEndingType = FileEndingType::Windows;
    mIndexOfLongestLine = -1;
    mUpdateCount = 0;
}

static void ListIndexOutOfBounds(int index) {
    throw IndexOutOfRange(index);
}



int SynEditStringList::parenthesisLevels(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mList.size()) {
        return mList[Index]->fRange.parenthesisLevel;
    } else
        return 0;
}

int SynEditStringList::bracketLevels(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mList.size()) {
        return mList[Index]->fRange.bracketLevel;
    } else
        return 0;
}

int SynEditStringList::braceLevels(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mList.size()) {
        return mList[Index]->fRange.braceLevel;
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

int SynEditStringList::lineColumns(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mList.size()) {
        if (mList[Index]->fColumns == -1) {
            return calculateLineColumns(Index);
        } else
            return mList[Index]->fColumns;
    } else
        return 0;
}

int SynEditStringList::leftBraces(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mList.size()) {
        return mList[Index]->fRange.leftBraces;
    } else
        return 0;
}

int SynEditStringList::rightBraces(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mList.size()) {
        return mList[Index]->fRange.rightBraces;
    } else
        return 0;
}

int SynEditStringList::lengthOfLongestLine() {
    QMutexLocker locker(&mMutex);
    if (mIndexOfLongestLine < 0) {
        int MaxLen = -1;
        mIndexOfLongestLine = -1;
        if (mList.count() > 0 ) {
            for (int i=0;i<mList.size();i++) {
                int len = lineColumns(i);
                if (len > MaxLen) {
                    MaxLen = len;
                    mIndexOfLongestLine = i;
                }
            }
        }
    }
    if (mIndexOfLongestLine >= 0)
        return mList[mIndexOfLongestLine]->fColumns;
    else
        return 0;
}

QString SynEditStringList::lineBreak() const
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

SynRangeState SynEditStringList::ranges(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index>=0 && Index < mList.size()) {
        return mList[Index]->fRange;
    } else {
         ListIndexOutOfBounds(Index);
    }
    return {0};
}

void SynEditStringList::insertItem(int Index, const QString &s)
{
    beginUpdate();
    PSynEditStringRec line = std::make_shared<SynEditStringRec>();
    line->fString = s;
    mIndexOfLongestLine = -1;
    mList.insert(Index,line);
    endUpdate();
}

void SynEditStringList::addItem(const QString &s)
{
    beginUpdate();
    PSynEditStringRec line = std::make_shared<SynEditStringRec>();
    line->fString = s;
    mIndexOfLongestLine = -1;
    mList.append(line);
    endUpdate();
}

bool SynEditStringList::getAppendNewLineAtEOF()
{
    QMutexLocker locker(&mMutex);
    return mAppendNewLineAtEOF;
}

void SynEditStringList::setAppendNewLineAtEOF(bool appendNewLineAtEOF)
{
    QMutexLocker locker(&mMutex);
    mAppendNewLineAtEOF = appendNewLineAtEOF;
}

void SynEditStringList::setRange(int Index, const SynRangeState& ARange)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>=mList.count()) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    mList[Index]->fRange = ARange;
    endUpdate();
}

QString SynEditStringList::getString(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>=mList.count()) {
        return QString();
    }
    return mList[Index]->fString;
}

int SynEditStringList::count()
{
    QMutexLocker locker(&mMutex);
    return mList.count();
}

void *SynEditStringList::getObject(int Index)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>=mList.count()) {
        return nullptr;
    }
    return mList[Index]->fObject;
}

QString SynEditStringList::text()
{
    QMutexLocker locker(&mMutex);
    return getTextStr();
}

void SynEditStringList::setText(const QString &text)
{
    QMutexLocker locker(&mMutex);
    putTextStr(text);
}

void SynEditStringList::setContents(const QStringList &text)
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    internalClear();
    if (text.count() > 0) {
        mIndexOfLongestLine = -1;
        int FirstAdded = mList.count();

        foreach (const QString& s,text) {
            addItem(s);
        }
        emit inserted(FirstAdded,text.count());
    }
}

QStringList SynEditStringList::contents()
{
    QMutexLocker locker(&mMutex);
    QStringList Result;
    SynEditStringRecList list = mList;
    foreach (const PSynEditStringRec& line, list) {
        Result.append(line->fString);
    }
    return Result;
}

void SynEditStringList::beginUpdate()
{
    if (mUpdateCount == 0) {
        setUpdateState(true);
    }
    mUpdateCount++;
}

void SynEditStringList::endUpdate()
{
    mUpdateCount--;
    if (mUpdateCount == 0) {
        setUpdateState(false);
    }
}


int SynEditStringList::add(const QString &s)
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    int Result = mList.count();
    insertItem(Result, s);
    emit inserted(Result,1);
    endUpdate();
    return Result;
}

void SynEditStringList::addStrings(const QStringList &Strings)
{
    QMutexLocker locker(&mMutex);
    if (Strings.count() > 0) {
        mIndexOfLongestLine = -1;
        beginUpdate();
        auto action = finally([this]{
            endUpdate();
        });
        int FirstAdded = mList.count();

        for (const QString& s:Strings) {
            addItem(s);
        }
        emit inserted(FirstAdded,Strings.count());
    }
}

int SynEditStringList::getTextLength()
{
    QMutexLocker locker(&mMutex);
    int Result = 0;
    foreach (const PSynEditStringRec& line, mList ) {
        Result += line->fString.length();
        if (mFileEndingType == FileEndingType::Windows) {
            Result += 2;
        } else {
            Result += 1;
        }
    }
    return Result;
}

void SynEditStringList::clear()
{
    QMutexLocker locker(&mMutex);
    internalClear();
}

void SynEditStringList::deleteLines(int Index, int NumLines)
{
    QMutexLocker locker(&mMutex);
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

void SynEditStringList::exchange(int Index1, int Index2)
{
    QMutexLocker locker(&mMutex);
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

void SynEditStringList::insert(int Index, const QString &s)
{
    QMutexLocker locker(&mMutex);
    if ((Index < 0) || (Index > mList.count())) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    insertItem(Index, s);
    emit inserted(Index,1);
    endUpdate();
}

void SynEditStringList::deleteAt(int Index)
{
    QMutexLocker locker(&mMutex);
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

QString SynEditStringList::getTextStr() const
{
    QString result;
    for (int i=0;i<mList.count()-1;i++) {
        const PSynEditStringRec& line = mList[i];
        result.append(line->fString);
        result.append(lineBreak());
    }
    if (mList.length()>0) {
        result.append(mList.back()->fString);
    }
    return result;
}

void SynEditStringList::putString(int Index, const QString &s) {
    QMutexLocker locker(&mMutex);
    if (Index == mList.count()) {
        add(s);
    } else {
        if (Index<0 || Index>=mList.count()) {
            ListIndexOutOfBounds(Index);
        }
        beginUpdate();
        mIndexOfLongestLine = -1;
        mList[Index]->fString = s;
        mList[Index]->fColumns = -1;
        emit putted(Index,1);
        endUpdate();
    }
}

void SynEditStringList::putObject(int Index, void *AObject)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>=mList.count()) {
        ListIndexOutOfBounds(Index);
    }
    beginUpdate();
    mList[Index]->fObject = AObject;
    endUpdate();
}

void SynEditStringList::setUpdateState(bool Updating)
{
    if (Updating)
        emit changing();
    else
        emit changed();
}

int SynEditStringList::calculateLineColumns(int Index)
{
    PSynEditStringRec line = mList[Index];

    line->fColumns = mEdit->stringColumns(line->fString,0);
    return line->fColumns;
}

void SynEditStringList::insertLines(int Index, int NumLines)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>mList.count()) {
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

void SynEditStringList::insertStrings(int Index, const QStringList &NewStrings)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>mList.count()) {
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

void SynEditStringList::insertText(int Index, const QString &NewText)
{
    QMutexLocker locker(&mMutex);
    if (Index<0 || Index>=mList.count()) {
        ListIndexOutOfBounds(Index);
    }
    if (NewText.isEmpty())
        return;
    QStringList lines = TextToLines(NewText);
    insertStrings(Index,lines);
}

void SynEditStringList::loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding)
{
    QMutexLocker locker(&mMutex);
    QFile file(filename);
    if (!file.open(QFile::ReadOnly ))
        throw FileError(tr("Can't open file '%1' for read!").arg(file.fileName()));
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
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
            if (allAscii) {
                allAscii = isTextAllAscii(line);
            }
            if (allAscii) {
                addItem(TrimRight(QString::fromLatin1(line)));
            } else {
                QString newLine = codec->toUnicode(line.constData(),line.length(),&state);
                if (state.invalidChars>0) {
                    needReread = true;
                    break;
                }
                addItem(TrimRight(newLine));
            }
            if (file.atEnd()){
                break;
            }
            line = file.readLine();
        }
        emit inserted(0,mList.count());
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
        addItem(TrimRight(line));
    }
    emit inserted(0,mList.count());
}



void SynEditStringList::saveToFile(QFile &file, const QByteArray& encoding,
                                   const QByteArray& defaultEncoding, QByteArray& realEncoding)
{
    QMutexLocker locker(&mMutex);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        throw FileError(tr("Can't open file '%1' for save!").arg(file.fileName()));
    if (mList.isEmpty())
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
    for (PSynEditStringRec& line:mList) {
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

void SynEditStringList::putTextStr(const QString &text)
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

void SynEditStringList::internalClear()
{
    if (!mList.isEmpty()) {
        beginUpdate();
        int oldCount = mList.count();
        mIndexOfLongestLine = -1;
        mList.clear();
        emit deleted(0,oldCount);
        endUpdate();
    }
}

FileEndingType SynEditStringList::getFileEndingType()
{
    QMutexLocker locker(&mMutex);
    return mFileEndingType;
}

void SynEditStringList::setFileEndingType(const FileEndingType &fileEndingType)
{
    QMutexLocker locker(&mMutex);
    mFileEndingType = fileEndingType;
}

bool SynEditStringList::empty()
{
    QMutexLocker locker(&mMutex);
    return mList.count()==0;
}

void SynEditStringList::resetColumns()
{
    QMutexLocker locker(&mMutex);
    mIndexOfLongestLine = -1;
    if (mList.count() > 0 ) {
        for (int i=0;i<mList.size();i++) {
            mList[i]->fColumns = -1;
        }
    }
}

void SynEditStringList::invalidAllLineColumns()
{
    QMutexLocker locker(&mMutex);
    mIndexOfLongestLine = -1;
    for (PSynEditStringRec& line:mList) {
        line->fColumns = -1;
    }
}

SynEditStringRec::SynEditStringRec():
    fString(),
    fObject(nullptr),
    fRange{0,0,0,0,0},
    fColumns(-1)
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
        return mItems.last()->changeReason();
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
        emit addedUndo();
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
