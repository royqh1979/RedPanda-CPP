#ifndef SYNEDITSTRINGLIST_H
#define SYNEDITSTRINGLIST_H

#include <QStringList>
#include "highlighter/base.h"
#include <QList>
#include <memory>
#include "MiscProcs.h"

enum SynEditStringFlag {
    sfHasTabs = 0x0001,
    sfHasNoTabs = 0x0002,
    sfExpandedLengthUnknown = 0x0004
};

typedef int SynEditStringFlags;

struct SynEditStringRec {
  QString fString;
  void * fObject;
  SynRangeState fRange;
  int fExpandedLength;
  SynEditStringFlags fFlags;
  int fParenthesisLevel;
  int fBracketLevel;
  int fBraceLevel;
};

typedef std::shared_ptr<SynEditStringRec> PSynEditStringRec;

typedef QList<PSynEditStringRec> SynEditStringRecList;

typedef std::shared_ptr<SynEditStringRecList> PSynEditStringRecList;

class SynEditStringList;

typedef std::shared_ptr<SynEditStringList> PSynEditStringList;

using StringListChangeCallback = void (*) (PSynEditStringList* object, int index, int count);

enum class SynEditFileFormat {
    Windows,
    Linux,
    Mac
};// Windows: CRLF, UNIX: LF, Mac: CR


class SynEditStringList : public QObject
{  
    Q_OBJECT
public:
    explicit SynEditStringList();

    int parenthesisLevels(int Index);
    int bracketLevels(int Index);
    int braceLevels(int Index);
    QString expandedStrings(int Index);
    int expandedStringLength(int Index);
    int lengthOfLongestLine();
    SynRangeState ranges(int Index);
    void setRange(int Index, SynRangeState ARange);
    void setParenthesisLevel(int Index, int level);
    void setBracketLevel(int Index, int level);
    void setBraceLevel(int Index, int level);
    QString get(int Index);
    int count();
    void* getObject(int Index);
    QString text();

    void put(int Index, const QString& s);
    void putObject(int Index, void * AObject);

    void beginUpdate();
    void endUpdate();

    int tabWidth();
    void setTabWidth(int value);
    int Add(const QString& s);
    int AddStrings(const QStringList& Strings);

    int getTextLength();
    void Clear();
    void Delete(int Index);
    procedure DeleteLines(Index, NumLines: integer);
    procedure Exchange(Index1, Index2: integer); override;
    procedure Insert(Index: integer; const S: string); override;
    procedure InsertLines(Index, NumLines: integer);
    procedure InsertStrings(Index: integer; NewStrings: TStrings);
    procedure InsertText(Index: integer; NewText: string);
    procedure LoadFromFile(const FileName: string); override;
    procedure SaveToFile(const FileName: string); override;
    procedure SaveToStream(Stream: TStream); override;
    procedure LoadFromStream(Stream: TStream); override;
    property AppendNewLineAtEOF: Boolean read fAppendNewLineAtEOF write fAppendNewLineAtEOF;
    property FileFormat: TSynEditFileFormat read fFileFormat write fFileFormat;
    property ExpandedStrings[Index: integer]: string read expandedStrings;
    property ExpandedStringLengths[Index: integer]: integer read expandedStringLength;
    property LengthOfLongestLine: integer read lengthOfLongestLine;
    property Ranges[Index: integer]: TSynEditRange read GetRange write setRange;
    property ParenthesisLevels[Index: integer]: integer read GetParenthesisLevel write setParenthesisLevel;
    property BracketLevels[Index: integer]: integer read GetBracketLevel write setBracketLevel;
    property BraceLevels[Index: integer]: integer read GetBraceLevel write setBraceLevel;
    property TabWidth: integer read fTabWidth write setTabWidth;
    property OnChange: TNotifyEvent read fOnChange write fOnChange;
    property OnChanging: TNotifyEvent read fOnChanging write fOnChanging;
    property OnCleared: TNotifyEvent read fOnCleared write fOnCleared;
    property OnDeleted: TStringListChangeEvent read fOnDeleted write fOnDeleted;
    property OnInserted: TStringListChangeEvent read fOnInserted
      write fOnInserted;
    property OnPutted: TStringListChangeEvent read fOnPutted write fOnPutted;
    property ConvertTabsProc: TConvertTabsProcEx read fConvertTabsProc;

    bool getAppendNewLineAtEOF() const;
    void setAppendNewLineAtEOF(bool appendNewLineAtEOF);

    ConvertTabsProcEx getConvertTabsProc() const;

signals:
    void changed();
    void changing();
    void cleared();
    void deleted(int index, int count);
    void inserted(int index, int count);
    void putted(int index, int count);
protected:
    QString GetTextStr();
    void SetUpdateState(bool Updating);
    void InsertItem(int Index, const QString& s);


private:
    SynEditStringRecList mList;

    //int mCount;
    //int mCapacity;
    SynEditFileFormat mFileFormat;
    bool mAppendNewLineAtEOF;
    ConvertTabsProcEx mConvertTabsProc;
    int mIndexOfLongestLine;
    int mTabWidth;
    int mUpdateCount;
    QString ExpandString(int Index);
};



#endif // SYNEDITSTRINGLIST_H
