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
#include "filesearchengine.h"
#include <QDir>
#include <QSet>
#include <QStack>
#include "basicsearcher.h"
#include "regexsearcher.h"

namespace QSynedit {

int FileSearchEngine::searchText(
        const QString &fileName, const QStringList &lines,
        const SearchQuery &query, const FileSearchCallbacks &callbacks)
{
    if (query.pattern.isEmpty())
        return 0;
    SearchOptions options = query.matchCase ? ssoMatchCase : ssoNone;
    std::unique_ptr<Searcher> searcher;
    if (query.useRegex) {
        std::unique_ptr<RegexSearcher> regexSearcher = std::make_unique<RegexSearcher>();
        regexSearcher->setOptions(options);
        regexSearcher->setPattern(query.pattern);
        searcher = std::move(regexSearcher);
    } else {
        searcher = std::make_unique<BasicSearcher>();
        searcher->setOptions(options);
        searcher->setPattern(query.pattern);
    }

    int totalMatches = 0;
    QVector<FileSearchMatch> batch;
    auto flush = [&] {
        if (!batch.isEmpty() && callbacks.onBatch) {
            callbacks.onBatch(batch);
            batch.clear();
        }
    };
    for (int line = 0; line < lines.size(); ++line) {
        if (callbacks.shouldCancel && callbacks.shouldCancel())
            break;
        const QString &lineText = lines.at(line);
        const QVector<TextMatch> matches = searcher->findAll(lineText);
        for (const TextMatch &match : matches) {
            if (query.wholeWord && !isWholeWordMatch(lineText, match))
                continue;
            batch.append({fileName, line, match.ch, match.length, lineText});
            totalMatches++;
            if (batch.size() == 256)
                flush();
        }
        if (callbacks.onProgress)
            callbacks.onProgress(line + 1, lines.size());
    }
    flush();
    return totalMatches;
}

int FileSearchEngine::searchFile(
        const QString &fileName, const SearchQuery &query,
        const FileSearchCallbacks &callbacks, const QByteArray &encoding)
{
    Document document;
    QByteArray realEncoding;
    try {
        document.loadFromFile(fileName, encoding, realEncoding);
    } catch (const BinaryFileError &) {
        return 0;
    } catch (const FileError &) {
        return 0;
    }
    return searchText(fileName, document.content(), query, callbacks);
}

QList<QFileInfo> FileSearchEngine::collectFiles(
        const FileSearchRequest &request,
        const ProgressCallback &progress, const CancelCallback &cancel)
{
    QList<QFileInfo> files;
    QStack<QDir> directories;
    QSet<QString> visited;
    directories.push(QDir(request.directory));
    QStringList filters = request.nameFilters;
    if (filters.isEmpty())
        filters = QStringList{"*.*"};
    int completed = 0;
    while (!directories.isEmpty()) {
        if (cancel && cancel())
            break;
        QDir directory = directories.pop();
        if (request.recursive) {
            const QFileInfoList entries = directory.entryInfoList(
                        QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
            for (const QFileInfo &entry : entries) {
                QString path = entry.absoluteFilePath();
                if (!visited.contains(path)) {
                    visited.insert(path);
                    directories.push(QDir(path));
                }
            }
        }
        files.append(directory.entryInfoList(filters, QDir::Files | QDir::NoSymLinks));
        completed++;
        if (progress)
            progress(completed, 0);
    }
    return files;
}

}
