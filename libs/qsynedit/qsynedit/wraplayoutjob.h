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
#ifndef QSYNEDIT_WRAPLAYOUTJOB_H
#define QSYNEDIT_WRAPLAYOUTJOB_H

#include <QPointer>
#include <QRunnable>
#include "qsynedit.h"

namespace QSynedit {

// standalone wrap-layout computation (used by QSynEdit::computeWrapLayout
// for on-demand work and by WrapLayoutJob for background work). All inputs
// are thread-local: the document is only read through its locked accessors,
// the syntaxer is a per-thread clone, fonts/metrics are value copies.
WrapLayout computeWrapLayoutForLine(const PDocument &document, int line,
                                    Syntaxer *syntaxer,
                                    const GlyphCalculator &glyphCalculator,
                                    const QFont &baseFont, int wrapWidth,
                                    bool wordWrap);

// background whole-document wrap-layout computation. Result batches are
// delivered by the layoutsReady signal (queued to the view); the view is
// the ONLY writer of the layout cache. Discipline: every batch is a fresh
// container, emitted once and never touched again (post-and-forget).
class WrapLayoutJob : public QObject, public QRunnable
{
    Q_OBJECT
public:
    WrapLayoutJob(const PDocument &document, int generation, int contentVersion,
                  PSyntaxer syntaxer,
                  const QFont &baseFont, const GlyphCalculator &glyphCalculator,
                  int wrapWidth, bool wordWrap, const PWrapJobSharedState &sharedState);
    void run() override;
signals:
    void layoutsReady(const QSynedit::WrapLayoutBatch &batch);
private:
    void postBatch(bool finished);
    // post the early-abort control signal: no layouts are delivered (they
    // could be stale); the view reschedules with fresh inputs
    void postAborted();
    PDocument mDocument;
    // the view's syntaxer, cloned at schedule time (carries the color
    // scheme's attribute styles, which affect glyph widths)
    PSyntaxer mSyntaxer;
    int mGeneration;
    int mContentVersion;
    // the document's content version at construction (= schedule) time;
    // the run loop aborts early once the document is modified
    int mStartVersion;
    QFont mBaseFont;
    GlyphCalculator mGlyphCalculator;
    int mWrapWidth;
    bool mWordWrap;
    PWrapJobSharedState mSharedState;
    QHash<size_t, WrapLayout> mLayouts;
    // seq -> line index at compute time, delivered alongside mLayouts so
    // the view can point-update its row index
    QHash<size_t, int> mLines;
};

}

#endif // QSYNEDIT_WRAPLAYOUTJOB_H
