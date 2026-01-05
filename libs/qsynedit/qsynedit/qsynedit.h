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
#ifndef QSYNEDIT_H
#define QSYNEDIT_H

#include <QAbstractScrollArea>
#include <QCursor>
#include <QDateTime>
#include <QFrame>
#include <QStringList>
#include <QTimer>
#include <QElapsedTimer>
#include <QWidget>
#include "gutter.h"
#include "codefolding.h"
#include "types.h"
#include "keystrokes.h"
#include "searcher/baseseacher.h"
#include "formatter/formatter.h"

namespace QSynedit {

enum class ScrollStyle {
    None, OnlyHorizontal, OnlyVertical, Both
};

enum class EditCaretType {
    VerticalLine=0, HorizontalLine=1, HalfBlock=2, Block=3
};

enum StatusChange {
    None = 0,
    AllCleared = 0x0001,
    CaretX = 0x0002,
    CaretY = 0x0004,
    LeftPos = 0x0008,
    TopPos = 0x0010,
    InsertMode = 0x0020,
    ModifyChanged = 0x0040,
    Selection = 0x0080,
    ReadOnlyChanged = 0x0100,
    Modified = 0x0200,
    Custom = 0x0400,
};

Q_DECLARE_FLAGS(StatusChanges, StatusChange)
Q_DECLARE_OPERATORS_FOR_FLAGS(StatusChanges)

enum class StateFlag  {
    HScrollbarChanged             = 0x0001,
    VScrollbarChanged             = 0x0002,
    DblClicked                    = 0x0004,
    WaitForDragging               = 0x0008,
    EnsureCaretVisible            = 0x0010,
    EnsureCaretVisibleForceMiddle = 0x0020,
};
Q_DECLARE_FLAGS(StateFlags,StateFlag)

Q_DECLARE_OPERATORS_FOR_FLAGS(StateFlags)

enum class EditorOption {
    AltSetsColumnMode =     0x00000001, //Holding down the Alt Key will put the selection mode into columnar format
    AutoIndent =            0x00000002, //Will auto calculate the indent when input
    LigatureSupport =       0x00000004, //Support ligaures in fonts like fira code
    DragDropEditing =       0x00000008, //Allows you to select a block of text and drag it within the document to another location
    DropFiles =             0x00000010, //Allows the editor accept OLE file drops
    EnhanceHomeKey =        0x00000020, //enhances home key positioning, similar to visual studio
    EnhanceEndKey =         0x00000040, //enhances End key positioning, similar to JDeveloper
    GroupUndo =             0x00000080, //When undoing/redoing actions, handle all continous changes of the same kind in one call instead undoing/redoing each command separately
    HalfPageScroll =        0x00000100, //When scrolling with page-up and page-down commands, only scroll a half page at a time
    AutoHideScrollbars =    0x00000200, //if enabled, then the scrollbars will only show when necessary.  If you have ScrollPastEOL, then it the horizontal bar will always be there (it uses MaxLength instead)
    KeepCaretX =            0x00000400 , //When moving through lines w/o Cursor Past EOL, keeps the X position of the cursor
    RightMouseMovesCursor=  0x00000800, //When clicking with the right mouse for a popup menu, move the cursor to that location
//    ScrollByOneLess =       0x00001000, //Forces scrolling to be one less
    ScrollPastEof =         0x00002000, //Allows the cursor to go past the end of file marker
    ScrollPastEol =         0x00004000, //Allows the cursor to go past the last character into the white space at the end of a line
    InvertMouseScroll =     0x00008000, //Shows the special Characters
//  SpecialLineDefaultFg = 0x00010000, //disables the foreground text color override when using the OnSpecialLineColor event
    TabIndent =             0x00020000, //When active <Tab> and <Shift><Tab> act as block indent, unindent when text is selected
    TabsToSpaces =          0x00040000, //Converts a tab character to a specified number of space characters
    ShowRainbowColor    =   0x00080000,
    SelectWordByDblClick =  0x00100000,
    ShowLeadingSpaces =     0x00200000,
    ShowTrailingSpaces =    0x00400000,
    ShowInnerSpaces =       0x00800000,
    ShowLineBreaks =        0x01000000,
    ForceMonospace =        0x02000000,
};

Q_DECLARE_FLAGS(EditorOptions, EditorOption)

Q_DECLARE_OPERATORS_FOR_FLAGS(EditorOptions)

enum class SearchAction {
    Replace,
    ReplaceAll,
    ReplaceAndExit,
    Skip,
    Exit
};

using SearchMatchedProc = std::function<SearchAction(const QString& sFound,
    const QString& sReplace, const CharPos& pos, int wordLen)>;
using SearchConfirmAroundProc = std::function<bool ()>;

struct GlyphPostionsListCache {
    QString str;
    QList<int> glyphCharList;
    QList<int> glyphPositionList;
    int strWidth;
};


class QSynEdit;
using PSynEdit = std::shared_ptr<QSynEdit>;

class TokenAttribute;
using PTokenAttribute = std::shared_ptr<TokenAttribute>;
class Document;
using PDocument = std::shared_ptr<Document>;
struct SyntaxState;
using PSyntaxState = std::shared_ptr<SyntaxState>;
class Syntaxer;
using PSyntaxer = std::shared_ptr<Syntaxer>;
class UndoList;
class RedoList;
using PUndoList = std::shared_ptr<UndoList>;
using PRedoList = std::shared_ptr<RedoList>;

enum class ChangeReason;

class QSynEdit : public QAbstractScrollArea
{
    Q_OBJECT
public:
    enum  class CharType {
        SpaceChar,
        NonSpaceChar
    };
    explicit QSynEdit(QWidget* parent=nullptr);
    QSynEdit(const QSynEdit&)=delete;
    QSynEdit& operator=(const QSynEdit&)=delete;


