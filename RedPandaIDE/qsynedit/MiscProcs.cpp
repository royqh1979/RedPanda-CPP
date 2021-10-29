#include "MiscProcs.h"
#include <QFile>
#include <QPainter>
#include <QTextStream>
#include <algorithm>

int minMax(int x, int mi, int ma)
{
    x = std::min(x, ma );
    return std::max( x, mi );
}

void swapInt(int &l, int &r)
{
    int tmp = r;
    r = l;
    l = tmp;
}

QPoint maxPoint(const QPoint &P1, const QPoint &P2)
{
    if ( (P2.y() > P1.y()) || ( (P2.y() == P1.y()) && (P2.x() > P1.x())) ) {
      return P2;
    } else {
      return P1;
    }
}

QPoint minPoint(const QPoint &P1, const QPoint &P2)
{
    if ( (P2.y() < P1.y()) || ( (P2.y() == P1.y()) && (P2.x() < P1.x())) ) {
      return P2;
    } else {
      return P1;
    }
}

PIntArray getIntArray(size_t Count, int InitialValue)
{
    return std::make_shared<IntArray>(Count,InitialValue);
}

void internalFillRect(QPainter *painter, const QRect &rcPaint, const QColor& color)
{
    painter->fillRect(rcPaint,color);
}


bool IsPowerOfTwo(int TabWidth) {
    if (TabWidth<2)
        return false;
    int nW = 2;
    do {
        if (nW >= TabWidth)
            break;
        nW <<= 1;
    } while (nW<0x10000);
    return (nW == TabWidth);
}

QString ConvertTabs1Ex(const QString &Line, int TabWidth, bool &HasTabs) {
    QString Result = Line;  // increment reference count only
    int nBeforeTab;
    if (GetHasTabs(Line, nBeforeTab)) {
        QChar* pDest;
        HasTabs = true;
        pDest = Result.data()+nBeforeTab+1;
        // this will make a copy of Line
        // We have at least one tab in the string, and the tab width is 1.
        // pDest points to the first tab char. We overwrite all tabs with spaces.
        while (*pDest!=0) {
            if (*pDest == '\t') {
                *pDest = ' ';
            };
            pDest++;
        }
    } else
        HasTabs = false;
    return Result;
}

QString ConvertTabs1(const QString &Line, int TabWidth) {
    bool HasTabs;
    return ConvertTabs1Ex(Line, TabWidth, HasTabs);
}

QString ConvertTabs2nEx(const QString &Line, int TabWidth, bool &HasTabs) {
    QString Result = Line;  // increment reference count only
    int DestLen;
    if (GetHasTabs(Line, DestLen)) {
        HasTabs = true;
        int pSrc = 1 + DestLen;
        // We have at least one tab in the string, and the tab width equals 2^n.
        // pSrc points to the first tab char in Line. We get the number of tabs
        // and the length of the expanded string now.
        int TabCount = 0;
        int TabMask = (TabWidth - 1) ^ 0x7FFFFFFF;
        do {
            if (Line[pSrc] == '\t') {
                DestLen =  (DestLen + TabWidth) & TabMask;
                TabCount++;
            } else
                DestLen ++ ;
        } while (pSrc < Line.length());
        // Set the length of the expanded string.
        Result.resize(DestLen);
        DestLen = 0;
        pSrc = 0;
        QChar * pDest = Result.data();
        // We use another TabMask here to get the difference to 2^n.
        TabMask = TabWidth - 1;
        do {
            if (Line[pSrc] == '\t') {
                int i = TabWidth - (DestLen & TabMask);
                DestLen += i;
                //This is used for both drawing and other stuff and is meant to be #9 and not #32
                do {
                    *pDest = '\t';
                    pDest ++ ;
                    i--;
                } while (i > 0);
                TabCount -- ;
                if (TabCount == 0) {
                    do {
                        pSrc++ ;
                        *pDest = Line[pSrc];
                        pDest++;
                    } while (pSrc < Line.length());
                    return Result;
                }
            } else {
                *pDest = Line[pSrc];
                pDest ++ ;
                DestLen ++;
            }
            pSrc++;
        } while (pSrc < Line.length());

    } else
        HasTabs = false;
    return Result;
}

