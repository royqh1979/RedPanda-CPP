#include "Types.h"
#include "SynEdit.h"
#include <QDebug>
ContentsCoord::ContentsCoord(const SynEdit *edit, int ch, int line)
{
    Q_ASSERT(edit!=nullptr);
    mEdit = edit;
    mChar = ch;
    mLine = line;
    normalize();
}

void ContentsCoord::normalize()
{
    if (mEdit->lines()->count()==0) {
        mChar = 0;
        mLine = 0;
        return;
    }
    int aLine = mLine;
    int aChar = mChar;
    int line = aLine-1;
    int lineCount = mEdit->lines()->count();
    if (line>=lineCount) {
        mChar = mEdit->lines()->getString(lineCount-1).length()+1;
        mLine = lineCount;
        return;
    }
    if (line<0) {
        mChar = 0;
        mLine = 0;
        return;
    }
    if (aChar<1) {
        while (true) {
            line--;
            if (line < 0) {
                mChar = 0;
                mLine = 0;
                return;
            }
            QString s = mEdit->lines()->getString(line);
            int len = s.length();
            aChar+=len+1;
            if (aChar>=1) {
                break;
            }
        }
    } else {
        while (true) {
            QString s =mEdit->lines()->getString(line);
            int len = s.length();
            if (aChar<=len+1) {
                break;
            }
            if (line == lineCount-1) {
                mChar = 1;
                mLine = lineCount+1;
                return;
            }
            aChar -= len+1;
            line++;
        }
    }
    mChar = aChar;
    mLine = line+1;
    return;
}

int ContentsCoord::line() const
{
    return mLine;
}

void ContentsCoord::setLine(int newLine)
{
    mLine = newLine;
}

bool ContentsCoord::atStart()
{
    return mLine<1;
}

bool ContentsCoord::atEnd()
{
    Q_ASSERT(mEdit!=nullptr);
    return mLine>mEdit->lines()->count();
}

const SynEdit *ContentsCoord::edit() const
{
    return mEdit;
}

const ContentsCoord &ContentsCoord::operator=(const ContentsCoord &coord)
{
    mEdit = coord.mEdit;
    mChar = coord.mChar;
    mLine = coord.mLine;
    return *this;
}

const ContentsCoord &ContentsCoord::operator=(const ContentsCoord &&coord)
{
    if (this!=&coord) {
        mEdit = coord.mEdit;
        mChar = coord.mChar;
        mLine = coord.mLine;
    }
    return *this;
}

bool ContentsCoord::operator==(const ContentsCoord &coord) const
{
    Q_ASSERT(mEdit == coord.mEdit);
    return (mLine == coord.mLine)
            && (mChar == coord.mChar);
}

bool ContentsCoord::operator<(const ContentsCoord &coord) const
{
    Q_ASSERT(mEdit == coord.mEdit);
    return (mLine < coord.mLine) || (mLine == coord.mLine && mChar < coord.mChar);
}

bool ContentsCoord::operator<=(const ContentsCoord &coord) const
{
    Q_ASSERT(mEdit == coord.mEdit);
    return (mLine < coord.mLine) || (mLine == coord.mLine && mChar <= coord.mChar);
}

bool ContentsCoord::operator>(const ContentsCoord &coord) const
{
    Q_ASSERT(mEdit == coord.mEdit);
    return (mLine > coord.mLine) || (mLine == coord.mLine && mChar > coord.mChar);
}

bool ContentsCoord::operator>=(const ContentsCoord &coord) const
{
    Q_ASSERT(mEdit == coord.mEdit);
    return (mLine > coord.mLine) || (mLine == coord.mLine && mChar >= coord.mChar);
}

size_t ContentsCoord::operator-(const ContentsCoord& coord) const
{
    Q_ASSERT(mEdit == coord.mEdit);
    if (mLine == coord.mLine) {
        return mChar - coord.mChar;
    } else if (mLine > coord.mLine) {
        size_t result = mEdit->lines()->getString(coord.mLine-1).length()+1-coord.mChar;
        int line = coord.mLine+1;
        while (line<=mLine-1) {
            result += mEdit->lines()->getString(line-1).length()+1;
            line++;
        }
        if (mLine<=mEdit->lines()->count()) {
            result += mChar;
        }
        return result;
    } else {
        return coord - (*this);
    }
}

const ContentsCoord &ContentsCoord::operator+=(int delta)
{
    mChar+=delta;
    normalize();
    return *this;
}

const ContentsCoord &ContentsCoord::operator-=(int delta)
{
    mChar-=delta;
    normalize();
    return *this;
}

BufferCoord ContentsCoord::toBufferCoord() const
{
    return BufferCoord{mChar,mLine};
}

ContentsCoord ContentsCoord::operator-(int delta) const
{
    Q_ASSERT(mEdit != nullptr);
    return ContentsCoord(mEdit,mChar-delta,mLine);
}

ContentsCoord ContentsCoord::operator+(int delta) const
{
    Q_ASSERT(mEdit != nullptr);
    return ContentsCoord(mEdit,mChar+delta,mLine);
}

QChar ContentsCoord::operator*() const
{
    Q_ASSERT(mEdit != nullptr);
    if (mLine < 1) {
        return QChar('\0');
    }
    if (mLine > mEdit->lines()->count()) {
        return QChar('\0');
    }
    QString s = mEdit->lines()->getString(mLine-1);
    if (mChar >= s.length()+1 ) {
        return QChar('\n');
    }
    //qDebug()<<mLine<<":"<<mChar<<" '"<<s<<"'";
    return s[mChar-1];
}

ContentsCoord::ContentsCoord()
{
    mEdit = nullptr;
    mLine = 0;
    mEdit = 0;
}

ContentsCoord::ContentsCoord(const ContentsCoord &coord):
    ContentsCoord(coord.mEdit,
                          coord.mChar,
                          coord.mLine)
{
}

int ContentsCoord::ch() const
{
    return mChar;
}

void ContentsCoord::setCh(int newChar)
{
    mChar = newChar;
}

bool BufferCoord::operator==(const BufferCoord &coord)
{
    return coord.Char == Char && coord.Line == Line;
}

bool BufferCoord::operator>=(const BufferCoord &coord)
{
    return (Line > coord.Line)
            || (Line == coord.Line && Char >= coord.Char);
}

bool BufferCoord::operator>(const BufferCoord &coord)
{
    return (Line > coord.Line)
            || (Line == coord.Line && Char > coord.Char);
}

bool BufferCoord::operator<(const BufferCoord &coord)
{
    return (Line < coord.Line)
            || (Line == coord.Line && Char < coord.Char);
}

bool BufferCoord::operator<=(const BufferCoord &coord)
{
    return (Line < coord.Line)
            || (Line == coord.Line && Char <= coord.Char);
}

bool BufferCoord::operator!=(const BufferCoord &coord)
{
    return coord.Char != Char || coord.Line != Line;
}
