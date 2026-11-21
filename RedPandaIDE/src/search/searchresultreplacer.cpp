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
#include "searchresultreplacer.h"
#include <algorithm>

SearchResultReplaceError SearchResultReplacer::replaceSelected(
        QSynedit::QSynEdit &editor,
        const SearchResults &results,
        const SearchResultTreeItem &file,
        const QString &replacement)
{
    QVector<PSearchResultTreeItem> selections;
    for (const PSearchResultTreeItem &item : file.results) {
        if (item->selected)
            selections.append(item);
    }
    if (selections.isEmpty())
        return SearchResultReplaceError::None;

    for (const PSearchResultTreeItem &item : selections) {
        if (item->line < 0 || item->line >= editor.lineCount()
                || item->start < 0 || item->len < 0)
            return SearchResultReplaceError::ContentChanged;
        QString lineText = editor.lineText(item->line);
        if (item->start + item->len > lineText.size()
                || lineText.mid(item->start, item->len) != item->matchedText)
            return SearchResultReplaceError::ContentChanged;
    }

    std::sort(selections.begin(), selections.end(),
              [](const PSearchResultTreeItem &left, const PSearchResultTreeItem &right) {
        if (left->line != right->line)
            return left->line > right->line;
        return left->start > right->start;
    });
    QSynedit::SearchQuery query{
        results.keyword(), results.useRegex(),
        results.options().testFlag(QSynedit::ssoMatchCase),
        results.options().testFlag(QSynedit::ssoWholeWord)};
    QSynedit::DocumentSearchSession session(
                editor.document(), query, {editor.fileBegin(), editor.fileEnd()});
    editor.beginEditing();
    for (const PSearchResultTreeItem &item : selections) {
        QSynedit::DocumentMatch match{
            {item->start, item->line}, item->len,
            editor.document()->contentVersion()};
        QString replacementText = session.replacementText(match, replacement);
        if (!editor.replaceMatch(match, replacementText)) {
            editor.endEditing();
            return SearchResultReplaceError::ContentChanged;
        }
    }
    editor.endEditing();
    return SearchResultReplaceError::None;
}
