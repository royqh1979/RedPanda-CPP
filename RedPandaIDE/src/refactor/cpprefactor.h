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
#ifndef REDPANDA_CPPREFACTOR_H
#define REDPANDA_CPPREFACTOR_H

#include <QObject>
#include <optional>
#include "../editor/cppexpressionextractor.h"
#include "../parser/cppparser.h"
#include "../project.h"

namespace QSynedit {
struct CharPos;
}

enum class SymbolReferenceScope {
    CurrentFile,
    WholeProject,
};

enum class SymbolReferenceRole {
    Declaration,
    Definition,
    Reference,
};

struct SymbolReference {
    QString fileName;
    int line = 0;
    int ch = 0;
    int length = 0;
    QString lineText;
    SymbolReferenceRole role = SymbolReferenceRole::Reference;
};

struct SymbolReferenceFile {
    QString fileName;
    int documentVersion = -1;
    QVector<SymbolReference> references;
};

struct SymbolReferenceResults {
    QString symbol;
    QString scopeFullName;
    SymbolReferenceScope scope = SymbolReferenceScope::CurrentFile;
    QVector<PStatement> targets;
    QVector<SymbolReferenceFile> files;

    int count() const;
};

enum class RenameResult {
    Success,
    NoSelection,
    InvalidInput,
    ParserUnavailable,
    SymbolNotFound,
    NotInCurrentFile,
    NameConflict,
    NotInScope,
    ContentChanged,
};

class CppRefactor : public QObject
{
    Q_OBJECT
public:
    using ContentProvider = std::function<bool (const QString &, QStringList &)>;
    explicit CppRefactor(
            std::shared_ptr<Project> project = nullptr,
            ContentProvider contentProvider = {},
            QObject *parent = nullptr);

    bool findOccurrences(
            Editor *editor,
            const QSynedit::CharPos &pos,
            SymbolReferenceResults &results);
    bool findOccurrences(
            Editor *editor,
            const QString &keyword,
            const QString &scopeFullName,
            SymbolReferenceScope scope,
            SymbolReferenceResults &results);

    QVector<PStatement> overloadsOf(Editor *editor, const QSynedit::CharPos &pos);
    RenameResult prepareRename(
            Editor *editor,
            const QSynedit::CharPos &pos,
            const QString &newWord,
            const std::optional<QVector<PStatement>> &selectedOverloads,
            SymbolReferenceResults &results);
    RenameResult applyRename(
            Editor *editor,
            const SymbolReferenceResults &results,
            const QString &newWord);
    RenameResult renameSymbol(
            Editor *editor,
            const QSynedit::CharPos &pos,
            const QString &newWord,
            const std::optional<QVector<PStatement>> &selectedOverloads = std::nullopt);
    RenameResult renameUndefinedLocalVariable(
            Editor *editor,
            const QSynedit::CharPos &pos,
            const QString &newWord);

private:
    bool doFindOccurrences(
            Editor *editor,
            const PStatement &statement,
            const QVector<PStatement> &targets,
            SymbolReferenceScope scope,
            SymbolReferenceResults &results);
    SymbolReferenceFile findOccurrencesInFile(
            const QString &fileName,
            const QByteArray &fileEncoding,
            const QVector<PStatement> &targets,
            const PCppParser &parser,
            const QStringList &openContent = {});
    QVector<PStatement> overloadsOf(
            const PCppParser &parser,
            const PStatement &statement) const;
    QVector<PStatement> collectRenameTargets(
            const PCppParser &parser,
            const PStatement &statement,
            const std::optional<QVector<PStatement>> &selectedOverloads) const;
    QVector<PStatement> collectClassTargets(
            const PCppParser &parser,
            const PStatement &classStatement) const;
    void collectOverrides(
            const PCppParser &parser,
            const PStatement &statement,
            QSet<PStatement> &targets,
            QSet<PStatement> &visited) const;
    RenameResult renameReferencesInEditor(
            Editor *editor,
            const SymbolReferenceFile &file,
            const QString &newWord);

private:
    std::shared_ptr<Project> mProject;
    ContentProvider mContentProvider;
};

#endif // REDPANDA_CPPREFACTOR_H