QString ConvertTabs2n(const QString &Line, int TabWidth) {
    bool HasTabs;
    return ConvertTabs2nEx(Line, TabWidth, HasTabs);
}

ConvertTabsProc GetBestConvertTabsProc(int TabWidth)
{
    if (TabWidth < 2)
        return &ConvertTabs1;
    else if (IsPowerOfTwo(TabWidth))
        return &ConvertTabs2n;
    else
        return &ConvertTabs;
}

QString ConvertTabs(const QString &Line, int TabWidth)
{
    bool HasTabs;
    return ConvertTabsEx(Line, TabWidth, HasTabs);
}

ConvertTabsProcEx GetBestConvertTabsProcEx(int TabWidth)
{
    if (TabWidth < 2)
        return &ConvertTabs1Ex;
    else if (IsPowerOfTwo(TabWidth))
        return &ConvertTabs2nEx;
    else
        return &ConvertTabsEx;
}

QString ConvertTabsEx(const QString &Line, int TabWidth, bool &HasTabs)
{
    QString Result = Line;  // increment reference count only
    int DestLen;
    int pSrc;
    QChar* pDest;
    if (GetHasTabs(Line, DestLen)) {
        HasTabs = true;
        pSrc = (DestLen+1);
        // We have at least one tab in the string, and the tab width is greater
        // than 1. pSrc points to the first tab char in Line. We get the number
        // of tabs and the length of the expanded string now.
        int TabCount = 0;
        do {
            if (Line[pSrc] == '\t') {
                DestLen = DestLen + TabWidth - DestLen % TabWidth;
                TabCount++;
            } else {
                DestLen ++;
            }
            pSrc++;
        } while (pSrc<Line.length());
        // Set the length of the expanded string.
        Result.resize(DestLen);
        DestLen = 0;
        pSrc = 0;
        pDest = Result.data();
        do {
            if (Line[pSrc] == '\t') {
                int i = TabWidth - (DestLen % TabWidth);
                DestLen+=i;
                do {
                    *pDest = '\t';
                    pDest++;
                    i--;
                } while (i != 0);
                TabCount--;
                if (TabCount == 0) {
                    do {
                        pSrc++;
                        *pDest = Line[pSrc];
                        pDest++;
                    } while (pSrc<Line.length());
                    return Result;
                }
            } else {
                *pDest = Line[pSrc];
                pDest++;
                DestLen++;
            }
            pSrc++;
        } while (pSrc<Line.length());
    } else
        HasTabs = false;
    return Result;
}

bool GetHasTabs(const QString &line, int &CharsBefore)
{
    bool result = false;
    CharsBefore = 0;
    if (!line.isEmpty()) {
        for (const QChar& ch:line) {
            if (ch == '\t') {
                result = true;
                break;
            }
            CharsBefore ++;
        }
    }
    return result;
}

int GetExpandedLength(const QString &aStr, int aTabWidth)
{
    int Result = 0;
    for (const QChar& ch : aStr) {
        if (ch == '\t') {
            Result += aTabWidth - (Result % aTabWidth);
        } else {
            Result ++;
        }
    }
    return Result;
}

int CharIndex2CaretPos(int Index, int TabWidth, const QString &Line)
{
    int Result;
    int iChar;
    if (Index > 1) {
        if ((TabWidth <= 1) || !GetHasTabs(Line, iChar) ) {
            Result = Index;
        } else {
            if (iChar + 1 >= Index) {
                Result = Index;
            } else {
                // iChar is number of chars before first Tab
                Result = iChar;
                // Index is *not* zero-based
                iChar++;
                Index -= iChar;
                int pNext = iChar;
                while (Index > 0) {
                    if (pNext>=Line.length()) {
                        Result += Index;
                        break;
                    }
                    if (Line[pNext] == '\t') {
                        Result += TabWidth;
                        Result -= Result % TabWidth;
                    } else
                        Result++;
                    Index--;
                    pNext++;
                }
                // done with zero-based computation
                Result++;
            }
        }
    } else
        Result = 1;
    return Result;
}

