/*
 * Copyright (C) 2026 Roy Qu (royqh1979@gmail.com)
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
#include "cppexpressionextractor.h"

#include <algorithm>
#include <qsynedit/document.h>
#include <qsynedit/qsynedit.h>
#include <qsynedit/syntaxer/cpp.h>

namespace {
struct Token {
    QString text;
    QSynedit::TokenType type;
    int inputIndex;
};

bool isAtom(const Token &token)
{
    if (token.type == QSynedit::TokenType::Identifier
            || token.type == QSynedit::TokenType::Number
            || token.type == QSynedit::TokenType::String
            || token.type == QSynedit::TokenType::Character) {
        return true;
    }
    static const QSet<QString> primaryKeywords{
        "this", "true", "false", "nullptr",
        "const_cast", "dynamic_cast", "reinterpret_cast", "static_cast",
        "decltype", "typeid"
    };
    return primaryKeywords.contains(token.text);
}

bool isMemberOperator(const QString &text)
{
    return text == "." || text == "->" || text == "::"
            || text == ".*" || text == "->*";
}

bool isCloseGroup(const QString &text)
{
    return text == ")" || text == "]" || text == "}";
}

bool isTemplateOpen(const QString &text)
{
    return !text.isEmpty()
            && std::all_of(text.begin(), text.end(), [](const QChar &ch) {
        return ch == '<';
    });
}

bool isTemplateClose(const QString &text)
{
    return !text.isEmpty()
            && std::all_of(text.begin(), text.end(), [](const QChar &ch) {
        return ch == '>';
    });
}

int updateGroupDepth(const QString &text, int &round, int &square, int &curly)
{
    if (text == ")")
        ++round;
    else if (text == "(")
        --round;
    else if (text == "]")
        ++square;
    else if (text == "[")
        --square;
    else if (text == "}")
        ++curly;
    else if (text == "{")
        --curly;
    return round + square + curly;
}

QVector<Token> readLine(const QSynedit::QSynEdit *editor,
                        const QSynedit::PSyntaxer &syntaxer, int line,
                        const QSynedit::CharPos &target, bool &targetRejected,
                        int &targetIndex)
{
    QVector<Token> result;
    editor->startParseLine(syntaxer.get(), line);
    while (!syntaxer->eol()) {
        Token token{
            syntaxer->getToken(),
            syntaxer->getTokenType(),
            -1
        };
        int tokenPos = syntaxer->getTokenPos();
        if (line == target.line
                && tokenPos <= target.ch
                && target.ch < tokenPos + token.text.length()) {
            if (token.type == QSynedit::TokenType::Comment
                    || token.type == QSynedit::TokenType::String) {
                targetRejected = true;
            } else if (token.type != QSynedit::TokenType::Space) {
                targetIndex = result.size();
            }
        }
        if (token.type != QSynedit::TokenType::Comment
                && token.type != QSynedit::TokenType::Space)
            result.append(token);
        syntaxer->next();
    }
    return result;
}

class BackwardTokenStream
{
public:
    BackwardTokenStream(const QSynedit::QSynEdit *editor,
                        const QSynedit::PSyntaxer &syntaxer,
                        QVector<Token> tokens, int index, int previousLine)
        : mEditor(editor), mSyntaxer(syntaxer), mTokens(std::move(tokens)),
          mIndex(index), mPreviousLine(previousLine)
    {
    }

    bool next(Token &token)
    {
        if (!ensureAvailable())
            return false;
        token = mTokens[mIndex++];
        return true;
    }

    bool peek(Token &token)
    {
        if (!ensureAvailable())
            return false;
        token = mTokens[mIndex];
        return true;
    }

    int index() const
    {
        return mIndex;
    }

private:
    bool ensureAvailable()
    {
        while (mIndex >= mTokens.size() && mPreviousLine >= 0) {
            QSynedit::CharPos noTarget{-1, -1};
            bool rejected = false;
            int targetIndex = -1;
            QVector<Token> line = readLine(mEditor, mSyntaxer, mPreviousLine, noTarget,
                                           rejected, targetIndex);
            --mPreviousLine;
            std::reverse(line.begin(), line.end());
            mTokens += line;
        }
        return mIndex < mTokens.size();
    }

    const QSynedit::QSynEdit *mEditor;
    QSynedit::PSyntaxer mSyntaxer;
    QVector<Token> mTokens;
    int mIndex;
    int mPreviousLine;
};

struct TemplateSuffix {
    int open = -1;
    int close = -1;
};

TemplateSuffix findTemplateSuffix(const QVector<Token> &tokens, int targetIndex)
{
    TemplateSuffix result;
    int angle = 0;
    int round = 0;
    int square = 0;
    int curly = 0;
    for (int i = 0; i < targetIndex; ++i) {
        const Token &token = tokens[i];
        if (round + square + curly > 0) {
            updateGroupDepth(token.text, round, square, curly);
            if (round < 0 || square < 0 || curly < 0)
                return {};
            continue;
        }
        if (isCloseGroup(token.text)) {
            updateGroupDepth(token.text, round, square, curly);
            continue;
        }
        if (angle > 0) {
            if (isTemplateClose(token.text)) {
                angle += token.text.length();
            } else if (isTemplateOpen(token.text)) {
                angle -= token.text.length();
                if (angle == 0 && i == targetIndex - 1) {
                    result.open = i;
                    return result;
                }
                if (angle < 0)
                    return {};
            } else if (token.text == ";" || token.text == "=") {
                return {};
            }
            continue;
        }
        if (isTemplateClose(token.text)) {
            angle = token.text.length();
            result.close = i;
        }
    }
    return {};
}

struct ExtractionResult {
    QVector<Token> expression;
    int nextIndex = -1;
};

ExtractionResult extractTokens(const QSynedit::QSynEdit *editor,
                               const QSynedit::PSyntaxer &syntaxer,
                               QVector<Token> tokens, int targetIndex, int previousLine)
{
    QVector<Token> expression;
    TemplateSuffix suffix;
    if (targetIndex > 0 && tokens[targetIndex - 1].text == "<")
        suffix = findTemplateSuffix(tokens, targetIndex);
    if (suffix.open >= 0) {
        for (int i = suffix.close; i <= suffix.open; ++i)
            expression.append(tokens[i]);
    }
    expression.append(tokens[targetIndex]);

    BackwardTokenStream stream(editor, syntaxer, std::move(tokens), targetIndex + 1, previousLine);
    int round = 0;
    int square = 0;
    int curly = 0;
    int angle = 0;
    bool needReceiver = false;
    bool needBase = false;
    bool memberChain = false;
    int nextIndex = -1;
    Token token;
    while (stream.next(token)) {
        if (token.text == "\\")
            continue;
        if (round + square + curly > 0) {
            expression.append(token);
            updateGroupDepth(token.text, round, square, curly);
            if (round < 0 || square < 0 || curly < 0)
                return {};
            if (round + square + curly == 0 && angle == 0)
                needBase = true;
            continue;
        }
        if (angle > 0) {
            expression.append(token);
            if (isTemplateClose(token.text)) {
                angle += token.text.length();
            } else if (isTemplateOpen(token.text)) {
                angle -= token.text.length();
                if (angle < 0)
                    return {};
                if (angle == 0)
                    needBase = true;
            } else if (isCloseGroup(token.text)) {
                updateGroupDepth(token.text, round, square, curly);
            }
            continue;
        }
        if (needReceiver) {
            if (isCloseGroup(token.text)) {
                expression.append(token);
                updateGroupDepth(token.text, round, square, curly);
                needReceiver = false;
                needBase = false;
                continue;
            }
            if (isTemplateClose(token.text)) {
                expression.append(token);
                angle = token.text.length();
                needReceiver = false;
                needBase = false;
                continue;
            }
            if (isAtom(token)) {
                expression.append(token);
                needReceiver = false;
                needBase = false;
                continue;
            }
            return {};
        }
        if (needBase) {
            if (isCloseGroup(token.text)) {
                expression.append(token);
                updateGroupDepth(token.text, round, square, curly);
                needBase = false;
                continue;
            }
            if (isTemplateClose(token.text)) {
                expression.append(token);
                angle = token.text.length();
                needBase = false;
                continue;
            }
            if (isAtom(token)) {
                expression.append(token);
                needBase = false;
                continue;
            }
            needBase = false;
        }
        if (isMemberOperator(token.text)) {
            expression.append(token);
            needReceiver = true;
            memberChain = true;
            continue;
        }
        if (token.text == "~") {
            Token previous;
            if (stream.peek(previous) && isMemberOperator(previous.text)) {
                expression.append(token);
                continue;
            }
        }
        if (token.text == "new" && memberChain) {
            expression.append(token);
        } else {
            nextIndex = stream.index() - 1;
        }
        break;
    }

    if (needReceiver || round != 0 || square != 0 || curly != 0 || angle != 0)
        return {};
    std::reverse(expression.begin(), expression.end());
    return {expression, nextIndex >= 0 ? nextIndex : stream.index()};
}
}

CppExpressionExtractor::CppExpressionExtractor(
        const QSynedit::QSynEdit *editor, const QSynedit::PConstSyntaxer &syntaxer)
    : mEditor(editor), mValid(false)
{
    auto cppSyntaxer = std::dynamic_pointer_cast<const QSynedit::CppSyntaxer>(syntaxer);
    Q_ASSERT(cppSyntaxer);
    if (!mEditor || !cppSyntaxer)
        return;
    mSyntaxer = syntaxer->createInstance();
    mValid = mSyntaxer != nullptr;
}

QStringList CppExpressionExtractor::extract(const QSynedit::CharPos &pos)
{
    if (!mValid || pos.ch < 0 || pos.line < 0 || pos.line >= mEditor->lineCount())
        return {};
    bool targetRejected = false;
    int targetIndex = -1;
    QVector<Token> line = readLine(mEditor, mSyntaxer, pos.line, pos,
                                   targetRejected, targetIndex);
    if (targetRejected || targetIndex < 0)
        return {};
    if (line[targetIndex].type != QSynedit::TokenType::Identifier)
        return {};
    targetIndex = line.size() - 1 - targetIndex;
    std::reverse(line.begin(), line.end());
    ExtractionResult extracted = extractTokens(
                mEditor, mSyntaxer, line, targetIndex, pos.line - 1);
    QStringList result;
    for (const Token &token : extracted.expression)
        result.append(token.text);
    return result;
}

QVector<CppExpressionUnit> CppExpressionExtractor::extractLine(
        int line, const QVector<CppExpressionInputToken> &input)
{
    QVector<CppExpressionUnit> result;
    if (!mValid || line < 0 || line >= mEditor->lineCount())
        return result;

    QVector<Token> tokens;
    for (int i = 0; i < input.size(); ++i) {
        if (input[i].type != QSynedit::TokenType::Comment
                && input[i].type != QSynedit::TokenType::Space) {
            tokens.append({input[i].text, input[i].type, i});
        }
    }
    std::reverse(tokens.begin(), tokens.end());
    for (int i = 0; i < tokens.size();) {
        const Token &target = tokens[i];
        if (target.type != QSynedit::TokenType::Identifier) {
            ++i;
            continue;
        }
        ExtractionResult extracted = extractTokens(
                    mEditor, mSyntaxer, tokens, i, line - 1);
        if (extracted.expression.isEmpty()) {
            ++i;
            continue;
        }
        CppExpressionUnit unit;
        unit.targetTokenIndex = target.inputIndex;
        for (const Token &token : extracted.expression) {
            unit.expression.append(token.text);
            unit.inputTokenIndexes.append(token.inputIndex);
        }
        result.append(unit);
        i = qMax(i + 1, extracted.nextIndex);
    }
    return result;
}

QStringList CppExpressionExtractor::extract(
        const QSynedit::QSynEdit *editor, const QSynedit::PConstSyntaxer &syntaxer,
        const QSynedit::CharPos &pos)
{
    CppExpressionExtractor extractor(editor, syntaxer);
    return extractor.extract(pos);
}
