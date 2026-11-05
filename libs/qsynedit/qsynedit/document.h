/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef QSYNEDIT_DOCUMENT_H
#define QSYNEDIT_DOCUMENT_H

#include <QStringList>
#include <QMutex>
#include <QVector>
#include <memory>
#include <QFile>
#include "miscprocs.h"
#include "types.h"
#include "qt_utils/utils.h"

namespace QSynedit {

class Document;

using SearchConfirmAroundProc = std::function<bool ()>;
/**
 * @brief The DocumentLine class
 *
 * Store one line of the document.
 * The linebreak is not included.
 *
 * When the line is displayed on the screen, each mark is called a glyph.
 * In unicode, a glyph may be represented by more than one code points (chars).
 * DocumentLine provides utility methods to retrieve the chars corresponding to one glyph,
 * and other functions.
 *
 * Most of the member methods are not thread safe. So they are declared as private
 * to prevent ill-usage. It shoulde only be used by the document internally.
 */
class DocumentLine {
public:
    explicit DocumentLine();
    DocumentLine(const DocumentLine&)=delete;
    DocumentLine& operator=(const DocumentLine&)=delete;

    /**
     * @brief get the line text
     * @return the line text
     */
    const QString& lineText() const { return mLineText; }

    size_t lineSeq() const { return mLineSeq; }

private:
    /**
     * @brief get total count of the glyphs in the line text
     *
     * The char count of the line text may not be the same as the glyphs count
     *
     * @return the glyphs count
     */
    int glyphsCount() const { return mGlyphStartCharList.length(); }

    /**
     * @brief get list of start index of the glyphs in the line text
     * @return start indice of the glyph.
     */
    const QList<int>& glyphStartCharList() const { return mGlyphStartCharList; }

    /**
     * @brief get start index of the chars representing the specified glyph.
     * @param i index of the glyph in the line (starting from 0)
     * @return char index in the line text (start from 0)
     */
    int glyphStartChar(int i) const;

    /**
     * @brief get count of the chars representing the specified glyph.
     * @param i index of the glyph in the line (starting from 0)
     * @return
     */
    int glyphLength(int i) const;

    /**
     * @brief get the chars representing the specified glyph.
     * @param i index of the glyph in the line (starting from 0)
     * @return the chars representing the specified glyph
     */
    QString glyph(int i) const;

    /**
     * @brief get the state of the syntax highlighter after this line is parsed
     * @return
     */
    PSyntaxState syntaxState() const { return mSyntaxState; }
    /**
     * @brief set the state of the syntax highlighter after this line is parsed
     * @param newSyntaxState
     */
    void setSyntaxState(const PSyntaxState &newSyntaxState) { mSyntaxState = newSyntaxState; }

    void setLineText(const QString &newLineText);
private:
    QString mLineText; /* the unicode code points of the text */
    /**
     * @brief Start positions of glyphs in mLineText
     *
     * A glyph may be defined by more than one code points.
     * Each lement of mGlyphStartCharList (position) is the start index
     *  of the code points in the mLineText.
     */
    QList<int> mGlyphStartCharList;
    /**
     * @brief state of the syntax highlighter after this line is parsed
     *
     * QSynedit use this state to speed up syntax highlight parsing.
     * Which is also used in auto-indent calculating and other functions.
     */
    PSyntaxState mSyntaxState;
    size_t mLineSeq;
    static size_t seqCounter;
    friend class Document;
};

typedef std::shared_ptr<DocumentLine> PDocumentLine;

typedef QVector<PDocumentLine> DocumentLines;

typedef std::shared_ptr<DocumentLines> PDocumentLines;

typedef std::shared_ptr<Document> PDocument;

class UndoList;
class RedoList;
using PUndoList = std::shared_ptr<UndoList>;
using PRedoList = std::shared_ptr<RedoList>;

class BinaryFileError : public FileError {
public:
    explicit BinaryFileError (const QString& reason);
};

/**
 * @brief The Document class
 *
 * Represents a document, which contains many lines.
 *
 */
class Document : public QObject
{  
    Q_OBJECT
public:
    explicit Document(QObject* parent=nullptr);
    Document(const Document&)=delete;
    Document& operator=(const Document&)=delete;

    /**
     * @brief get nesting level of parenthesis at the end of the specified line
     *
     * It's thread safe.
     *
     * @param line line index (starts from 0)
     * @return
     */
    int parenthesisLevel(int line) const;

