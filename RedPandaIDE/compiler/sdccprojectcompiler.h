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
#ifndef SDCCPROJECTCOMPILER_H
#define SDCCPROJECTCOMPILER_H

#include "projectcompiler.h"
#include <QObject>
#include <QFile>

class Project;
class SDCCProjectCompiler : public ProjectCompiler
{
    Q_OBJECT
public:
    SDCCProjectCompiler(std::shared_ptr<Project> project);
    SDCCProjectCompiler(const SDCCProjectCompiler&)=delete;
    SDCCProjectCompiler& operator=(const SDCCProjectCompiler&)=delete;
    void buildMakeFile();

private:
    void createStandardMakeFile();
    void newMakeFile(QFile& file);
    void writeMakeHeader(QFile& file);
    void writeMakeDefines(QFile& file);
    void writeMakeTarget(QFile& file);
    void writeMakeIncludes(QFile& file);
    void writeMakeClean(QFile& file);
    void writeMakeObjFilesRules(QFile& file);
    void writeln(QFile& file, const QString& s="");
    // Compiler interface
protected:
    bool prepareForCompile() override;
    bool prepareForRebuild() override;
};

#endif // PROJECTCOMPILER_H
