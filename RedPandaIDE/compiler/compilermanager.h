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
#ifndef COMPILERMANAGER_H
#define COMPILERMANAGER_H

#include <QObject>
#include <QMutex>
#include "../utils.h"
#include "../common.h"

class Runner;
class Compiler;
class Project;
class OJProblemCase;
using POJProblemCase = std::shared_ptr<OJProblemCase>;
class CompilerManager : public QObject
{
    Q_OBJECT
public:
    explicit CompilerManager(QObject *parent = nullptr);

    bool compiling();
    bool backgroundSyntaxChecking();
    bool running();

    void compile(const QString& filename, const QByteArray& encoding, bool rebuild, bool silent=false,bool onlyCheckSyntax=false);
    void compileProject(std::shared_ptr<Project> project, bool rebuild, bool silent=false,bool onlyCheckSyntax=false);
    void cleanProject(std::shared_ptr<Project> project);
    void buildProjectMakefile(std::shared_ptr<Project> project);
    void checkSyntax(const QString&filename, const QByteArray& encoding, const QString& content, std::shared_ptr<Project> project);
    void run(const QString& filename, const QString& arguments, const QString& workDir);
    void runProblem(const QString& filename, const QString& arguments, const QString& workDir, POJProblemCase problemCase);
    void runProblem(const QString& filename, const QString& arguments, const QString& workDir, QVector<POJProblemCase> problemCases);
    void stopRun();
    void stopAllRunners();
    void stopPausing();
    void stopCompile();
    void stopCheckSyntax();
    bool canCompile(const QString& filename);
    int compileErrorCount() const;

    int syntaxCheckErrorCount() const;

    int compileIssueCount() const;

    int syntaxCheckIssueCount() const;

signals:
    void signalStopAllRunners();

private slots:
    void onRunnerTerminated();
    void onRunnerPausing();
    void onCompileFinished();
    void onCompileIssue(PCompileIssue issue);
    void onSyntaxCheckFinished();
    void onSyntaxCheckIssue(PCompileIssue issue);

private:
    Compiler* mCompiler;
    int mCompileErrorCount;
    int mCompileIssueCount;
    int mSyntaxCheckErrorCount;
    int mSyntaxCheckIssueCount;
    Compiler* mBackgroundSyntaxChecker;
    Runner* mRunner;
    QMutex mCompileMutex;
    QMutex mBackgroundSyntaxCheckMutex;
    QMutex mRunnerMutex;
};

class CompileError : public BaseError {
public:
    explicit CompileError(const QString& reason);
};

#endif // COMPILERMANAGER_H
