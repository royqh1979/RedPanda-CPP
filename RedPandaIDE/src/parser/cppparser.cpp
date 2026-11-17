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
#include "cppparser.h"

#include "cppdeclarationparser.h"
#include "cppstatementbuilder.h"
#include "cppsyntaxparser.h"
#include <qt_utils/utils.h>
#include <QApplication>
#include <QDebug>
#include <QDate>
#include <QElapsedTimer>
#include <QQueue>
#include <QTime>

static QAtomicInt cppParserCount(0);

CppParser::CppParser()
    : mParserId(cppParserCount.fetchAndAddRelaxed(1)), mLanguage(ParserLanguage::CPlusPlus),
      mSerialCount(0), mUniqId(0), mEnabled(true), mSharedByFiles(false),
      mIsSystemHeader(false), mFilesScannedCount(0), mFilesToScanCount(0),
      mLockCount(0), mParsing(false), mStopForReset(false)
{
    updateSerialId();
    mPreprocessor.setFileInvalidationHandler(
                [this](const QString &fileName) { internalInvalidateFile(fileName); });
}

CppParser::~CppParser()
{
    resetParser();
}

void CppParser::addHardDefineByLine(const QString &line)
{
    QMutexLocker locker(&mMutex);
    mPreprocessor.addHardDefineByLine(line.trimmed());
}

void CppParser::addProjectFile(const QString &fileName, bool needScan)
{
    QMutexLocker locker(&mMutex);
    mProjectFiles.insert(fileName);
    mPreprocessor.addProjectFile(fileName);
    if (needScan && !mPreprocessor.hasReplayRecord(fileName))
        mFilesToScan.insert(fileName);
}

void CppParser::addIncludePath(const QString &value)
{
    QMutexLocker locker(&mMutex);
    mPreprocessor.addIncludePath(value);
}

void CppParser::removeProjectFile(const QString &value)
{
    QMutexLocker locker(&mMutex);
    mProjectFiles.remove(value);
    mPreprocessor.removeProjectFile(value);
    mFilesToScan.remove(value);
    internalInvalidateFile(value);
}

void CppParser::addProjectIncludePath(const QString &value)
{
    QMutexLocker locker(&mMutex);
    mPreprocessor.addProjectIncludePath(value);
}

void CppParser::clearIncludePaths()
{
    QMutexLocker locker(&mMutex);
    mPreprocessor.clearIncludePaths();
}

void CppParser::clearProjectIncludePaths()
{
    QMutexLocker locker(&mMutex);
    mPreprocessor.clearProjectIncludePaths();
}

void CppParser::clearProjectFiles()
{
    QMutexLocker locker(&mMutex);
    mProjectFiles.clear();
    mPreprocessor.clearProjectFiles();
    mFilesToScan.clear();
}

void CppParser::internalParse(const QString &fileName)
{
    if (fileName.isEmpty())
        return;
    QElapsedTimer timer;
    timer.start();
    mPreprocessor.setScanOptions(true, true);
    mPreprocessor.preprocess(fileName);
    qint64 preprocessMs = timer.restart();
    mStatementList.setCppVersion(mPreprocessor.cppVersion());

    CppTokenizer tokenizer;
    tokenizer.tokenize(mPreprocessor.result());
    qint64 tokenizeMs = timer.restart();
#ifdef REDPANDA_PARSER_TEST
    mPreprocessedForTest = mPreprocessor.result();
    mTokenizerTokensForTest = tokenizer.tokens();
#endif
    CppTokenCursor cursor(tokenizer);
    CppStatementBuilder builder(mStatementList);
    const QHash<QString, QStringList> &ancestors = mPreprocessor.boundaryAncestors();
    qint64 parseDeclarationsMs = 0;
    qint64 buildStatementsMs = 0;
    QElapsedTimer stepTimer;
    QHash<QString, qint64> declarationFileTimes;
    QElapsedTimer declarationFileTimer;
    declarationFileTimer.start();
    QHash<QString, int> occurrences;
    QString currentFile;
    QString currentAncestor;
    while (!cursor.atEnd()) {
        if (mStopForReset)
            break;
        const CppTokenizer::Token &token = cursor.current();
        if (token.kind == CppTokenizer::TokenKind::FileBoundary) {
            declarationFileTimes[currentFile] += declarationFileTimer.restart();
            currentFile = token.fileName;
            int occurrence = occurrences.value(currentFile, 0);
            occurrences[currentFile] = occurrence + 1;
            currentAncestor = ancestors.value(currentFile).value(occurrence);
            cursor.advance();
            continue;
        }
        if (token.kind == CppTokenizer::TokenKind::Preprocessor) {
            if (token.text.startsWith("#define ")) {
                QString name, args, value;
                mPreprocessor.getDefineParts(token.text.mid(7).trimmed(), name, args, value);
                if (!name.isEmpty()) {
                    auto statement = StatementFactory::createPreprocessor(PStatement(), name, name);
                    std::dynamic_pointer_cast<Statement>(statement)->args = args;
                    std::dynamic_pointer_cast<Statement>(statement)->value = value;
                    statement->definition = StatementOccurrence{
                        currentFile,
                        SourceRange{token.position,
                                    SourcePosition{token.position.ch + token.text.length() - 1,
                                                   token.position.line}},
                        currentAncestor, {}};
                    setStatementHasDefinition(statement, true);
                    statement->properties.setFlag(StatementProperty::InProject,
                                                  mProjectFiles.contains(currentFile));
                    statement->properties.setFlag(StatementProperty::InSystemHeader,
                        !currentAncestor.isEmpty()
                                || ::isSystemHeaderFile(currentFile, mPreprocessor.includePaths()));
                    mStatementList.add(statement);
                    PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(currentFile);
                    if (fileInfo)
                        fileInfo->addStatement(statement);
                }
            }
            cursor.advance();
            continue;
        }
        DeclarationInfo declaration;
        if (!mStopForReset) {
            CppSyntaxParser parser(cursor);
            parser.setCppVersion(mPreprocessor.cppVersion());
            stepTimer.start();
            if (parser.parseDeclaration(declaration)) {
                parseDeclarationsMs += stepTimer.restart();
                StatementBuildContext context;
                context.fileName = declaration.fileName.isEmpty() ? currentFile : declaration.fileName;
                context.boundaryAncestor = currentAncestor;
                context.fileInfo = mPreprocessor.findFileInfo(context.fileName).get();
                context.inProject = mProjectFiles.contains(context.fileName);
                context.inSystemHeader = !currentAncestor.isEmpty()
                        || ::isSystemHeaderFile(context.fileName, mPreprocessor.includePaths());
                stepTimer.start();
                builder.build(declaration, context);
                buildStatementsMs += stepTimer.restart();
                continue;
            }
        }
        cursor.advance();
    }
    declarationFileTimes[currentFile] += declarationFileTimer.restart();
    QList<QPair<QString, qint64>> sortedTimes;
    for (auto it = declarationFileTimes.constBegin(); it != declarationFileTimes.constEnd(); ++it)
        sortedTimes.append(qMakePair(it.key(), it.value()));
    std::sort(sortedTimes.begin(), sortedTimes.end(), [](const auto &left, const auto &right) {
        return left.second > right.second;
    });
    for (int i = 0; i < qMin(10, sortedTimes.count()); ++i)
        qWarning() << "PARSE_FILE_TIMING" << sortedTimes[i].first << sortedTimes[i].second;
    stepTimer.start();
    mPreprocessor.clearTempResults();
    qint64 clearTempResultsMs = stepTimer.restart();
    qWarning() << "PARSE_TIMING" << fileName
               << "preprocess" << preprocessMs
               << "tokenize" << tokenizeMs
               << "parseDeclarations" << parseDeclarationsMs
               << "buildStatements" << buildStatementsMs
               << "clearTempResults" << clearTempResultsMs
               << "declarations" << timer.restart();
}