    int lineCount() const;
    /**
     * Returns how many rows are there in the editor
     * @return
     */
    int displayLineCount() const;

    /**
     * @brief displayX
     * @return
     */
    DisplayCoord displayXY() const {
        return bufferToDisplayPos(caretXY());
    }
    int displayX() const {
        return displayXY().x;
    }
    int displayY() const {
        return displayXY().row;
    }
    CharPos caretXY() const {
        return CharPos{caretX(),caretY()};
    }
    int caretX() const {
        return mCaretX;
    }
    int caretY() const {
        return mCaretY;
    }

    void invalidateGutter();
    void invalidateGutterLine(int aLine);
    void invalidateGutterLines(int startLine, int endLine);

    int yposToRow(int y) const {
        return std::max(1, ((y + mTopPos) / mTextHeight) + 1 );
    }

    DisplayCoord pixelsToNearestGlyphPos(int aX, int aY) const;
    DisplayCoord pixelsToGlyphPos(int aX, int aY) const;
    QPoint displayCoordToPixels(const DisplayCoord& coord) const;
    DisplayCoord bufferToDisplayPos(const CharPos& p) const;
    CharPos displayToBufferPos(const DisplayCoord& p) const;

    //normalized buffer coord operations
//    ContentsCoord fromCharPos(const CharPos& p) const;
//    ContentsCoord createNormalizedCharPos(int aChar,int aLine) const;
//    QStringList getContents(const ContentsCoord& pStart,const ContentsCoord& pEnd);
//    QString getJoinedContents(const ContentsCoord& pStart,const ContentsCoord& pEnd, const QString& joinStr);

