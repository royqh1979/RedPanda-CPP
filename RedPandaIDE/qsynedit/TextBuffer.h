#ifndef SYNEDITSTRINGLIST_H
#define SYNEDITSTRINGLIST_H

#include <QStringList>
#include "highlighter/base.h"
#include <memory>

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

typedef std::vector<PSynEditStringRec> SynEditStringRecList;

typedef std::shared_ptr<SynEditStringRecList> PSynEditStringRecList;

class SynEditStringList;

typedef std::shared_ptr<SynEditStringList> PSynEditStringList;

using StringListChangeCallback = void (*) (PSynEditStringList* object, int index, int count);

enum class SynEditFileFormat {
    Windows,
    Linux,
    Mac
};// Windows: CRLF, UNIX: LF, Mac: CR



class SynEditStringList : public QStringList
{

public:
    SynEditStringList();
};



#endif // SYNEDITSTRINGLIST_H
