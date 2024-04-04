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
#ifndef SYNEDITSTRINGLIST_H
#define SYNEDITSTRINGLIST_H

#include <QStringList>
#include "syntaxer/syntaxer.h"
#include <QFontMetrics>
#include <QMutex>
#include <QVector>
#include <memory>
#include <QFile>
#include "miscprocs.h"
#include "types.h"
#include "qt_utils/utils.h"

namespace QSynedit {

int searchForSegmentIdx(const QList<int> &segList, int minVal, int maxVal, int value);
int calcSegmentInterval(const QList<int> &segList, int maxVal, int idx);
int segmentIntervalStart(const QList<int> &segList, int minVal, int maxVal, int idx);
QList<int> calcGlyphStartCharList(const QString &text);
void expandGlyphStartCharList(const QString& strAdded, int oldStrLen, QList<int> &glyphStartCharList);

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
    using UpdateWidthFunc = std::function<QList<int>(const QString&, const QList<int> &, int &)>;

    explicit DocumentLine(UpdateWidthFunc updateWidthFunc);
    DocumentLine(const DocumentLine&)=delete;
    DocumentLine& operator=(const DocumentLine&)=delete;

private:
    /**
     * @brief get total count of the glyphs in the line text
     *
     * The char count of the line text may not be the same as the glyphs count
     *
     * @return the glyphs count
     */
    int glyphsCount() const {
        return mGlyphStartCharList.length();
    }

    /**
     * @brief get list of start index of the glyphs in the line text
     * @return start indice of the glyph.
     */
    const QList<int>& glyphStartCharList() const {
        return mGlyphStartCharList;
    }

    /**
     * @brief get list of start position of the glyphs in the line text
     * @return start positions of the glyph (in pixel)
     */
    const QList<int>& glyphStartPositionList();

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
     * @brief get start position of the specified glyph.
     * @param i index of the glyph in the line (starting from 0)
     * @return start position in the line (pixel)
     */
    int glyphStartPosition(int i);

    /**
     * @brief get width （pixels) of the specified glyph.
     * @param i index of the glyph of the line (starting from 0)
     * @return
     */
    int glyphWidth(int i);

    /**
     * @brief get the line text
     * @return the line text
     */
    const QString& lineText() const { return mLineText; }

    /**
     * @brief get the width (pixel) of the line text
     * @return the width (in width)
     */
    int width(bool forceUpdate=false);

    /**
     * @brief get the state of the syntax highlighter after this line is parsed
     * @return
     */
    const SyntaxState& syntaxState() const { return mSyntaxState; }
    /**
     * @brief set the state of the syntax highlighter after this line is parsed
     * @param newSyntaxState
     */
    void setSyntaxState(const SyntaxState &newSyntaxState) { mSyntaxState = newSyntaxState; }

    void setLineText(const QString &newLineText);
    void updateWidth();
    void invalidateWidth() { mWidth = -1; mGlyphStartPositionList.clear(); mIsTempWidth = true;}
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
     * @brief start columns of the glyphs
     *
     * A glyph may occupy more than one columns in the screen.
     * Each elements of mGlyphStartPositionList is the columns occupied by the glyph.
     * The width of a glyph is affected by the font used to display,
     * so it must be recalculated each time the font is changed.
     */
    QList<int> mGlyphStartPositionList;
    /**
     * @brief state of the syntax highlighter after this line is parsed
     *
     * QSynedit use this state to speed up syntax highlight parsing.
     * Which is also used in auto-indent calculating and other functions.
     */
    SyntaxState mSyntaxState;
    /**
     * @brief total width (pixel) of the line text
     *
     * The width of glyphs is affected by the font used to display,
     * so it must be recalculated each time the font is changed.
     */
    int mWidth;
    bool mIsTempWidth;
    UpdateWidthFunc mUpdateWidthFunc;

    friend class Document;
};

typedef std::shared_ptr<DocumentLine> PDocumentLine;

typedef QVector<PDocumentLine> DocumentLines;

typedef std::shared_ptr<DocumentLines> PDocumentLines;