    /**
     * @brief get nesting level of brackets at the end of the specified line
     *
     * It's thread safe
     *
     * @param line line index (starts from 0)
     * @return
     */
    int bracketLevel(int line) const;

    /**
     * @brief get nesting level of braces at the end of the specified line
     *
     * It's thread safe
     *
     * @param line line index (starts from 0)
     * @return
     */
    int braceLevel(int line) const;

    /**
     * @brief get block (indent) level of the specified line
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @return
     */
    int blockLevel(int line) const;

    /**
     * @brief get count of new blocks (indent) started on the specified line
     * @param line line index (starts frome 0)
     * @return
     */
    int blockStarted(int line) const;

    /**
     * @brief get count of blocks (indent) ended on the specified line
     * @param line line index (starts frome 0)
     * @return
     */
    int blockEnded(int line) const;

    /**
     * @brief get line break of the current document
     *
     * @return
     */
    QString lineBreak() const;

    /**
     * @brief get state of the syntax highlighter after parsing the specified line.
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @return
     */
    PSyntaxState getSyntaxState(int line) const;

    /**
     * @brief set state of the syntax highlighter after parsing the specified line.
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @param state the new state
     */
    void setSyntaxState(int line, const PSyntaxState& state);

    /**
     * @brief get line text of the specified line.
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @return
     */
    QString getLine(int line) const;

    size_t getLineSeq(int line) const;

    /**
     * @brief get count of lines in the document
     *
     * It's thread safe.
     *
     * @return
     */
    int count() const;

    /**
     * @brief get all the text in the document.
     *
     * Lines are concatenated by line breaks (by lineBreak()).
     * It's thread safe.
     *
     * @return
     */
    QString text() const;

    /**
     * @brief set the text of the document
     *
     * It's thread safe.
     *
     * @param text
     */
    void setText(const QString& text);

    /**
     * @brief set the text of the document
     *
     * It's thread safe.
     *
     * @param text
     */
    void setContents(const QStringList& text);

    /**
     * @brief get all the lines in the document.
     *
     * It's thread safe.
     *
     * @return
     */
    QStringList content() const;

    /**
     * @brief replace the text of a line. Reparses the line (and, when
     * parseToEnd is set and the syntaxer needs line states, the following
     * lines until the states converge). The reparse runs even when the text
     * is unchanged: its purpose is also to repair syntax states from this
     * line onward, which may be stale after earlier edits (see the
     * doDeleteText delete+setLine protocol).
     * @param parseToEnd declares the syntax-state invalidation scope of this
     * edit: pass false only when the edit cannot invalidate the following
     * lines' states
     */
    void putLine(int index, const QString& s, bool parseToEnd);

    void beginUpdate();
    void endUpdate();
    /**
     * @brief whether the document is inside an update batch
     * (beginUpdate/endUpdate not balanced yet)
     */
    bool inUpdate() const;

    /**
     * @brief the undo/redo histories are owned by the document and shared
     * by all attached views (see design doc chapter 1 and 2.5)
     */
    PUndoList undoList() const;
    PRedoList redoList() const;

    /**
     * @brief the modified flag is a document-level property (like the
     * undo/redo stacks): it tells whether the shared content differs from
     * its saved state. Views read it and forward UI notifications; any view
     * may set it (e.g. on save), which updates every attached view.
     * With skipUndo=false, setting true clears the undo/redo stacks and
     * setting false marks the undo list's current position as the save
     * point (same semantics as the old per-view QSynEdit::setModified).
     */
    bool modified() const;
    void setModified(bool value, bool skipUndo=false);

    /**
     * @brief the document's own syntaxer: a bare instance (no color scheme
     * applied) used solely for the syntax-state cascade. It is the single
     * writer of per-line PSyntaxState when multiple views share the document.
     *
     * The instance itself is never exposed. The only external accesses are
     * the two below, both under the document lock:
     */
    /**
     * @brief a fresh clone of the document's syntaxer (default-configured;
     * thread-local scan state). The only way outsiders may obtain parsing
     * capability — e.g. the compiler-thread WORDXY path clones from here.
     */
    PSyntaxer createSyntaxerInstance() const;
    /**
     * @brief the document's language (a value, not the instance)
     */
    ProgrammingLanguage syntaxerLanguage() const;
    /**
     * @brief replace the document's syntaxer, reparse the whole document and
     * emit syntaxerChanged(). GUI thread only. Color schemes are a view-side
     * concern and are NOT propagated (views keep their own configured
     * instances; non-initiating views adopt the new language with default
     * attributes).
     */
    void setSyntaxer(const PSyntaxer &syntaxer);