void CppParser::internalInvalidateFile(const QString &fileName)
{
    if (fileName.isEmpty())
        return;
    StatementList statements = mStatementList.statements();
    for (const PStatement &statement : statements) {
        if (!mStatementList.isInFile(statement, fileName))
            continue;
        bool definitionMatches = statementHasDefinition(statement)
                && (statement->definition.fileName == fileName
                    || statement->definition.boundaryAncestor == fileName);
        for (const StatementOccurrence &occurrence : statement->declarations) {
            QString owner = occurrence.boundaryAncestor.isEmpty()
                    ? occurrence.fileName : occurrence.boundaryAncestor;
            if (occurrence.fileName != fileName && owner != fileName)
                continue;
            PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(occurrence.fileName);
            if (fileInfo)
                fileInfo->removeStatement(statement);
        }
        if (statement->kind == StatementKind::Namespace) {
            mStatementList.removeNamespaceDeclarations(statement, fileName);
            if (statement->declarations.isEmpty())
                mStatementList.deleteStatement(statement);
            continue;
        }

        if (definitionMatches) {
            PParsedFileInfo definitionInfo = mPreprocessor.findFileInfo(
                        statement->definition.fileName);
            if (definitionInfo)
                definitionInfo->removeStatement(statement);
            statement->definition = StatementOccurrence{};
            setStatementHasDefinition(statement, false);
        }
        mStatementList.removeDeclarations(statement, fileName);
        if (statement->declarations.isEmpty() && !statementHasDefinition(statement))
            mStatementList.deleteStatement(statement);
    }
    mPreprocessor.removeFileInfo(fileName);
    mPreprocessor.removeReplayRecord(fileName);
}

void CppParser::internalInvalidateFiles(const QSet<QString> &files)
{
    for (const QString &file : files)
        internalInvalidateFile(file);
}

QSet<QString> CppParser::calculateFilesToBeReparsed(const QString &fileName)
{
    if (fileName.isEmpty())
        return {};
    return mPreprocessor.calculateAffectedFiles(fileName);
}

void CppParser::invalidateFile(const QString &fileName)
{
    if (!mEnabled)
        return;
    {
        QMutexLocker locker(&mMutex);
        if (mParsing || mLockCount > 0)
            return;
        updateSerialId();
        mParsing = true;
    }
    QSet<QString> files = calculateFilesToBeReparsed(fileName);
    internalInvalidateFiles(files);
    {
        QMutexLocker locker(&mMutex);
        mParsing = false;
    }
}

QStringList CppParser::sortFilesByIncludeRelations(const QSet<QString> &files)
{
    QStringList result;
    QSet<QString> savedReplayRoots = mPreprocessor.replayRootFiles();
    for (const QString &file : files) {
        if (mPreprocessor.hasReplayRecord(file))
            continue;
        mPreprocessor.setScanOptions(false, true);
        mPreprocessor.preprocess(file);
        mPreprocessor.clearTempResults();
    }

    QSet<QString> remaining = files;
    while (!remaining.isEmpty()) {
        bool found = false;
        for (const QString &file : remaining) {
            bool hasInclude = false;
            PParsedFileInfo info = mPreprocessor.findFileInfo(file);
            if (info) {
                for (const QString &include : info->includes()) {
                    if (remaining.contains(include)) {
                        hasInclude = true;
                        break;
                    }
                }
            }
            if (!hasInclude) {
                result.push_front(file);
                remaining.remove(file);
                found = true;
                break;
            }
        }
        if (!found) {
            for (const QString &file : remaining) {
                result.push_front(file);
                remaining.remove(file);
            }
        }
    }
    QSet<QString> newReplayRoots = mPreprocessor.replayRootFiles();
    for (const QString &file : newReplayRoots) {
        if (!savedReplayRoots.contains(file))
            mPreprocessor.removeReplayRecord(file);
    }
    return result;
}

