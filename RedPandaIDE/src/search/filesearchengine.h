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
#ifndef REDPANDA_FILESEARCHENGINE_H
#define REDPANDA_FILESEARCHENGINE_H

#include <QFileInfo>
#include <functional>
#include <qsynedit/searcher/documentsearchsession.h>

struct FileSearchMatch {
    QString fileName;
    int line;
    int ch;
    int length;
    QString lineText;
};

struct FileSearchRequest {
    QString directory;
    QStringList nameFilters;
    bool recursive = true;
};

using ProgressCallback = std::function<void (int completed, int total)>;
using CancelCallback = std::function<bool ()>;
using MatchBatchCallback = std::function<void (QVector<FileSearchMatch>)>;

struct FileSearchCallbacks {
    MatchBatchCallback onBatch;
    ProgressCallback onProgress;
    CancelCallback shouldCancel;
};

class FileSearchEngine {
public:
    FileSearchEngine() = default;

    int searchText(
            const QString &fileName,
            const QStringList &lines,
            const QSynedit::SearchQuery &query,
            const FileSearchCallbacks &callbacks);
    int searchFile(
            const QString &fileName,
            const QSynedit::SearchQuery &query,
            const FileSearchCallbacks &callbacks,
            const QByteArray &encoding = ENCODING_AUTO_DETECT);
    QList<QFileInfo> collectFiles(
            const FileSearchRequest &request,
            const ProgressCallback &progress = {},
            const CancelCallback &cancel = {});
};

#endif // REDPANDA_FILESEARCHENGINE_H
