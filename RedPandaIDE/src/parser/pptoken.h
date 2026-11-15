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
#ifndef PPTOKEN_H
#define PPTOKEN_H

#include <QString>
#include <QStringList>
#include <QVector>

/*
 * Light-weight preprocessing-token tokenizer and token structures for
 * the macro expansion engine. See docs/cpppreprocessor-optimization.md
 * (phase 0.6) for the overall design.
 */

enum class PpTokenKind {
    Identifier,
    Number,
    StringLiteral,      // "..." (unterminated strings run to end of line)
    CharLiteral,        // '...'
    RawStringFragment,  // one physical line of a raw string (may be a
                        // continuation fragment of a multi-line raw string)
    Punct,              // single character; '#' and '##' are separate tokens
    EndOfLine,          // line boundary marker
    EndOfInput
};

struct PpToken {
    PpTokenKind kind = PpTokenKind::EndOfInput;
    QString text;          // exact spelling
    QString whitespaceBefore; // exact whitespace that preceded the token ("" if none)

    bool isWordLike() const {
        return kind == PpTokenKind::Identifier || kind == PpTokenKind::Number;
    }
};

/*
 * Pull-based tokenizer over a list of (already comment-stripped and
 * line-spliced) source lines. Strings and character literals do not
 * cross line boundaries; raw strings do, and are emitted as one
 * RawStringFragment token per physical line so output lines stay
 * aligned with source lines.
 */
class PpTokenizer
{
public:
    PpTokenizer(const QStringList& lines, int startLine = 0, int startCol = 0);

    PpToken next();
    // current position (line of the next token to read)
    int line() const { return mLine; }
    QString prefixBefore(const PpToken &token) const;
    QString lineText(int line) const;

    static bool isIdentStartChar(const QChar& ch) { return ch.isLetter() || ch == '_'; }
    static bool isWordChar(const QChar& ch) {
        return ch == '_' || ch.isLetterOrNumber();
    }

private:
    PpToken readWord(const QString& line);
    PpToken readNumber(const QString& line);
    PpToken readStringOrChar(const QString& line, QChar quote);
    PpToken makeSimple(PpTokenKind kind, const QString& text) const;

private:
    const QStringList* mLines; // not owned
    int mLine;
    int mCol;
    enum class State { Other, Raw } mState = State::Other;
    QString mRawDelimiter;
    QString mWhitespace;
};

/*
 * One entry of a precompiled macro replacement list (compiled once per
 * #define; replaces the old formatValue template).
 */
struct ReplToken {
    enum class Kind { Text, Argument, VarOptBegin, VarOptEnd } kind = Kind::Text;
    PpToken token;        // Kind::Text - the body token (kind/text/whitespace preserved)
    int argIndex = -1;    // Kind::Argument - index into the parameter list
    bool stringize = false;   // preceded by '#'
    bool pasteBefore = false; // preceded by '##'
    QString whitespaceBefore;   // Kind::Argument - whitespace before the
                                // parameter (or before '#' when stringized)
};

using ReplTokenList = QVector<ReplToken>;

/*
 * Split a macro parameter list (the text between the outer parens of a
 * function-like macro definition) at depth-0 commas. Parentheses,
 * braces and brackets are tracked, so parameters like void(*)(int)
 * split correctly (angle brackets are intentionally not tracked,
 * consistent with real compilers).
 */
QStringList splitMacroParams(const QString& args);

/*
 * Compile a macro body into a replacement list: parameter references
 * become Argument entries (with stringize/pasteBefore flags from
 * neighboring '#'/'##' tokens), everything else is merged into Text
 * entries with original spacing preserved.
 * argNames: parameter names in order; "__VA_ARGS__" maps to the index
 * of a "..." entry if present.
 */
ReplTokenList compileReplacementList(const QString& value, const QStringList& argNames);

#endif // PPTOKEN_H
