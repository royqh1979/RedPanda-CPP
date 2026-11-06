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
#include "wraplayoutjob.h"
#include <QMetaObject>
#include <QThreadPool>

namespace QSynedit {

WrapLayoutJob::WrapLayoutJob(const PDocument &document, int generation, int contentVersion,
                             PSyntaxer syntaxer,
                             const QFont &baseFont, const GlyphCalculator &glyphCalculator,
                             int wrapWidth, bool wordWrap, const PWrapJobSharedState &sharedState):
    mDocument(document),
    mSyntaxer(syntaxer),
    mGeneration(generation),
    mContentVersion(contentVersion),
    mStartVersion(document->contentVersion()),
    mBaseFont(baseFont),
    mGlyphCalculator(glyphCalculator),
    mWrapWidth(wrapWidth),
    mWordWrap(wordWrap),
    mSharedState(sharedState)
{
}

void WrapLayoutJob::run()
{
    PSyntaxer syntaxer = mSyntaxer;
    int count = mDocument->count();
    bool contentStale = false;
    for (int line=0; line<count; line++) {
        {
            QMutexLocker locker(&mSharedState->mutex);
            if (mSharedState->viewDead || mGeneration != mSharedState->latestGeneration)
                break;
        }
        // early stop: the document is being (or has been) modified since
        // this job was scheduled; whatever we computed so far is delivered
        // through the normal batches, but finishing now would wrongly mark
        // the whole document as computed
        if (mDocument->inUpdate() || mDocument->contentVersion() != mStartVersion) {
            contentStale = true;
            break;
        }
        size_t seq = mDocument->getLineSeq(line);
        WrapLayout layout = computeWrapLayoutForLine(
                    mDocument, line, syntaxer.get(), mGlyphCalculator, mBaseFont, mWrapWidth, mWordWrap);
        mLayouts.insert(seq, layout);
        mLines.insert(seq, line);
        if (mLayouts.count() >= 256) {
            QMutexLocker locker(&mSharedState->mutex);
            postBatch(false);
        }
    }
    {
        QMutexLocker locker(&mSharedState->mutex);
        if (!mSharedState->viewDead && mGeneration == mSharedState->latestGeneration) {
            if (contentStale)
                postAborted();
            else
                postBatch(true);
        }
        // notify the view (which may be waiting in its destructor) that this
        // job has exited
        mSharedState->jobsInFlight--;
        mSharedState->jobsDone.wakeAll();
    }
}

void WrapLayoutJob::postBatch(bool finished)
{
    if (mSharedState->viewDead || mGeneration != mSharedState->latestGeneration)
        return;
    WrapLayoutBatch batch;
    batch.generation = mGeneration;
    batch.contentVersion = mContentVersion;
    batch.document = mDocument;
    batch.layouts = mLayouts;
    batch.lines = mLines;
    batch.finished = finished;
    emit layoutsReady(batch);
    // post-and-forget: this job never reads/writes mLayouts again after
    // posting; the connection owns its own copy now
    mLayouts.clear();
    mLines.clear();
}

void WrapLayoutJob::postAborted()
{
    if (mSharedState->viewDead || mGeneration != mSharedState->latestGeneration)
        return;
    WrapLayoutBatch batch;
    batch.generation = mGeneration;
    batch.contentVersion = mContentVersion;
    batch.document = mDocument;
    batch.aborted = true;
    emit layoutsReady(batch);
}

}
