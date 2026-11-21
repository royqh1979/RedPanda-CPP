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
#include "documentsearchsession.h"
#include "basicsearcher.h"
#include "regexsearcher.h"

namespace QSynedit {

bool SearchRange::contains(const CharPos &pos, int length) const
{
    CharPos matchEnd{pos.ch + length, pos.line};
    return pos >= begin && matchEnd <= end;
}

static bool isWordCodePoint(uint codePoint)
{
    QChar::Category category = QChar::category(codePoint);
    switch (category) {
    case QChar::Letter_Uppercase:
    case QChar::Letter_Lowercase:
    case QChar::Letter_Titlecase:
    case QChar::Letter_Modifier:
    case QChar::Letter_Other:
    case QChar::Number_DecimalDigit:
    case QChar::Number_Letter:
    case QChar::Number_Other:
    case QChar::Mark_NonSpacing:
    case QChar::Mark_SpacingCombining:
    case QChar::Mark_Enclosing:
    case QChar::Punctuation_Connector:
        return true;
    default:
        return false;
    }
}

static uint codePointBefore(const QString &text, int pos)
{
    int index = pos - 1;
    if (index > 0 && text.at(index).isLowSurrogate()
            && text.at(index - 1).isHighSurrogate()) {
        return QChar::surrogateToUcs4(text.at(index - 1), text.at(index));
    }
    return text.at(index).unicode();
}

static uint codePointAt(const QString &text, int pos)
{
    if (text.at(pos).isHighSurrogate() && pos + 1 < text.size()
            && text.at(pos + 1).isLowSurrogate())
        return QChar::surrogateToUcs4(text.at(pos), text.at(pos + 1));
    return text.at(pos).unicode();
}

bool isWholeWordMatch(const QString &lineText, const TextMatch &match)
{
    if (match.length == 0)
        return true;
    if (match.ch < 0 || match.length < 0
            || match.ch + match.length > lineText.size())
        return false;
    if (match.ch > 0
            && isWordCodePoint(codePointBefore(lineText, match.ch)))
        return false;
    int end = match.ch + match.length;
    return end == lineText.size()
            || !isWordCodePoint(codePointAt(lineText, end));
}

DocumentSearchSession::DocumentSearchSession(
        PDocument document, SearchQuery query, SearchRange scope)
    : mDocument(std::move(document)),
      mQuery(std::move(query)),
      mScope(scope)
{
    SearchOptions options = mQuery.matchCase ? ssoMatchCase : ssoNone;
    if (mQuery.useRegex) {
        std::unique_ptr<RegexSearcher> searcher = std::make_unique<RegexSearcher>();
        searcher->setOptions(options);
        searcher->setPattern(mQuery.pattern);
        mSearcher = std::move(searcher);
    } else {
        mSearcher = std::make_unique<BasicSearcher>();
        mSearcher->setOptions(options);
        mSearcher->setPattern(mQuery.pattern);
    }
}

std::optional<DocumentMatch> DocumentSearchSession::findNext(
        CharPos from, SearchDirection direction, WrapMode wrap)
{
    if (!mDocument || mQuery.pattern.isEmpty())
        return std::nullopt;
    std::optional<DocumentMatch> result = findInRange(from, mScope, direction);
    if (result || wrap == WrapMode::Never)
        return result;
    if (direction == SearchDirection::Forward) {
        SearchRange wrappedRange{mScope.begin, from};
        return findInRange(mScope.begin, wrappedRange, direction);
    }
    SearchRange wrappedRange{from, mScope.end};
    return findInRange(mScope.end, wrappedRange, direction);
}

QString DocumentSearchSession::replacementText(
        const DocumentMatch &match, const QString &replacement) const
{
    QString occurrence = mDocument->getLine(match.start.line).mid(
                match.start.ch, match.length);
    return mSearcher->replace(occurrence, replacement);
}

std::optional<DocumentMatch> DocumentSearchSession::findInRange(
        CharPos from, const SearchRange &range, SearchDirection direction)
{
    int step = direction == SearchDirection::Forward ? 1 : -1;
    int line = from.line;
    while (line >= range.begin.line && line <= range.end.line
           && line >= 0 && line < mDocument->count()) {
        QString lineText = mDocument->getLine(line);
        QVector<TextMatch> matches = mSearcher->findAll(lineText);
        if (direction == SearchDirection::Backward) {
            for (int i = matches.size() - 1; i >= 0; --i) {
                const TextMatch &match = matches.at(i);
                if (line == from.line && match.ch + match.length > from.ch)
                    continue;
                CharPos start{match.ch, line};
                if (!range.contains(start, match.length))
                    continue;
                if (mQuery.wholeWord && !isWholeWordMatch(lineText, match))
                    continue;
                return DocumentMatch{start, match.length, mDocument->contentVersion()};
            }
        } else {
            for (const TextMatch &match : matches) {
                if (line == from.line && match.ch < from.ch)
                    continue;
                CharPos start{match.ch, line};
                if (!range.contains(start, match.length))
                    continue;
                if (mQuery.wholeWord && !isWholeWordMatch(lineText, match))
                    continue;
                return DocumentMatch{start, match.length, mDocument->contentVersion()};
            }
        }
        line += step;
    }
    return std::nullopt;
}

}