    /**
     * @brief unconditionally reparse the whole document (no early stop on
     * state convergence) and emit linesReparsed(0, count())
     */
    void reparseAll();

    int addLine(const QString& s);
    void addLines(const QStringList& strings);

    int getTextLength() const;
    void clear();
    // The edit primitives below reparse the affected lines synchronously
    // (before endUpdate). parseToEnd declares the syntax-state invalidation
    // scope of the edit; it has no default on purpose: callers must state the
    // assumption explicitly. For deleteLine(s) the reparse only runs when
    // parseToEnd is set (deletion alone keeps the shifted lines' states valid
    // for line-local syntaxers).
    void deleteLine(int index, bool parseToEnd);
    void deleteLines(int index, int numLines, bool parseToEnd);
    void moveLine(int from, int to, bool parseToEnd);
    void insertLine(int index, const QString& s, bool parseToEnd);
    void insertLines(int index, int numLines, bool parseToEnd);

    PDocumentLine findLineBySeq(size_t lineSeq) const;

    void loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding);
    void saveToFile(QFile& file, const QByteArray& encoding, QByteArray& realEncoding) const;

    QString glyph(int line, int glyphIdx) const;
    QString glyphAt(int line, int charPos) const;

    int charToGlyphStartChar(int line, int charPos) const;
    //int columnToGlyphStartColumn(int line, int charPos);

    int glyphCount(int line) const;
    /**
     * @brief get start index of the chars representing the specified glyph in the specified line.
     *
     * It's thread safe.
     *
     * @param line index of the line in the document (starting from 0)
     * @param glyphIdx index of the glyph in the line (starting from 0)
     * @return char index in the line text (start from 0)
     */
    int glyphStartChar(int line, int glyphIdx) const;

    int glyphEndChar(int line, int glyphIdx) const;

    /**
     * @brief get count of the chars representing the specified glyph in the specified line.
     *
     * It's thread safe.
     *
     * @param line index of the line in the document (starting from 0)
     * @param glyphIdx index of the glyph in the line (starting from 0)
     * @return
     */
    int glyphLength(int line, int glyphIdx) const;

    /**
     * @brief get index of the glyph represented by the specified char
     *
     * It's thread safe.
     *
     * @param line index of the line (starting from 0)
     * @param charIdx position of the char in the line text (starting from 0)
     * @return glyph index in the line (starting from 0)
     */
    int charToGlyphIndex(int line, int charPos) const;

    /**
     * @brief the glyph segmentation of the line (font-independent)
     *
     * It's thread safe.
     */
    QList<int> getGlyphStartCharList(int line, const QString &lineText);
    QList<int> getGlyphStartCharList(int line);

    bool getAppendNewLineAtEOF() const;
    void setAppendNewLineAtEOF(bool appendNewLineAtEOF);

    NewlineType getNewlineType() const;
    void setNewlineType(const NewlineType &fileEndingType);