bool CppParser::parseFile(const QString &fileName, bool inProject,
                          const QString &contextFilename, bool onlyIfNotParsed, bool updateView)
{
    if (!mEnabled)
        return false;
    {
        {
            QMutexLocker locker(&mMutex);
            if (mParsing) {
                mLastParseFileCommand = std::make_unique<ParseFileCommand>();
                mLastParseFileCommand->fileName = fileName;
                mLastParseFileCommand->inProject = inProject;
                mLastParseFileCommand->contextFilename = contextFilename;
                mLastParseFileCommand->onlyIfNotParsed = onlyIfNotParsed;
                mLastParseFileCommand->updateView = updateView;
                return false;
            }
            if (mLockCount > 0)
                return false;
            mParsing = true;
            updateSerialId();
        }
        if (updateView)
            emit onBusy();
        emit parseStarted();
    }
    auto action = finally([&, this] {
        QMutexLocker locker(&mMutex);
        emit parseFinished(mFilesScannedCount, updateView ? 1 : 0);
        mParsing = false;
    });
    if (onlyIfNotParsed && mPreprocessor.hasReplayRecord(fileName))
        return false;

    if (inProject) {
        QSet<QString> filesToReparse = calculateFilesToBeReparsed(fileName);
        QStringList files = sortFilesByIncludeRelations(filesToReparse);
        internalInvalidateFiles(filesToReparse);
        mFilesToScanCount = files.count();
        mFilesScannedCount = 0;
        for (const QString &file : files) {
            ++mFilesScannedCount;
            emit progress(file, mFilesToScanCount, mFilesScannedCount);
            if (!mPreprocessor.hasReplayRecord(file))
                internalParse(file);
        }
    } else {
        internalInvalidateFile(fileName);
        internalInvalidateFile(contextFilename);
        mFilesToScanCount = 2;
        mFilesScannedCount = 0;
        ++mFilesScannedCount;
        emit progress(fileName, mFilesToScanCount, mFilesScannedCount);
        internalParse(contextFilename);
        if (fileName != contextFilename
                && !mPreprocessor.includesFile(contextFilename, fileName))
            internalParse(fileName);
    }
    return true;
}

void CppParser::parseFileList(bool updateView)
{
    if (!mEnabled)
        return;
    {
        {
            QMutexLocker locker(&mMutex);
            if (mParsing || mLockCount > 0)
                return;
            updateSerialId();
            mParsing = true;
        }
        if (updateView)
            emit onBusy();
        emit parseStarted();
    }
    auto action = finally([&, this] {
        {
            QMutexLocker locker(&mMutex);
            mParsing = false;
        }
        emit parseFinished(mFilesScannedCount, updateView ? 1 : 0);
    });
    mFilesScannedCount = 0;
    mFilesToScanCount = mFilesToScan.count();
    QStringList files = sortFilesByIncludeRelations(mFilesToScan);
    for (const QString &file : files) {
        ++mFilesScannedCount;
        emit progress(file, mFilesToScanCount, mFilesScannedCount);
        if (!mPreprocessor.hasReplayRecord(file))
            internalParse(file);
    }
    mFilesToScan.clear();
}

CppParser::PParseFileCommand CppParser::retrievePendingParseFileCommand()
{
    QMutexLocker locker(&mMutex);
    return std::move(mLastParseFileCommand);
}


QList<PStatement> CppParser::getListOfFunctions(const QString &fileName,
                                                const QString &phrase, int line) const
{
    QMutexLocker locker(&mMutex);
    QList<PStatement> result;
    if (mParsing || phrase.isEmpty())
        return result;
    PStatement function = doFindStatementOf(fileName, phrase, line);
    if (!function)
        return result;

    QList<PStatement> scopes;
    PStatement parent = function->parentScope.lock();
    scopes.append(parent);

    QSet<const AbstractStatement *> seen;
    for (const PStatement &scope : scopes) {
        const auto candidates = mStatementList.childrenStatements(scope).values(function->command);
        for (const std::weak_ptr<AbstractStatement> &weak : candidates) {
            PStatement candidate = weak.lock();
            if (candidate && candidate->kind == StatementKind::InlineAlias)
                candidate = std::dynamic_pointer_cast<InlineAliasStatement>(candidate)->target.lock();
            if (!candidate || seen.contains(candidate.get()))
                continue;
            if (candidate->kind != StatementKind::Function
                    && candidate->kind != StatementKind::Constructor
                    && candidate->kind != StatementKind::Destructor
                    && candidate->kind != StatementKind::OverloadedOperator
                    && candidate->kind != StatementKind::LiteralOperator) {
                continue;
            }
            if (!visibleStatement(fileName, candidate))
                continue;
            seen.insert(candidate.get());
            result.append(candidate);
        }
    }
    return result;
}

PStatement CppParser::findScopeStatement(const QString &filename, int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    PParsedFileInfo info = mPreprocessor.findFileInfo(filename);
    return info ? info->findScopeAtLine(line) : PStatement();
}

