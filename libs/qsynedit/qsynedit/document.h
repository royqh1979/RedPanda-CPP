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

struct DocumentLine {
  QString lineText;
  SyntaxState syntaxState;
  int columns;  //
public:
  explicit DocumentLine();
  DocumentLine(const DocumentLine&)=delete;
  DocumentLine& operator=(const DocumentLine&)=delete;
};

typedef std::shared_ptr<DocumentLine> PDocumentLine;

typedef QVector<PDocumentLine> DocumentLines;

typedef std::shared_ptr<DocumentLines> PDocumentLines;

class Document;

typedef std::shared_ptr<Document> PDocument;

class BinaryFileError : public FileError {
public:
    explicit BinaryFileError (const QString& reason);
};

class Document : public QObject
{  
    Q_OBJECT
public:
    explicit Document(const QFont& font, const QFont& nonAsciiFont, QObject* parent=nullptr);
    Document(const Document&)=delete;
    Document& operator=(const Document&)=delete;

    int parenthesisLevel(int index);
    int bracketLevel(int index);
    int braceLevel(int index);
    int lineColumns(int index);
    int blockLevel(int index);
    int blockStarted(int index);
    int blockEnded(int index);
    int lengthOfLongestLine();
    QString lineBreak() const;
    SyntaxState getSyntaxState(int index);
    void setSyntaxState(int index, const SyntaxState& range);
    QString getLine(int index);
    int count();
    QString text();
    void setText(const QString& text);
    void setContents(const QStringList& text);
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
    int stringColumns(const QString& line, int colsBefore) const;
    int charColumns(QChar ch) const;

    bool getAppendNewLineAtEOF();
    void setAppendNewLineAtEOF(bool appendNewLineAtEOF);

    NewlineType getNewlineType();
    void setNewlineType(const NewlineType &fileEndingType);

    bool empty();

    void resetColumns();
    int tabWidth() const {
        return mTabWidth;
    }
    void setTabWidth(int newTabWidth);

    const QFontMetrics &fontMetrics() const;
    void setFontMetrics(const QFont &newFont, const QFont& newNonAsciiFont);

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
    QString getTextStr() const;
    void setUpdateState(bool Updating);
    void insertItem(int Index, const QString& s);
    void addItem(const QString& s);
    void putTextStr(const QString& text);
    void internalClear();
private:
    bool tryLoadFileByEncoding(QByteArray encodingName, QFile& file);
    void loadUTF16BOMFile(QFile& file);
    void loadUTF32BOMFile(QFile& file);
    void saveUTF16File(QFile& file, QTextCodec* codec);
    void saveUTF32File(QFile& file, QTextCodec* codec);

private:
    DocumentLines mLines;

    //SynEdit* mEdit;

    QFontMetrics mFontMetrics;
    QFontMetrics mNonAsciiFontMetrics;
    int mTabWidth;
    int mCharWidth;
    //int mCount;
    //int mCapacity;
    NewlineType mNewlineType;
    bool mAppendNewLineAtEOF;
    int mIndexOfLongestLine;
    int mUpdateCount;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QRecursiveMutex mMutex;
#else
    QMutex mMutex;
#endif

    int calculateLineColumns(int Index);
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
