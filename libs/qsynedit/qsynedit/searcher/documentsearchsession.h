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
#ifndef QSYNEDIT_DOCUMENTSEARCHSESSION_H
#define QSYNEDIT_DOCUMENTSEARCHSESSION_H

#include <optional>
#include "../document.h"
#include "baseseacher.h"

namespace QSynedit {

struct SearchQuery {
    QString pattern;
    bool useRegex = false;
    bool matchCase = false;
    bool wholeWord = false;
};

struct SearchRange {
    CharPos begin;
    CharPos end;

    bool contains(const CharPos &pos, int length) const;
};

struct DocumentMatch {
    CharPos start;
    int length;
    int documentVersion;
};

enum class SearchDirection {
    Forward,
    Backward,
};

enum class WrapMode {
    Never,
    Once,
};

bool isWholeWordMatch(const QString &lineText, const TextMatch &match);

class DocumentSearchSession {
public:
    DocumentSearchSession(PDocument document, SearchQuery query, SearchRange scope);

    std::optional<DocumentMatch> findNext(
            CharPos from,
            SearchDirection direction,
            WrapMode wrap = WrapMode::Never);
    QString replacementText(const DocumentMatch &match, const QString &replacement) const;
    const SearchRange &scope() const { return mScope; }
    void setScope(const SearchRange &scope) { mScope = scope; }
    const PDocument &document() const { return mDocument; }

private:
    std::optional<DocumentMatch> findInRange(
            CharPos from, const SearchRange &range, SearchDirection direction);

private:
    PDocument mDocument;
    SearchQuery mQuery;
    SearchRange mScope;
    std::unique_ptr<Searcher> mSearcher;
};

}

#endif // QSYNEDIT_DOCUMENTSEARCHSESSION_H
