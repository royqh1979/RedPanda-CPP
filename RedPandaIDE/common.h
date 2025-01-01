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
#ifndef COMMON_H
#define COMMON_H
#include <QString>
#include <memory>
#include <QMetaType>

enum class CppCompileType {
    Normal,
    PreprocessOnly,
    GenerateAssemblyOnly,
    GenerateGimpleOnly,
};

enum class CompileIssueType {
    Other,
    Warning,
    Info,
    Note,
    Error,
};

struct CompileIssue {
    QString filename;
    int line;
    int column;
    int endColumn;
    QString description;
    CompileIssueType type;
};

typedef std::shared_ptr<CompileIssue> PCompileIssue;

Q_DECLARE_METATYPE(PCompileIssue);

#endif // COMMON_H
