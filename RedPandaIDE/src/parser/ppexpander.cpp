/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#include "ppexpander.h"

#include <utility>

PpExpander::PpExpander(const DefineLookup &lookup)
    : mLookup(lookup)
{
}

void PpExpander::setConditionMode(bool on, const HasIncludeFunc &hasInclude,
                                  const ConditionRefSink &refSink)
{
    mConditionMode = on;
    mHasInclude = hasInclude;
    mRefSink = refSink;
}

PpToken PpExpander::nextToken()
{
    // Exhausted frames are popped lazily, but never below mPopFloor:
    // a frame carries the blue paint of the macro that produced it,
    // and the paint must survive while nested expansions spawned by
    // the frame (e.g. argument pre-expansion) are still running.
    while (mFrames.count() > mPopFloor) {
        Frame& top = mFrames.last();
        if (top.pos < top.tokens.count())
            return std::move(top.tokens[top.pos++]);
        mFrames.removeLast();
    }
    if (mInput)
        return mInput->next();
    PpToken token;
    token.kind = PpTokenKind::EndOfInput;
    return token;
}

bool PpExpander::isPainted(const QString &name) const
{
    for (const Frame& frame : mFrames) {
        if (frame.paintedName == name)
            return true;
    }
    return false;
}

QString PpExpander::tokensToString(const QVector<PpToken> &tokens)
{
    QString result;
    for (const PpToken& token : tokens) {
        result += token.whitespaceBefore;
        result += token.text;
    }
    return result;
}

int PpExpander::expandLine(PpTokenizer &input, QString &outText)
{
    mInput = &input;
    mFrames.clear();
    QVector<PpToken> out;
    int linesConsumed = 1;
    QString pendingWhitespace;
    QString prefix;
    bool expandSuffix = mConditionMode;
    while (true) {
        PpToken token = nextToken();
        if (token.kind == PpTokenKind::EndOfInput)
            break;
        if (token.kind == PpTokenKind::EndOfLine) {
            if (expandSuffix) {
                // line boundary: emit any whitespace the boundary carries
                // (whitespace-only lines and trailing whitespace)
                if (!token.whitespaceBefore.isEmpty()) {
                    PpToken ws;
                    ws.kind = PpTokenKind::Punct;
                    ws.whitespaceBefore = token.whitespaceBefore;
                    out.append(ws);
                }
            } else {
                prefix = input.lineText(input.line() - 1);
            }
            break;
        }
        if (!expandSuffix && token.kind == PpTokenKind::Identifier && mLookup(token.text)) {
            prefix = input.prefixBefore(token);
            expandSuffix = true;
        }
        if (expandSuffix)
            linesConsumed += processToken(token, out, pendingWhitespace);
    }
    if (!pendingWhitespace.isEmpty()) {
        PpToken token;
        token.kind = PpTokenKind::Punct;
        token.whitespaceBefore = pendingWhitespace;
        out.append(token);
    }
    outText = prefix + tokensToString(out);
    return linesConsumed;
}

QVector<PpToken> PpExpander::expandTokens(const QVector<PpToken> &tokens)
{
    // expand a token sequence in isolation (used for argument
    // pre-expansion and by the compatibility wrappers); the real input
    // is disconnected so nothing can be consumed from it
    QVector<PpToken> result;
    PpTokenizer* savedInput = mInput;
    mInput = nullptr;
    int savedFloor = mPopFloor;
    // the sub-pump may not pop frames below its own base: outer
    // exhausted frames carry the blue paint that scopes this expansion
    mPopFloor = mFrames.count();
    mFrames.append(Frame{tokens, 0, QString()});
    QString pendingWhitespace;
    while (mFrames.count() > mPopFloor) {
        PpToken token = nextToken();
        if (token.kind == PpTokenKind::EndOfInput)
            break;
        processToken(token, result, pendingWhitespace);
    }
    if (!pendingWhitespace.isEmpty()) {
        // an expansion ending with an empty macro leaves its
        // whitespace behind
        PpToken ws;
        ws.kind = PpTokenKind::Punct;
        ws.whitespaceBefore = pendingWhitespace;
        result.append(ws);
    }
    mPopFloor = savedFloor;
    mInput = savedInput;
    return result;
}