PParsedFileInfo CppParser::findFileInfo(const QString &filename) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    return mPreprocessor.findFileInfo(filename);
}

PReplayRecord CppParser::findReplayRecord(const QString &filename) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    return mPreprocessor.findReplayRecord(filename);
}

QString CppParser::findFirstTemplateParamOf(const QString &fileName, const QString &phrase,
                                            const PStatement &currentScope) const
{
    Q_UNUSED(fileName);
    TypeInfo type = CppTypeResolver(mStatementList).resolve(parseTypeInfo(phrase), {}, currentScope);
    return type.templateArguments.isEmpty() ? QString() : type.templateArguments.first().toString();
}

QString CppParser::findTemplateParamOf(const QString &fileName, const QString &phrase, int index,
                                       const PStatement &currentScope) const
{
    Q_UNUSED(fileName);
    TypeInfo type = CppTypeResolver(mStatementList).resolve(parseTypeInfo(phrase), {}, currentScope);
    return index >= 0 && index < type.templateArguments.count()
            ? type.templateArguments[index].toString() : QString();
}

PStatement CppParser::findFunctionAt(const QString &fileName, int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    PParsedFileInfo info = mPreprocessor.findFileInfo(fileName);
    if (!info)
        return {};
    for (const std::weak_ptr<AbstractStatement> &weak : info->statements()) {
        PStatement statement = weak.lock();
        if (!statement || (statement->kind != StatementKind::Function
                           && statement->kind != StatementKind::Constructor
                           && statement->kind != StatementKind::Destructor))
            continue;
        StatementOccurrence declaration = statementDeclarationForNavigation(statement, fileName);
        StatementOccurrence definition = statementDefinitionForNavigation(statement);
        if ((declaration.range.isValid()
             && declaration.range.start.line <= line
             && line <= declaration.range.end.line)
                || (definition.range.isValid()
                    && definition.range.start.line <= line
                    && line <= definition.range.end.line))
            return statement;
    }
    return {};
}

int CppParser::findLastOperator(const QString &phrase) const
{
    for (int i = phrase.length() - 2; i >= 0; --i) {
        if ((phrase[i] == '-' && phrase[i + 1] == '>')
                || (phrase[i] == ':' && phrase[i + 1] == ':'))
            return i;
    }
    return phrase.lastIndexOf('.');
}

PStatementList CppParser::findNamespace(const QString &name, bool includeInline) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return std::make_shared<StatementList>();
    return doFindNamespace(name, includeInline);
}

PStatementList CppParser::doFindNamespace(const QString &name, bool includeInline) const
{
    Q_UNUSED(includeInline);
    auto result = std::make_shared<StatementList>();
    PStatement statement = CppTypeResolver(mStatementList).findStatement(name);
    if (statement && statement->kind == StatementKind::Namespace)
        result->append(statement);
    return result;
}

PStatement CppParser::findStatement(const QString &fullname) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    CppTypeResolver resolver(mStatementList);
    return resolver.findStatement(fullname);
}

PStatement CppParser::findStatementOf(const QString &fileName, const QString &phrase, int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    return doFindStatementOf(fileName, phrase, line);
}

PStatement CppParser::doFindStatementOf(const QString &fileName, const QString &phrase, int line) const
{
    PParsedFileInfo info = mPreprocessor.findFileInfo(fileName);
    PStatement scope = info ? info->findScopeAtLine(line) : PStatement();
    QStringList parts;
    CppTokenizer tokenizer;
    tokenizer.tokenize({phrase});
    for (const CppTokenizer::Token &token : tokenizer.tokens())
        parts.append(token.text);
    return doFindStatementOf(fileName, parts, scope);
}

PStatement CppParser::findStatementOf(const QString &fileName, const QString &phrase,
                                      const PStatement &currentScope,
                                      PStatement &parentScopeType) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    return doFindStatementOf(fileName, phrase, currentScope, parentScopeType);
}

PStatement CppParser::doFindStatementOf(const QString &fileName, const QString &phrase,
                                        const PStatement &currentScope,
                                        PStatement &parentScopeType) const
{
    CppTokenizer tokenizer;
    tokenizer.tokenize({phrase});
    QStringList expression;
    for (const CppTokenizer::Token &token : tokenizer.tokens())
        expression.append(token.text);
    EvalResult result = doEvalExpression(fileName, expression, currentScope);
    parentScopeType = result.type.declaration.lock();
    return visibleStatement(fileName, result.statement ? result.statement : parentScopeType);
}

PStatement CppParser::findStatementOf(const QString &fileName, const QStringList &expression,
                                      const PStatement &currentScope) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    return doFindStatementOf(fileName, expression, currentScope);
}

PStatement CppParser::doFindStatementOf(const QString &fileName, const QStringList &expression,
                                        const PStatement &currentScope) const
{
    QStringList copy = expression;
    EvalResult result = doEvalExpression(fileName, copy, currentScope);
    return visibleStatement(fileName,
                            result.statement ? result.statement : result.type.declaration.lock());
}

PStatement CppParser::findStatementOf(const QString &fileName, const QStringList &expression,
                                      int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    return doFindStatementOf(fileName, expression, line);
}

PStatement CppParser::doFindStatementOf(const QString &fileName, const QStringList &expression,
                                        int line) const
{
    PParsedFileInfo info = mPreprocessor.findFileInfo(fileName);
    PStatement scope = info ? info->findScopeAtLine(line) : PStatement();
    return doFindStatementOf(fileName, expression, scope);
}

