/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#ifndef CPPPARSER_H
#define CPPPARSER_H

#include <QMutex>
#include <QObject>
#include <QThread>
#include <QVector>
#include "statementmodel.h"
#include "cpppreprocessor.h"
#include "cppexpressionevaluator.h"

class CppParser;
using PCppParser = std::shared_ptr<CppParser>;

class CppParser : public QObject
{
    Q_OBJECT
public:
    struct ParseFileCommand {
        QString fileName;
        bool inProject;
        QString contextFilename;
        bool onlyIfNotParsed;
        bool updateView;
    };
    using PParseFileCommand = std::unique_ptr<ParseFileCommand>;

    explicit CppParser();
    CppParser(const CppParser&)=delete;
    CppParser& operator=(const CppParser&)=delete;
    ~CppParser();

    void addHardDefineByLine(const QString& line);
    void addProjectFile(const QString &fileName, bool needScan);
    void addIncludePath(const QString& value);
    void removeProjectFile(const QString& value);
    void addProjectIncludePath(const QString& value);
    void clearIncludePaths();
    void clearProjectIncludePaths();
    void clearProjectFiles();
    QList<PStatement> getListOfFunctions(const QString& fileName, const QString& phrase, int line) const;
    PStatement findScopeStatement(const QString& filename, int line) const;
    PParsedFileInfo findFileInfo(const QString &filename) const;
    PReplayRecord findReplayRecord(const QString &filename) const;
    QString findFirstTemplateParamOf(const QString& fileName, const QString& phrase,
                                     const PStatement& currentScope) const;
    QString findTemplateParamOf(const QString& fileName, const QString& phrase, int index,
                                const PStatement& currentScope) const;
    PStatement findFunctionAt(const QString& fileName, int line) const;
    int findLastOperator(const QString& phrase) const;
    PStatementList findNamespace(const QString& name, bool includeInline = false) const;
    PStatement findStatement(const QString& fullname) const;
    QList<PStatement> membersOfStatement(const PStatement &scopeStatement) const;
    QList<PStatement> resolveMembersOfClass(const PStatement &classStatement,
                                            const PStatement &accessScope) const;
    PStatement findStatementOf(const QString& fileName, const QString& phrase, int line) const;
    PStatement findStatementOf(const QString& fileName, const QString& phrase,
                               const PStatement& currentScope, PStatement& parentScopeType) const;
    PStatement findStatementOf(const QString& fileName, const QStringList& expression,
                               const PStatement& currentScope) const;
    PStatement findStatementOf(const QString& fileName, const QStringList& expression, int line) const;
    PStatement findAliasedStatement(const PStatement& statement) const;
    QStringList getFunctionParameterNames(const PStatement& statement) const;
    QList<PStatement> listTypeStatements(const QString& fileName, int line) const;
    QList<PStatement> listLiteralOperators(const QString& fileName, int line) const;
    EvalResult evalExpression(const QString& fileName, QStringList& expression,
                              const PStatement& currentScope,
                              ExpressionEvalTrace *trace = nullptr) const;
    EvalResult evalExpression(const QString &fileName, const CppTokenizer::TokenList &expression,
                              const PStatement &currentScope,
                              ExpressionEvalTrace *trace = nullptr) const;
    PStatement findTypeDefinitionOf(const QString& fileName, const QString& typeName,
                                    const PStatement& currentClass) const;
    PStatement findTypeDef(const PStatement& statement, const QString& fileName) const;
    bool freeze();
    bool freeze(const QString& serialId);
    QStringList getClassesList() const;
    QStringList getFileDirectIncludes(const QString& filename) const;
    QSet<QString> getIncludedFiles(const QString& filename) const;
    QSet<QString> getFileUsings(const QString& filename) const;
    QString getHeaderFileName(const QString& relativeTo, const QString& headerName,
                              bool fromNext = false) const;
    void invalidateFile(const QString& fileName);
#ifdef REDPANDA_PARSER_TEST
    void invalidateFileRecordForTest(const QString &fileName) { internalInvalidateFile(fileName); }
    CppPreprocessor::EventCounts preprocessorEventCountsForTest(const QString &fileName) const {
        return mPreprocessor.eventCounts(fileName);
    }
    const QStringList &preprocessedForTest() const { return mPreprocessedForTest; }
    const CppTokenizer::TokenList &tokenizerTokensForTest() const { return mTokenizerTokensForTest; }
#endif
    bool isLineVisible(const QString& fileName, int line) const;
    bool isIncludeLine(const QString &line) const;
    bool isIncludeNextLine(const QString &line) const;
    bool isProjectHeaderFile(const QString& fileName) const;
    bool isSystemHeaderFile(const QString& fileName) const;
    void parseHardDefines();
    CppCompilerType compilerType() const;
    bool parsing() const;
    int cppVersion() const;
    void resetParser();
    void unFreeze();
    bool hasReplayRecord(const QString& fileName) const;
    bool isFileParsedInContext(const QString &fileName, const QString &contextFileName) const;
    QString prettyPrintStatement(const PStatement& statement, const QString& filename, int line = -1) const;
    bool enabled() const;
    void setEnabled(bool newEnabled);
    const QSet<QString> &filesToScan() const;
    void setFilesToScan(const QSet<QString> &newFilesToScan);
    void setOnGetFileStream(const GetFileStreamFunc &newOnGetFileStream);
    int parserId() const;
    const QString &serialId() const;
    const QSet<QString>& includePaths();
    const QSet<QString>& projectIncludePaths();
    const StatementModel &statementList() const;
    ParserLanguage language() const;
    void setLanguage(ParserLanguage newLanguage);
    const QSet<QString> &projectFiles() const;
    QList<QString> namespaces();