int PpExpander::processToken(const PpToken &token,
                             QVector<PpToken> &out,
                             QString &pendingWhitespace)
{
    if (token.kind != PpTokenKind::Identifier) {
        out.append(withPendingWs(token, pendingWhitespace));
        return 0;
    }
    if (mConditionMode) {
        if (token.text == QLatin1String("defined")) {
            handleDefined(out, token.whitespaceBefore);
            return 0;
        }
        if (token.text == QLatin1String("__has_include")) {
            handleHasInclude(out, token.whitespaceBefore);
            return 0;
        }
        if (token.text == QLatin1String("__has_builtin")) {
            handleHasBuiltin(out, token.whitespaceBefore);
            return 0;
        }
        // C++ alternative tokens (iso646 keywords)
        static const QHash<QString, QString> altTokens = {
            {"and", "&&"}, {"and_eq", "&="}, {"bitand", "&"},
            {"bitor", "|"}, {"compl", "~"}, {"not", "!"},
            {"not_eq", "!="}, {"or", "||"}, {"or_eq", "|="},
            {"xor", "^"}, {"xor_eq", "^="},
        };
        auto altIt = altTokens.constFind(token.text);
        if (altIt != altTokens.constEnd()) {
            PpToken t = token;
            t.kind = PpTokenKind::Punct;
            t.text = altIt.value();
            out.append(withPendingWs(t, pendingWhitespace));
            return 0;
        }
    }
    PDefine define = mLookup(token.text);
    if (mConditionMode && mRefSink
            && token.text != QLatin1String("true")
            && token.text != QLatin1String("false"))
        mRefSink(token.text);
    if (define && !isPainted(define->name)) {
        if (define->args.isEmpty()) {
            // object-like macro
            QVector<PpToken> replacement = define->valueTokens;
            if (replacement.isEmpty()) {
                // empty expansion: carry the whitespace over to the
                // next emitted token
                pendingWhitespace += token.whitespaceBefore;
            } else {
                replacement[0].whitespaceBefore =
                        pendingWhitespace + token.whitespaceBefore;
                pendingWhitespace.clear();
                mFrames.append(Frame{replacement, 0, define->name});
            }
            return 0;
        }
        // function-like macro: must be followed by '(' to expand;
        // whitespace (including newlines) may precede the parenthesis
        int peekEols = 0;
        QVector<PpToken> peeked;
        PpToken next;
        while (true) {
            next = nextToken();
            if (next.kind == PpTokenKind::EndOfLine) {
                peekEols++;
                peeked.append(next);
                continue;
            }
            peeked.append(next);
            break;
        }
        if (next.kind == PpTokenKind::Punct && next.text == "(") {
            return expandFunctionLike(next, token, define, out,
                                      pendingWhitespace)
                    + peekEols;
        }
        // not an invocation: emit the name and push the peeked tokens
        // back; an EndOfLine among them terminates this logical line
        // and the rest is reprocessed by the caller as ordinary lines
        out.append(withPendingWs(token, pendingWhitespace));
        mFrames.append(Frame{peeked, 0, QString()});
        return 0;
    }
    if (mConditionMode && !define) {
        // undefined identifiers in a condition evaluate to 0
        // (and true/false to 1/0)
        PpToken t = token;
        t.kind = PpTokenKind::Number;
        t.text = (token.text == QLatin1String("true")) ? "1" : "0";
        out.append(withPendingWs(t, pendingWhitespace));
        return 0;
    }
    out.append(withPendingWs(token, pendingWhitespace));
    return 0;
}

PpToken PpExpander::withPendingWs(const PpToken &token,
                                  QString &pendingWhitespace)
{
    if (pendingWhitespace.isEmpty())
        return token;
    PpToken t = token;
    t.whitespaceBefore = pendingWhitespace + t.whitespaceBefore;
    pendingWhitespace.clear();
    return t;
}

int PpExpander::expandFunctionLike(const PpToken &openParen,
                                   const PpToken &nameToken,
                                   const PDefine &define,
                                   QVector<PpToken> &out,
                                   QString &pendingWhitespace)
{
    QVector<QVector<PpToken>> args;
    QVector<PpToken> collected;
    int firstEolIndex = -1;
    int eolCount = 0;
    bool collectedOk = collectArgs(collected, args, firstEolIndex, eolCount);
    bool ok = collectedOk && argumentCountMatches(define, args);
    if (!ok) {
        // ill-formed invocation: emit the name and re-scan the tokens
        // collected on the first physical line; the remaining lines are
        // processed again by the caller as ordinary lines
        out.append(withPendingWs(nameToken, pendingWhitespace));
        QVector<PpToken> pushBack;
        pushBack.append(openParen);
        int boundary = (firstEolIndex >= 0)
                ? firstEolIndex
                : collected.count();
        for (int i = 0; i < boundary; i++)
            pushBack.append(collected[i]);
        if (!pushBack.isEmpty())
            mFrames.append(Frame{pushBack, 0, QString()});
        return 0;
    }
    QVector<PpToken> replacement = substitute(define, args);
    if (replacement.isEmpty()) {
        pendingWhitespace += nameToken.whitespaceBefore;
    } else {
        replacement[0].whitespaceBefore =
                pendingWhitespace + nameToken.whitespaceBefore;
        pendingWhitespace.clear();
        mFrames.append(Frame{replacement, 0, define->name});
    }
    return eolCount;
}