    int leftSpaces(const QString& line) const;
    QString genSpaces(int charCount) const;
    int charToGlyphLeft(int line, int ch) const;
    int charToGlyphLeft(int line, const QString& s, int ch) const;
    //int charToColumn(const QString& s, int aChar) const;
    int xposToGlyphStartChar(int line, int xpos) const;
    int xposToGlyphStartChar(int line, const QString& s, int xpos) const;
    int xposToGlyphLeft(int line, int xpos) const;
    //int xposToGlyphRight(int line, int xpos) const;
    int stringWidth(const QString& line, int left) const;
    int getLineIndent(const QString& line) const;
    int rowToLine(int aRow) const;
    int lineToRow(int aLine) const;
    int foldRowToLine(int row) const;
    int foldLineToRow(int line) const;
    void setDefaultKeystrokes();
    void setExtraKeystrokes();
    void invalidateLine(int line);
    void invalidateLines(int startLine, int endLine);
    void invalidateSelection();
    void invalidateRect(const QRect& rect);
    void invalidate();
    bool selAvail() const;
    bool colSelAvail() const;
    QString wordAtCursor() const;
    QString tokenAt(const CharPos& XY) const;

    QChar charAt(const CharPos& pos) const;

    bool inSelection(const CharPos& pos) const;
    CharPos findNextChar(const CharPos &pos, CharType type) const;
    CharPos findPrevChar(const CharPos &pos, CharType type) const;
    CharPos nextSpaceChar(const CharPos &pos) const { return findNextChar(pos, CharType::SpaceChar); }
    CharPos nextNonSpaceChar(const CharPos &pos) const { return findNextChar(pos, CharType::NonSpaceChar); }
    CharPos prevSpaceChar(const CharPos &pos) const { return findPrevChar(pos, CharType::SpaceChar); }
    CharPos prevNonSpaceChar(const CharPos &pos) const { return findPrevChar(pos, CharType::NonSpaceChar); }

    bool inWord(const CharPos& pos) const;

    CharPos getTokenBegin(const CharPos& pos) const;
    CharPos getTokenEnd(const CharPos& pos) const;

    CharPos prevWordBegin(CharPos) const;
    CharPos prevWordEnd(const CharPos &pos) const;
    CharPos nextWordBegin(const CharPos & pos) const;
    CharPos nextWordEnd(const CharPos &pos) const;
    CharPos fileBegin() const { return CharPos{0,0}; }
    CharPos fileEnd() const;

    //Caret
    void showCaret();
    void hideCaret();
    void setCaretX(int ch) {
        setCaretXY({ch,mCaretY});
    }
    void setCaretY(int line) {
        setCaretXY({mCaretX,line});
    }
    void setCaretXY(const CharPos& pos);
    void setCaretXYCentered(const CharPos& pos);
    void setCaretAndSelection(const CharPos& posCaret,
                              const CharPos& posSelBegin,
                              const CharPos& posSelEnd);

    bool inputMethodOn() const {
        return !mInputPreeditString.isEmpty();
    }

    void collapseAll();
    void unCollpaseAll();
    void uncollapseAroundLine(int line);
    void uncollapseAroundLines(int startLine, int count);
    bool collapse(int fromLine, int toLine);
    bool uncollapase(int fromLine, int toLine);
#ifdef QSYNEDIT_TEST
    int codeBlockCount() const;
    bool hasCodeBlock(int fromLine, int toLine) const; // for testing
    int subBlockCounts(int fromLine, int toLine) const;
    bool isCollapsed(int fromLine, int toLine) const;
#endif
    PCodeBlock foldHidesLine(int line);
    void setSelLength(int len);
    void setSelText(const QString& text);

    void replaceLine(int line, const QString& lineText) { processCommand(EditCommand::ReplaceLine, line, lineText);};

    int searchReplace(const QString& sSearch, const QString& sReplace,
                      const CharPos & scopeBegin,
                      const CharPos & scopeEnd,
                      CharPos& newScopeEnd,
                      SearchOptions options,
               Searcher *searchEngine,  SearchMatchedProc matchedCallback = nullptr,
                      SearchConfirmAroundProc confirmAroundCallback = nullptr);

    int maxScrollWidth() const;
    int maxScrollHeight() const;

    bool getTokenAttriAtRowCol(const CharPos& pos, QString& token,
      PTokenAttribute& attri) const;
    bool getTokenAttriAtRowCol(const CharPos& pos, QString& token,
      PTokenAttribute& attri, PSyntaxState &syntaxState) const;
    bool getTokenAttriAtRowCol(const CharPos& pos, QString& token,
      int &start, PTokenAttribute& attri) const;
    bool getTokenAttriAtRowCol(const CharPos& pos, QString& token,
      int &start, PTokenAttribute& attri, PSyntaxState &syntaxState) const;