PStatement CppParser::findAliasedStatement(const PStatement &statement) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    if (!statement)
        return {};
    if (statement->kind == StatementKind::InlineAlias)
        return std::dynamic_pointer_cast<InlineAliasStatement>(statement)->target.lock();
    if (statement->kind != StatementKind::Alias && statement->kind != StatementKind::Typedef)
        return statement;
    CppTypeResolver resolver(mStatementList);
    if (!std::dynamic_pointer_cast<Statement>(statement)->value.isEmpty()) {
        PStatement target = resolver.findStatement(std::dynamic_pointer_cast<Statement>(statement)->value, statement->parentScope.lock());
        if (target)
            return target;
    }
    return resolver.resolve(std::dynamic_pointer_cast<Statement>(statement)->typeInfo, {}, statement->parentScope.lock()).declaration.lock();
}

QStringList CppParser::getFunctionParameterNames(const PStatement &statement) const
{
    QStringList result;
    if (!statement)
        return result;
    for (const ParameterInfo &parameter : std::dynamic_pointer_cast<Statement>(statement)->parameters) {
        if (!parameter.name.isEmpty())
            result.append(parameter.name);
    }
    return result;
}

QList<PStatement> CppParser::listTypeStatements(const QString &fileName, int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return {};
    return doListStatements(fileName, line, {
        StatementKind::Class, StatementKind::EnumType, StatementKind::EnumClassType,
        StatementKind::Typedef, StatementKind::Alias
    });
}

QList<PStatement> CppParser::listLiteralOperators(const QString &fileName, int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return {};
    return doListStatements(fileName, line, {StatementKind::LiteralOperator});
}

QList<PStatement> CppParser::doListStatements(const QString &fileName, int line,
                                              const QSet<StatementKind> &kinds) const
{
    QList<PStatement> result;
    PParsedFileInfo info = mPreprocessor.findFileInfo(fileName);
    if (!info)
        return result;
    for (const std::weak_ptr<AbstractStatement> &weak : info->statements()) {
        PStatement statement = weak.lock();
        if (!statement || !kinds.contains(statement->kind))
            continue;
        StatementOccurrence declaration = statementDeclarationForNavigation(statement, fileName);
        if (line < 0 || (declaration.range.start.line <= line
                         && line <= declaration.range.end.line))
            result.append(statement);
    }
    return result;
}

EvalResult CppParser::evalExpression(const QString &fileName, QStringList &expression,
                                     const PStatement &currentScope) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return {};
    return doEvalExpression(fileName, expression, currentScope);
}

EvalResult CppParser::doEvalExpression(const QString &fileName, QStringList &expression,
                                       const PStatement &currentScope) const
{
    Q_UNUSED(fileName);
    const bool profile = qEnvironmentVariableIsSet("REDPANDA_PARSER_PROFILE");
    QElapsedTimer timer;
    if (profile)
        timer.start();
    CppTypeResolver::EvaluationContext evaluation;
    evaluation.allowModelChanges = mParsing;
    CppTypeResolver resolver(mStatementList, &evaluation);
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(mStatementList, resolver, operations);
    CppTokenizer tokenizer;
    const QString expressionText = expression.join(' ');
    tokenizer.tokenize({expressionText});
    EvalResult result = evaluator.evaluate(tokenizer, currentScope, &evaluation);
    if (profile)
        qWarning() << "EVAL_PROFILE" << timer.nsecsElapsed() / 1000000.0 << expressionText;
    return result;
}

PStatement CppParser::findTypeDefinitionOf(const QString &fileName, const QString &typeName,
                                           const PStatement &currentClass) const
{
    Q_UNUSED(fileName);
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    CppTypeResolver resolver(mStatementList);
    return resolver.resolve(parseTypeInfo(typeName), {}, currentClass).declaration.lock();
}

PStatement CppParser::findTypeDef(const PStatement &statement, const QString &fileName) const
{
    Q_UNUSED(fileName);
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return nullptr;
    if (!statement)
        return {};
    CppTypeResolver resolver(mStatementList);
    return resolver.resolve(std::dynamic_pointer_cast<Statement>(statement)->typeInfo, {}, statement->parentScope.lock()).declaration.lock();
}

bool CppParser::freeze()
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return false;
    ++mLockCount;
    return true;
}

bool CppParser::freeze(const QString &serialId)
{
    QMutexLocker locker(&mMutex);
    if (mParsing || mSerialId != serialId)
        return false;
    ++mLockCount;
    return true;
}

QStringList CppParser::getClassesList() const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return {};
    QSet<QString> names;
    for (const PStatement &statement : mStatementList.statements()) {
        if (statement->kind == StatementKind::Class)
            names.insert(statement->fullName);
    }
    return names.values();
}

QStringList CppParser::getFileDirectIncludes(const QString &filename) const
{
    QMutexLocker locker(&mMutex);
    PParsedFileInfo info = mPreprocessor.findFileInfo(filename);
    return info ? info->directIncludes() : QStringList();
}

QSet<QString> CppParser::getIncludedFiles(const QString &filename) const
{
    QMutexLocker locker(&mMutex);
    PParsedFileInfo info = mPreprocessor.findFileInfo(filename);
    return info ? info->includes() : QSet<QString>();
}

QSet<QString> CppParser::getFileUsings(const QString &filename) const
{
    QMutexLocker locker(&mMutex);
    PParsedFileInfo info = mPreprocessor.findFileInfo(filename);
    return info ? info->usings() : QSet<QString>();
}