bool PpExpander::argumentCountMatches(const PDefine &define,
                                      const QVector<QVector<PpToken>> &args) const
{
    if (define->paramNames.isEmpty()) {
        // zero-parameter macro: only an empty argument list matches
        return args.count() == 1 && args[0].isEmpty();
    }
    int varIndex = define->paramNames.indexOf("...");
    int namedCount = (varIndex >= 0) ? varIndex : define->paramNames.count();
    if (varIndex < 0)
        return args.count() == namedCount;
    return args.count() >= namedCount;
}

bool PpExpander::collectArgs(QVector<PpToken> &collected,
                             QVector<QVector<PpToken>> &args,
                             int &firstEolIndex, int &eolCount)
{
    int depth = 1;
    QVector<PpToken> current;
    QString joinWs;
    while (true) {
        PpToken token = nextToken();
        if (token.kind == PpTokenKind::EndOfInput)
            return false;
        if (token.kind == PpTokenKind::EndOfLine) {
            if (firstEolIndex < 0)
                firstEolIndex = collected.count();
            eolCount++;
            // multi-line invocations join lines with a single space
            joinWs = " ";
            continue;
        }
        if (!joinWs.isEmpty()) {
            if (token.whitespaceBefore.isEmpty())
                token.whitespaceBefore = joinWs;
            joinWs.clear();
        }
        collected.append(token);
        if (token.kind == PpTokenKind::Punct) {
            if (token.text == "(") {
                depth++;
            } else if (token.text == ")") {
                depth--;
                if (depth == 0) {
                    args.append(current);
                    return true;
                }
            } else if (token.text == "," && depth == 1) {
                args.append(current);
                current.clear();
                continue;
            }
        }
        current.append(token);
    }
}

QVector<PpToken> PpExpander::substitute(const PDefine &define,
                                        const QVector<QVector<PpToken>> &args)
{
    QVector<PpToken> out;
    int varIndex = define->paramNames.indexOf("...");
    bool varArgsEmpty = false;
    if (varIndex >= 0) {
        varArgsEmpty = true;
        for (int i = varIndex; i < args.count(); i++) {
            if (!args[i].isEmpty()) {
                varArgsEmpty = false;
                break;
            }
        }
    }
    int varOptSkip = 0;
    for (const ReplToken& entry : define->replacementList) {
        switch (entry.kind) {
        case ReplToken::Kind::VarOptBegin:
            if (varArgsEmpty)
                varOptSkip++;
            continue;
        case ReplToken::Kind::VarOptEnd:
            if (varOptSkip > 0)
                varOptSkip--;
            continue;
        default:
            break;
        }
        if (varOptSkip > 0)
            continue;

        QVector<PpToken> piece;
        if (entry.kind == ReplToken::Kind::Text) {
            piece.append(entry.token);
        } else {
            // Argument
            QVector<PpToken> argTokens;
            if (entry.argIndex >= 0 && entry.argIndex < args.count()) {
                if (varIndex >= 0 && entry.argIndex >= varIndex) {
                    // join the variadic arguments with commas
                    for (int i = varIndex; i < args.count(); i++) {
                        if (i > varIndex) {
                            PpToken comma;
                            comma.kind = PpTokenKind::Punct;
                            comma.text = ",";
                            argTokens.append(comma);
                        }
                        argTokens += args[i];
                    }
                } else {
                    argTokens = args[entry.argIndex];
                }
            }
            if (entry.stringize) {
                PpToken token;
                token.kind = PpTokenKind::StringLiteral;
                token.text = stringize(argTokens);
                token.whitespaceBefore = entry.whitespaceBefore;
                piece.append(token);
            } else {
                if (!entry.pasteBefore) {
                    // arguments not adjacent to ## are expanded before
                    // substitution (ISO 6.10.3.1)
                    argTokens = expandTokens(argTokens);
                    if (!argTokens.isEmpty()) {
                        // the body's spacing at the parameter position
                        // overrides the call-site whitespace (trim)
                        argTokens[0].whitespaceBefore = entry.whitespaceBefore;
                    }
                }
                piece = argTokens;
            }
        }

        if (entry.pasteBefore && !out.isEmpty()) {
            if (piece.isEmpty()) {
                // empty paste operand: drop a trailing comma
                // (the GNU ", ## __VA_ARGS__" idiom)
                if (out.last().kind == PpTokenKind::Punct
                        && out.last().text == ",") {
                    out.removeLast();
                }
            } else if (varIndex >= 0 && entry.argIndex >= varIndex) {
                // ", ## __VA_ARGS__": when the variadic arguments are
                // not empty, the comma stays and they are emitted
                // without merging (GNU idiom)
                if (piece[0].whitespaceBefore.isEmpty())
                    piece[0].whitespaceBefore = " ";
                out += piece;
            } else {
                PpToken first = piece.takeFirst();
                out.last() = mergeTokens(out.last(), first);
                out += piece;
            }
        } else {
            out += piece;
        }
    }
    return out;
}