    void getTokenAttriList(int line, QStringList &lstToken, QList<int> &lstPos, QList<PTokenAttribute> lstAttri) const;
    QSet<int> getTokenBorders(int line) const;

    void beginEditing();
    void endEditing();
    bool editing() const {return mEditingCount!=0;} // for testing
    void addCaretToUndo();
    void addLeftTopToUndo();
    void addSelectionToUndo();

    void processCommand(EditCommand Command, QVariant data=QVariant(),QVariant data2=QVariant());


    //Commands
    void trimTrailingSpaces() { processCommand(EditCommand::TrimTrailingSpaces); }
    void clear() { processCommand(EditCommand::ClearAll); }

    virtual void cutToClipboard() { processCommand(EditCommand::Cut);}
    virtual void copyToClipboard() { processCommand(EditCommand::Copy);}
    virtual void pasteFromClipboard() { processCommand(EditCommand::Paste);}
    virtual void undo()  { processCommand(EditCommand::Undo);}
    virtual void redo()  { processCommand(EditCommand::Redo);}
    virtual void zoomIn()  { processCommand(EditCommand::ZoomIn);}
    virtual void zoomOut()  { processCommand(EditCommand::ZoomOut);}
    virtual void selectAll() {
        processCommand(EditCommand::SelectAll);
    }
    virtual void selectWord() {
        processCommand(EditCommand::SelWord);
    }
    virtual void tab() { processCommand(EditCommand::Tab);}
    virtual void shifttab() { processCommand(EditCommand::ShiftTab);}
    virtual void toggleComment() { processCommand(EditCommand::ToggleComment);}
    virtual void toggleBlockComment() { processCommand(EditCommand::ToggleBlockComment);}
    virtual void matchBracket() { processCommand(EditCommand::MatchBracket);}
    virtual void moveSelUp(){ processCommand(EditCommand::MoveSelUp);}
    virtual void moveSelDown(){ processCommand(EditCommand::MoveSelDown);}

    virtual CharPos getMatchingBracket();
    virtual CharPos getMatchingBracket(const CharPos &pos);
    void startParseLine(Syntaxer *syntaxer, int lineIndex) const;
    void startParseLine(Syntaxer *syntaxer, int lineIndex, const QString lineText) const;

    QStringList content();
    QString text();

    CharPos ensureCharPosValid(const CharPos& coord) const;
    bool validLine(int line) const;
    bool validInDoc(int line, int ch) const;
    bool validInDoc(const CharPos& pos) const { return validInDoc(pos.line, pos.ch); }
    bool validGlyphStart(const CharPos& pos) const;
    bool getPositionOfMouse(CharPos& aPos) const;
    bool getLineOfMouse(int& line) const;
    bool pointToCharLine(const QPoint& point, CharPos& coord) const;
    bool pointToLine(const QPoint& point, int& line) const;
    bool isIdentChar(const QChar& ch) const;
    bool isIdentStartChar(const QChar& ch) const;
    bool isSpaceChar(const QChar& ch) const;

    void setRainbowAttrs(const PTokenAttribute &attr0,
                         const PTokenAttribute &attr1,
                         const PTokenAttribute &attr2,
                         const PTokenAttribute &attr3);

    void updateMouseCursor();

    bool isCaretVisible();

    // setter && getters
    int topPos() const;
    void setTopPos(int value);

    int linesInWindow() const;

    int leftPos() const;
    void setLeftPos(int value);

    const CharPos &selBegin() const;
    const CharPos &selEnd() const;
    int selectionBeginLine() const;
    int selectionEndLine() const;

    void clearSelection();
    void setSelBeginEnd(const CharPos &beginPos, const CharPos &endPos);

    SelectionMode activeSelectionMode() const;
    void setActiveSelectionMode(const SelectionMode &Value);

