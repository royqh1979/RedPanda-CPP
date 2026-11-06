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
#include "document.h"
#include "glyphcalculator.h"
#include "qt_utils/utils.h"
#include "syntaxer/textfile.h"
#include <QDataStream>
#include <QFile>
#include <QTextStream>
#include <QMutexLocker>
#include <stdexcept>
#include <QMessageBox>
#include <optional>
#include "qt_utils/charsetinfo.h"
#include <QDateTime>
#include <QDebug>

namespace QSynedit {

Document::Document(QObject *parent):
    QObject{parent},
    mMutex{}
{
    mAppendNewLineAtEOF = true;
    mNewlineType = NewlineType::Windows;
    mUpdateCount = 0;
    mUndoList = std::make_shared<UndoList>();
    mRedoList = std::make_shared<RedoList>();
    mSyntaxer = std::make_shared<TextSyntaxer>();
    // adding a new change invalidates the redo history; this is a
    // document-level concern (the stacks are shared by all views)
    connect(mUndoList.get(), &UndoList::addedUndo, this, [this](){
        if (!mUndoList->insideRedo() &&
                mUndoList->peekItem() &&
                mUndoList->peekItem()->changeReason()!=ChangeReason::GroupBreak)
            mRedoList->clear();
    });
    ensureHasLine();
    // a fresh document is born self-consistent: states are never null
    reparseAll();
}

PUndoList Document::undoList() const
{
    return mUndoList;
}

PRedoList Document::redoList() const
{
    return mRedoList;
}

bool Document::modified() const
{
    return mModified;
}

void Document::setModified(bool value, bool skipUndo)
{
    if (value == mModified)
        return;
    mModified = value;
    if (!skipUndo) {
        if (value) {
            mUndoList->clear();
            mRedoList->clear();
        } else {
            mUndoList->setInitialState();
        }
    }
    emit modifiedChanged();
}

PSyntaxer Document::createSyntaxerInstance() const
{
    QMutexLocker locker(&mMutex);
    return mSyntaxer->createInstance();
}

ProgrammingLanguage Document::syntaxerLanguage() const
{
    QMutexLocker locker(&mMutex);
    return mSyntaxer->language();
}

void Document::setSyntaxer(const PSyntaxer &syntaxer)
{
    Q_ASSERT(syntaxer);
    if (!syntaxer)
        return;
    {
        QMutexLocker locker(&mMutex);
        mSyntaxer = syntaxer;
        reparseAll();
    }
    emit syntaxerChanged();
}

void Document::reparseAll()
{
    QMutexLocker locker(&mMutex);
    mSyntaxer->resetState();
    for (int i = 0; i < mLines.size(); i++) {
        mSyntaxer->setLine(i, mLines[i]->lineText(), mLines[i]->lineSeq());
        mSyntaxer->nextToEol();
        mLines[i]->setSyntaxState(mSyntaxer->getState());
    }
    // wholesale reparse: the block pairing must be rebuilt
    mFoldStateDirty = true;
    mReparseToLine = mLines.size();
    emit linesReparsed(0, mLines.size());
}

// code-block pairing deltas of two syntax states: blockStarted/blockEnded
// are properties of the line's own text and never go stale when lines
// shift (unlike blockLevel, which is position-dependent and would
// false-positive when a later batch merely repairs stale states). A null
// old state means a brand-new line, which changes the pairing only if it
// opens or closes a block itself
static bool sameCodeBlockDeltas(const PSyntaxState &oldState, const PSyntaxState &newState)
{
    if (!oldState) {
        return !(newState->blockStarted
                 || newState->blockEnded);
    }
    return oldState->blockStarted == newState->blockStarted
            && oldState->blockEnded == newState->blockEnded;
}

int Document::reparseLines(int startLine, int endLine, bool parseToEnd)
{
    QMutexLocker locker(&mMutex);
    bool toDocumentEnd = parseToEnd && mSyntaxer->needsLineState();
    int maxLine = toDocumentEnd ? mLines.size() : endLine;
    startLine = std::max(0,startLine);
    endLine = std::min(endLine, (int)mLines.size());
    maxLine = std::min(maxLine, (int)mLines.size());

    if (startLine >= maxLine) {
        emit linesReparsed(startLine, INT_MAX);
        return startLine;
    }

    if (startLine == 0) {
        mSyntaxer->resetState();
    } else if (PSyntaxState prevState = getSyntaxState(startLine-1)) {
        mSyntaxer->setState(prevState);
    } else {
        // the predecessor's state is unknown (e.g. content filled via the
        // bulk-load path, which does not reparse): parse from the start
        mSyntaxer->resetState();
        startLine = 0;
    }
    int line = startLine;
    do {
        mSyntaxer->setLine(line, getLine(line), getLineSeq(line));
        mSyntaxer->nextToEol();
        PSyntaxState state = mSyntaxer->getState();
        PSyntaxState oldState = getSyntaxState(line);
        if (line >= endLine && state->equals(oldState) ) {
            break;
        }
        if (!mFoldStateDirty && !sameCodeBlockDeltas(oldState, state))
            mFoldStateDirty = true;
        setSyntaxState(line, state);
        line++;
    } while (line < maxLine);
    // `line` is the stabilization line. When the reparse converged there
    // (verified equal, or reached the document end), states through it are
    // final; when it only ran to the requested range end (parseToEnd=false
    // without convergence), states below it are unverified.
    bool converged = (line < maxLine) || (line == (int)mLines.size());
    mReparseToLine = std::max(mReparseToLine, converged ? line + 1 : line);
    emit linesReparsed(startLine, line);
    return line;
}

bool Document::lineHasCodeBlockDelta(int line) const
{
    PSyntaxState state = mLines[line]->syntaxState();
    if (!state)
        // unknown: conservative
        return true;
    return state->blockStarted || state->blockEnded;
}

void Document::notifyUndoRedoStateChanged()
{
    emit undoRedoStateChanged();
}

int Document::parenthesisLevel(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        return mLines[line]->syntaxState()->parenthesisLevel;
    } else
        return 0;
}