QString CppParser::getHeaderFileName(const QString &relativeTo, const QString &headerName,
                                     bool fromNext) const
{
    QMutexLocker locker(&mMutex);
    QString currentDir = extractFileDir(relativeTo);
    QStringList includes;
    QStringList projectIncludes;
    bool found = false;
    if (fromNext && mPreprocessor.includePaths().contains(currentDir)) {
        for (const QString &path : mPreprocessor.includePathList()) {
            if (found)
                includes.append(path);
            else if (path == currentDir)
                found = true;
        }
        projectIncludes = mPreprocessor.projectIncludePathList();
    } else if (fromNext && mPreprocessor.projectIncludePaths().contains(currentDir)) {
        includes = mPreprocessor.includePathList();
        for (const QString &path : mPreprocessor.projectIncludePathList()) {
            if (found)
                includes.append(path);
            else if (path == currentDir)
                found = true;
        }
    } else {
        includes = mPreprocessor.includePathList();
        projectIncludes = mPreprocessor.projectIncludePathList();
    }
    return ::getHeaderFilename(relativeTo, headerName, includes, projectIncludes);
}

bool CppParser::isLineVisible(const QString &fileName, int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return true;
    PParsedFileInfo info = mPreprocessor.findFileInfo(fileName);
    return !info || info->isLineVisible(line);
}

bool CppParser::isIncludeLine(const QString &line) const
{
    QString trimmed = line.trimmed();
    return trimmed.startsWith('#') && trimmed.mid(1).trimmed().startsWith("include");
}

bool CppParser::isIncludeNextLine(const QString &line) const
{
    QString trimmed = line.trimmed();
    return trimmed.startsWith('#') && trimmed.mid(1).trimmed().startsWith("include_next");
}

bool CppParser::isProjectHeaderFile(const QString &fileName) const
{
    return isHFile(fileName) && mProjectFiles.contains(fileName);
}

bool CppParser::isSystemHeaderFile(const QString &fileName) const
{
    QMutexLocker locker(&mMutex);
    return ::isSystemHeaderFile(fileName, mPreprocessor.includePaths());
}

void CppParser::parseHardDefines()
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return;
    for (const PDefine &define : mPreprocessor.hardDefines()) {
        auto statement = StatementFactory::createPreprocessor(PStatement(), define->name, define->name);
        std::dynamic_pointer_cast<Statement>(statement)->args = define->args;
        std::dynamic_pointer_cast<Statement>(statement)->value = define->value;
        setStatementHasDefinition(statement, true);
        mStatementList.add(statement);
    }
}

bool CppParser::parsing() const
{
    QMutexLocker locker(&mMutex);
    return mParsing;
}

int CppParser::cppVersion() const
{
    QMutexLocker locker(&mMutex);
    return mPreprocessor.cppVersion();
}

void CppParser::resetParser()
{
    while (true) {
        {
            QMutexLocker locker(&mMutex);
            if (!mParsing && mLockCount == 0) {
                mParsing = true;
                break;
            }
            mPreprocessor.stopForParserReset();
            mStopForReset = true;
        }
        QThread::msleep(50);
        QCoreApplication *app = QApplication::instance();
        if (app)
            app->processEvents();
    }
    auto action = finally([this] {
        QMutexLocker locker(&mMutex);
        mParsing = false;
        mStopForReset = false;
    });
    emit onBusy();
    mUniqId = 0;
    mIsSystemHeader = false;
    mFilesScannedCount = 0;
    mFilesToScanCount = 0;
    mStatementList.clear();
    mProjectFiles.clear();
    mFilesToScan.clear();
    mPreprocessor.clear();
}

void CppParser::unFreeze()
{
    QMutexLocker locker(&mMutex);
    if (mLockCount > 0)
        --mLockCount;
}

bool CppParser::hasReplayRecord(const QString &fileName) const
{
    QMutexLocker locker(&mMutex);
    return !mParsing && mPreprocessor.hasReplayRecord(fileName);
}

bool CppParser::isFileParsedInContext(const QString &fileName, const QString &contextFileName) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing || contextFileName.isEmpty()
            || !mPreprocessor.hasReplayRecord(contextFileName))
        return false;
    return fileName == contextFileName
            || mPreprocessor.includesFile(contextFileName, fileName);
}


QString CppParser::getScopePrefix(const PStatement &statement) const
{
    switch (statement->accessibility) {
    case StatementAccessibility::Public:
        return "public";
    case StatementAccessibility::Private:
        return "private";
    case StatementAccessibility::Protected:
        return "protected";
    default:
        return "";
    }
}

