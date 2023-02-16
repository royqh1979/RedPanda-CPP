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
#ifndef XMAKECOMPILER_H
#define XMAKECOMPILER_H

#include "compiler.h"
#include <QObject>
#include <QFile>

class Project;
class XMakeCompiler : public Compiler
{
    Q_OBJECT
public:
    XMakeCompiler(std::shared_ptr<Project> project, bool silent,bool onlyCheckSyntax);
    XMakeCompiler(const XMakeCompiler&)=delete;
    XMakeCompiler& operator=(const XMakeCompiler&)=delete;
    void buildXMakeFile();

    bool onlyClean() const;
    void setOnlyClean(bool newOnlyClean);

private:
    void newXMakeFile(QFile& file);
    void writeln(QFile& file, const QString& s="");
    // Compiler interface
private:
    bool mOnlyClean;
protected:
    bool prepareForCompile() override;
    bool prepareForRebuild() override;
};

#endif // XMakeCompiler_H
