#ifndef TYPES_H
#define TYPES_H

#include <QIcon>
#include <QList>
#include <QFlags>

enum class SynSelectionMode {smNormal, smLine, smColumn};

struct BufferCoord {
    int Char;
    int Line;
    bool operator==(const BufferCoord& coord);
    bool operator>=(const BufferCoord& coord);
    bool operator>(const BufferCoord& coord);
    bool operator<(const BufferCoord& coord);
    bool operator<=(const BufferCoord& coord);
    bool operator!=(const BufferCoord& coord);
};

class SynEdit;
/**
 * Nomalized buffer posistion:
 * (0,0) means at the start of the file ('\0')
 * (1,count of lines+1) means at the end of the file ('\0')
 * (length of the line+1, line) means at the line break of the line ('\n')
 */

class ContentsCoord {
public:
    ContentsCoord();
    ContentsCoord(const ContentsCoord& coord);
    int ch() const;
    void setCh(int newChar);

    int line() const;
    void setLine(int newLine);
    bool atStart();
    bool atEnd();
    const SynEdit *edit() const;
    const ContentsCoord& operator=(const ContentsCoord& coord);
    const ContentsCoord& operator=(const ContentsCoord&& coord);
    bool operator==(const ContentsCoord& coord) const;
    bool operator<(const ContentsCoord& coord) const;
    bool operator<=(const ContentsCoord& coord) const;
    bool operator>(const ContentsCoord& coord) const;
    bool operator>=(const ContentsCoord& coord) const;
    size_t operator-(const ContentsCoord& coord) const;
    const ContentsCoord& operator+=(int delta);
    const ContentsCoord& operator-=(int delta);
    ContentsCoord operator+(int delta) const;
    ContentsCoord operator-(int delta) const;
    BufferCoord toBufferCoord() const;
    QChar operator*() const;
private:
    ContentsCoord(const SynEdit* edit, int ch, int line);
    void normalize();
private:
    int mChar;
    int mLine;
    const SynEdit* mEdit;
    friend class SynEdit;
};

struct DisplayCoord {
    int Column;
    int Row;
};

enum SynFontStyle {
    fsNone = 0,
    fsBold = 0x0001,
    fsItalic = 0x0002,
    fsUnderline = 0x0004,
    fsStrikeOut = 0x0008
};

Q_DECLARE_FLAGS(SynFontStyles,SynFontStyle)

Q_DECLARE_OPERATORS_FOR_FLAGS(SynFontStyles)

using PSynIcon = std::shared_ptr<QIcon>;
using SynIconList = QList<PSynIcon>;
using PSynIconList = std::shared_ptr<SynIconList>;

enum class SynEditingAreaType {
  eatRectangleBorder,
  eatWaveUnderLine,
  eatUnderLine
};

struct SynEditingArea {
    int beginX;
    int endX;
    QColor color;
    SynEditingAreaType type;
};


using PSynEditingArea = std::shared_ptr<SynEditingArea>;
using SynEditingAreaList = QList<PSynEditingArea>;
using PSynEditingAreaList = std::shared_ptr<SynEditingAreaList>;


#endif // TYPES_H