typedef std::shared_ptr<Document> PDocument;

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
    explicit Document(const QFont& font, QObject* parent=nullptr);
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
    int parenthesisLevel(int line);

    /**
     * @brief get nesting level of brackets at the end of the specified line
     *
     * It's thread safe
     *
     * @param line line index (starts from 0)
     * @return
     */
    int bracketLevel(int line);

    /**
     * @brief get nesting level of braces at the end of the specified line
     *
     * It's thread safe
     *
     * @param line line index (starts from 0)
     * @return
     */
    int braceLevel(int line);

    /**
     * @brief get width of the specified line
     *
     * It's thread safe
     *
     * @param line line index (starts frome 0)
     * @return
     */
    int lineWidth(int line);

    /**
     * @brief get width of the specified text / line
     *
     * It's thread safe.
     * If the new text is the same as the line text, it just
     * returns the line width pre-calculated.
     * If the new text is not the same as the line text, it
     * calculates the width of the new text and return.
     *
     * @param line line index (starts frome 0)
     * @param newText the new text
     * @return
     */
    int lineWidth(int line, const QString &newText);

    /**
     * @brief get block (indent) level of the specified line
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @return
     */
    int blockLevel(int line);

    /**
     * @brief get count of new blocks (indent) started on the specified line
     * @param line line index (starts frome 0)
     * @return
     */
    int blockStarted(int line);

    /**
     * @brief get count of blocks (indent) ended on the specified line
     * @param line line index (starts frome 0)
     * @return
     */
    int blockEnded(int line);

    /**
     * @brief get index of the longest line (has the max width)
     *
     * It's thread safe.
     *
     * @return
     */
    int maxLineWidth();

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
    SyntaxState getSyntaxState(int line);

    /**
     * @brief set state of the syntax highlighter after parsing the specified line.
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @param state the new state
     */
    void setSyntaxState(int line, const SyntaxState& state);

    /**
     * @brief get line text of the specified line.
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @return
     */
    QString getLine(int line);

    /**
     * @brief get count of the glyphs on the specified line.
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @return
     */
    int getLineGlyphsCount(int line);

    // /**
    //  * @brief get position list of the glyphs on the specified line.
    //  *
    //  * It's thread safe.
    //  * Each element of the list is the index of the starting char in the line text.
    //  *
    //  * @param line line index (starts frome 0)
    //  * @return
    //  */
    // QList<int> getGlyphPositions(int index);

    /**
     * @brief get count of lines in the document
     *
     * It's thread safe.
     *
     * @return
     */
    int count();

    /**
     * @brief get all the text in the document.
     *
     * Lines are concatenated by line breaks (by lineBreak()).
     * It's thread safe.
     *
     * @return
     */
    QString text();

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
    QStringList contents();

    void putLine(int index, const QString& s, bool notify=true);

    void beginUpdate();
    void endUpdate();

    int addLine(const QString& s);
    void addLines(const QStringList& strings);

    int getTextLength();
    void clear();
    void deleteAt(int index);
    void deleteLines(int index, int numLines);
    void exchange(int index1, int index2);
    void insertLine(int index, const QString& s);
    void insertLines(int index, int numLines);

    void loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding);
    void saveToFile(QFile& file, const QByteArray& encoding,
                    const QByteArray& defaultEncoding, QByteArray& realEncoding);

    QString glyph(int line, int glyphIdx);
    QString glyphAt(int line, int charPos);

    int charToGlyphStartChar(int line, int charPos);
    //int columnToGlyphStartColumn(int line, int charPos);
    /**
     * @brief calculate display width of a string
     *
     * The string may contains the tab char, whose width depends on the tab size and it's position
     *
     * @param str the string to be displayed
     * @param left start x pos of the string
     * @return width of the string, don't including colsBefore
     */
    int stringWidth(const QString &str, int left) const;

    int stringWidth(const QString &str, int left, const QFontMetrics &fontMetrics);

    int glyphCount(int line);
    /**
     * @brief get start index of the chars representing the specified glyph in the specified line.
     *
     * It's thread safe.
     *
     * @param line index of the line in the document (starting from 0)
     * @param glyphIdx index of the glyph in the line (starting from 0)
     * @return char index in the line text (start from 0)
     */
    int glyphStartChar(int line, int glyphIdx);

    /**
     * @brief get count of the chars representing the specified glyph in the specified line.
     *
     * It's thread safe.
     *
     * @param line index of the line in the document (starting from 0)
     * @param glyphIdx index of the glyph in the line (starting from 0)
     * @return
     */
    int glyphLength(int line, int glyphIdx);

    /**
     * @brief get start column of the specified glyph in the specified line.
     *
     * It's thread safe.
     *
     * @param line index of the line in the document (starting from 0)
     * @param glyphIdx index of the glyph in the line (starting from 0)
     * @return the column (starting from 1)
     */
    int glyphStartPostion(int line, int glyphIdx);

    /**
     * @brief get width (in columns) of the specified glyph in the specified line.
     *
     * It's thread safe.
     *
     * @param line index of the line in the document (starting from 0)
     * @param glyphIdx index of the glyph in the line (starting from 0)
     * @return
     */
    int glyphWidth(int line, int glyphIdx);

    int glyphWidth(const QString &glyph, int left) const;

    /**
     * @brief get index of the glyph represented by the specified char
     *
     * It's thread safe.
     *
     * @param line index of the line (starting from 0)
     * @param charIdx position of the char in the line text (starting from 0)
     * @return glyph index in the line (starting from 0)
     */
    int charToGlyphIndex(int line, int charPos);

    /**
     * @brief get index of the glyph displayed on the specified column
     *
     * It's thread safe.
     *
     * @param line index of the line (starting from 0)
     * @param column the column (starting from 1)
     * @return glyph index in the line (starting from 0)
     */
    int xposToGlyphIndex(int line, int xpos);

    int charToGlyphStartPosition(int line, int charPos);
    int xposToGlyphStartChar(int line, int xpos);
    int charToGlyphStartPosition(int line, const QString newStr, int charPos);
    int xposToGlyphStartChar(int line, const QString newStr, int xpos);

    int updateGlyphStartPositionList(
            const QString& lineText,
            const QList<int> &glyphStartCharList,
            int startChar, int endChar,
            const QFontMetrics &fontMetrics,
            QList<int> &glyphStartPositionList,
            int left, int &right, int &startGlyph, int &endGlyph) const;

    bool getAppendNewLineAtEOF();
    void setAppendNewLineAtEOF(bool appendNewLineAtEOF);

    NewlineType getNewlineType();
    void setNewlineType(const NewlineType &fileEndingType);

    bool empty();

    int tabSize() const {
        return mTabSize;
    }

    int tabWidth() const {
        return mTabSize * spaceWidth();
    }

    int spaceWidth() const {
        if (mForceMonospace)
            return mCharWidth;
        return mSpaceWidth;
    }

    void setTabSize(int newTabSize);

    const QFontMetrics &fontMetrics() const;
    void setFont(const QFont &newFont);

    bool forceMonospace() const;
    void setForceMonospace(bool newForceMonospace);