    int charWidth() const {
        return mCharWidth;
    }

    int viewWidth() const{
        return clientWidth() - mGutterWidth - 2;
    }

    int gutterWidth() const;
    void setGutterWidth(int value);

    bool modified() const;
    void setModified(bool Value, bool skipUndo=false);

    PSyntaxer syntaxer() const;
    void setSyntaxer(const PSyntaxer &syntaxer);

    bool useCodeFolding() const;
    void setUseCodeFolding(bool value);

    CodeFoldingOptions & codeFolding();

    QString displayLineText();
    QString lineText() const;
    QString lineText(int line) const;
    size_t lineSeq(int line) const;

    bool findLineTextBySeq(size_t lineSeq, QString& text) const;

    const std::shared_ptr<const Document> document() const;
    bool empty();

    void loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding);
    void setContent(const QString& text);
    void setContent(const QStringList& text);

    QString selText() const;
    QStringList selContent() const;
    int selCount() const;

    QStringList getContent(CharPos startPos, CharPos endPos, SelectionMode mode) const;
    void reparseDocument();

    QString lineBreak() const;

    EditorOptions getOptions() const;
    void setOptions(const EditorOptions &Value);

    int tabSize() const;
    void setTabSize(int tabSize);
    int tabWidth() const;

    QColor caretColor() const;
    void setCaretColor(const QColor &caretColor);

    QColor activeLineColor() const;
    void setActiveLineColor(const QColor &activeLineColor);

    EditCaretType overwriteCaret() const;
    void setOverwriteCaret(const EditCaretType &overwriteCaret);

    EditCaretType insertCaret() const;
    void setInsertCaret(const EditCaretType &insertCaret);

    Gutter& gutter();

    bool readOnly() const;
    void setReadOnly(bool readOnly);

    void setInsertMode(bool value);
    bool insertMode() const;

    bool canUndo() const;
    bool canRedo() const;

    int textHeight() const;

    const QColor &selectedForeground() const;
    void setSelectedForeground(const QColor &newSelectedForeground);

    const QColor &selectedBackground() const;
    void setSelectedBackground(const QColor &newSelectedBackground);

    int rightEdge() const;
    void setRightEdge(int newRightEdge);

    const QColor &rightEdgeColor() const;
    void setRightEdgeColor(const QColor &newRightEdgeColor);

    bool caretUseTextColor() const;
    void setCaretUseTextColor(bool newCaretUseTextColor);

    const PTokenAttribute &rainbowAttr0() const;

    const PTokenAttribute &rainbowAttr1() const;

    const PTokenAttribute &rainbowAttr2() const;

    const PTokenAttribute &rainbowAttr3() const;

    int mouseWheelScrollSpeed() const;
    void setMouseWheelScrollSpeed(int newMouseWheelScrollSpeed);

    const QColor &foregroundColor() const;
    void setForegroundColor(const QColor &newForegroundColor);

    const QColor &backgroundColor() const;
    void setBackgroundColor(const QColor &newBackgroundColor);

    bool isEmpty();

    int mouseSelectionScrollSpeed() const;
    void setMouseSelectionScrollSpeed(int newMouseSelectionScrollSpeed);

    ScrollStyle scrollBars() const;
    void setScrollBars(ScrollStyle newScrollBars);

    double lineSpacingFactor() const;
    void setLineSpacingFactor(double newLineSpacingFactor);

    const QDateTime &lastModifyTime() const;

