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
#ifndef STDINCOMPILER_H
#define STDINCOMPILER_H

#include "compiler.h"

class StdinCompiler : public Compiler
{
    Q_OBJECT

public:
    explicit StdinCompiler(const QString& filename, const QByteArray& encoding, const QString& content, bool onlyCheckSyntax);
    StdinCompiler(const StdinCompiler&)=delete;
    StdinCompiler& operator=(const StdinCompiler&)=delete;

protected:
    bool prepareForCompile() override;

private:
    QString mContent;
    QByteArray mEncoding;

    // Compiler interface
protected:
    QByteArray pipedText() override;

    // Compiler interface
protected:
    bool prepareForRebuild() override;

};

#endif // STDINCOMPILER_H