    bool empty() const;

#ifdef QSYNEDIT_TEST
    QList<int> getGlyphStartCharListForTest(int line);
#endif

signals:
    void changed();
    void changing();
    /**
     * @brief emitted once at the outermost endUpdate with the ordered,
     * coalesced line-level changes accumulated in the batch. When emitted,
     * the document is quiescent (inUpdate()==false). Slots must only do
     * idempotent, fact-driven maintenance; cross-component consistency
     * repair (e.g. re-snapping the selection to the caret) is forbidden.
     */
    void contentChanged(const DocumentChangeSet &changes, bool foldStateDirty);
    /**
     * @brief emitted after an undo/redo operation has been performed on the
     * shared stacks (by whichever view triggered it), so all views can
     * re-sync their modified status
     */
    void undoRedoStateChanged();
    /**
     * @brief emitted when the document's modified flag has actually changed
     * (by whichever view set it, including via undo/redo reaching/leaving
     * the save point)
     */
    void modifiedChanged();
    /**
     * @brief emitted after the document's syntaxer has been replaced and the
     * whole document reparsed with it. Views attached to this document must
     * adopt the new language (createInstance() from syntaxer()) and refresh.
     */
    void syntaxerChanged();
    /**
     * @brief emitted synchronously after reparseLines()/reparseAll() have
     * updated the syntax states of [startLine, endLine). endLine is
     * exclusive; INT_MAX means "to the end of the document". Views use it to
     * repaint the reparsed range (including the reparse tail that lies
     * outside the edited lines); when emitted mid-batch the view defers the
     * repaint until after the fold rescan in the contentChanged slot.
     */
    void linesReparsed(int startLine, int endLine);
protected:
    QString getTextStr() const;
    void insertItem(int line, const QString& s);
    void addItem(const QString& s);
    void internalClear();
private:
    void ensureHasLine();
    void recordChange(DocumentChangeKind kind, int line, int count);
    // reparse lines [startLine, endLine) with mSyntaxer and store the states;
    // when parseToEnd is set and the syntaxer needs line states, parsing
    // continues past endLine until the states converge. Emits linesReparsed
    // with the actually parsed range; returns the stop line (exclusive).
    // Only called by the edit primitives, synchronously and under mMutex.
    int reparseLines(int startLine, int endLine, bool parseToEnd);
public:
    /**
     * @brief emit undoRedoStateChanged(); called by the view that performed
     * an undo/redo so every attached view re-syncs its modified status
     */
    void notifyUndoRedoStateChanged();
    int charToGlyphIndex(const QString& str, const QList<int> &glyphStartCharList, int charPos) const;
    QList<int> getGlyphStartPositionList(int line);
    bool tryLoadFileByEncoding(QByteArray encodingName, QFile& file);
    /**
     * @brief whether the stored syntax state of `line` carries
     * code-block pairing deltas (blockStarted/blockEnded/
     * blockEndedLastLine). Unparsed lines (null state) are treated
     * conservatively as carrying deltas.
     */
    bool lineHasCodeBlockDelta(int line) const;
    void loadUTF16BOMFile(QFile& file);
    void loadUTF32BOMFile(QFile& file);
private:
    DocumentLines mLines;
    QMap<size_t,PDocumentLine> mLineSeqIndice;

    NewlineType mNewlineType;
    bool mAppendNewLineAtEOF;
    int mUpdateCount;
    DocumentChangeSet mPendingChanges;
    bool mFoldStateDirty = false;

    PUndoList mUndoList;
    PRedoList mRedoList;
    PSyntaxer mSyntaxer;
    bool mModified = false;

    mutable QRecursiveMutex mMutex;
};

enum class ChangeReason {
    InsertLine,
    Insert,
    Delete,
    ColumnDelete, //delete a column band; positions are snapped per line
    Caret, //just restore the Caret, allowing better Undo behavior
    Selection, //restore Selection
    GroupBreak,
    LeftTop,
    LineBreak,
    MoveLine,
    ReplaceLine,
    Input,
    DeleteChar,
    DeletePreviousChar,
    MergeWithPrevLine,
    MergeWithNextLine,
    DuplicateLine,
    Nothing // undo list empty
  };

struct CaretAndSelectionInfo{
    CharPos caret;
    CharPos selBegin;
    CharPos selEnd;
    SelectionMode selMode;
    CaretAndSelectionInfo(const CharPos &newCaret, const CharPos &newSelBegin,const CharPos &newSelEnd, SelectionMode newSelMode);
};

using PCaretAndSelectionInfo = std::shared_ptr<CaretAndSelectionInfo>;

class UndoItem {
private:
    ChangeReason mChangeReason;
    SelectionMode mChangeSelMode;
    CharPos mChangeStartPos;
    CharPos mChangeEndPos;
    QStringList mChangeText;
    size_t mChangeNumber;
public:
    UndoItem(ChangeReason reason,
        SelectionMode selMode,
        CharPos startPos,
        CharPos endPos,
        const QStringList& text,
        int number);
    virtual ~UndoItem();

    ChangeReason changeReason() const;
    SelectionMode changeSelMode() const;
    CharPos changeStartPos() const;
    CharPos changeEndPos() const;
    QStringList changeText() const;
    size_t changeNumber() const;
};

/**
 * @brief Undo item for a column-mode band deletion.
 *
 * Column deletion snaps the band's edges to glyph boundaries on each
 * line independently, so the per-line left edge can not be derived
 * from the recorded band positions afterwards (it is lost as soon as
 * the glyphs are gone, e.g. under non-uniform glyph widths). Keep them
 * here, parallel to changeText(), so undo can reinsert the deleted
 * text at the exact per-line positions.
 *
 * Derived from UndoItem so that plain items don't pay for the list.
 */