int Document::bracketLevel(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        return mLines[line]->syntaxState()->bracketLevel;
    } else
        return 0;
}

int Document::braceLevel(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        return mLines[line]->syntaxState()->braceLevel;
    } else
        return 0;
}

int Document::blockLevel(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        return mLines[line]->syntaxState()->blockLevel;
    } else
        return 0;
}

int Document::blockStarted(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        return mLines[line]->syntaxState()->blockStarted;
    } else
        return 0;
}

int Document::blockEnded(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line>=0 && line < mLines.size()) {
        int result = mLines[line]->syntaxState()->blockEnded;
//        if (index+1 < mLines.size())
//            result += mLines[index+1]->syntaxState.blockEndedLastLine;
        return result;
    } else
        return 0;
}

QString Document::lineBreak() const
{
    switch(mNewlineType) {
    case NewlineType::Unix:
        return "\n";
    case NewlineType::Windows:
        return "\r\n";
    case NewlineType::MacOld:
        return "\r";
    }
    return "\n";
}

PSyntaxState Document::getSyntaxState(int line) const
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(line>=0 && line < mLines.size());
    // Crash Prevention
    if (line<0 || line>=count())
        return std::make_shared<SyntaxState>();
    PSyntaxState state = mLines[line]->syntaxState();
    if (!state)
        // not parsed yet (e.g. freshly inserted line): hand out an empty
        // state instead of nullptr so callers never have to null-check
        return std::make_shared<SyntaxState>();
    return state;
}

void Document::insertItem(int line, const QString &s)
{
    beginUpdate();
    PDocumentLine documentLine = std::make_shared<DocumentLine>();
    documentLine->setLineText(s);
    mLines.insert(line,documentLine);
    mLineSeqIndice.insert(documentLine->lineSeq(), documentLine);
    Q_ASSERT(mLineSeqIndice.count() == mLines.count());
    recordChange(DocumentChangeKind::Insert, line, 1);
    endUpdate();
}

void Document::addItem(const QString &s)
{
    beginUpdate();
    PDocumentLine line = std::make_shared<DocumentLine>();
    line->setLineText(s);
    mLines.append(line);
    mLineSeqIndice.insert(line->lineSeq(), line);
    Q_ASSERT(mLineSeqIndice.count() == mLines.count());
    recordChange(DocumentChangeKind::Insert, mLines.count()-1, 1);
    endUpdate();
}

bool Document::getAppendNewLineAtEOF() const
{
    QMutexLocker locker(&mMutex);
    return mAppendNewLineAtEOF;
}

void Document::setAppendNewLineAtEOF(bool appendNewLineAtEOF)
{
    QMutexLocker locker(&mMutex);
    mAppendNewLineAtEOF = appendNewLineAtEOF;
}

void Document::setSyntaxState(int line, const PSyntaxState& state)
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(line >=0 && line < mLines.count());
    mLines[line]->setSyntaxState(state);
}

QString Document::getLine(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    return mLines[line]->lineText();
}

size_t Document::getLineSeq(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return 0;
    return mLines[line]->lineSeq();
}

int Document::count() const
{
    QMutexLocker locker(&mMutex);
    return mLines.count();
}

QString Document::text() const
{
    QMutexLocker locker(&mMutex);
    return getTextStr();
}

void Document::setText(const QString &text)
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    auto action = finally([this]{
        endUpdate();
    });
    internalClear();
    if (!text.isEmpty()) {
        int pos = 0;
        int start;
        while (pos < text.length()) {
            start = pos;
            while (pos<text.length()) {
                if (text[pos] == '\r' || text[pos] == '\n') {
                    break;
                }
                pos++;
            }
            addItem(text.mid(start,pos-start));
            if (pos>=text.length())
                break;
            if (text[pos] == '\r')
                pos++;
            if (text[pos] == '\n')
                pos++;
        }
    }
    ensureHasLine();
    // the bulk-load path bypasses the edit primitives, so it reparses
    // explicitly: after setText the document is always fully parsed
    reparseAll();
}

void Document::setContents(const QStringList &text)
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    internalClear();
    if (text.count() > 0) {
        foreach (const QString& s,text) {
            addItem(s);
        }
    }
    ensureHasLine();
    // the bulk-load path bypasses the edit primitives, so it reparses
    // explicitly: after setContents the document is always fully parsed
    reparseAll();
    endUpdate();
}

QStringList Document::content() const
{
    QMutexLocker locker(&mMutex);
    QStringList result;
    DocumentLines list = mLines;
    foreach (const PDocumentLine& line, list) {
        result.append(line->lineText());
    }
    return result;
}

void Document::beginUpdate()
{
    if (mUpdateCount == 0) {
        mFoldStateDirty = false;
        mReparseToLine = -1;
        emit changing();
    }
    mUpdateCount++;
}

