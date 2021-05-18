#ifndef SYNEDITSTRINGLIST_H
#define SYNEDITSTRINGLIST_H

#include <QStringList>
#include "highlighter/base.h"
#include <QVector>
#include <memory>
#include "MiscProcs.h"
#include "../utils.h"
#include "Types.h"

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
  int fColumns;  //
  int fParenthesisLevel;
  int fBracketLevel;
  int fBraceLevel;

public:
  explicit SynEditStringRec();
};

typedef std::shared_ptr<SynEditStringRec> PSynEditStringRec;

typedef QVector<PSynEditStringRec> SynEditStringRecList;

typedef std::shared_ptr<SynEditStringRecList> PSynEditStringRecList;

class SynEditStringList;

typedef std::shared_ptr<SynEditStringList> PSynEditStringList;

using StringListChangeCallback = std::function<void(PSynEditStringList* object, int index, int count)>;

class QFile;

class SynEdit;
class SynEditStringList : public QObject
{  
    Q_OBJECT
public:
    explicit SynEditStringList(SynEdit* pEdit,QObject* parent=nullptr);

    int parenthesisLevels(int Index);
    int bracketLevels(int Index);
    int braceLevels(int Index);
    int lineColumns(int Index);
    int lengthOfLongestLine();
    SynRangeState ranges(int Index);
    void setRange(int Index, SynRangeState ARange);
    void setParenthesisLevel(int Index, int level);
    void setBracketLevel(int Index, int level);
    void setBraceLevel(int Index, int level);
    QString getString(int Index);
    int count();
    void* getObject(int Index);
    QString text();
    void setText(const QString& text);

    void putString(int Index, const QString& s);
    void putObject(int Index, void * AObject);

    void beginUpdate();
    void endUpdate();

    int add(const QString& s);
    int addStrings(const QStringList& Strings);

    int getTextLength();
    void clear();
    void deleteAt(int Index);
    void deleteLines(int Index, int NumLines);
    void Exchange(int Index1, int Index2);
    void Insert(int Index, const QString& s);
    void InsertLines(int Index, int NumLines);
    void InsertStrings(int Index, const QStringList& NewStrings);
    void InsertText(int Index,const QString& NewText);
    void LoadFromFile(QFile& file, const QByteArray& encoding, QByteArray& realEncoding);
    void SaveToFile(QFile& file, const QByteArray& encoding, QByteArray& realEncoding);

    bool getAppendNewLineAtEOF() const;
    void setAppendNewLineAtEOF(bool appendNewLineAtEOF);

    ConvertTabsProcEx getConvertTabsProc() const;

    FileEndingType getFileEndingType() const;
    void setFileEndingType(const FileEndingType &fileEndingType);
public slots:
    void invalidAllLineColumns();

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
    void PutTextStr(const QString& text);

private:
    SynEditStringRecList mList;

    SynEdit* mEdit;
    //int mCount;
    //int mCapacity;
    FileEndingType mFileEndingType;
    bool mAppendNewLineAtEOF;
    ConvertTabsProcEx mConvertTabsProc;
    int mIndexOfLongestLine;
    int mUpdateCount;

    int calculateLineColumns(int Index);
};

enum class SynChangeReason {crInsert, crPaste, crDragDropInsert,
  //several undo entries can be chained together via the ChangeNumber
  //see also TCustomSynEdit.[Begin|End]UndoBlock methods
  crDeleteAfterCursor, crDelete,
  crLineBreak, crIndent, crUnindent,
  crSilentDelete, crSilentDeleteAfterCursor,
  crAutoCompleteBegin, crAutoCompleteEnd,
  crPasteBegin, crPasteEnd, //for pasting, since it might do a lot of operations
  crSpecial1Begin, crSpecial1End,
  crSpecial2Begin, crSpecial2End,
  crCaret, //just restore the Caret, allowing better Undo behavior
  crSelection, //restore Selection
  crNothing,
  crGroupBreak,
  crDeleteAll
  };
class SynEditUndoItem {
private:
    SynChangeReason mChangeReason;
    SynSelectionMode mChangeSelMode;
    BufferCoord mChangeStartPos;
    BufferCoord mChangeEndPos;
    QString mChangeStr;
    int mChangeNumber;
public:
    SynEditUndoItem(SynChangeReason reason,
        SynSelectionMode selMode,
        BufferCoord startPos,
        BufferCoord endPos,
        const QString& str,
        int number);

    SynChangeReason changeReason() const;
    SynSelectionMode changeSelMode() const;
    BufferCoord changeStartPos() const;
    BufferCoord changeEndPos() const;
    QString changeStr() const;
    int changeNumber() const;
};
using PSynEditUndoItem = std::shared_ptr<SynEditUndoItem>;

class SynEditUndoList : public QObject {
    Q_OBJECT
public:
    explicit SynEditUndoList();

    void AddChange(SynChangeReason AReason, const BufferCoord& AStart, const BufferCoord& AEnd,
      const QString& ChangeText, SynSelectionMode SelMode);

    void AddGroupBreak();
    void BeginBlock();
    void Clear();
    void DeleteItem(int index);
    void EndBlock();
    SynChangeReason LastChangeReason();
    void Lock();
    PSynEditUndoItem PeekItem();
    PSynEditUndoItem PopItem();
    void PushItem(PSynEditUndoItem Item);
    void Unlock();

    bool CanUndo();
    int ItemCount();

    int maxUndoActions() const;
    void setMaxUndoActions(int maxUndoActions);
    bool initialState();
    PSynEditUndoItem item(int index);
    void setInitialState(const bool Value);
    void setItem(int index, PSynEditUndoItem Value);

    int blockChangeNumber() const;
    void setBlockChangeNumber(int blockChangeNumber);

    int blockCount() const;

    bool insideRedo() const;
    void setInsideRedo(bool insideRedo);

    bool fullUndoImposible() const;

signals:
    void addedUndo();

protected:
    int mBlockChangeNumber;
    int mBlockCount;
    bool mFullUndoImposible;
    QVector<PSynEditUndoItem> mItems;
    int mLockCount;
    int mMaxUndoActions;
    int mNextChangeNumber;
    int mInitialChangeNumber;
    bool mInsideRedo;
    void EnsureMaxEntries();
};

using PSynEditUndoList = std::shared_ptr<SynEditUndoList>;

#endif // SYNEDITSTRINGLIST_H
