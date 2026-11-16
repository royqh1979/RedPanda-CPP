/*
 * Copyright (C) 2020-2025 Roy Qu (royqh1979@gmail.com)
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
#ifndef PPEXPANDER_H
#define PPEXPANDER_H

#include <functional>
#include "pptoken.h"
#include "parserutils.h"

/*
 * Token-stream macro expansion engine. See
 * docs/cpppreprocessor-optimization.md (phase 0.6) for the design.
 *
 * The engine reads pp-tokens from a PpTokenizer (the "input") and from
 * a stack of expansion frames pushed on top of it (macro replacement
 * lists under rescan). Each frame carries the name of the macro that
 * produced it ("blue paint"); an identifier is not expanded while a
 * frame painted with its name is on the stack (ISO 6.10.3.4).
 */
class PpExpander
{
public:
    // Look up a macro by name; return null when undefined.
    using DefineLookup = std::function<PDefine(const QString&)>;
    // __has_include support: is the given header text resolvable?
    using HasIncludeFunc = std::function<bool(const QString&)>;
    // Sink for macro names referenced by evaluated conditions
    // (environment-dependency recording).
    using ConditionRefSink = std::function<void(const QString&)>;

    explicit PpExpander(const DefineLookup& lookup);

    // Condition mode (#if/#elif expressions): undefined identifiers
    // evaluate to 0, defined/__has_include are handled as operators,
    // and referenced names are reported to the ref sink.
    void setConditionMode(bool on,
                          const HasIncludeFunc& hasInclude = HasIncludeFunc(),
                          const ConditionRefSink& refSink = ConditionRefSink());

    // Expand one logical line from the input. Returns the number of
    // physical lines consumed (>= 1; extra lines were consumed by
    // multi-line macro invocations).
    int expandLine(PpTokenizer& input, QString& outText);

    // Expand a token sequence in isolation (used for argument
    // pre-expansion and by the compatibility wrappers).
    QVector<PpToken> expandTokens(const QVector<PpToken>& tokens);

    static QString tokensToString(const QVector<PpToken>& tokens);

private:
    struct Frame {
        QVector<PpToken> tokens;
        int pos = 0;
        QString paintedName; // empty = anonymous push-back frame
    };

    // Pull the next token: expansion frames first, then the input.
    PpToken nextToken();
    bool isPainted(const QString& name) const;
    // Process one token from the pump; returns the number of physical
    // input lines consumed (only > 0 for multi-line invocations).
    int processToken(const PpToken& token,
                     QVector<PpToken>& out,
                     QString& pendingWhitespace);
    PpToken withPendingWs(const PpToken& token, QString& pendingWhitespace);
    int expandFunctionLike(const PpToken& openParen,
                           const PpToken& nameToken,
                           const PDefine& define,
                           QVector<PpToken>& out,
                           QString& pendingWhitespace);
    bool argumentCountMatches(const PDefine& define,
                              const QVector<QVector<PpToken>>& args) const;
    // Collect comma-separated arguments after an opening parenthesis.
    // `collected` receives every token read (for push-back on failure);
    // returns false on ill-formed input (unterminated at end of input).
    bool collectArgs(QVector<PpToken>& collected,
                     QVector<QVector<PpToken>>& args,
                     int& firstEolIndex, int& eolCount);
    QVector<PpToken> substitute(const PDefine& define,
                                const QVector<QVector<PpToken>>& args);
    PpToken mergeTokens(const PpToken& first, const PpToken& second);
    QString stringize(const QVector<PpToken>& tokens) const;
    void handleDefined(QVector<PpToken>& out, const QString& whitespace);
    void handleHasInclude(QVector<PpToken>& out, const QString& whitespace);

private:
    DefineLookup mLookup;
    QVector<Frame> mFrames;
    int mPopFloor = 0; // nextToken never pops frames below this level
    PpTokenizer* mInput = nullptr;
    bool mConditionMode = false;
    HasIncludeFunc mHasInclude;
    ConditionRefSink mRefSink;
};

#endif // PPEXPANDER_H
