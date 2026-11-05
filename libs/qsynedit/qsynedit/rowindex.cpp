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
#include "rowindex.h"

#include <QtGlobal>
#include <algorithm>

namespace QSynedit {

struct RowIndexNode {
    unsigned int priority;
    RowIndexNode *left = nullptr;
    RowIndexNode *right = nullptr;
    int lines = 1;      // subtree line count
    int totalSegs = 1;  // subtree sum of segment counts
    int hiddenSegs = 0; // subtree sum of segment counts of hidden lines
    int segs = 1;       // this line's segment count (>= 1)
    bool hidden = false;
    // lazy tag applied to the whole subtree: 0 none, 1 all hidden,
    // 2 all visible. A pending tag implies uniform subtree sums, and
    // structural operations (split/merge) push tags before touching a
    // node, so a mixed node never carries a tag.
    char tag = 0;
};

namespace {

int linesOf(const RowIndexNode *n)
{
    return n ? n->lines : 0;
}

int totalOf(const RowIndexNode *n)
{
    return n ? n->totalSegs : 0;
}

int hiddenOf(const RowIndexNode *n)
{
    return n ? n->hiddenSegs : 0;
}

int visibleOf(const RowIndexNode *n)
{
    return n ? n->totalSegs - n->hiddenSegs : 0;
}

// the line's own hidden flag; exact even when an ancestor range update
// left a pending tag on this node (tagged subtrees are uniform)
bool effectiveHidden(const RowIndexNode *n)
{
    return n->tag ? (n->tag == 1) : n->hidden;
}

void recalc(RowIndexNode *n)
{
    n->lines = 1 + linesOf(n->left) + linesOf(n->right);
    n->totalSegs = n->segs + totalOf(n->left) + totalOf(n->right);
    n->hiddenSegs = (n->hidden ? n->segs : 0)
            + hiddenOf(n->left) + hiddenOf(n->right);
}

void applyTag(RowIndexNode *n, char tag)
{
    if (!n)
        return;
    n->tag = tag;
    n->hiddenSegs = (tag == 1) ? n->totalSegs : 0;
}

// materialize a pending lazy tag into the node's own flag and its
// children; subtree sums do not change (tags are applied eagerly)
void push(RowIndexNode *n)
{
    if (!n || !n->tag)
        return;
    char tag = n->tag;
    n->hidden = (tag == 1);
    applyTag(n->left, tag);
    applyTag(n->right, tag);
    n->tag = 0;
}

// first k lines go to l
void split(RowIndexNode *n, int k, RowIndexNode *&l, RowIndexNode *&r)
{
    if (!n) {
        l = r = nullptr;
        return;
    }
    push(n);
    if (linesOf(n->left) >= k) {
        split(n->left, k, l, n->left);
        r = n;
    } else {
        split(n->right, k - linesOf(n->left) - 1, n->right, r);
        l = n;
    }
    recalc(n);
}

RowIndexNode *merge(RowIndexNode *a, RowIndexNode *b)
{
    if (!a)
        return b;
    if (!b)
        return a;
    if (a->priority < b->priority) {
        push(a);
        a->right = merge(a->right, b);
        recalc(a);
        return a;
    } else {
        push(b);
        b->left = merge(a, b->left);
        recalc(b);
        return b;
    }
}

void freeTree(RowIndexNode *n)
{
    if (!n)
        return;
    freeTree(n->left);
    freeTree(n->right);
    delete n;
}

void setSegCountRec(RowIndexNode *n, int line, int segs)
{
    push(n);
    int l = linesOf(n->left);
    if (line < l) {
        setSegCountRec(n->left, line, segs);
    } else if (line == l) {
        n->segs = segs;
    } else {
        setSegCountRec(n->right, line - l - 1, segs);
    }
    recalc(n);
}

// every leaf gets `segs`; hidden flags stay, so hiddenSegs is rebuilt
// bottom-up. A pending lazy tag overrides the (stale) own-flag math:
// tagged subtrees are uniform by invariant.
void resetSegCountsRec(RowIndexNode *n, int segs)
{
    if (n->left)
        resetSegCountsRec(n->left, segs);
    if (n->right)
        resetSegCountsRec(n->right, segs);
    n->segs = segs;
    recalc(n);
    if (n->tag)
        n->hiddenSegs = (n->tag == 1) ? n->totalSegs : 0;
}

// node covers the absolute line range [base, base + n->lines)
void setHiddenRec(RowIndexNode *n, int base, int from, int to, char tag)
{
    int end = base + n->lines - 1;
    if (to < base || from > end)
        return;
    if (from <= base && end <= to) {
        applyTag(n, tag);
        return;
    }
    push(n);
    int selfIdx = base + linesOf(n->left);
    if (n->left)
        setHiddenRec(n->left, base, from, to, tag);
    if (from <= selfIdx && selfIdx <= to)
        n->hidden = (tag == 1);
    if (n->right)
        setHiddenRec(n->right, selfIdx + 1, from, to, tag);
    recalc(n);
}

// sum of segment counts over the first `line` lines (0 <= line <= count).
// `lines` and `totalSegs` are exact on every node (lazy tags only affect
// hiddenSegs), so this needs no tag handling.
int totalPrefix(const RowIndexNode *n, int line)
{
    int acc = 0;
    while (n && line > 0) {
        int l = linesOf(n->left);
        if (line < l) {
            n = n->left;
            continue;
        }
        acc += totalOf(n->left);
        if (line == l)
            break;
        acc += n->segs;
        line -= l + 1;
        n = n->right;
    }
    return acc;
}

// sum of hidden segment counts over the first `line` lines. hiddenSegs
// values below a pending lazy tag are stale, so the effective hidden
// state is carried down the descent: under tag 1 the subtree is uniform
// hidden (hidden sum == total sum, exact via totalPrefix); under tag 2
// it contributes nothing.
int hiddenPrefix(const RowIndexNode *n, int line)
{
    int acc = 0;
    char tag = 0; // effective uniform state inherited from ancestors
    while (n && line > 0) {
        // the outermost tag wins: anything applied below an already
        // tagged node required pushing the ancestor first, so a
        // descendant tag seen here is older than the ancestor's
        if (!tag && n->tag)
            tag = n->tag;
        if (tag == 2)
            return acc; // all visible below
        if (tag == 1)
            return acc + totalPrefix(n, line); // all hidden below
        int l = linesOf(n->left);
        if (line < l) {
            n = n->left;
            continue;
        }
        acc += hiddenOf(n->left);
        if (line == l)
            break;
        if (n->hidden)
            acc += n->segs;
        line -= l + 1;
        n = n->right;
    }
    return acc;
}

// closest visible line strictly after `afterLine`, searching the subtree
// covering [base, base + n->lines)
int nextVisibleRec(const RowIndexNode *n, int base, int afterLine)
{
    if (!n)
        return -1;
    int end = base + n->lines - 1;
    if (end <= afterLine)
        return -1;
    if (hiddenOf(n) == totalOf(n))
        return -1; // fully hidden
    if (hiddenOf(n) == 0)
        return std::max(base, afterLine + 1); // fully visible
    // mixed subtree: no pending tag (tags imply uniform), descend
    int selfIdx = base + linesOf(n->left);
    int r = nextVisibleRec(n->left, base, afterLine);
    if (r >= 0)
        return r;
    if (selfIdx > afterLine && !effectiveHidden(n))
        return selfIdx;
    return nextVisibleRec(n->right, selfIdx + 1, afterLine);
}

int prevVisibleRec(const RowIndexNode *n, int base, int beforeLine)
{
    if (!n)
        return -1;
    int end = base + n->lines - 1;
    if (base >= beforeLine)
        return -1;
    if (hiddenOf(n) == totalOf(n))
        return -1; // fully hidden
    if (hiddenOf(n) == 0)
        return std::min(end, beforeLine - 1); // fully visible
    int selfIdx = base + linesOf(n->left);
    int r = prevVisibleRec(n->right, selfIdx + 1, beforeLine);
    if (r >= 0)
        return r;
    if (selfIdx < beforeLine && !effectiveHidden(n))
        return selfIdx;
    return prevVisibleRec(n->left, base, beforeLine);
}

}

RowIndex::RowIndex():
    mRoot(nullptr),
    mRngState(0x9e3779b9u)
{
}

RowIndex::~RowIndex()
{
    freeTree(mRoot);
}

int RowIndex::count() const
{
    return linesOf(mRoot);
}

void RowIndex::clear()
{
    freeTree(mRoot);
    mRoot = nullptr;
}

void RowIndex::reset(int lineCount)
{
    clear();
    insertLines(0, lineCount);
}

void RowIndex::insertLines(int at, int count)
{
    if (count <= 0)
        return;
    at = std::max(0, std::min(at, this->count()));
    RowIndexNode *a, *b;
    split(mRoot, at, a, b);
    for (int i = 0; i < count; i++) {
        RowIndexNode *node = new RowIndexNode;
        node->priority = nextPriority();
        a = merge(a, node);
    }
    mRoot = merge(a, b);
}

void RowIndex::eraseLines(int at, int count)
{
    if (at < 0) {
        count += at;
        at = 0;
    }
    count = std::min(count, this->count() - at);
    if (count <= 0)
        return;
    RowIndexNode *a, *b, *c;
    split(mRoot, at, a, b);
    split(b, count, b, c);
    freeTree(b);
    mRoot = merge(a, c);
}

int RowIndex::segCount(int line) const
{
    Q_ASSERT(line >= 0 && line < count());
    const RowIndexNode *n = mRoot;
    for (;;) {
        int l = linesOf(n->left);
        if (line < l) {
            n = n->left;
        } else if (line == l) {
            return n->segs;
        } else {
            line -= l + 1;
            n = n->right;
        }
    }
}

void RowIndex::setSegCount(int line, int n)
{
    Q_ASSERT(line >= 0 && line < count());
    Q_ASSERT(n >= 1);
    setSegCountRec(mRoot, line, n);
}

void RowIndex::resetSegCounts(int n)
{
    Q_ASSERT(n >= 1);
    if (mRoot)
        resetSegCountsRec(mRoot, n);
}

bool RowIndex::isHidden(int line) const
{
    Q_ASSERT(line >= 0 && line < count());
    const RowIndexNode *n = mRoot;
    for (;;) {
        // a pending tag means the whole subtree is uniform
        if (n->tag)
            return n->tag == 1;
        int l = linesOf(n->left);
        if (line < l) {
            n = n->left;
        } else if (line == l) {
            return n->hidden;
        } else {
            line -= l + 1;
            n = n->right;
        }
    }
}

void RowIndex::setHidden(int fromLine, int toLine, bool hidden)
{
    if (!mRoot)
        return;
    fromLine = std::max(fromLine, 0);
    toLine = std::min(toLine, count() - 1);
    if (fromLine > toLine)
        return;
    setHiddenRec(mRoot, 0, fromLine, toLine, hidden ? 1 : 2);
}

int RowIndex::totalSegsBefore(int line) const
{
    line = std::max(0, std::min(line, count()));
    return totalPrefix(mRoot, line);
}

int RowIndex::hiddenSegsBefore(int line) const
{
    line = std::max(0, std::min(line, count()));
    return hiddenPrefix(mRoot, line);
}

int RowIndex::visibleSegsBefore(int line) const
{
    return totalSegsBefore(line) - hiddenSegsBefore(line);
}

int RowIndex::totalSegCount() const
{
    return totalOf(mRoot);
}

int RowIndex::visibleSegCount() const
{
    return visibleOf(mRoot);
}

void RowIndex::lineSegAtVisibleRow(int row, int &line, int &segment) const
{
    line = 0;
    segment = 0;
    if (!mRoot)
        return;
    int vis = visibleOf(mRoot);
    if (vis <= 0)
        return;
    row = std::max(0, std::min(row, vis - 1));
    const RowIndexNode *n = mRoot;
    int idxBase = 0;
    // hiddenSegs below a pending lazy tag are stale, so the effective
    // uniform state is carried down the descent (tag 1 subtrees have
    // visible count 0 and are never entered; tag 2 means all visible)
    char tag = 0;
    for (;;) {
        if (!tag && n->tag) // outermost tag wins (see hiddenPrefix)
            tag = n->tag;
        int leftVis, selfVis;
        if (tag == 2) {
            leftVis = totalOf(n->left);
            selfVis = n->segs;
        } else if (tag == 1) {
            // unreachable: all-hidden subtrees have visible count 0 and
            // are never routed into; kept for robustness
            leftVis = 0;
            selfVis = 0;
        } else {
            leftVis = visibleOf(n->left);
            selfVis = n->hidden ? 0 : n->segs;
        }
        if (row < leftVis) {
            n = n->left;
        } else if (row < leftVis + selfVis) {
            line = idxBase + linesOf(n->left);
            segment = row - leftVis;
            return;
        } else {
            row -= leftVis + selfVis;
            idxBase += linesOf(n->left) + 1;
            n = n->right;
        }
    }
}

int RowIndex::nextVisibleLine(int line) const
{
    return nextVisibleRec(mRoot, 0, line);
}

int RowIndex::prevVisibleLine(int line) const
{
    return prevVisibleRec(mRoot, 0, line);
}

unsigned int RowIndex::nextPriority()
{
    // xorshift32: deterministic shape, no dependency on document content
    mRngState ^= mRngState << 13;
    mRngState ^= mRngState >> 17;
    mRngState ^= mRngState << 5;
    return mRngState;
}

}
