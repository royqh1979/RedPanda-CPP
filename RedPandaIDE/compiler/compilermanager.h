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
#include "qt_utils/utils.h"
#include "../utils.h"
#include "../common.h"

class Runner;
class Project;
class Compiler;
class ProjectCompiler;
class OJProblem;
using POJProblem = std::shared_ptr<OJProblem>;
class OJProblemCase;
using POJProblemCase = std::shared_ptr<OJProblemCase>;
class CompilerManager : public QObject
{
    Q_OBJECT
public:
    explicit CompilerManager(QObject *parent = nullptr);
    CompilerManager(const CompilerManager&)=delete;
    CompilerManager& operator=(const CompilerManager&)=delete;
    ~CompilerManager();

    bool compiling() const;
    bool backgroundSyntaxChecking() const;
    bool running() const;

    void compile(const QString& filename, FileType fileType, const QByteArray& encoding, bool rebuild, CppCompileType compileType);
    void compileProject(std::shared_ptr<Project> project, bool rebuild);
    void cleanProject(std::shared_ptr<Project> project);
    void buildProjectMakefile(std::shared_ptr<Project> project);
    void checkSyntax(const QString&filename, const QByteArray& encoding, const QString& content, std::shared_ptr<Project> project);
    void run(
            const QString& filename,
            const QString& arguments,
            const QString& workDir,
            const QStringList& extraBinDir);
    void runProblem(
            const QString& filename, const QString& arguments, const QString& workDir, POJProblemCase problemCase,
            const POJProblem& problem
            );
    void runProblem(const QString& filename, const QString& arguments, const QString& workDir, const QVector<POJProblemCase> &problemCases,
                    const POJProblem& problem
                    );
    void stopRun();
    void stopAllRunners();
    void stopPausing();
    void stopCompile();
    void stopCheckSyntax();
    bool canCompile(const QString& filename) const;
    int compileErrorCount() const;

    int syntaxCheckErrorCount() const;

    int compileIssueCount() const;

    int syntaxCheckIssueCount() const;

signals:
    void signalStopAllRunners();
    void compileFinished(const QString& filename, bool isSyntaxCheck);

private slots:
    void doRunProblem(const QString& filename, const QString& arguments, const QString& workDir, const QVector<POJProblemCase> &problemCases,
                      const POJProblem& problem
                      );
    void onRunnerTerminated();
    void onRunnerPausing();
    void onCompileFinished(const QString& filename);
    void onCompileIssue(PCompileIssue issue);
    void onSyntaxCheckFinished(const QString& filename);
    void onSyntaxCheckIssue(PCompileIssue issue);
private:
    ProjectCompiler* createProjectCompiler(std::shared_ptr<Project> project);
private:
    Compiler* mCompiler;
    int mCompileErrorCount;
    int mCompileIssueCount;
    int mSyntaxCheckErrorCount;
    int mSyntaxCheckIssueCount;
    Compiler* mBackgroundSyntaxChecker;
    Runner* mRunner;
    PNonExclusiveTemporaryFileOwner mTempFileOwner;
    mutable QRecursiveMutex mCompileMutex;
    mutable QRecursiveMutex mBackgroundSyntaxCheckMutex;
    mutable QRecursiveMutex mRunnerMutex;
};

class CompileError : public BaseError {
public:
    explicit CompileError(const QString& reason);
};

#endif // COMPILERMANAGER_H
