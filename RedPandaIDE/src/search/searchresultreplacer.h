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
#ifndef REDPANDA_SEARCHRESULTREPLACER_H
#define REDPANDA_SEARCHRESULTREPLACER_H

#include <qsynedit/qsynedit.h>
#include "../widgets/searchresultview.h"

enum class SearchResultReplaceError {
    None,
    ContentChanged,
};

class SearchResultReplacer {
public:
    static SearchResultReplaceError replaceSelected(
            QSynedit::QSynEdit &editor,
            const SearchResults &results,
            const SearchResultTreeItem &file,
            const QString &replacement);
};

#endif // REDPANDA_SEARCHRESULTREPLACER_H