void Document::endUpdate()
{
    Q_ASSERT(mUpdateCount > 0);
    mUpdateCount--;
    if (mUpdateCount == 0) {
        if (!mPendingChanges.isEmpty()) {
            DocumentChangeSet changes;
            changes.swap(mPendingChanges);
            // NOTE: no "dirty implies reparsed" assert here: the convergence
            // contract holds at the VIEW-edit level (QSynEdit edit paths
            // always reparse within the batch; asserted in
            // onDocumentContentChanged). Directly driven primitives may
            // legitimately emit dirty-without-reparse batches (e.g. a lone
            // deleteLine with parseToEnd=false).
            emit contentChanged(changes, mFoldStateDirty, mReparseToLine);
        }
        emit changed();
    }
}

bool Document::inUpdate() const
{
    return mUpdateCount > 0;
}

int Document::contentVersion() const
{
    return mContentVersion.load();
}

void Document::recordChange(DocumentChangeKind kind, int line, int count)
{
    // sanity checks against the post-mutation state
    Q_ASSERT(line >= 0);    
#ifdef QT_DEBUG
    switch (kind) {
    case DocumentChangeKind::Insert:
    case DocumentChangeKind::SetLine:
        Q_ASSERT(count > 0 && line + count <= mLines.count());
        break;
    case DocumentChangeKind::Delete:
        // after removal, line may equal count() if lines were deleted at the end
        Q_ASSERT(count > 0 && line <= mLines.count());
        break;
    case DocumentChangeKind::Move:
        // for Move, count carries the target line
        Q_ASSERT(line < mLines.count() && count >= 0 && count < mLines.count());
        break;
    }
#endif
    // first change of the batch: bump the content version so background
    // wrap-layout workers notice the modification and abort early
    if (mPendingChanges.isEmpty())
        mContentVersion.fetch_add(1);
    // Only same-kind, adjacent entries coalesce. No cross-kind merging or
    // cancellation: the sequence must faithfully mirror what the primitives
    // did, so consumers can replay it naively.
    if (!mPendingChanges.isEmpty()) {
        DocumentChange &last = mPendingChanges.last();
        if (last.kind == kind) {
            switch (kind) {
            case DocumentChangeKind::Insert:
            case DocumentChangeKind::SetLine:
                if (last.line + last.count == line) {
                    last.count += count;
                    return;
                }
                break;
            case DocumentChangeKind::Delete:
                if (last.line == line) {
                    last.count += count;
                    return;
                }
                break;
            case DocumentChangeKind::Move:
                break;
            }
        }
    }
    mPendingChanges.append(DocumentChange{kind, line, count});
}


int Document::addLine(const QString &s)
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    int result = mLines.count();
    insertItem(result, s);
    endUpdate();
    return result;
}

void Document::addLines(const QStringList &strings)
{
    QMutexLocker locker(&mMutex);
    if (strings.count() > 0) {
        beginUpdate();
        auto action = finally([this]{
            endUpdate();
        });
        for (const QString& s:strings) {
            addItem(s);
        }
    }
}

int Document::getTextLength() const
{
    QMutexLocker locker(&mMutex);
    int result = 0;
    foreach (const PDocumentLine& line, mLines ) {
        result += line->glyphsCount();
        if (mNewlineType == NewlineType::Windows) {
            result += 2;
        } else {
            result += 1;
        }
    }
    return result;
}

void Document::clear()
{
    QMutexLocker locker(&mMutex);
    beginUpdate();
    // keep the last line as the remaining empty line instead of
    // deleting everything and inserting a fresh one
    if (mLines.count() > 1)
        deleteLines(0, mLines.count()-1, false);
    if (!mLines[0]->lineText().isEmpty()) {
        putLine(0, "", true);
    } else {
        // the kept line's state may be stale (it was parsed in its old
        // position); reparse it so the cleared document is self-consistent
        reparseLines(0, 1, true);
    }
    endUpdate();
}

void Document::deleteLines(int index, int numLines, bool parseToEnd)
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(numLines>0);
    Q_ASSERT(index >= 0 && index < mLines.count());
    beginUpdate();
    int linesAfter = mLines.count() - (index + numLines);
    if (linesAfter < 0) {
       numLines = mLines.count() - index;
    }
    for (int i=0;i<numLines;i++) {
        mLineSeqIndice.remove(mLines[index+i]->lineSeq());
        if (!mFoldStateDirty && lineHasCodeBlockDelta(index+i))
            mFoldStateDirty = true;
    }
    mLines.remove(index,numLines);
    Q_ASSERT(mLineSeqIndice.count() == mLines.count());
    recordChange(DocumentChangeKind::Delete, index, numLines);
    ensureHasLine();
    if (parseToEnd)
        reparseLines(index, index+numLines, true);
    endUpdate();
}

void Document::moveLine(int from, int to, bool parseToEnd)
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(from >= 0 && from < mLines.count());
    Q_ASSERT(to >= 0 && to < mLines.count());
    beginUpdate();
    if (!mFoldStateDirty && lineHasCodeBlockDelta(from))
        mFoldStateDirty = true;
    PDocumentLine temp = mLines[from];
    mLines.remove(from);
    mLines.insert(to, temp);
    recordChange(DocumentChangeKind::Move, from, to);
    Q_ASSERT(mLineSeqIndice.count() == mLines.count());
    reparseLines(std::min(from,to), std::max(from,to)+1, parseToEnd);
    endUpdate();
}

void Document::insertLine(int index, const QString &s, bool parseToEnd)
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(index>=0 && index<=mLines.count());
    beginUpdate();
    insertItem(index, s);
    reparseLines(index, index+1, parseToEnd);
    endUpdate();
}