    const PFormatter &formatter() const;
    void setFormatter(const PFormatter &newFormatter);
signals:
    void linesDeleted(int firstLine, int count);
    void linesInserted(int firstLine, int count);
    void lineMoved(int from, int to);
    void changed();
    void gutterClicked(Qt::MouseButton button, int x, int y, int line);
    void statusChanged(QSynedit::StatusChanges changes);
    void fontChanged();
    void tabSizeChanged();
#ifdef QSYNEDIT_TEST
    void foldsRescaned(); // for test
    void linesReparesd(int start, int count);
#endif
protected:
    virtual bool onGetSpecialLineColors(int Line,
         QColor& foreground, QColor& backgroundColor) ;
    virtual void onGetEditingAreas(int Line, EditingAreaList& areaList);
    virtual void onGutterGetText(int aLine, QString& aText);
    virtual void onGutterPaint(QPainter& painter, int aLine, int X, int Y);
    virtual void onPaint(QPainter& painter);
    virtual void onPreparePaintHighlightToken(int line,
            int aChar, const QString& token, PTokenAttribute attr,
            FontStyles& style, QColor& foreground, QColor& background);
protected:
    void replaceAll(const QString& text);
    int clientWidth() const;
    int clientHeight() const;
    int clientTop() const;
    int clientLeft() const;
    QRect clientRect() const;
    void doSelectLine();
    void beginInternalChanges();
    void endInternalChanges();
    void beginMergeCaretAndSelectionStatusChange();
    void endMergeCaretAndSelectionStatusChange();

    PSyntaxState calcSyntaxStateAtLine(int line, const QString &newLineText, bool handleLastBackSlash = true) const;
    bool dragging() const { return mDragging; }

    void invalidateAllNonTempLineWidth();
    void setStatusChanged(StatusChanges changes);

private:
    int calcLineAlignedTopPos(int currentValue, bool passFirstLine);
    void ensureLineAlignedWithTop(void);
    void computeCaret();
    void computeScroll(bool isDragging);
    void selCurrentToken();
    void selTokenAt(const CharPos &pos);

    void synFontChanged();

    void doSetSelText(const QString& value);

    void updateLastCaretX();
    void ensureCaretVisible(bool ForceToMiddle=false);
    void scrollWindow(int dx,int dy);
    void setCaretDisplayXY(const DisplayCoord& aPos, bool ensureCaretVisible = true);
    void internalSetSelBegin(const CharPos &value);
    void internalSetSelEnd(const CharPos &value);
    void internalSetCaretXY(CharPos value, bool ensureVisible = true);
    void internalSetCaretX(int value);
    void notifyStatusChange(StatusChanges changes);
    void updateHScrollbar();
    void doUpdateHScrollbar();
    void updateVScrollbar();
    void doUpdateVScrollbar();
    void updateCaret();
    void recalcCharExtent();
    void updateModifiedStatusForUndoRedo();
    int reparseLines(int startLine, int endLine, bool toDocumentEnd);
    //void reparseLine(int line);
    void uncollapse(const PCodeBlock &foldRange);
    void collapse(const PCodeBlock &foldRange);
    void processCodeBlocksOnLinesInserted(int line, int count);
    void processFoldsOnLinesDeleted(int line, int count);
    void processFoldsOnLineMoved(int from, int to);
    void rescanCodeBlocks(); // rescan for folds
    void internalScanCodeBlocks();
    PCodeBlock foldStartAtLine(int Line) const;
    //QString substringByColumns(const QString& s, int startColumn, int& colLen);
    PCodeBlock foldAroundLine(int line);
    PCodeBlock foldAroundLineEx(int line, bool wantCollapsed, bool acceptFromLine, bool acceptToLine);
    PCodeBlock checkFoldRange(const QVector<PCodeBlock> &blocksToCheck,int line, bool wantCollapsed, bool AcceptFromLine, bool AcceptToLine);
    PCodeBlock foldEndAtLine(int line);
    void paintCaret(QPainter& painter, const QRect rcClip);
    int textOffset() const;
    EditCommand TranslateKeyCode(int key, Qt::KeyboardModifiers modifiers);
    /**
     * Move the caret to right DX columns
     * @param DX
     * @param SelectionCommand
     */
    void moveCaretHorz(int deltaX, bool isSelection);
    void moveCaretVert(int deltaY, bool isSelection);
    void moveCaretAndSelection(const CharPos& ptBefore, const CharPos& ptAfter,
                               bool isSelection, bool ensureCaretVisible = true);
    void moveCaretToLineStart(bool isSelection);
    void moveCaretToLineEnd(bool isSelection, bool ensureCaretVisible = true);
    void doGotoBlockStart(bool isSelection);
    void doGotoBlockEnd(bool isSelection);
    void doGotoEditorStart(bool isSelection);
    void doGotoEditorEnd(bool isSelection);
    void doDeleteSelection();
    void doSetSelTextPrimitive(const QStringList& text);
    void properSetLine(int line, const QString& sLineText, bool parseToEnd);
    void properInsertLine(int line, const QString& sLineText, bool parseToEnd);
    void properDeleteLines(int line, int count, bool parseToEnd);
    void properDeleteLine(int line, bool parseToEnd) { properDeleteLines(line, 1, parseToEnd); }
    void properInsertLines(int line, int count, bool parseToEnd);
    void properMoveLine(int from, int to, bool parseToEnd);