public slots:
    void invalidateAllLineWidth();

signals:
    void changed();
    void changing();
    void cleared();
    void deleted(int startLine, int count);
    void inserted(int startLine, int count);
    void putted(int line);
    void maxLineWidthChanged();
protected:
    QString getTextStr() const;
    void setUpdateState(bool Updating);
    void insertItem(int line, const QString& s);
    void addItem(const QString& s);
    void putTextStr(const QString& text);
    void internalClear();
private:
    bool lineWidthValid(int line);
    void beginSetLinesWidth();
    void endSetLinesWidth();
    void setLineWidth(int line, int newWidth, const QList<int> glyphStartPositionList);
    void updateMaxLineWidthChanged();
    void updateMaxLineWidthAndNotify();

    int glyphWidth(const QString& glyph, int left,
                   const QFontMetrics &fontMetrics,
                   bool forceMonospace) const;

    int xposToGlyphIndex(int strWidth, QList<int> glyphPositionList, int xpos) const;
    int charToGlyphIndex(const QString& str, QList<int> glyphStartCharList, int charPos) const;
    QList<int> calcLineWidth(const QString& lineText, const QList<int> &glyphStartCharList, int &width);
    QList<int> calcGlyphPositionList(const QString& lineText, const QList<int> &glyphStartCharList,
                                     const QFontMetrics &fontMetrics,
                                     int left, int &right) const;
    QList<int> calcGlyphPositionList(const QString& lineText, const QList<int> &glyphStartCharList, int left, int &right) const;
    QList<int> calcGlyphPositionList(const QString& lineText, int &width) const;
    QList<int> getGlyphStartCharList(int line, const QString &lineText);
    QList<int> getGlyphStartCharList(int line);
    QList<int> getGlyphStartPositionList(int line);
    int getLineWidth(int line);
    bool tryLoadFileByEncoding(QByteArray encodingName, QFile& file);
    void loadUTF16BOMFile(QFile& file);
    void loadUTF32BOMFile(QFile& file);
    void saveUTF16File(QFile& file, QTextCodec* codec);
    void saveUTF32File(QFile& file, QTextCodec* codec);