void Document::deleteLine(int index, bool parseToEnd)
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(index>=0 && index<mLines.count());
    beginUpdate();
    if (!mFoldStateDirty && lineHasCodeBlockDelta(index))
        mFoldStateDirty = true;
    mLineSeqIndice.remove(mLines[index]->lineSeq());
    mLines.removeAt(index);
    Q_ASSERT(mLineSeqIndice.count() == mLines.count());
    recordChange(DocumentChangeKind::Delete, index, 1);
    ensureHasLine();
    if (parseToEnd)
        reparseLines(index, index+1, true);
    endUpdate();
}

QString Document::getTextStr() const
{
    QString result;
    for (int i=0;i<mLines.count()-1;i++) {
        const PDocumentLine& line = mLines[i];
        result.append(line->lineText());
        result.append(lineBreak());
    }
    if (mLines.length()>0) {
        result.append(mLines.back()->lineText());
    }
    return result;
}

void Document::putLine(int index, const QString &s, bool parseToEnd) {
    QMutexLocker locker(&mMutex);
    Q_ASSERT(index>=0 && index < mLines.count());
    beginUpdate();
    if (mLines[index]->lineText() != s) {
        mLines[index]->setLineText(s);
        recordChange(DocumentChangeKind::SetLine, index, 1);
    }
    // the reparse is NOT skipped for unchanged text: its job is not (only) to
    // parse the new text, but to repair potentially stale syntax states from
    // this line onward (e.g. doDeleteText deletes lines with parseToEnd=false
    // and relies on the following setLine(..., true) to reconverge the states
    // of the shifted lines, whose text may be unchanged)
    reparseLines(index, index+1, parseToEnd);
    endUpdate();
}

void Document::insertLines(int index, int numLines, bool parseToEnd)
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(index>=0 && index<=mLines.count());
    Q_ASSERT(numLines>0);
    beginUpdate();
    PDocumentLine line;
    mLines.insert(index,numLines,line);
    for (int i=index;i<index+numLines;i++) {
        mLines[i] = std::make_shared<DocumentLine>();
        mLineSeqIndice.insert(mLines[i]->lineSeq(), mLines[i]);
    }
    Q_ASSERT(mLineSeqIndice.count() == mLines.count());
    recordChange(DocumentChangeKind::Insert, index, numLines);
    reparseLines(index, index+numLines, parseToEnd);
    endUpdate();
}

bool Document::tryLoadFileByEncoding(QByteArray encodingName, QFile& file) {
    TextDecoder decoder(encodingName);
    if (!decoder.isValid())
        return false;
    file.reset();
    internalClear();
    while (true) {
        if (file.atEnd()){
            break;
        }
        QByteArray line = file.readLine();
        if (line.endsWith("\r\n")) {
            line.remove(line.length()-2,2);
        } else if (line.endsWith("\r")) {
            line.remove(line.length()-1,1);
        } else if (line.endsWith("\n")){
            line.remove(line.length()-1,1);
        }
        auto [ok, newLine] = decoder.decode(line);
        if (!ok) {
            return false;
        }
        addItem(newLine);
    }
    return true;
}

void Document::loadUTF16BOMFile(QFile &file)
{
    TextDecoder decoder = TextDecoder::decoderForUtf16();
    if (!decoder.isValid())
        return;
    file.reset();
    internalClear();
    QByteArray buf = file.readAll();
    if (buf.length()<2)
        return;
    QString text = decoder.decodeUnchecked(buf.mid(2));
    this->setText(text);
}

void Document::loadUTF32BOMFile(QFile &file)
{
    TextDecoder decoder = TextDecoder::decoderForUtf32();
    if (!decoder.isValid())
        return;
    file.reset();
    internalClear();
    QByteArray buf = file.readAll();
    if (buf.length()<4)
        return;
    QString text = decoder.decodeUnchecked(buf.mid(4));
    this->setText(text);
}

PDocumentLine Document::findLineBySeq(size_t lineSeq) const
{
    QMutexLocker locker(&mMutex);
    return mLineSeqIndice.value(lineSeq);
}

