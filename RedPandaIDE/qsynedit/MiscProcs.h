#ifndef MISCPROCS_H
#define MISCPROCS_H
#include <QPoint>
#include <vector>
#include <memory>
#include <QString>
#include <QSet>
#include "highlighter/base.h"
#include <QPaintDevice>
#include <QTextStream>
#include <QVector>
#include <initializer_list>
//#include <QRect>
//#include <QColor>

class QPainter;
class QRect;
class QColor;

using IntArray = QVector<int>;
using PIntArray = std::shared_ptr<IntArray>;

int MinMax(int x, int mi, int ma);
int MulDiv(int a, int b, int c);
void SwapInt(int& l, int &r);
QPoint MaxPoint(const QPoint& P1, const QPoint& P2);
QPoint MinPoint(const QPoint& P1, const QPoint& P2);

PIntArray GetIntArray(size_t Count, int InitialValue);

void InternalFillRect(QPainter* painter, const QRect& rcPaint, const QColor& color);

// Converting tabs to spaces: To use the function several times it's better
// to use a function pointer that is set to the fastest conversion function.
using ConvertTabsProc = std::function<QString(const QString&, int)>;

ConvertTabsProc GetBestConvertTabsProc(int TabWidth);

// This is the slowest conversion function which can handle TabWidth <> 2^n.
QString ConvertTabs(const QString& Line, int TabWidth);

using ConvertTabsProcEx = std::function<QString(const QString&, int, bool& )>;

ConvertTabsProcEx GetBestConvertTabsProcEx(int TabWidth);
// This is the slowest conversion function which can handle TabWidth <> 2^n.
QString ConvertTabsEx(const QString& Line, int TabWidth, bool& HasTabs);

bool GetHasTabs(const QString& line, int& CharsBefore);

int GetExpandedLength(const QString& aStr, int aTabWidth);

int CharIndex2CaretPos(int Index, int TabWidth,
  const QString& Line);

int CaretPos2CharIndex(int Position, int TabWidth, const QString& Line,
  bool& InsideTabChar);

// search for the first char of set AChars in Line, starting at index Start
int StrScanForCharInSet(const QString& Line, int Start, const QSet<QChar>&  AChars);
// the same, but searching backwards
int StrRScanForCharInSet(const QString& Line, int Start, const QSet<QChar>&  AChars);

int GetEOL(const QString& Line, int start);

// Remove all '/' characters from string by changing them into '\.'.
// Change all '\' characters into '\\' to allow for unique decoding.
QString EncodeString(const QString & s);

// Decodes string, encoded with EncodeString.
QString DecodeString(const QString& s);

using  HighlighterAttriProc = std::function<bool(PSynHighlighter Highlighter,
    PSynHighlighterAttribute Attri, const QString& UniqueAttriName,
    std::initializer_list<void *> Params)>;

// Enums all child highlighters and their attributes of a TSynMultiSyn through a
// callback function.
// This function also handles nested TSynMultiSyns including their MarkerAttri.
bool EnumHighlighterAttris(PSynHighlighter Highlighter,
                           bool SkipDuplicates, HighlighterAttriProc highlighterAttriProc,
                           std::initializer_list<void *> Params);


// Calculates Frame Check Sequence (FCS) 16-bit Checksum (as defined in RFC 1171)
uint16_t CalcFCS(unsigned char* ABuf, int ABufSize);

void SynDrawGradient(QPaintDevice* ACanvas, const QColor& AStartColor, const QColor& AEndColor,
  int ASteps, const QRect& ARect, bool AHorizontal);

SynFontStyles getFontStyles(const QFont& font);

/**
 * Find the first occurency of word char in s, starting from startPos
 * Note: the index of first char in s in 1
 * @return index of the char founded , 0 if not found
 */
int StrScanForWordChar(const QString& s, int startPos);

/**
 * Find the first occurency of non word char in s, starting from startPos
 * Note: the index of first char in s in 1
 * @return index of the char founded , 0 if not found
 */
int StrScanForNonWordChar(const QString& s, int startPos);

/**
 * Find the first occurency of word char in s right to left, starting from startPos
 * Note: the index of first char in s in 1
 * @return index of the char founded , 0 if not found
 */
int StrRScanForWordChar(const QString& s, int startPos);

/**
 * Find the first occurency of non word char in s  right to left, starting from startPos
 * Note: the index of first char in s in 1
 * @return index of the char founded , 0 if not found
 */
int StrRScanForNonWordChar(const QString& s, int startPos);

#endif // MISCPROCS_H
