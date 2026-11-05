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
#pragma once

namespace QSynedit {

struct RowIndexNode;

/**
 * @brief Augmented implicit treap over per-line segment counts and hidden
 * flags ("RowIndex" in docs/qsynedit-wordwrap-plan.md).
 *
 * Each line is a leaf holding its wrap segment count (>= 1; a cache-or-1
 * estimate until the background layout refines it) and a hidden flag
 * (inside a collapsed fold). Internal nodes cache the subtree sums
 * (totalSegs, hiddenSegs), so every query below is O(log N):
 *
 *  - row mapping: visibleSegsBefore(line) is the global display row of
 *    (line, segment 0); lineSegAtVisibleRow(row) is the inverse;
 *  - scrollbar range: visibleSegCount() is the root sum, O(1);
 *  - fold visibility: isHidden / setHidden (range lazy tag) /
 *    nextVisibleLine / prevVisibleLine (collapsed ranges are skipped
 *    whole, never stepped through).
 *
 * setHidden() callers pass *effective* collapsed ranges only (top-level
 * collapsed blocks never overlap), but the implementation uses
 * assignment-semantics lazy tags and stays correct for arbitrary ranges.
 *
 * Line indices are implicit (in-order position), so insertLines /
 * eraseLines are O(log N) structural operations, not shifts of stored
 * indices.
 */
class RowIndex
{
public:
    RowIndex();
    ~RowIndex();
    RowIndex(const RowIndex &) = delete;
    RowIndex &operator=(const RowIndex &) = delete;

    int count() const;
    void clear();
    // k lines, each with 1 segment, all visible
    void reset(int lineCount);

    // new lines get segs=1 and visible; `at` is clamped to [0, count()]
    void insertLines(int at, int count);
    // the range is clamped to the valid line range
    void eraseLines(int at, int count);

    int segCount(int line) const;
    // n must be >= 1
    void setSegCount(int line, int n);
    // bulk reset of every line's segment count (e.g. back to the
    // cache-or-1 estimate after the layout cache is invalidated);
    // hidden flags and lazy tags are preserved. O(N), no allocation.
    void resetSegCounts(int n = 1);

    bool isHidden(int line) const;
    // inclusive line range
    void setHidden(int fromLine, int toLine, bool hidden);

    // prefix sums over the first `line` lines (line may be count())
    int totalSegsBefore(int line) const;
    int hiddenSegsBefore(int line) const;
    int visibleSegsBefore(int line) const;

    int totalSegCount() const;
    int visibleSegCount() const;

    // global visible row -> (line, segment); row is clamped into
    // [0, visibleSegCount()-1]; an empty or fully hidden tree yields (0, 0)
    void lineSegAtVisibleRow(int row, int &line, int &segment) const;

    // closest visible line strictly after/before `line`; -1 when none
    int nextVisibleLine(int line) const;
    int prevVisibleLine(int line) const;

private:
    RowIndexNode *mRoot;
    unsigned int mRngState;

    unsigned int nextPriority();
};

}