#ifdef QSYNEDIT_TEST
QList<int> Document::getGlyphStartCharListForTest(int line) const
{
    QMutexLocker locker(&mMutex);
    return mLines[line]->glyphStartCharList();;
}
#endif
void Document::loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding)
{
    QMutexLocker locker(&mMutex);
    QFile file(filename);
    if (!file.open(QFile::ReadOnly))
        throw FileError(tr("Can't open file '%1' for read!").arg(file.fileName()));
    beginUpdate();
    internalClear();
    auto action = finally([this]{
        ensureHasLine();
        // the bulk-load path bypasses the edit primitives, so it reparses
        // explicitly: after loadFromFile the document is always fully parsed
        reparseAll();
        endUpdate();
    });
    //test for utf8 / utf 8 bom
    if (encoding == ENCODING_AUTO_DETECT) {
        if (file.atEnd()) {
            realEncoding = ENCODING_ASCII;
            return;
        }
        QByteArray line = file.readLine();
        std::optional<TextDecoder> decoder;
        bool needReread = false;
        bool allAscii = true;
        //test for BOM
        if ((line.length()>=3) && ((unsigned char)line[0]==0xEF) && ((unsigned char)line[1]==0xBB) && ((unsigned char)line[2]==0xBF) ) {
            realEncoding = ENCODING_UTF8_BOM;
            line = line.mid(3);
            decoder = TextDecoder::decoderForUtf8();
        } else if ((line.length()>=4) && ((unsigned char)line[0]==0xFF) && ((unsigned char)line[1]==0xFE)
                   && ((unsigned char)line[2]==0x00)
                   && ((unsigned char)line[3]==0x00)) {
            realEncoding = ENCODING_UTF32_BOM;
            loadUTF32BOMFile(file);
            return;
        } else if ((line.length()>=2) && ((unsigned char)line[0]==0xFF) && ((unsigned char)line[1]==0xFE)) {
            realEncoding = ENCODING_UTF16_BOM;
            loadUTF16BOMFile(file);
            return;
        } else {
            realEncoding = ENCODING_UTF8;
            decoder = TextDecoder::decoderForUtf8();
        }
        if (!decoder.has_value())
            throw FileError(tr("Can't load codec '%1'!").arg(QString(realEncoding)));
        if (line.endsWith("\r\n")) {
            mNewlineType = NewlineType::Windows;
        } else if (line.endsWith("\n")) {
            mNewlineType = NewlineType::Unix;
        } else if (line.endsWith("\r")) {
            mNewlineType = NewlineType::MacOld;
        }

        internalClear();
        while (true) {
            if (line.endsWith("\r\n")) {
                line.remove(line.length()-2,2);
            } else if (line.endsWith("\r")) {
                line.remove(line.length()-1,1);
            } else if (line.endsWith("\n")){
                line.remove(line.length()-1,1);
            }
            if (isBinaryContent(line))
                throw BinaryFileError(tr("'%1' is a binaray File!").arg(filename));
            if (allAscii) {
                allAscii = isTextAllAscii(line);
            }
            if (allAscii) {
                addItem(QString::fromLatin1(line));
            } else {
                auto [ok, newLine] = decoder->decode(line);
                if (!ok) {
                    needReread = true;
                    break;
                }
                addItem(newLine);
            }
            if (file.atEnd()){
                break;
            }
            line = file.readLine();
        }
        if (!needReread) {
            if (allAscii)
                realEncoding = ENCODING_ASCII;
            return;
        }
        realEncoding = pCharsetInfoManager->getDefaultSystemEncoding();
        if (tryLoadFileByEncoding(realEncoding,file)) {
            return;
        }
        QList<PCharsetInfo> charsets = pCharsetInfoManager->findCharsetByLocale(pCharsetInfoManager->localeName());
        if (!charsets.isEmpty()) {

            QSet<QByteArray> encodingSet;
            for (int i=0;i<charsets.size();i++) {
                encodingSet.insert(charsets[i]->name);
            }
            encodingSet.remove(realEncoding);
            foreach (const QByteArray& encodingName,encodingSet) {
                if (encodingName == ENCODING_UTF8)
                    continue;
                if (tryLoadFileByEncoding(encodingName,file)) {
                    //qDebug()<<encodingName;
                    realEncoding = encodingName;
                    return;
                }
            }
        }
    } else {
        realEncoding = encoding;
    }

    if (realEncoding == ENCODING_SYSTEM_DEFAULT) {
        realEncoding = pCharsetInfoManager->getDefaultSystemEncoding();
    }
    file.reset();
    QByteArray data = file.readAll();
    QString text;
    QTextStream textStream(&text);
    if (realEncoding == ENCODING_UTF8_BOM) {
        textStream.setAutoDetectUnicode(true);
        text = QString::fromUtf8(data);
    } else {
        textStream.setAutoDetectUnicode(false);
        TextDecoder decoder(realEncoding);
        text = decoder.decodeUnchecked(data);
    }
    QString lineText;
    internalClear();
    while (textStream.readLineInto(&lineText)) {
        if (lineText.endsWith("\r\n")) {
            lineText.remove(lineText.length()-2,2);
        } else if (lineText.endsWith("\r")) {
            lineText.remove(lineText.length()-1,1);
        } else if (lineText.endsWith("\n")){
            lineText.remove(lineText.length()-1,1);
        }
        addItem(lineText);
    }
}



void Document::saveToFile(QFile &file, const QByteArray& encoding, QByteArray& realEncoding) const
{
    Q_ASSERT(encoding!=ENCODING_AUTO_DETECT);
    QMutexLocker locker(&mMutex);
    std::optional<TextEncoder> encoder;
    realEncoding = encoding;
    QString codecName = realEncoding;
    if (realEncoding == ENCODING_UTF16_BOM || realEncoding == ENCODING_UTF16) {
        encoder = TextEncoder::encoderForUtf16LE();
        codecName = ENCODING_UTF16;
    } else if (realEncoding == ENCODING_UTF32_BOM || realEncoding == ENCODING_UTF32) {
        encoder = TextEncoder::encoderForUtf32LE();
        codecName = ENCODING_UTF32;
    } else if (realEncoding == ENCODING_UTF8_BOM) {
        encoder = TextEncoder::encoderForUtf8();
        codecName = ENCODING_UTF8;
    } else if (realEncoding == ENCODING_SYSTEM_DEFAULT) {
        encoder = TextEncoder::encoderForSystem();
        codecName = realEncoding;
    } else {
        encoder = TextEncoder(realEncoding);
    }
    if (!encoder.has_value() || !encoder->isValid())
        throw FileError(tr("Can't load codec '%1'!").arg(codecName));

    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        throw FileError(tr("Can't open file '%1' for save!").arg(file.fileName()));
    if (mLines.isEmpty())
        return;
    if (realEncoding == ENCODING_UTF16_BOM) {
        file.write("\xFF\xFE", 2);
    } else if (realEncoding == ENCODING_UTF32_BOM) {
        file.write("\xFF\xFE\x00\x00", 4);
    } else if (realEncoding == ENCODING_UTF8_BOM) {
        file.putChar(0xEF);
        file.putChar(0xBB);
        file.putChar(0xBF);
    }
    bool allAscii = true;
    QByteArray data;
    for (const PDocumentLine& line:mLines) {
        QString text = line->lineText()+lineBreak();
        data = encoder->encodeUnchecked(text);
        if (allAscii) {
            allAscii = (data==text.toLatin1());
        }
        if (file.write(data)!=data.size())
            throw FileError(tr("Data not correctly writed to file '%1'.").arg(file.fileName()));
    }
    if (allAscii) {
        realEncoding = ENCODING_ASCII;
    } else if (realEncoding == ENCODING_SYSTEM_DEFAULT) {
        if (encoder->name().compare("System",Qt::CaseInsensitive)==0) {
            realEncoding = pCharsetInfoManager->getDefaultSystemEncoding();
        } else {
            realEncoding = encoder->name();
        }
    }
}

