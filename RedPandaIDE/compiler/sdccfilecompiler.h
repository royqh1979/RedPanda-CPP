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
#ifndef SDCCFILECOMPILER_H
#define SDCCFILECOMPILER_H

#include "compiler.h"

class SDCCFileCompiler : public Compiler
{
    Q_OBJECT
public:
    SDCCFileCompiler(const QString& filename, const QByteArray& encoding,
                 CppCompileType compileType, bool onlyCheckSyntax);
    SDCCFileCompiler(const SDCCFileCompiler&)=delete;
    SDCCFileCompiler& operator=(const SDCCFileCompiler&)=delete;

protected:
    bool prepareForCompile() override;
    bool beforeRunExtraCommand(int idx) override;

private:
    QByteArray mEncoding;
    CppCompileType mCompileType;
    QDateTime mStartCompileTime;
    QString mIhxFilename;
    QString mRelFilename;
    bool mNoStartup;
    // Compiler interface
protected:
    bool prepareForRebuild() override;
};

#endif // SDCCFILECOMPILER_H