PpToken PpExpander::mergeTokens(const PpToken &first, const PpToken &second)
{
    // ## pastes two tokens into one; the merged spelling is re-lexed
    QString merged = first.text + second.text;
    PpToken token;
    token.whitespaceBefore = first.whitespaceBefore;
    QStringList lines{merged};
    PpTokenizer tokenizer(lines);
    PpToken relexed = tokenizer.next();
    PpToken after = tokenizer.next();
    if (after.kind == PpTokenKind::EndOfLine
            || after.kind == PpTokenKind::EndOfInput) {
        // exactly one token
        token.kind = relexed.kind;
        token.text = relexed.text;
    } else {
        // ill-formed paste: keep the merged text as opaque punct
        token.kind = PpTokenKind::Punct;
        token.text = merged;
    }
    return token;
}

QString PpExpander::stringize(const QVector<PpToken> &tokens) const
{
    QString content;
    for (const PpToken& token : tokens) {
        content += token.whitespaceBefore;
        content += token.text;
    }
    QString result = "\"";
    for (const QChar& ch : content) {
        if (ch == '\\' || ch == '"')
            result += '\\';
        result += ch;
    }
    result += '"';
    return result;
}

void PpExpander::handleDefined(QVector<PpToken> &out, const QString &whitespace)
{
    // defined(X) or defined X in a #if expression
    PpToken next = nextToken();
    QString name;
    bool ok = false;
    if (next.kind == PpTokenKind::Punct && next.text == "(") {
        PpToken ident = nextToken();
        PpToken close = nextToken();
        ok = (ident.kind == PpTokenKind::Identifier)
                && (close.kind == PpTokenKind::Punct && close.text == ")");
        if (ok)
            name = ident.text;
    } else if (next.kind == PpTokenKind::Identifier) {
        name = next.text;
        ok = true;
    }
    PpToken result;
    result.kind = PpTokenKind::Number;
    result.whitespaceBefore = whitespace;
    if (!ok) {
        result.text = "0";
    } else {
        if (mRefSink)
            mRefSink(name);
        result.text = mLookup(name) ? "1" : "0";
    }
    out.append(result);
}

void PpExpander::handleHasInclude(QVector<PpToken> &out, const QString &whitespace)
{
    // __has_include(<header>) or __has_include("header")
    PpToken next = nextToken();
    PpToken result;
    result.kind = PpTokenKind::Number;
    result.text = "0";
    result.whitespaceBefore = whitespace;
    if (!(next.kind == PpTokenKind::Punct && next.text == "(")) {
        out.append(result);
        return;
    }
    QVector<PpToken> args;
    int depth = 1;
    bool ok = false;
    while (true) {
        PpToken token = nextToken();
        if (token.kind == PpTokenKind::EndOfInput)
            break;
        if (token.kind == PpTokenKind::Punct) {
            if (token.text == "(") {
                depth++;
            } else if (token.text == ")") {
                depth--;
                if (depth == 0) {
                    ok = true;
                    break;
                }
            }
        }
        args.append(token);
    }
    if (ok && mHasInclude
            && mHasInclude(tokensToString(args).trimmed())) {
        result.text = "1";
    }
    out.append(result);
}

void PpExpander::handleHasBuiltin(QVector<PpToken> &out, const QString &whitespace)
{
    PpToken next = nextToken();
    PpToken result;
    result.kind = PpTokenKind::Number;
    result.text = "0";
    result.whitespaceBefore = whitespace;
    if (!(next.kind == PpTokenKind::Punct && next.text == "(")) {
        out.append(result);
        return;
    }
    QVector<PpToken> args;
    int depth = 1;
    bool ok = false;
    while (true) {
        PpToken token = nextToken();
        if (token.kind == PpTokenKind::EndOfInput)
            break;
        if (token.kind == PpTokenKind::Punct) {
            if (token.text == "(") {
                ++depth;
            } else if (token.text == ")") {
                --depth;
                if (depth == 0) {
                    ok = true;
                    break;
                }
            }
        }
        args.append(token);
    }
    static const QSet<QString> supported{
        "__is_array", "__is_trivially_destructible", "__is_trivially_constructible",
        "__is_trivially_assignable", "__is_trivially_copyable",
        "__add_lvalue_reference", "__add_rvalue_reference", "__add_pointer",
        "__remove_extent", "__remove_reference_t", "__remove_cv", "__remove_cv_t"
    };
    QString name = tokensToString(args).trimmed();
    if (ok && mLookup("__clang__") && supported.contains(name))
        result.text = "1";
    out.append(result);
}