QString Document::glyph(int line, int glyphIdx) const
{
    QMutexLocker locker(&mMutex);
    return mLines[line]->glyph(glyphIdx);
}

QString Document::glyphAt(int line, int charPos) const
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(line>=0 && line<count());
    // Crash Prevention
    if (line<0 || line>=count())
        return 0;
    QList<int> glyphStartCharList = mLines[line]->glyphStartCharList();
    int glyphIdx = charToGlyphIndex(mLines[line]->lineText(), glyphStartCharList, charPos);
    return mLines[line]->glyph(glyphIdx);
}

int Document::charToGlyphStartChar(int line, int charPos) const
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(line>=0 && line<count());
    // Crash Prevention
    if (line<0 || line>=count())
        return 0;
    QList<int> glyphStartCharList = mLines[line]->glyphStartCharList();
    int glyphIdx = charToGlyphIndex(mLines[line]->lineText(), glyphStartCharList, charPos);
    return mLines[line]->glyphStartChar(glyphIdx);
}

int Document::glyphCount(int line) const
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(line>=0 && line<count());
    // Crash Prevention
    if (line<0 || line>=count())
        return 0;
    return mLines[line]->glyphsCount();
}

int Document::glyphStartChar(int line, int glyphIdx) const
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(line>=0 && line<count());
    // Crash Prevention
    if (line<0 || line>=count())
        return 0;
    return mLines[line]->glyphStartChar(glyphIdx);
}

int Document::glyphEndChar(int line, int glyphIdx) const
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(line>=0 && line<count());
    // Crash Prevention
    if (line<0 || line>=count())
        return 0;
    return mLines[line]->glyphStartChar(glyphIdx)+mLines[line]->glyphLength(glyphIdx);
}

int Document::glyphLength(int line, int glyphIdx) const
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(line>=0 && line<count());
    // Crash Prevention
    if (line<0 || line>=count())
        return 0;
    return mLines[line]->glyphLength(glyphIdx);
}

int Document::charToGlyphIndex(int line, int charIdx) const
{
    QMutexLocker locker(&mMutex);
    Q_ASSERT(line>=0 && line<count());
    // Crash Prevention
    if (line<0 || line>=count())
        return 0;
    QList<int> glyphStartCharList = mLines[line]->glyphStartCharList();
    return charToGlyphIndex(mLines[line]->lineText(), glyphStartCharList, charIdx);
}

int Document::charToGlyphIndex(const QString& str, const QList<int> &glyphStartCharList, int charIdx) const
{
    return searchForSegmentIdx(glyphStartCharList, str.length(), charIdx);
}

void Document::internalClear()
{
    Q_ASSERT(mLines.count() == mLineSeqIndice.count());
    beginUpdate();
    int oldCount = mLines.count();
    mLines.clear();
    mLineSeqIndice.clear();
    if (oldCount > 0)
        recordChange(DocumentChangeKind::Delete, 0, oldCount);
    endUpdate();
}

void Document::ensureHasLine()
{
    if (mLines.isEmpty())
        insertItem(0,"");
}

QList<int> Document::getGlyphStartCharList(int line, const QString &lineText) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count() || mLines[line]->lineText()!=lineText)
        return calcGlyphStartCharList(lineText);
    return mLines[line]->glyphStartCharList();
}

QList<int> Document::getGlyphStartCharList(int line) const
{
    QMutexLocker locker(&mMutex);
    if (line<0 || line>=count())
        return QList<int>();
    return mLines[line]->glyphStartCharList();
}

NewlineType Document::getNewlineType() const
{
    QMutexLocker locker(&mMutex);
    return mNewlineType;
}

void Document::setNewlineType(const NewlineType &fileEndingType)
{
    QMutexLocker locker(&mMutex);
    mNewlineType = fileEndingType;
}

bool Document::empty() const
{
    QMutexLocker locker(&mMutex);
    return mLines.count()==1 && mLines[0]->lineText().length()==0;
}

//Reserve 0
size_t DocumentLine::seqCounter = 1;

DocumentLine::DocumentLine():
    mSyntaxState{},
    mLineSeq{seqCounter++}
{

}

int DocumentLine::glyphLength(int i) const
{
    return calcSegmentInterval(mGlyphStartCharList, mLineText.length(), i);
}

QString DocumentLine::glyph(int i) const
{
   if (i<0 || i>=mGlyphStartCharList.length())
       return QString();
   return mLineText.mid(glyphStartChar(i),glyphLength(i));
}

void DocumentLine::setLineText(const QString &newLineText)
{
    mLineText = newLineText;
    mGlyphStartCharList = calcGlyphStartCharList(newLineText);
}

int DocumentLine::glyphStartChar(int i) const
{
   if (i<0)
       return 0;
   if (i>=mGlyphStartCharList.length())
       return mLineText.length();
   return mGlyphStartCharList[i];
}