private:
    DocumentLines mLines;

    DocumentLine::UpdateWidthFunc mUpdateDocumentLineWidthFunc;

    //SynEdit* mEdit;

    QFontMetrics mFontMetrics;
    int mTabSize;
    int mCharWidth;
    int mSpaceWidth;
    //int mCount;
    //int mCapacity;
    NewlineType mNewlineType;
    bool mAppendNewLineAtEOF;
    int mIndexOfLongestLine;
    int mUpdateCount;
    bool mForceMonospace;

    int mSetLineWidthLockCount;
    bool mMaxLineChangedInSetLinesWidth;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QRecursiveMutex mMutex;
#else
    QMutex mMutex;
#endif

    friend class QSynEditPainter;
};

enum class ChangeReason {
    Insert,
    Delete,
    Caret, //just restore the Caret, allowing better Undo behavior
    Selection, //restore Selection
    GroupBreak,
    LeftTop,
    LineBreak,
    MoveSelectionUp,
    MoveSelectionDown,
    ReplaceLine,
    Nothing // undo list empty
  };

class UndoItem {
private:
    ChangeReason mChangeReason;
    SelectionMode mChangeSelMode;
    BufferCoord mChangeStartPos;
    BufferCoord mChangeEndPos;
    QStringList mChangeText;
    size_t mChangeNumber;
    unsigned int mMemoryUsage;
public:
    UndoItem(ChangeReason reason,
        SelectionMode selMode,
        BufferCoord startPos,
        BufferCoord endPos,
        const QStringList& text,
        int number);

    ChangeReason changeReason() const;
    SelectionMode changeSelMode() const;
    BufferCoord changeStartPos() const;
    BufferCoord changeEndPos() const;
    QStringList changeText() const;
    size_t changeNumber() const;
    unsigned int memoryUsage() const;
};

using PUndoItem = std::shared_ptr<UndoItem>;

class UndoList : public QObject {
    Q_OBJECT
public:
    explicit UndoList();

    void addChange(ChangeReason reason, const BufferCoord& start, const BufferCoord& end,
      const QStringList& changeText, SelectionMode selMode);

    void restoreChange(ChangeReason reason, const BufferCoord& start, const BufferCoord& end,
                       const QStringList& changeText, SelectionMode selMode, size_t changeNumber);

    void restoreChange(PUndoItem item);

    void addGroupBreak();
    void beginBlock();
    void endBlock();

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

    int maxMemoryUsage() const;
    void setMaxMemoryUsage(int newMaxMemoryUsage);

signals:
    void addedUndo();
protected:
    void ensureMaxEntries();
    bool inBlock();
    unsigned int getNextChangeNumber();
    void addMemoryUsage(PUndoItem item);
    void reduceMemoryUsage(PUndoItem item);
protected:
    size_t mBlockChangeNumber;
    int mBlockLock;
    int mBlockCount; // count of action blocks;
    int mMemoryUsage;
    size_t mLastPoppedItemChangeNumber;
    size_t mLastRestoredItemChangeNumber;
    bool mFullUndoImposible;
    QVector<PUndoItem> mItems;
    int mMaxUndoActions;
    int mMaxMemoryUsage;
    unsigned int mNextChangeNumber;
    unsigned int mInitialChangeNumber;
    bool mInsideRedo;
};

class RedoList : public QObject {
    Q_OBJECT
public:
    explicit RedoList();

    void addRedo(ChangeReason AReason, const BufferCoord& AStart, const BufferCoord& AEnd,
                 const QStringList& ChangeText, SelectionMode SelMode, size_t changeNumber);
    void addRedo(PUndoItem item);

    void clear();
    ChangeReason lastChangeReason();
    bool isEmpty();
    PUndoItem peekItem();
    PUndoItem popItem();

    bool canRedo();
    int itemCount();

protected:
    QVector<PUndoItem> mItems;
};


using PUndoList = std::shared_ptr<UndoList>;
using PRedoList = std::shared_ptr<RedoList>;

}

#endif // SYNEDITSTRINGLIST_H
