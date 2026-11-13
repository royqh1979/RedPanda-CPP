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
#include <QDate>
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
    if (needScan && !mPreprocessor.fileScanned(fileName))
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
    mPreprocessor.setScanOptions(true, true);
    mPreprocessor.preprocess(fileName);

    CppTokenizer tokenizer;
    tokenizer.tokenize(mPreprocessor.result());
#ifdef REDPANDA_PARSER_TEST
    mPreprocessedForTest = mPreprocessor.result();
    mTokenizerTokensForTest = tokenizer.tokens();
#endif
    CppTokenCursor cursor(tokenizer);
    CppStatementBuilder builder(mStatementList);
    const QHash<QString, QStringList> &ancestors = mPreprocessor.boundaryAncestors();
    QHash<QString, int> occurrences;
    QString currentFile;
    QString currentAncestor;
    while (!cursor.atEnd()) {
        if (mStopForReset)
            break;
        const CppTokenizer::Token &token = cursor.current();
        if (token.kind == CppTokenizer::TokenKind::FileBoundary) {
            currentFile = token.fileName;
            int occurrence = occurrences.value(currentFile, 0);
            occurrences[currentFile] = occurrence + 1;
            currentAncestor = ancestors.value(currentFile).value(occurrence);
            cursor.advance();
            continue;
        }
        if (token.kind == CppTokenizer::TokenKind::Preprocessor) {
            cursor.advance();
            continue;
        }
        DeclarationInfo declaration;
        if (!mStopForReset) {
            CppSyntaxParser parser(cursor);
            if (parser.parseDeclaration(declaration)) {
                StatementBuildContext context;
                context.fileName = declaration.fileName.isEmpty() ? currentFile : declaration.fileName;
                context.boundaryAncestor = currentAncestor;
                context.fileInfo = mPreprocessor.findFileInfo(context.fileName).get();
                context.inProject = mProjectFiles.contains(context.fileName);
                context.inSystemHeader = !currentAncestor.isEmpty()
                        || ::isSystemHeaderFile(context.fileName, mPreprocessor.includePaths());
                builder.build(declaration, context);
                continue;
            }
        }
        cursor.advance();
    }
    mPreprocessor.clearTempResults();
}

void CppParser::internalInvalidateFile(const QString &fileName)
{
    if (fileName.isEmpty())
        return;
    StatementList statements = mStatementList.statements();
    for (const PStatement &statement : statements) {
        bool declarationMatches = statement->declarationBoundaryAncestor == fileName
                || (statement->declarationBoundaryAncestor.isEmpty()
                    && statement->fileName == fileName);
        bool definitionMatches = statement->hasDefinition()
                && (statement->definitionBoundaryAncestor == fileName
                    || (statement->definitionBoundaryAncestor.isEmpty()
                        && statement->definitionFileName == fileName));
        if (!declarationMatches && !definitionMatches)
            continue;

        PParsedFileInfo declarationInfo = mPreprocessor.findFileInfo(
                    statement->declarationBoundaryAncestor.isEmpty()
                    ? statement->fileName : statement->declarationBoundaryAncestor);
        if (declarationInfo)
            declarationInfo->removeStatement(statement);
        if (statement->hasDefinition()) {
            PParsedFileInfo definitionInfo = mPreprocessor.findFileInfo(
                        statement->definitionBoundaryAncestor.isEmpty()
                        ? statement->definitionFileName : statement->definitionBoundaryAncestor);
            if (definitionInfo)
                definitionInfo->removeStatement(statement);
        }

        if (definitionMatches) {
            statement->setHasDefinition(false);
            statement->definitionRange = statement->declarationRange;
            statement->definitionFileName = statement->fileName;
            statement->definitionBoundaryAncestor.clear();
        }
        if (!declarationMatches)
            continue;
        if (statement->hasDefinition()) {
            statement->fileName = statement->definitionFileName;
            statement->declarationRange = statement->definitionRange;
            statement->declarationBoundaryAncestor = statement->definitionBoundaryAncestor;
            PParsedFileInfo newOwner = mPreprocessor.findFileInfo(
                        statement->declarationBoundaryAncestor.isEmpty()
                        ? statement->fileName : statement->declarationBoundaryAncestor);
            if (newOwner)
                newOwner->addStatement(statement);
            continue;
        }
        mStatementList.deleteStatement(statement);
    }
    mPreprocessor.removeFileInfo(fileName);
    mPreprocessor.removeScannedFile(fileName);
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
    mParsing = false;
}