UndoList::UndoList():QObject()
{
    mNextChangeNumber = 1;
    mInsideRedo = false;

    mBlockChangeNumber=0;
    mBlockLock=0;
    mLastPoppedItemChangeNumber=0;
    mInitialChangeNumber = 0;
    mLastRestoredItemChangeNumber=0;
}

PUndoItem UndoList::addChange(ChangeReason reason, const CharPos &startPos,
                                const CharPos &endPos, const QStringList& changeText,
                                SelectionMode selMode)
{
    // every change is recorded inside a beginBlock/endBlock pair; the
    // addedUndo signal is emitted once per non-empty block by endBlock
    Q_ASSERT(inBlock());
    PUndoItem  newItem = std::make_shared<UndoItem>(
                reason,
                selMode,startPos,endPos,changeText,
                mBlockChangeNumber);
    mItems.append(newItem);
    return newItem;
}

PUndoItem UndoList::addColumnDeleteChange(const CharPos &startPos, const CharPos &endPos,
                                          const QStringList &changeText,
                                          const QList<int> &perLineLeftChars)
{
    Q_ASSERT(inBlock());
    PUndoItem newItem = std::make_shared<ColumnDeleteUndoItem>(
                SelectionMode::Column, startPos, endPos, changeText,
                mBlockChangeNumber, perLineLeftChars);
    mItems.append(newItem);
    return newItem;
}

void UndoList::restoreChange(ChangeReason reason, const CharPos &startPos,
                             const CharPos &endPos, const QStringList &changeText,
                             SelectionMode selMode, size_t changeNumber)
{
    PUndoItem  newItem = std::make_shared<UndoItem>(reason,
                                                    selMode,startPos,endPos,changeText,
                                                    changeNumber);
    restoreChange(newItem);
}

void UndoList::restoreChange(PUndoItem item)
{
    size_t changeNumber = item->changeNumber();
    mItems.append(item);
    if (changeNumber>mNextChangeNumber)
        mNextChangeNumber=changeNumber;
    if (changeNumber!=mLastRestoredItemChangeNumber) {
        emit addedUndo();
    }
    mLastRestoredItemChangeNumber=changeNumber;
}

void UndoList::addGroupBreak()
{
    if (!canUndo())
        return;

    if (lastChangeReason() != ChangeReason::GroupBreak) {
        addChange(ChangeReason::GroupBreak, {0,0}, {0,0}, QStringList(), SelectionMode::Normal);
    }
}

void UndoList::beginBlock(const CharPos &caret, const CharPos &selBegin, const CharPos &selEnd, SelectionMode selMode)
{
//    qDebug()<<"begin block";
    if (mBlockLock==0) {
        mBlockChangeNumber = getNextChangeNumber();
        mCaretInfoBeforeChange.insert(
                    mBlockChangeNumber,
                    std::make_shared<CaretAndSelectionInfo>(
                        caret,
                        selBegin,
                        selEnd,
                        selMode
                        ));
    }
    mBlockLock++;

}

void UndoList::clear()
{
    mItems.clear();
    mInitialChangeNumber=0;
    mLastPoppedItemChangeNumber=0;
    mLastRestoredItemChangeNumber=0;
    mBlockLock=0;
    mCaretInfoAfterChange.clear();
    mCaretInfoBeforeChange.clear();
}

void UndoList::endBlock(const CharPos &caret, const CharPos &selBegin, const CharPos &selEnd, SelectionMode selMode)
{
//    qDebug()<<"end block";
    if (mBlockLock > 0) {
        mBlockLock--;
        if (mBlockLock == 0)  {
            size_t iBlockID = mBlockChangeNumber;
            mBlockChangeNumber = 0;
            if (mItems.count() > 0 && peekItem()->changeNumber() == iBlockID) {
                mCaretInfoAfterChange.insert(
                            iBlockID,
                            std::make_shared<CaretAndSelectionInfo>(
                                caret,
                                selBegin,
                                selEnd,
                                selMode
                                ));
                emit addedUndo();
            }
        }
    }
}

PCaretAndSelectionInfo UndoList::caretAndSelBeforeChange(size_t changeNumber)
{
    PCaretAndSelectionInfo info = mCaretInfoBeforeChange.value(changeNumber);
    Q_ASSERT(info!=nullptr);
    return info;
}

PCaretAndSelectionInfo UndoList::caretAndSelAfterChange(size_t changeNumber)
{
    PCaretAndSelectionInfo info = mCaretInfoAfterChange.value(changeNumber);
    Q_ASSERT(info!=nullptr);
    return info;
}

void UndoList::restoreCaretAndSelInfos(size_t changeNumber, const PCaretAndSelectionInfo &beforeInfo, const PCaretAndSelectionInfo &afterInfo)
{
    mCaretInfoBeforeChange.insert(changeNumber, beforeInfo);
    mCaretInfoAfterChange.insert(changeNumber, afterInfo);
}

void UndoList::removeCaretAndSelInfo(size_t changeNumber)
{
    mCaretInfoAfterChange.remove(changeNumber);
    mCaretInfoBeforeChange.remove(changeNumber);
}

bool UndoList::inBlock()
{
    return mBlockLock>0;
}

unsigned int UndoList::getNextChangeNumber()
{
    return mNextChangeNumber++;
}

ChangeReason UndoList::lastChangeReason()
{
    if (mItems.count() == 0)
        return ChangeReason::Nothing;
    else
        return mItems.last()->changeReason();
}

bool UndoList::isEmpty()
{
    return mItems.count()==0;
}

PUndoItem UndoList::peekItem()
{
    if (mItems.count() == 0)
        return PUndoItem();
    else
        return mItems.last();
}