class ColumnDeleteUndoItem : public UndoItem {
private:
    QList<int> mPerLineLeftChars;
public:
    ColumnDeleteUndoItem(SelectionMode selMode,
        CharPos startPos,
        CharPos endPos,
        const QStringList& text,
        int number,
        const QList<int>& perLineLeftChars);

    // left-edge char index of the deleted band on each line,
    // parallel to changeText()
    QList<int> perLineLeftChars() const;
};

using PUndoItem = std::shared_ptr<UndoItem>;

class UndoList : public QObject {
    Q_OBJECT
public:
    explicit UndoList();

    PUndoItem addChange(ChangeReason reason, const CharPos& start, const CharPos& end,
      const QStringList& changeText, SelectionMode selMode);

    PUndoItem addColumnDeleteChange(const CharPos& start, const CharPos& end,
      const QStringList& changeText, const QList<int>& perLineLeftChars);

    void restoreChange(ChangeReason reason, const CharPos& start, const CharPos& end,
                       const QStringList& changeText, SelectionMode selMode, size_t changeNumber);

    void restoreChange(PUndoItem item);

    void addGroupBreak();
    void beginBlock(const CharPos &caret, const CharPos &selBegin, const CharPos &selEnd, SelectionMode selMode);
    void endBlock(const CharPos &caret, const CharPos &selBegin, const CharPos &selEnd, SelectionMode selMode);

    PCaretAndSelectionInfo caretAndSelBeforeChange(size_t changeNumber);
    PCaretAndSelectionInfo caretAndSelAfterChange(size_t changeNumber);

    void restoreCaretAndSelInfos(size_t changeNumber, const PCaretAndSelectionInfo &beforeInfo,
                                 const PCaretAndSelectionInfo &afterInfo);

    void removeCaretAndSelInfo(size_t changeNumber);

    void clear();
    ChangeReason lastChangeReason();
    bool isEmpty();
    PUndoItem peekItem();
    PUndoItem popItem();

    bool canUndo();
    int itemCount();

    int maxUndoActions() const;
    void setMaxUndoActions(int maxUndoActions);
    bool initialState();
    void setInitialState();

    bool insideRedo() const;
    void setInsideRedo(bool insideRedo);

    bool fullUndoImposible() const;

signals:
    void addedUndo();
protected:
    bool inBlock();
    unsigned int getNextChangeNumber();

protected:
    size_t mBlockChangeNumber;
    int mBlockLock;
    size_t mLastPoppedItemChangeNumber;
    size_t mLastRestoredItemChangeNumber;
    bool mFullUndoImposible;
    QVector<PUndoItem> mItems;
    size_t mNextChangeNumber;
    size_t mInitialChangeNumber;
    bool mInsideRedo;
    QMap<size_t, PCaretAndSelectionInfo> mCaretInfoBeforeChange;
    QMap<size_t, PCaretAndSelectionInfo> mCaretInfoAfterChange;
};

class RedoList : public QObject {
    Q_OBJECT
public:
    explicit RedoList();

    void addRedo(ChangeReason reason, const CharPos& startPos, const CharPos& endPos,
                 const QStringList& changeText, SelectionMode selMode, size_t changeNumber);
    void addRedo(PUndoItem item);

    void addCaretAndSelectionInfo(size_t changeNumber, const PCaretAndSelectionInfo &beforeInfo,
                                  const PCaretAndSelectionInfo &afterInfo);
    PCaretAndSelectionInfo caretAndSelBeforeChange(size_t changeNumber);
    PCaretAndSelectionInfo caretAndSelAfterChange(size_t changeNumber);
    void removeCaretAndSelInfo(size_t changeNumber);
    void clear();
    ChangeReason lastChangeReason();
    bool isEmpty();
    PUndoItem peekItem();
    PUndoItem popItem();

    bool canRedo();
    int itemCount();

protected:
    QVector<PUndoItem> mItems;
    QMap<size_t, PCaretAndSelectionInfo> mCaretInfoBeforeChange;
    QMap<size_t, PCaretAndSelectionInfo> mCaretInfoAfterChange;
};


}

#endif // SYNEDITSTRINGLIST_H