    static bool isIdentifier(const QString& token) {
        return !token.isEmpty()
                && ((token.startsWith("::") && token.length() > 2)
                    || token.front() == '_' || token.front().isLetter());
    }
    static constexpr bool isSpaceChar(const QChar& ch) { return ch == ' ' || ch == '\t'; }

    bool sharedByFiles() const;
    void setSharedByFiles(bool newSharedByFiles);

    static void parseFileBlocking(PCppParser parser, const QString &fileName, bool inProject,
                                  const QString &contextFilename, bool onlyIfNotParsed = false,
                                  bool updateView = true);
    static void parseFileNonBlocking(PCppParser parser, const QString &fileName, bool inProject,
                                     const QString &contextFilename, bool onlyIfNotParsed = false,
                                     bool updateView = true);

signals:
    void progress(const QString& fileName, int total, int current);
    void onBusy();
    void parseStarted();
    void parseFinished(int total, int updateView);

private:
    bool parseFile(const QString& fileName, bool inProject, const QString& contextFilename,
                   bool onlyIfNotParsed = false, bool updateView = true);
    PParseFileCommand retrievePendingParseFileCommand();
    void internalParse(const QString& fileName);
    CppEvaluationOptions evaluationOptions() const;
    void internalInvalidateFile(const QString& fileName);
    void internalInvalidateFiles(const QSet<QString>& files);
    QSet<QString> calculateFilesToBeReparsed(const QString& fileName);
    QStringList sortFilesByIncludeRelations(const QSet<QString>& files);
    void internalClear();

    PStatement doFindStatementOf(const QString& fileName, const QString& phrase, int line) const;
    PStatement doFindStatementOf(const QString& fileName, const QString& phrase,
                                 const PStatement& currentScope, PStatement& parentScopeType) const;
    PStatement doFindStatementOf(const QString& fileName, const QStringList& expression,
                                 const PStatement& currentScope) const;
    PStatement doFindStatementOf(const QString& fileName, const QStringList& expression, int line) const;
    EvalResult doEvalExpression(const QString& fileName, QStringList& expression,
                                const PStatement& currentScope,
                                ExpressionEvalTrace *trace = nullptr) const;
    EvalResult doEvalExpression(const QString &fileName, const CppTokenizer &tokenizer,
                                const PStatement &currentScope,
                                ExpressionEvalTrace *trace) const;
    PStatementList doFindNamespace(const QString& name, bool includeInline = false) const;
    QList<PStatement> doMembersOfStatement(const PStatement &scopeStatement) const;
    PStatement findStatementStartingFrom(const QString& fileName, const QString& phrase,
                                         const PStatement& startScope) const;
    PStatement findMemberOfStatement(const QString& phrase, const PStatement& scopeStatement) const;
    QList<PStatement> findMembersOfStatement(const QString& phrase,
                                             const PStatement& scopeStatement) const;
    PStatement findStatementInScope(const QString& name, const PStatement& scope) const;
    PStatement findStatementInNamespace(const QString& name, const QString& namespaceName) const;
    PStatement visibleStatement(const QString& fileName, const PStatement& statement) const;
    QList<PStatement> doListStatements(const QString& fileName, int line,
                                       const QSet<StatementKind>& kinds) const;
    TypeInfo parseTypeInfo(const QString& text) const;
    PStatement findMacro(const QString& phrase, const QString& fileName) const;
    QString getScopePrefix(const PStatement& statement) const;
    void updateSerialId();

private:
    int mParserId;
    ParserLanguage mLanguage;
    int mSerialCount;
    QString mSerialId;
    int mUniqId;
    bool mEnabled;
    bool mSharedByFiles;
    bool mIsSystemHeader;
    mutable StatementModel mStatementList;
    CppPreprocessor mPreprocessor;
    QSet<QString> mProjectFiles;
    QSet<QString> mFilesToScan;
    int mFilesScannedCount;
    int mFilesToScanCount;
    int mLockCount;
    bool mParsing;
    bool mStopForReset;
    PParseFileCommand mLastParseFileCommand;
    mutable QRecursiveMutex mMutex;
#ifdef REDPANDA_PARSER_TEST
    QStringList mPreprocessedForTest;
    CppTokenizer::TokenList mTokenizerTokensForTest;
#endif

    friend class CppFileParserThread;
};

class CppFileParserThread : public QThread {
    Q_OBJECT
public:
    explicit CppFileParserThread(PCppParser parser, const QString &fileName, bool inProject,
                                 const QString &contextFilename, bool onlyIfNotParsed = false,
                                 bool updateView = true, QObject *parent = nullptr);
    ~CppFileParserThread();
private:
    PCppParser mParser;
    QString mFileName;
    bool mInProject;
    QString mContextFilename;
    bool mOnlyIfNotParsed;
    bool mUpdateView;
    int mId;
protected:
    void run() override;
};
using PCppParserThread = std::shared_ptr<CppFileParserThread>;

#endif // CPPPARSER_H