QStringList CppParser::sortFilesByIncludeRelations(const QSet<QString> &files)
{
    QStringList result;
    QSet<QString> savedScannedFiles = mPreprocessor.scannedFiles();
    for (const QString &file : files) {
        if (mPreprocessor.fileScanned(file))
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
    QSet<QString> newScannedFiles = mPreprocessor.scannedFiles();
    for (const QString &file : newScannedFiles) {
        if (!savedScannedFiles.contains(file))
            mPreprocessor.removeScannedFile(file);
    }
    return result;
}

bool CppParser::parseFile(const QString &fileName, bool inProject,
                          const QString &contextFilename, bool onlyIfNotParsed, bool updateView)
{
    if (!mEnabled)
        return false;
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
        if (updateView)
            emit onBusy();
        emit parseStarted();
    }
    auto action = finally([&, this] {
        QMutexLocker locker(&mMutex);
        emit parseFinished(mFilesScannedCount, updateView ? 1 : 0);
        mParsing = false;
    });
    if (onlyIfNotParsed && mPreprocessor.fileScanned(fileName))
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
            if (!mPreprocessor.fileScanned(file))
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
        if (!mPreprocessor.fileScanned(fileName))
            internalParse(fileName);
    }
    return true;
}

void CppParser::parseFileList(bool updateView)
{
    if (!mEnabled)
        return;
    {
        QMutexLocker locker(&mMutex);
        if (mParsing || mLockCount > 0)
            return;
        updateSerialId();
        mParsing = true;
        if (updateView)
            emit onBusy();
        emit parseStarted();
    }
    auto action = finally([&, this] {
        mParsing = false;
        emit parseFinished(mFilesScannedCount, updateView ? 1 : 0);
    });
    mFilesScannedCount = 0;
    mFilesToScanCount = mFilesToScan.count();
    QStringList files = sortFilesByIncludeRelations(mFilesToScan);
    for (const QString &file : files) {
        ++mFilesScannedCount;
        emit progress(file, mFilesToScanCount, mFilesScannedCount);
        if (!mPreprocessor.fileScanned(file))
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
    PParsedFileInfo info = mPreprocessor.findFileInfo(fileName);
    if (!info)
        return result;
    for (const std::weak_ptr<Statement> &weak : info->statements()) {
        PStatement statement = weak.lock();
        if (!statement || statement->command != phrase)
            continue;
        if (statement->kind != StatementKind::Function
                && statement->kind != StatementKind::Constructor
                && statement->kind != StatementKind::Destructor
                && statement->kind != StatementKind::OverloadedOperator
                && statement->kind != StatementKind::LiteralOperator)
            continue;
        bool declarationMatches = statement->declarationRange.isValid()
                && statement->declarationRange.start.line <= line
                && line <= statement->declarationRange.end.line;
        bool definitionMatches = statement->hasDefinition()
                && statement->definitionRange.start.line <= line
                && line <= statement->definitionRange.end.line;
        if (declarationMatches || definitionMatches)
            result.append(statement);
    }
    return result;
}

PStatement CppParser::findScopeStatement(const QString &filename, int line) const
{
    QMutexLocker locker(&mMutex);
    PParsedFileInfo info = mPreprocessor.findFileInfo(filename);
    return info ? info->findScopeAtLine(line) : PStatement();
}

PParsedFileInfo CppParser::findFileInfo(const QString &filename) const
{
    QMutexLocker locker(&mMutex);
    return mPreprocessor.findFileInfo(filename);
}

QString CppParser::findFirstTemplateParamOf(const QString &fileName, const QString &phrase,
                                            const PStatement &currentScope) const
{
    Q_UNUSED(fileName);
    Q_UNUSED(currentScope);
    TypeInfo type = parseTypeInfo(phrase);
    return type.templateArguments.isEmpty() ? QString() : type.templateArguments.first().toString();
}

QString CppParser::findTemplateParamOf(const QString &fileName, const QString &phrase, int index,
                                       const PStatement &currentScope) const
{
    Q_UNUSED(fileName);
    Q_UNUSED(currentScope);
    TypeInfo type = parseTypeInfo(phrase);
    return index >= 0 && index < type.templateArguments.count()
            ? type.templateArguments[index].toString() : QString();
}

PStatement CppParser::findFunctionAt(const QString &fileName, int line) const
{
    QMutexLocker locker(&mMutex);
    PParsedFileInfo info = mPreprocessor.findFileInfo(fileName);
    if (!info)
        return {};
    for (const std::weak_ptr<Statement> &weak : info->statements()) {
        PStatement statement = weak.lock();
        if (!statement || (statement->kind != StatementKind::Function
                           && statement->kind != StatementKind::Constructor
                           && statement->kind != StatementKind::Destructor))
            continue;
        if ((statement->declarationRange.isValid()
             && statement->declarationRange.start.line <= line
             && line <= statement->declarationRange.end.line)
                || (statement->hasDefinition()
                    && statement->definitionRange.start.line <= line
                    && line <= statement->definitionRange.end.line))
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

PStatementList CppParser::findNamespace(const QString &name) const
{
    QMutexLocker locker(&mMutex);
    auto result = std::make_shared<StatementList>();
    for (const PStatement &statement : mStatementList.statements()) {
        if (statement->kind == StatementKind::Namespace && statement->fullName == name)
            result->append(statement);
    }
    return result;
}

PStatement CppParser::findStatement(const QString &fullname) const
{
    QMutexLocker locker(&mMutex);
    CppTypeResolver resolver(mStatementList);
    return resolver.findStatement(fullname);
}

PStatement CppParser::findStatementOf(const QString &fileName, const QString &phrase, int line) const
{
    PStatement scope = findScopeStatement(fileName, line);
    QStringList parts;
    CppTokenizer tokenizer;
    tokenizer.tokenize({phrase});
    for (const CppTokenizer::Token &token : tokenizer.tokens())
        parts.append(token.text);
    return findStatementOf(fileName, parts, scope);
}

PStatement CppParser::findStatementOf(const QString &fileName, const QString &phrase,
                                      const PStatement &currentScope,
                                      PStatement &parentScopeType) const
{
    CppTokenizer tokenizer;
    tokenizer.tokenize({phrase});
    QStringList expression;
    for (const CppTokenizer::Token &token : tokenizer.tokens())
        expression.append(token.text);
    EvalResult result = evalExpression(fileName, expression, currentScope);
    parentScopeType = result.type.declaration.lock();
    return visibleStatement(fileName, result.statement ? result.statement : parentScopeType);
}

PStatement CppParser::findStatementOf(const QString &fileName, const QStringList &expression,
                                      const PStatement &currentScope) const
{
    QStringList copy = expression;
    EvalResult result = evalExpression(fileName, copy, currentScope);
    return visibleStatement(fileName,
                            result.statement ? result.statement : result.type.declaration.lock());
}

PStatement CppParser::findStatementOf(const QString &fileName, const QStringList &expression,
                                      int line) const
{
    return findStatementOf(fileName, expression, findScopeStatement(fileName, line));
}

PStatement CppParser::findAliasedStatement(const PStatement &statement) const
{
    if (!statement)
        return {};
    if (statement->kind != StatementKind::Alias && statement->kind != StatementKind::Typedef)
        return statement;
    CppTypeResolver resolver(mStatementList);
    if (!statement->value.isEmpty()) {
        PStatement target = resolver.findStatement(statement->value, statement->parentScope.lock());
        if (target)
            return target;
    }
    return resolver.resolve(statement->typeInfo, {}, statement->parentScope.lock()).declaration.lock();
}

QStringList CppParser::getFunctionParameterNames(const PStatement &statement) const
{
    QStringList result;
    if (!statement)
        return result;
    for (const ParameterInfo &parameter : statement->parameters) {
        if (!parameter.name.isEmpty())
            result.append(parameter.name);
    }
    return result;
}

QList<PStatement> CppParser::listTypeStatements(const QString &fileName, int line) const
{
    return doListStatements(fileName, line, {
        StatementKind::Class, StatementKind::EnumType, StatementKind::EnumClassType,
        StatementKind::Typedef, StatementKind::Alias
    });
}

QList<PStatement> CppParser::listLiteralOperators(const QString &fileName, int line) const
{
    return doListStatements(fileName, line, {StatementKind::LiteralOperator});
}

QList<PStatement> CppParser::doListStatements(const QString &fileName, int line,
                                              const QSet<StatementKind> &kinds) const
{
    QMutexLocker locker(&mMutex);
    QList<PStatement> result;
    PParsedFileInfo info = mPreprocessor.findFileInfo(fileName);
    if (!info)
        return result;
    for (const std::weak_ptr<Statement> &weak : info->statements()) {
        PStatement statement = weak.lock();
        if (!statement || !kinds.contains(statement->kind))
            continue;
        if (line < 0 || (statement->declarationRange.start.line <= line
                         && line <= statement->declarationRange.end.line))
            result.append(statement);
    }
    return result;
}

EvalResult CppParser::evalExpression(const QString &fileName, QStringList &expression,
                                     const PStatement &currentScope) const
{
    Q_UNUSED(fileName);
    QMutexLocker locker(&mMutex);
    CppTypeResolver resolver(mStatementList);
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(mStatementList, resolver, operations);
    CppTokenizer tokenizer;
    tokenizer.tokenize({expression.join(' ')});
    return evaluator.evaluate(tokenizer, currentScope);
}

PStatement CppParser::findTypeDefinitionOf(const QString &fileName, const QString &typeName,
                                           const PStatement &currentClass) const
{
    Q_UNUSED(fileName);
    CppTypeResolver resolver(mStatementList);
    return resolver.resolve(parseTypeInfo(typeName), {}, currentClass).declaration.lock();
}

PStatement CppParser::findTypeDef(const PStatement &statement, const QString &fileName) const
{
    Q_UNUSED(fileName);
    if (!statement)
        return {};
    CppTypeResolver resolver(mStatementList);
    return resolver.resolve(statement->typeInfo, {}, statement->parentScope.lock()).declaration.lock();
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
        auto statement = std::make_shared<Statement>();
        statement->command = define->name;
        statement->fullName = define->name;
        statement->kind = StatementKind::Preprocessor;
        statement->scope = StatementScope::Global;
        statement->accessibility = StatementAccessibility::None;
        statement->args = define->args;
        statement->value = define->value;
        statement->declarationRange = SourceRange{{-1, -1}, {-1, -1}};
        statement->setHasDefinition(true);
        mStatementList.add(statement);
    }
}

bool CppParser::parsing() const
{
    return mParsing;
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

bool CppParser::fileScanned(const QString &fileName) const
{
    QMutexLocker locker(&mMutex);
    return !mParsing && mPreprocessor.fileScanned(fileName);
}

bool CppParser::isFileParsed(const QString &filename) const
{
    return mPreprocessor.fileScanned(filename);
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
            if (!statement->args.isEmpty())
                result += ' ' + statement->args;
            if (!statement->value.isEmpty())
                result += ' ' + statement->value;
        }
        break;
    case StatementKind::EnumClassType:
        result = "enum class " + statement->command;
        break;
    case StatementKind::EnumType:
        result = "enum " + statement->command;
        break;
    case StatementKind::Enum:
        if (!statement->type().isEmpty())
            result = statement->type() + "::";
        result += statement->command;
        if (!statement->value.isEmpty())
            result += "(" + statement->value + ")";
        break;
    case StatementKind::Typedef:
        result = "typedef " + statement->type() + " " + statement->command;
        if (!statement->args.isEmpty())
            result += " " + statement->args;
        break;
    case StatementKind::Alias:
        result = "using " + statement->type();
        break;
    case StatementKind::Function:
    case StatementKind::Variable:
    case StatementKind::Parameter:
    case StatementKind::Class:
        if (statement->scope != StatementScope::Local)
            result = getScopePrefix(statement) + ' ';
        result += statement->type() + ' ';
        if (statement->kind == StatementKind::Variable && statement->args.startsWith("("))
            result += "(*" + statement->fullName + ")";
        else
            result += statement->fullName;
        result += statement->args;
        break;
    case StatementKind::OverloadedOperator:
        if (statement->scope != StatementScope::Local)
            result = getScopePrefix(statement) + ' ';
        result += statement->type() + " operator " + statement->fullName + statement->args;
        break;
    case StatementKind::LiteralOperator:
        if (statement->scope != StatementScope::Local)
            result = getScopePrefix(statement) + ' ';
        result += statement->type() + " operator \"\"" + statement->fullName + statement->args;
        break;
    case StatementKind::Namespace:
        result = statement->fullName;
        break;
    case StatementKind::Constructor:
        result = getScopePrefix(statement) + QObject::tr("constructor") + ' '
                + statement->type() + ' ' + statement->fullName + statement->args;
        break;
    case StatementKind::Destructor:
        result = getScopePrefix(statement) + QObject::tr("destructor") + ' '
                + statement->type() + ' ' + statement->fullName + statement->args;
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

PStatement CppParser::findMemberOfStatement(const QString &phrase,
                                            const PStatement &scopeStatement) const
{
    if (!scopeStatement)
        return {};
    const auto values = scopeStatement->children.values(phrase);
    for (const std::weak_ptr<Statement> &weak : values) {
        if (PStatement result = weak.lock())
            return result;
    }
    return {};
}

QList<PStatement> CppParser::findMembersOfStatement(const QString &phrase,
                                                    const PStatement &scopeStatement) const
{
    QList<PStatement> result;
    if (!scopeStatement)
        return result;
    const auto values = scopeStatement->children.values(phrase);
    for (const std::weak_ptr<Statement> &weak : values) {
        if (PStatement statement = weak.lock())
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
    for (const std::weak_ptr<Statement> &weak : values) {
        if (PStatement result = weak.lock())
            return result;
    }
    return {};
}

PStatement CppParser::findStatementInNamespace(const QString &name,
                                               const QString &namespaceName) const
{
    PStatementList list = findNamespace(namespaceName);
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
    if (!info)
        return statement;
    if (statement->declarationBoundaryAncestor.isEmpty())
        return statement->fileName == fileName ? statement : PStatement();
    const QString &ancestor = statement->declarationBoundaryAncestor;
    return ancestor == fileName || info->including(ancestor) ? statement : PStatement();
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
    for (const std::weak_ptr<Statement> &weak : values) {
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