PUndoItem UndoList::popItem()
{
    if (mItems.count() == 0)
        return PUndoItem();
    else {
        PUndoItem item = mItems.last();
//        qDebug()<<"popped"<<item->changeNumber()<<item->changeText()<<(int)item->changeReason()<<mLastPoppedItemChangeNumber;
        mLastPoppedItemChangeNumber =  item->changeNumber();
        mItems.removeLast();
        return item;
    }
}

bool UndoList::canUndo()
{
    return mItems.count()>0;
}

int UndoList::itemCount()
{
    return mItems.count();
}

bool UndoList::initialState()
{
    if (itemCount() == 0) {
        return mInitialChangeNumber==0;
    } else {
        return peekItem()->changeNumber() == mInitialChangeNumber;
    }
}

void UndoList::setInitialState()
{
    if (itemCount() == 0)
        mInitialChangeNumber = 0;
    else
        mInitialChangeNumber = peekItem()->changeNumber();
}

bool UndoList::insideRedo() const
{
    return mInsideRedo;
}

void UndoList::setInsideRedo(bool insideRedo)
{
    mInsideRedo = insideRedo;
}

SelectionMode UndoItem::changeSelMode() const
{
    return mChangeSelMode;
}

CharPos UndoItem::changeStartPos() const
{
    return mChangeStartPos;
}

CharPos UndoItem::changeEndPos() const
{
    return mChangeEndPos;
}

QStringList UndoItem::changeText() const
{
    return mChangeText;
}

size_t UndoItem::changeNumber() const
{
    return mChangeNumber;
}

UndoItem::UndoItem(ChangeReason reason, SelectionMode selMode,
                                 CharPos startPos, CharPos endPos,
                                 const QStringList& text, int number)
{
    mChangeReason = reason;
    mChangeSelMode = selMode;
    mChangeStartPos = startPos;
    mChangeEndPos = endPos;
    mChangeText = text;
    mChangeNumber = number;
    int length=0;
    foreach (const QString& s, text) {
        length+=s.length();
    }
}

UndoItem::~UndoItem()
{
}

ColumnDeleteUndoItem::ColumnDeleteUndoItem(SelectionMode selMode, CharPos startPos, CharPos endPos,
                                           const QStringList &text, int number,
                                           const QList<int> &perLineLeftChars):
    UndoItem(ChangeReason::ColumnDelete, selMode, startPos, endPos, text, number),
    mPerLineLeftChars(perLineLeftChars)
{
}

QList<int> ColumnDeleteUndoItem::perLineLeftChars() const
{
    return mPerLineLeftChars;
}

ChangeReason UndoItem::changeReason() const
{
    return mChangeReason;
}

RedoList::RedoList()
{

}

void RedoList::addRedo(ChangeReason reason, const CharPos &startPos, const CharPos &endPos, const QStringList &changeText, SelectionMode SelMode, size_t changeNumber)
{
    PUndoItem  newItem = std::make_shared<UndoItem>(
                reason,
                SelMode,startPos,endPos,changeText,
                changeNumber);
    mItems.append(newItem);
}

void RedoList::addRedo(PUndoItem item)
{
    mItems.append(item);
}

void RedoList::addCaretAndSelectionInfo(size_t changeNumber, const PCaretAndSelectionInfo &beforeInfo, const PCaretAndSelectionInfo &afterInfo)
{
    mCaretInfoBeforeChange.insert(changeNumber, beforeInfo);
    mCaretInfoAfterChange.insert(changeNumber, afterInfo);
}

PCaretAndSelectionInfo RedoList::caretAndSelBeforeChange(size_t changeNumber)
{
    PCaretAndSelectionInfo info = mCaretInfoBeforeChange.value(changeNumber);
    Q_ASSERT(info!=nullptr);
    return info;
}

PCaretAndSelectionInfo RedoList::caretAndSelAfterChange(size_t changeNumber)
{
    PCaretAndSelectionInfo info = mCaretInfoAfterChange.value(changeNumber);
    Q_ASSERT(info!=nullptr);
    return info;
}

void RedoList::removeCaretAndSelInfo(size_t changeNumber)
{
    mCaretInfoBeforeChange.remove(changeNumber);
    mCaretInfoAfterChange.remove(changeNumber);
}

void RedoList::clear()
{
    mItems.clear();
    mCaretInfoAfterChange.clear();
    mCaretInfoBeforeChange.clear();
}

ChangeReason RedoList::lastChangeReason()
{
    if (mItems.count() == 0)
        return ChangeReason::Nothing;
    else
        return mItems.last()->changeReason();
}

bool RedoList::isEmpty()
{
    return mItems.isEmpty();
}

PUndoItem RedoList::peekItem()
{
    if (mItems.count() == 0)
        return PUndoItem();
    else
        return mItems.last();
}

PUndoItem RedoList::popItem()
{
    if (mItems.count() == 0)
        return PUndoItem();
    else {
        PUndoItem item = mItems.last();
        mItems.removeLast();
        return item;
    }
}

bool RedoList::canRedo()
{
    return mItems.count()>0;
}

int RedoList::itemCount()
{
    return mItems.count();
}

BinaryFileError::BinaryFileError(const QString& reason):
    FileError(reason)
{

}

CaretAndSelectionInfo::CaretAndSelectionInfo(const CharPos &newCaret, const CharPos &newSelBegin, const CharPos &newSelEnd, SelectionMode newSelMode)
    : caret{newCaret}, selBegin{newSelBegin}, selEnd{newSelEnd}, selMode{newSelMode}
{
}

}