QString CppParser::prettyPrintStatement(const PStatement &statement, const QString &filename,
                                        int line) const
{
    if (!statement)
        return {};
    QString result;
    switch (statement->kind) {
    case StatementKind::Preprocessor:
        if (statement->command == "__FILE__")
            result = '"' + filename + '"';
        else if (statement->command == "__LINE__")
            result = QString("\"%1\"").arg(line);
        else if (statement->command == "__DATE__")
            result = QString("\"%1\"").arg(QDate::currentDate().toString(Qt::ISODate));
        else if (statement->command == "__TIME__")
            result = QString("\"%1\"").arg(QTime::currentTime().toString(Qt::ISODate));
        else {
            result = "#define";
            if (!statement->command.isEmpty())
                result += ' ' + statement->command;
            if (!std::dynamic_pointer_cast<Statement>(statement)->args.isEmpty())
                result += ' ' + std::dynamic_pointer_cast<Statement>(statement)->args;
            if (!std::dynamic_pointer_cast<Statement>(statement)->value.isEmpty())
                result += ' ' + std::dynamic_pointer_cast<Statement>(statement)->value;
        }
        break;
    case StatementKind::EnumClassType:
        result = "enum class " + statement->command;
        break;
    case StatementKind::EnumType:
        result = "enum " + statement->command;
        break;
    case StatementKind::Enum:
        if (!statementType(statement).isEmpty())
            result = statementType(statement) + "::";
        result += statement->command;
        if (!std::dynamic_pointer_cast<Statement>(statement)->value.isEmpty())
            result += "(" + std::dynamic_pointer_cast<Statement>(statement)->value + ")";
        break;
    case StatementKind::Typedef:
        result = "typedef " + statementType(statement) + " " + statement->command;
        if (!std::dynamic_pointer_cast<Statement>(statement)->args.isEmpty())
            result += " " + std::dynamic_pointer_cast<Statement>(statement)->args;
        break;
    case StatementKind::Alias:
        result = "using " + statementType(statement);
        break;
    case StatementKind::InlineAlias:
        result = statement->command;
        break;
    case StatementKind::Function:
    case StatementKind::Variable:
    case StatementKind::Parameter:
    case StatementKind::Class:
        if (statement->scope != StatementScope::Local)
            result = getScopePrefix(statement) + ' ';
        result += statementType(statement) + ' ';
        if (statement->kind == StatementKind::Variable && std::dynamic_pointer_cast<Statement>(statement)->args.startsWith("("))
            result += "(*" + statement->fullName + ")";
        else
            result += statement->fullName;
        result += std::dynamic_pointer_cast<Statement>(statement)->args;
        break;
    case StatementKind::OverloadedOperator:
        if (statement->scope != StatementScope::Local)
            result = getScopePrefix(statement) + ' ';
        result += statementType(statement) + " operator " + statement->fullName + std::dynamic_pointer_cast<Statement>(statement)->args;
        break;
    case StatementKind::LiteralOperator:
        if (statement->scope != StatementScope::Local)
            result = getScopePrefix(statement) + ' ';
        result += statementType(statement) + " operator \"\"" + statement->fullName + std::dynamic_pointer_cast<Statement>(statement)->args;
        break;
    case StatementKind::Namespace:
        result = statement->fullName;
        break;
    case StatementKind::Constructor:
        result = getScopePrefix(statement) + QObject::tr("constructor") + ' '
                + statementType(statement) + ' ' + statement->fullName + std::dynamic_pointer_cast<Statement>(statement)->args;
        break;
    case StatementKind::Destructor:
        result = getScopePrefix(statement) + QObject::tr("destructor") + ' '
                + statementType(statement) + ' ' + statement->fullName + std::dynamic_pointer_cast<Statement>(statement)->args;
        break;
    default:
        break;
    }
    return result;
}

PStatement CppParser::findStatementStartingFrom(const QString &fileName, const QString &phrase,
                                                const PStatement &startScope) const
{
    Q_UNUSED(fileName);
    CppTypeResolver resolver(mStatementList);
    return resolver.findStatement(phrase, startScope);
}

QList<PStatement> CppParser::membersOfStatement(const PStatement &scopeStatement) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return {};
    return doMembersOfStatement(scopeStatement);
}

QList<PStatement> CppParser::doMembersOfStatement(const PStatement &scopeStatement) const
{
    if (scopeStatement && scopeStatement->kind == StatementKind::InlineAlias)
        return {};
    CppTypeResolver resolver(mStatementList);
    return resolver.membersOf(scopeStatement);
}

StatementAccessibility CppParser::memberAccessibility(
        const PStatement &scopeStatement, const PStatement &member) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return StatementAccessibility::None;
    CppTypeResolver resolver(mStatementList);
    return resolver.memberAccessibility(scopeStatement, member);
}

PStatement CppParser::findMemberOfStatement(const QString &phrase,
                                            const PStatement &scopeStatement) const
{
    for (const PStatement &statement : doMembersOfStatement(scopeStatement)) {
        if (statement->command == phrase)
            return statement;
    }
    return {};
}

QList<PStatement> CppParser::findMembersOfStatement(const QString &phrase,
                                                    const PStatement &scopeStatement) const
{
    QList<PStatement> result;
    for (const PStatement &statement : doMembersOfStatement(scopeStatement)) {
        if (statement->command == phrase)
            result.append(statement);
    }
    return result;
}

PStatement CppParser::findStatementInScope(const QString &name, const PStatement &scope) const
{
    PStatement current = scope;
    while (current) {
        if (PStatement result = findMemberOfStatement(name, current))
            return result;
        current = current->parentScope.lock();
    }
    const auto values = mStatementList.childrenStatements().values(name);
    for (const std::weak_ptr<AbstractStatement> &weak : values) {
        if (PStatement result = weak.lock())
            return result;
    }
    return {};
}

PStatement CppParser::findStatementInNamespace(const QString &name,
                                               const QString &namespaceName) const
{
    PStatementList list = doFindNamespace(namespaceName);
    for (const PStatement &space : *list) {
        if (PStatement result = findMemberOfStatement(name, space))
            return result;
    }
    return {};
}

PStatement CppParser::visibleStatement(const QString &fileName,
                                       const PStatement &statement) const
{
    if (!statement)
        return {};
    PParsedFileInfo info = mPreprocessor.findFileInfo(fileName);
    return mStatementList.visibleFrom(statement, fileName, info) ? statement : PStatement();
}

