#include "TextBuffer.h"
#include <stdexcept>
#include "../utils.h"

SynEditStringList::SynEditStringList()
{
    mAppendNewLineAtEOF = true;
    mFileFormat = SynEditFileFormat::Windows;
    mIndexOfLongestLine = -1;
    mUpdateCount = 0;
    setTabWidth(8);
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
    PSynEditStringRec line;
    line->fString=s;
    line->fObject = nullptr;
    line->fRange = {0,0};
    line->fExpandedLength = -1;
    line->fParenthesisLevel = 0;
    line->fBracketLevel = 0;
    line->fBraceLevel = 0;
    line->fFlags = SynEditStringFlag::sfExpandedLengthUnknown;
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
        throw new std::out_of_range(QObject::tr("Index % s out of range").arg(Index).toLocal8Bit());
    }
    beginUpdate();
    mList[Index]->fRange = ARange;
    endUpdate();
}

void SynEditStringList::setParenthesisLevel(int Index, int level)
{
    if (Index<0 || Index>=mList.count()) {
        throw new std::out_of_range(QObject::tr("Index % s out of range").arg(Index).toLocal8Bit());
    }
    beginUpdate();
    mList[Index]->fParenthesisLevel = level;
    endUpdate();
}

void SynEditStringList::setBracketLevel(int Index, int level)
{
    if (Index<0 || Index>=mList.count()) {
        throw new std::out_of_range(QObject::tr("Index % s out of range").arg(Index).toLocal8Bit());
    }
    beginUpdate();
    mList[Index]->fBracketLevel = level;
    endUpdate();
}

void SynEditStringList::setBraceLevel(int Index, int level)
{
    if (Index<0 || Index>=mList.count()) {
        throw new std::out_of_range(QObject::tr("Index % s out of range").arg(Index).toLocal8Bit());
    }
    beginUpdate();
    mList[Index]->fBraceLevel = level;
    endUpdate();
}

QString SynEditStringList::get(int Index)
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

void SynEditStringList::beginUpdate()
{
    if (mUpdateCount == 0) {
        SetUpdateState(true);
    }
    mUpdateCount++;
}

void SynEditStringList::endUpdate()
{
    FUpdateCount--;
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

int SynEditStringList::Add(const QString &s)
{
    beginUpdate();
    int Result = mList.count();
    InsertItem(Result, s);
    emit inserted(Result,1);
    endUpdate();
    return Result;
}

int SynEditStringList::AddStrings(const QStringList &Strings)
{
    if (Strings.count() > 0) {
        mIndexOfLongestLine = -1;
        beginUpdate();
        auto action = finally([this]{
            endUpdate();
        });
        int FirstAdded = mList.count();

        for (const QString& s:Strings) {
            Add(s);
        }
        emit inserted(FirstAdded,Strings.count());
    }
}

int SynEditStringList::getTextLength()
{
    int Result = 0;
    for (const PSynEditStringRec& line: mList ) {
        Result += line->fString.length();
        if (mFileFormat == SynEditFileFormat::Windows) {
            Result += 2;
        } else {
            Result += 1;
        }
    }
}

void SynEditStringList::Clear()
{
    if (!mList.isEmpty()) {
        beginUpdate();
        mIndexOfLongestLine = -1;
        mList.clear();
        endUpdate();
    }
}

void SynEditStringList::Delete(int Index)
{
    if ((Index < 0) || (Index >= mList.count())) {
        throw new std::out_of_range(QObject::tr("Index % s out of range").arg(Index).toLocal8Bit());
    }
    beginUpdate();
    mList.removeAt(Index);
    mIndexOfLongestLine = -1;
    emit deleted(Index,1);
    endUpdate();
}

QString SynEditStringList::GetTextStr()
{
    QString Result;
    for (PSynEditStringRec& line:mList) {
        Result.append(line->fString);
        switch(mFileFormat) {
        case SynEditFileFormat::Linux:
            Result.append('\n');
        case SynEditFileFormat::Windows:
            Result.append("\r\n");
        case SynEditFileFormat::Mac:
            Result.append("\r");
    }
    return Result;
}

void SynEditStringList::put(int Index, const QString &s)
{
    if (Index == mList.count()) {
        Add(s);
    } else {
        if (Index<0 || Index>=mList.count()) {
            throw new std::out_of_range(QObject::tr("Index % s out of range").arg(Index).toLocal8Bit());
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
        throw new std::out_of_range(QObject::tr("Index % s out of range").arg(Index).toLocal8Bit());
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