    //primitive edit operations
    void doDeleteText(CharPos startPos, CharPos endPos, SelectionMode mode);
    void doInsertText(const CharPos& pos, const QStringList& text, SelectionMode mode, int startLine, int endLine);
    void doInsertTextByNormalMode(const CharPos& pos, const QStringList& text);
    void doInsertTextByColumnMode(const CharPos& pos, const QStringList& text, int startLine, int endLine);

    void doExpandSelection(const CharPos& pos);
    void doShrinkSelection(const CharPos& pos);

    bool shouldInsertAfterCurrentLine(int line, const QString &newLineText, const QString &newLineText2, bool undoingItem) const;
    bool shouldDeleteNextLine(int line, const QString &currentLineText, const QString &nextLineText) const;
    void calcEffectiveFromToLine(const CharPos &beginPos, const CharPos &endPos, int &fromLine, int &toLine);

    int calcIndentSpaces(int line, const QString& lineText, bool addIndent);

    void processGutterClick(QMouseEvent* event);

    void clearUndo();
    bool canDoBlockIndent() const;

    QRect calculateCaretRect() const;
    QRect calculateInputCaretRect() const;

    //Commands
    void doTrimTrailingSpaces();
    void doDeletePrevChar();
    void doDeleteCurrentChar();
    void doMergeWithNextLine();
    void doMergeWithPrevLine();
    void doDeleteToEOL();
    void doDeleteCurrentToken();

    void doDeleteToWordStart();
    void doDeleteToWordEnd();
    void doDeleteCurrentTokenAndTralingSpaces();
    void doDeleteFromBOL();
    void doDeleteCurrentLine();

    void doDuplicate();
    void doDuplicateSelection();
    void doDuplicateCurrentLine();
    void doMoveSelUp();
    void doMoveSelDown();
    void doClearAll();
    void doBreakLine();
    void doReplaceLine(int line, const QString &lineText);
    void doTabKey();
    void doShiftTabKey();
    void doBlockIndent();
    void doBlockUnindent();
    void internalInputStr(const QString& inputStr);
    void internalClearAll();
    void doInputStr(const QString& chText);
    void doCutToClipboard();
    void doCopyToClipboard();
    void internalDoCopyToClipboard(const QString& s);
    void doPasteFromClipboard();
    void doUndo();
    void doUndoItem();
    void doRedo();
    void doRedoItem();
    void doZoomIn();
    void doZoomOut();
    void doSelectAll();
    void doComment();
    void doUncomment();
    void doToggleComment();
    void doToggleBlockComment();
    void doMouseScroll(bool isDragging, int scrollX, int scrollY);
    void doUndoRedoInput(const QString &oldGlyph,
                       const CharPos &changeStartPos,
                       const CharPos &changeEndPos,
                       SelectionMode changeSelectionMode,
                       size_t changeNumber,
                       bool undo);

    QString getDisplayStringAtLine(int line) const;

    void onLinesDeleted(int line, int count);
    void onLinesInserted(int line, int count);
    void onLineMoved(int from, int to);