int CaretPos2CharIndex(int Position, int TabWidth, const QString &Line, bool &InsideTabChar)
{
    int Result;
    int iPos;
    InsideTabChar = false;
    if (Position > 1) {
        if ( (TabWidth <= 1) || !GetHasTabs(Line, iPos) ) {
            Result = Position;
        } else {
            if (iPos + 1 >= Position) {
                Result = Position;
            } else {
                // iPos is number of chars before first #9
                Result = iPos + 1;
                int pNext = Result;
                // for easier computation go zero-based (mod-operation)
                Position -=1;
                while (iPos < Position) {
                    if (pNext>=Line.length())
                        break;
                    if (Line[pNext] == '\t') {
                        iPos+=TabWidth;
                        iPos-=iPos % TabWidth;
                        if (iPos > Position) {
                          InsideTabChar = true;
                          break;
                        }
                    } else
                        iPos++;
                    Result++;
                    pNext++;
                }
            }
        }
    } else
        Result = Position;
    return Result;
}

int StrScanForCharInSet(const QString &Line, int Start, const QSet<QChar>& AChars)
{
    for (int i=Start;i<Line.length();i++) {
        if (AChars.contains(Line[i])) {
            return i;
        }
    }
    return -1;
}

int StrRScanForCharInSet(const QString &Line, int Start, const QSet<QChar> &AChars)
{
    for (int i=Line.size()-1;i>=Start;i--) {
        if (AChars.contains(Line[i])) {
            return i;
        }
    }
    return -1;
}

int GetEOL(const QString &Line, int start)
{
    if (start<0 || start>=Line.size()) {
        return start;
    }
    for (int i=start;i<Line.size();i++) {
        if (Line[i] == '\n' || Line[i] == '\r') {
            return i;
        }
    }
    return Line.size();
}

QString EncodeString(const QString &s)
{
    QString Result;
    Result.resize(s.length()*2); // worst case
    int j=0;
    for (const QChar& ch: s) {
        if (ch == '\\' ) {
            Result[j] = '\\';
            Result[j+1] = '\\';
            j+=2;
        } else if (ch == '/') {
            Result[j] = '\\';
            Result[j+1] = '.';
            j+=2;
        } else {
            Result[j]=ch;
            j+=1;
        }
    }
    Result.resize(j);
    return Result;
}

QString DecodeString(const QString &s)
{
    QString Result;
    Result.resize(s.length()); // worst case
    int j = 0;
    int i = 0;
    while (i < s.length()) {
        if (i<s.length()-1 && s[i] == '\\' ) {
            if (s[i+1] == '\\') {
                Result[j]= '\\';
                i+=2;
                j+=1;
                continue;
            } else if (s[i+1] == '.') {
                Result[j]= '/';
                i+=2;
                j+=1;
                continue;
            }
        }
        Result[j] = Result[i];
        i+=1;
        j+=1;
    }
    Result.resize(j);
    return Result;
}

bool InternalEnumHighlighterAttris(PSynHighlighter Highlighter,
                                   bool SkipDuplicates,
                                   HighlighterAttriProc highlighterAttriProc,
                                   std::initializer_list<void *>& Params,
                                   SynHighlighterList& HighlighterList) {
    bool Result = true;
    if (HighlighterList.indexOf(Highlighter)>0) {
        if (SkipDuplicates)
            return Result;
    } else {
        HighlighterList.append(Highlighter);
    }
    if (Highlighter->getClass() == SynHighlighterClass::Composition) {
        //todo: handle composition highlighter
    } else if (Highlighter) {
        for (PSynHighlighterAttribute pAttr: Highlighter->attributes()){
            QString UniqueAttriName = Highlighter->getName()
                    +  QString("%1").arg(HighlighterList.indexOf(Highlighter)) + '.'
                    + pAttr->name();
            Result = highlighterAttriProc(Highlighter, pAttr,
                                          UniqueAttriName, Params);
            if (!Result)
                break;
        }
    }
    return Result;
}

