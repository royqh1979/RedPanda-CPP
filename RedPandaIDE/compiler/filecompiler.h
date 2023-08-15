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
#ifndef FILECOMPILER_H
#define FILECOMPILER_H

#include "compiler.h"

class FileCompiler : public Compiler
{
    Q_OBJECT
public:
    FileCompiler(const QString& filename, const QByteArray& encoding,
                 CppCompileType compileType,
                 bool onlyCheckSyntax);
    FileCompiler(const FileCompiler&)=delete;
    FileCompiler& operator=(const FileCompiler&)=delete;

protected:
    bool prepareForCompile() override;

private:
    QByteArray mEncoding;
    CppCompileType mCompileType;
    // Compiler interface
protected:
    bool prepareForRebuild() override;
};

#endif // FILECOMPILER_H