    void addGroupUndoBreak();
    void addChangeToUndo(ChangeReason reason, const CharPos& start, const CharPos& end,
                 const QStringList& changeText, SelectionMode selMode);
private slots:
    void onMaxLineWidthChanged();
    void updateHScrollBarLater();
    void onBookMarkOptionsChanged();
    void onGutterChanged();
    void onLinesChanged();
    void onLinesChanging();
    //void onRedoAdded();
    void onScrollTimeout();
    void onDraggingScrollTimeout();
    void onUndoAdded();
    void onSizeOrFontChanged();
    void onChanged();
    void onHScrolled(int value);
    void onVScrolled(int value);

private:
    std::shared_ptr<QImage> mContentImage;
    QVector<PCodeBlock> mCodeBlocks;
    CodeFoldingOptions mCodeFolding;
    int mEditingCount;
    bool mUseCodeFolding;
    bool  mAlwaysShowCaret;
    CharPos mSelectionBegin;
    CharPos mSelectionEnd;
    int mCaretX;
    int mLastCaretColumn;
    int mCaretY;
    int mCharWidth;
    QFont mFontDummy;
    bool mMouseMoved;
    QPoint mMouseOrigin;

    bool mInserting;
    PDocument mDocument;
    int mLinesInWindow;
    int mLeftPos;
    int mPaintLock; // lock counter for internal calculations
    bool mReadOnly;
    int mRightEdge;
    QColor mRightEdgeColor;
    ScrollStyle mScrollBars;
    int mTextHeight;
    int mTopPos;
    PSyntaxer mSyntaxer;
    QColor mSelectedForeground;
    QColor mSelectedBackground;
    QColor mForegroundColor;
    QColor mBackgroundColor;
    QColor mCaretColor;
    PTokenAttribute mRainbowAttr0;
    PTokenAttribute mRainbowAttr1;
    PTokenAttribute mRainbowAttr2;
    PTokenAttribute mRainbowAttr3;

    bool mCaretUseTextColor;
    QColor mActiveLineColor;
    PUndoList mUndoList;
    PRedoList mRedoList;
    QPoint mMouseDownPos;
    EditCaretType mOverwriteCaret;
    EditCaretType  mInsertCaret;
    QPoint mCaretOffset;
    EditKeyStrokes mKeyStrokes;
    bool mModified;
    QDateTime mLastModifyTime;
    double mLineSpacingFactor;
    SelectionMode mActiveSelectionMode; //mode of the active selection
    bool mWantReturns;
    bool mWantTabs;
    Gutter mGutter;
    StateFlags mStateFlags;
    EditorOptions mOptions;
    StatusChanges  mStatusChanges;
    int mLastKey;
    Qt::KeyboardModifiers mLastKeyModifiers;
    QTimer*  mScrollTimer;

    PSynEdit  fChainedEditor;

    bool mIsScrolling;
    int mMergeCaretStatusChangeLock;
    CharPos mCaretBeforeMerging;
    CharPos mSelBeginBeforeMerging;
    CharPos mSelEndBeforeMerging;
    SelectionMode mSelModeBeforeMerging;
    bool mUndoing;
    int mGutterWidth;
    //caret blink related
    int m_blinkTimerId;
    int m_blinkStatus;

    QCursor mDefaultCursor;

    QString mInputPreeditString;

    int mMouseWheelScrollSpeed;
    int mMouseSelectionScrollSpeed;

    CharPos mDragCaretSave;
    CharPos mDragSelBeginSave;
    CharPos mDragSelEndSave;
    bool mDropped;
    int mWheelAccumulatedDeltaX;
    int mWheelAccumulatedDeltaY;

    PFormatter mFormatter;
    GlyphPostionsListCache mGlyphPostionCacheForInputMethod;
    bool mDragging;

friend class QSynEditPainter;

// QWidget interface
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    bool event(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

// QAbstractScrollArea interface
protected:
    bool viewportEvent(QEvent * event) override;

    // QWidget interface
public:
    QVariant inputMethodQuery(Qt::InputMethodQuery property) const override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
};

}

Q_DECLARE_METATYPE(QSynedit::StatusChanges);
#endif // QSYNEDIT_H