bool EnumHighlighterAttris(PSynHighlighter Highlighter, bool SkipDuplicates,
                           HighlighterAttriProc highlighterAttriProc,
                           std::initializer_list<void *> Params)
{
    if (!Highlighter || !highlighterAttriProc) {
        return false;
    }

    SynHighlighterList HighlighterList;
    return InternalEnumHighlighterAttris(Highlighter, SkipDuplicates,
        highlighterAttriProc, Params, HighlighterList);
}

uint16_t fcstab[] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

uint16_t CalcFCS(unsigned char *ABuf, int ABufSize)
{
    uint16_t CurFCS = 0xffff;
    unsigned char* P = ABuf;
    while (ABufSize>0) {
        CurFCS = (CurFCS >> 8) ^ fcstab[(CurFCS ^ *P) & 0xff];
        ABufSize -- ;
        P ++ ;
    }
    return CurFCS;
}

void SynDrawGradient(QPaintDevice *ACanvas, const QColor &AStartColor, const QColor &AEndColor, int , const QRect &ARect, bool AHorizontal)
{
    QPainter painter(ACanvas);
    if (AHorizontal) {
        int Size = ARect.right() - ARect.left();
        QLinearGradient gradient(0,0,Size,0);
        gradient.setColorAt(0,AStartColor);
        gradient.setColorAt(1,AEndColor);
        painter.fillRect(ARect,gradient);
    } else {
        int Size = ARect.bottom() - ARect.top();
        QLinearGradient gradient(0,0,0,Size);
        gradient.setColorAt(0,AStartColor);
        gradient.setColorAt(1,AEndColor);
        painter.fillRect(ARect,gradient);
    }
}

int mulDiv(int a, int b, int c)
{
    //todo: handle overflow?
    return a*b/c;
}

SynFontStyles getFontStyles(const QFont &font)
{
    SynFontStyles styles;
    styles.setFlag(SynFontStyle::fsBold, font.bold());
    styles.setFlag(SynFontStyle::fsItalic, font.italic());
    styles.setFlag(SynFontStyle::fsUnderline, font.underline());
    styles.setFlag(SynFontStyle::fsStrikeOut, font.strikeOut());
    return styles;
}

bool isWordChar(const QChar& ch) {
    return (ch == '_') || ch.isLetterOrNumber();
}

int StrScanForWordChar(const QString &s, int startPos)
{
    for (int i=startPos-1;i<s.length();i++) {
        if (isWordChar(s[i])) {
            return i+1;
        }
    }
    return 0;
}

int StrScanForNonWordChar(const QString &s, int startPos)
{
    for (int i=startPos-1;i<s.length();i++) {
        if (!isWordChar(s[i])) {
            return i+1;
        }
    }
    return 0;
}

int StrRScanForWordChar(const QString &s, int startPos)
{
    int i = startPos-1;
    while (i>=0) {
        if (isWordChar(s[i]))
            return i+1;
        i--;
    }
    return 0;
}

int StrRScanForNonWordChar(const QString &s, int startPos)
{
    int i = startPos-1;
    while (i>=0) {
        if (!isWordChar(s[i]))
            return i+1;
        i--;
    }
    return 0;
}

int CountLines(const QString &Line, int start)
{
    int Result = 0;
    int i=start;
    while (i<Line.length()) {
        if (Line[i]=='\r')
            i++;
        if (Line[i]=='\n')
            i++;
        Result ++ ;
        i = GetEOL(Line,i);
    }
    return Result;
}

void ensureNotAfter(BufferCoord &cord1, BufferCoord &cord2)
{
    if((cord1.Line > cord2.Line) || (
                cord1.Line == cord2.Line &&
                cord1.Char > cord2.Char)) {
        std::swap(cord1,cord2);
    }
}

BufferCoord minBufferCoord(const BufferCoord &P1, const BufferCoord &P2)
{
    if ( (P2.Line < P1.Line) || ( (P2.Line == P1.Line) && (P2.Char < P1.Char)) ) {
      return P2;
    } else {
      return P1;
    }
}

BufferCoord maxBufferCoord(const BufferCoord &P1, const BufferCoord &P2)
{
    if ( (P2.Line > P1.Line) || ( (P2.Line == P1.Line) && (P2.Char > P1.Char)) ) {
      return P2;
    } else {
      return P1;
    }
}