TypeInfo CppParser::parseTypeInfo(const QString &text) const
{
    if (text.trimmed().isEmpty())
        return {};
    CppTokenizer tokenizer;
    tokenizer.tokenize({text + " __value;"});
    CppTokenCursor cursor(tokenizer);
    CppDeclarationParser parser(cursor);
    DeclarationInfo declaration;
    if (!parser.parseSimpleDeclaration(declaration) || declaration.declarators.isEmpty())
        return {};
    TypeInfo result = declaration.specifiers.type;
    result.derivations += declaration.declarators.first().derivations;
    return result;
}

PStatement CppParser::findMacro(const QString &phrase, const QString &fileName) const
{
    Q_UNUSED(fileName);
    const auto values = mStatementList.childrenStatements().values(phrase);
    for (const std::weak_ptr<AbstractStatement> &weak : values) {
        PStatement statement = weak.lock();
        if (statement && statement->kind == StatementKind::Preprocessor)
            return statement;
    }
    return {};
}

bool CppParser::enabled() const
{
    return mEnabled;
}

void CppParser::setEnabled(bool newEnabled)
{
    if (mEnabled != newEnabled) {
        mEnabled = newEnabled;
        if (!mEnabled)
            resetParser();
    }
}

const QSet<QString> &CppParser::filesToScan() const
{
    return mFilesToScan;
}

void CppParser::setFilesToScan(const QSet<QString> &newFilesToScan)
{
    mFilesToScan = newFilesToScan;
}

void CppParser::setOnGetFileStream(const GetFileStreamFunc &newOnGetFileStream)
{
    mPreprocessor.setOnGetFileStream(newOnGetFileStream);
}

int CppParser::parserId() const
{
    return mParserId;
}

const QString &CppParser::serialId() const
{
    return mSerialId;
}

const QSet<QString> &CppParser::includePaths()
{
    return mPreprocessor.includePaths();
}

const QSet<QString> &CppParser::projectIncludePaths()
{
    return mPreprocessor.projectIncludePaths();
}

const StatementModel &CppParser::statementList() const
{
    return mStatementList;
}

ParserLanguage CppParser::language() const
{
    return mLanguage;
}

void CppParser::setLanguage(ParserLanguage newLanguage)
{
    mLanguage = newLanguage;
}

const QSet<QString> &CppParser::projectFiles() const
{
    return mProjectFiles;
}

QList<QString> CppParser::namespaces()
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return {};
    QSet<QString> result;
    for (const PStatement &statement : mStatementList.statements()) {
        if (statement->kind == StatementKind::Namespace)
            result.insert(statement->fullName);
    }
    return result.values();
}

bool CppParser::sharedByFiles() const
{
    return mSharedByFiles;
}

void CppParser::setSharedByFiles(bool newSharedByFiles)
{
    mSharedByFiles = newSharedByFiles;
}

void CppParser::updateSerialId()
{
    mSerialId = QString("%1:%2").arg(mParserId).arg(++mSerialCount);
}

void CppParser::internalClear()
{
}

void CppParser::parseFileBlocking(PCppParser parser, const QString &fileName, bool inProject,
                                  const QString &contextFilename, bool onlyIfNotParsed,
                                  bool updateView)
{
    if (!parser || !parser->enabled())
        return;
    while (parser->parsing()) {
        QThread::msleep(100);
        QCoreApplication *app = QCoreApplication::instance();
        if (app)
            app->processEvents();
    }
    parser->parseFile(fileName, inProject, contextFilename, onlyIfNotParsed, updateView);
}

void CppParser::parseFileNonBlocking(PCppParser parser, const QString &fileName, bool inProject,
                                     const QString &contextFilename, bool onlyIfNotParsed,
                                     bool updateView)
{
    if (!parser || !parser->enabled())
        return;
    CppFileParserThread *thread = new CppFileParserThread(
                parser, fileName, inProject, contextFilename, onlyIfNotParsed, updateView);
    thread->start();
}

void CppParser::parseFileListNonBlocking(PCppParser parser, bool updateView)
{
    if (!parser || !parser->enabled())
        return;
    CppFileListParserThread *thread = new CppFileListParserThread(parser, updateView);
    thread->start();
}

CppFileParserThread::CppFileParserThread(PCppParser parser, const QString &fileName, bool inProject,
                                         const QString &contextFilename, bool onlyIfNotParsed,
                                         bool updateView, QObject *parent)
    : QThread(parent), mParser(parser), mFileName(fileName), mInProject(inProject),
      mContextFilename(contextFilename), mOnlyIfNotParsed(onlyIfNotParsed), mUpdateView(updateView)
{
    connect(this, &QThread::finished, this, &QObject::deleteLater);
}

CppFileParserThread::~CppFileParserThread() = default;

void CppFileParserThread::run()
{
    if (!mParser)
        return;
    if (mParser->parseFile(mFileName, mInProject, mContextFilename,
                           mOnlyIfNotParsed, mUpdateView)) {
        CppParser::PParseFileCommand command;
        while ((command = mParser->retrievePendingParseFileCommand()) != nullptr) {
            if (!mParser->parseFile(command->fileName, command->inProject,
                                    command->contextFilename, command->onlyIfNotParsed,
                                    command->updateView))
                break;
        }
    }
}

CppFileListParserThread::CppFileListParserThread(PCppParser parser, bool updateView,
                                                 QObject *parent)
    : QThread(parent), mParser(parser), mUpdateView(updateView)
{
    connect(this, &QThread::finished, this, &QObject::deleteLater);
}

void CppFileListParserThread::run()
{
    if (mParser && !mParser->parsing())
        mParser->parseFileList(mUpdateView);
}
