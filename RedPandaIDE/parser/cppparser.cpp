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
#include "cppparser.h"
#include "parserutils.h"
#include "../utils.h"
#include "qsynedit/syntaxer/cpp.h"

#include <QApplication>
#include <QDate>
#include <QHash>
#include <QQueue>
#include <QRegularExpression>
#include <QThread>
#include <QTime>

static QAtomicInt cppParserCount(0);

static QString calcFullname(const QString& parentName, const QString& name) {
    QString s;
    s.reserve(parentName.size()+2+name.size());
    s+= parentName;
    s+= "::";
    s+= name;
    return s;
}

CppParser::CppParser(QObject *parent) : QObject(parent),
    mMutex()
{
    mParserId = cppParserCount.fetchAndAddRelaxed(1);
    mLanguage = ParserLanguage::CPlusPlus;
    mSerialCount = 0;
    updateSerialId();
    mUniqId = 0;
    mParsing = false;
    //mStatementList ; // owns the objects
    //mFilesToScan;
    //mIncludePaths;
    //mProjectIncludePaths;
    //mProjectFiles;
    // mCurrentScope;
    //mCurrentClassScope;
    //mSkipList;
    mParseLocalHeaders = true;
    mParseGlobalHeaders = true;
    mLockCount = 0;
    mIsSystemHeader = false;
    mIsHeader = false;
    mIsProjectFile = false;

    mCppKeywords = CppKeywords;
    mCppTypeKeywords = CppTypeKeywords;
    mEnabled = true;

    internalClear();

    //mNamespaces;
    //mBlockBeginSkips;
    //mBlockEndSkips;
    //mInlineNamespaceEndSkips;
}

CppParser::~CppParser()
{
    //qDebug()<<"delete parser";
    while (true) {
        //wait for all methods finishes running
        {
            QMutexLocker locker(&mMutex);
            if (!mParsing && (mLockCount == 0)) {
              mParsing = true;
              break;
            }
        }
        //qDebug()<<"waiting for parse finished";
        QThread::msleep(50);
        QCoreApplication* app = QApplication::instance();
        app->processEvents();
    }
    //qDebug()<<"-------- parser deleted ------------";
}

void CppParser::addHardDefineByLine(const QString &line)
{
    QMutexLocker  locker(&mMutex);
    if (line.startsWith('#')) {
        mPreprocessor.addHardDefineByLine(line.mid(1).trimmed());
    } else {
        mPreprocessor.addHardDefineByLine(line);
    }
}

void CppParser::addIncludePath(const QString &value)
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.addIncludePath(value);
}

void CppParser::removeProjectFile(const QString &value)
{
    QMutexLocker locker(&mMutex);

    mProjectFiles.remove(value);
    mFilesToScan.remove(value);
}

void CppParser::addProjectIncludePath(const QString &value)
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.addProjectIncludePath(value);
}

void CppParser::clearIncludePaths()
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.clearIncludePaths();
}

void CppParser::clearProjectIncludePaths()
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.clearProjectIncludePaths();
}

void CppParser::clearProjectFiles()
{
    QMutexLocker  locker(&mMutex);
    mProjectFiles.clear();
}

QList<PStatement> CppParser::getListOfFunctions(const QString &fileName, const QString &phrase, int line) const
{
    QMutexLocker locker(&mMutex);
    QList<PStatement> result;
    if (mParsing)
        return result;

    QStringList expression = splitExpression(phrase);
    PStatement statement = doFindStatementOf(fileName, expression, line);
    if (!statement)
        return result;
    if (statement->kind == StatementKind::Preprocessor) {
        if (statement->args.isEmpty()) {
            QString name = expandMacro(statement->value);
            statement = doFindStatementOf(fileName, name ,line);
            if (!statement)
                return result;
        }
    }
    while(statement && statement->kind == StatementKind::Alias)
        statement = doFindAliasedStatement(statement);
    if (!statement)
        return result;
    PStatement parentScope;
    if (statement->kind == StatementKind::Class) {
        parentScope = statement;
    } else
        parentScope = statement->parentScope.lock();
    if (parentScope && parentScope->kind == StatementKind::Namespace) {
        PStatementList namespaceStatementsList = doFindNamespace(parentScope->fullName);
        if (namespaceStatementsList) {
            for (PStatement& namespaceStatement  : *namespaceStatementsList) {
                result.append(
                            getListOfFunctions(fileName,line,statement,namespaceStatement));
            }
        }
    } else
        result.append(
                    getListOfFunctions(fileName,line,statement,parentScope)
                    );
    return result;
}

PStatement CppParser::findScopeStatement(const QString &filename, int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing) {
        return PStatement();
    }
    return doFindScopeStatement(filename,line);
}

PStatement CppParser::doFindScopeStatement(const QString &filename, int line) const
{
    PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(filename);
    if (!fileInfo)
        return PStatement();

    return fileInfo->findScopeAtLine(line);
}

PParsedFileInfo CppParser::findFileInfo(const QString &filename) const
{
    QMutexLocker locker(&mMutex);
    PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(filename);
    return fileInfo;
}
QString CppParser::findFirstTemplateParamOf(const QString &fileName, const QString &phrase, const PStatement& currentScope) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return "";
    return doFindFirstTemplateParamOf(fileName,phrase,currentScope);
}

QString CppParser::findTemplateParamOf(const QString &fileName, const QString &phrase, int index, const PStatement &currentScope) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return "";
    return doFindTemplateParamOf(fileName,phrase,index,currentScope);
}

PStatement CppParser::findFunctionAt(const QString &fileName, int line) const
{
    QMutexLocker locker(&mMutex);
    PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(fileName);
    if (!fileInfo)
        return PStatement();
    for (const PStatement& statement : fileInfo->statements()) {
        if (statement->kind != StatementKind::Function
                && statement->kind != StatementKind::Constructor
                && statement->kind != StatementKind::Destructor)
            continue;
        if (statement->line == line || statement->definitionLine == line)
            return statement;
    }
    return PStatement();
}

int CppParser::findLastOperator(const QString &phrase) const
{
    int phraseLength = phrase.length();
    int i = phraseLength-1;

    // Obtain stuff after first operator
    while (i>=0) {
        if ((i+1<phraseLength) &&
                (phrase[i + 1] == '>') && (phrase[i] == '-'))
            return i;
        else if ((i+1<phraseLength) &&
                 (phrase[i + 1] == ':') && (phrase[i] == ':'))
            return i;
        else if (phrase[i] == '.')
            return i;
        i--;
    }
    return -1;
}

PStatementList CppParser::findNamespace(const QString &name) const
{
    QMutexLocker locker(&mMutex);
    return doFindNamespace(name);
}

PStatementList CppParser::doFindNamespace(const QString &name) const
{
    return mNamespaces.value(name,PStatementList());
}


PStatement CppParser::findStatement(const QString &fullname) const
{
    QMutexLocker locker(&mMutex);
    return doFindStatement(fullname);
}

PStatement CppParser::doFindStatement(const QString &fullname) const
{
    if (fullname.isEmpty())
        return PStatement();
    QStringList phrases = fullname.split("::");
    if (phrases.isEmpty())
        return PStatement();
    PStatement parentStatement;
    PStatement statement;

    for (int i=(phrases[0].isEmpty()?1:0);i<phrases.count();i++) {
        const QString& phrase=phrases[i];
        if (parentStatement && parentStatement->kind == StatementKind::Namespace) {
            PStatementList lst = doFindNamespace(parentStatement->fullName);
            foreach (const PStatement& namespaceStatement, *lst) {
                statement = findMemberOfStatement(phrase,namespaceStatement);
                if (statement)
                    break;
            }
        } else {
            statement = findMemberOfStatement(phrase,parentStatement);
        }
        if (!statement)
            return PStatement();
        parentStatement = statement;
    }
    return statement;
}

PStatement CppParser::findStatementOf(const QString &fileName, const QString &phrase, int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PStatement();
    return doFindStatementOf(fileName,phrase,line);
}
PStatement CppParser::doFindStatementOf(const QString &fileName, const QString &phrase, int line) const
{
    return doFindStatementOf(fileName,phrase,doFindScopeStatement(fileName,line));
}

PStatement CppParser::findStatementOf(const QString &fileName,
                                      const QString &phrase,
                                      const PStatement& currentScope,
                                      PStatement &parentScopeType) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PStatement();
    return doFindStatementOf(fileName,phrase,currentScope,parentScopeType);
}

PStatement CppParser::doFindStatementOf(const QString &fileName,
                                      const QString &phrase,
                                      const PStatement& currentScope,
                                      PStatement &parentScopeType) const
{
    PStatement result;
    parentScopeType = currentScope;

    //find the start scope statement
    QString namespaceName, remainder;
    QString nextScopeWord,operatorToken,memberName;
    PStatement statement;
    getFullNamespace(phrase, namespaceName, remainder);
    if (!namespaceName.isEmpty()) {  // (namespace )qualified Name
        PStatementList namespaceList = doFindNamespace(namespaceName);

        if (!namespaceList || namespaceList->isEmpty())
            return PStatement();

        if (remainder.isEmpty())
            return namespaceList->front();

        remainder = splitPhrase(remainder,nextScopeWord,operatorToken,memberName);

        for (PStatement& currentNamespace: *namespaceList) {
            statement = findMemberOfStatement(nextScopeWord,currentNamespace);
            if (statement)
                break;
        }

        //not found in namespaces;
        if (!statement)
            return PStatement();
        // found in namespace
        parentScopeType = statement->parentScope.lock();
    } else if ((phrase.length()>2) &&
               (phrase[0]==':') && (phrase[1]==':')) {
        //global
        remainder= phrase.mid(2);
        remainder= splitPhrase(remainder,nextScopeWord,operatorToken,memberName);
        statement= findMemberOfStatement(nextScopeWord,PStatement());
        if (!statement)
            return PStatement();
        parentScopeType = currentScope;
    } else {
        //unqualified name
        parentScopeType = currentScope;
        remainder = splitPhrase(remainder,nextScopeWord,operatorToken,memberName);
        statement = findStatementStartingFrom(fileName,nextScopeWord,parentScopeType);
        if (!statement)
            return PStatement();
    }

    if (!memberName.isEmpty() && (statement->kind == StatementKind::Typedef)) {
        PStatement typeStatement = doFindTypeDefinitionOf(fileName,statement->type, parentScopeType);
        if (typeStatement)
            statement = typeStatement;
    }

    //using alias like 'using std::vector;'
    while (statement->kind == StatementKind::Alias) {
        statement = doFindAliasedStatement(statement);
        if (!statement)
            return PStatement();
    }

    if (statement->kind == StatementKind::Constructor) {
        // we need the class, not the construtor
        statement = statement->parentScope.lock();
        if (!statement)
            return PStatement();
    }
    PStatement lastScopeStatement;
    QString typeName;
    PStatement typeStatement;
    while (!memberName.isEmpty()) {
        if (statement->kind!=StatementKind::Class
                && operatorToken == "::") {
            return PStatement();
        }
        if (statement->kind == StatementKind::Variable
                || statement->kind ==  StatementKind::Parameter
                || statement->kind ==  StatementKind::Function) {

            bool isSTLContainerFunctions = false;

            if (statement->kind == StatementKind::Function){
                PStatement parentScope = statement->parentScope.lock();
                if (parentScope
                    && STLContainers.contains(parentScope->fullName)
                    && STLElementMethods.contains(statement->command)
                    && lastScopeStatement) {
                    isSTLContainerFunctions = true;
                    PStatement lastScopeParent = lastScopeStatement->parentScope.lock();
                    typeName=doFindFirstTemplateParamOf(fileName,lastScopeStatement->type,
                                                      lastScopeParent );
                    typeStatement=doFindTypeDefinitionOf(fileName, typeName,
                                                       lastScopeParent );
                } else if (parentScope
                    && STLMaps.contains(parentScope->fullName)
                    && STLElementMethods.contains(statement->command)
                    && lastScopeStatement) {
                    isSTLContainerFunctions = true;
                    PStatement lastScopeParent = lastScopeStatement->parentScope.lock();
                    typeName=doFindTemplateParamOf(fileName,lastScopeStatement->type,1,
                                                      lastScopeParent );
                    typeStatement=doFindTypeDefinitionOf(fileName, typeName,
                                                       lastScopeParent );
                }
            }
            if (!isSTLContainerFunctions)
                typeStatement = doFindTypeDefinitionOf(fileName,statement->type, statement->parentScope.lock());

            //it's stl smart pointer
            if ((typeStatement)
                    && STLPointers.contains(typeStatement->fullName)
                    && (operatorToken == "->")) {
                PStatement parentScope = statement->parentScope.lock();
                typeName=doFindFirstTemplateParamOf(fileName,statement->type, parentScope);
                typeStatement=doFindTypeDefinitionOf(fileName, typeName,parentScope);
            } else if ((typeStatement)
                       && STLContainers.contains(typeStatement->fullName)
                       && nextScopeWord.endsWith(']')) {
                //it's a std container
                PStatement parentScope = statement->parentScope.lock();
                typeName = doFindFirstTemplateParamOf(fileName,statement->type,
                                                    parentScope);
                typeStatement = doFindTypeDefinitionOf(fileName, typeName,
                                                     parentScope);
            } else if ((typeStatement)
                       && STLMaps.contains(typeStatement->fullName)
                       && nextScopeWord.endsWith(']')) {
                //it's a std container
                PStatement parentScope = statement->parentScope.lock();
                typeName = doFindFirstTemplateParamOf(fileName,statement->type,
                                                    parentScope);
                typeStatement = doFindTypeDefinitionOf(fileName, typeName,
                                                     parentScope);
            }
            lastScopeStatement = statement;
            if (typeStatement)
                statement = typeStatement;
        } else
            lastScopeStatement = statement;
        remainder = splitPhrase(remainder,nextScopeWord,operatorToken,memberName);
        PStatement memberStatement = findMemberOfStatement(nextScopeWord,statement);
        if (!memberStatement)
            return PStatement();

        parentScopeType=statement;
        statement = memberStatement;
        if (!memberName.isEmpty() && (statement->kind == StatementKind::Typedef)) {
            PStatement typeStatement = doFindTypeDefinitionOf(fileName,statement->type, parentScopeType);
            if (typeStatement)
                statement = typeStatement;
        }
    }
    return statement;
}

PEvalStatement CppParser::evalExpression(
        const QString &fileName,
        QStringList &phraseExpression,
        const PStatement &currentScope) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PEvalStatement();
//    qDebug()<<phraseExpression;
    int pos = 0;
    return doEvalExpression(fileName,
                            phraseExpression,
                            pos,
                            currentScope,
                            PEvalStatement(),
                            true,
                            true);
}

PStatement CppParser::doFindStatementOf(const QString &fileName, const QString &phrase, const PStatement& currentClass) const
{
    PStatement statementParentType;
    return doFindStatementOf(fileName,phrase,currentClass,statementParentType);
}


PStatement CppParser::findStatementOf(const QString &fileName, const QStringList &expression, const PStatement &currentScope) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PStatement();
    return doFindStatementOf(fileName,expression,currentScope);
}

PStatement CppParser::doFindStatementOf(const QString &fileName, const QStringList &expression, const PStatement &currentScope) const
{
    QString memberOperator;
    QStringList memberExpression;
    QStringList ownerExpression = getOwnerExpressionAndMember(expression,memberOperator,memberExpression);
    if (memberExpression.isEmpty()) {
        return PStatement();
    }
    QString phrase = memberExpression[0];
    if (memberOperator.isEmpty()) {
        return findStatementStartingFrom(fileName,phrase,currentScope);
    } else if (ownerExpression.isEmpty()) {
        return findMemberOfStatement(fileName, phrase,PStatement());
    } else {
        int pos = 0;
        PEvalStatement ownerEvalStatement = doEvalExpression(fileName,
                                ownerExpression,
                                pos,
                                currentScope,
                                PEvalStatement(),
                                true,false);
        if (!ownerEvalStatement) {
            return PStatement();
        }
        if (ownerEvalStatement->effectiveTypeStatement &&
                ownerEvalStatement->effectiveTypeStatement->kind == StatementKind::Namespace) {
            PStatementList lst = doFindNamespace(ownerEvalStatement->effectiveTypeStatement->fullName);
            foreach (const PStatement& namespaceStatement, *lst) {
                PStatement statement = findMemberOfStatement(phrase,namespaceStatement);
                if (statement)
                    return statement;
            }
            return PStatement();
        } else if (ownerEvalStatement->typeStatement
                   && STLIterators.contains(ownerEvalStatement->typeStatement->command)
                   && memberOperator=="->"
                      ) {
            PStatement parentScope = ownerEvalStatement->typeStatement->parentScope.lock();
            if (STLContainers.contains(parentScope->fullName)) {
                QString typeName=doFindFirstTemplateParamOf(fileName,ownerEvalStatement->templateParams, parentScope);
                PStatement typeStatement=doFindTypeDefinitionOf(fileName, typeName,parentScope);
                if (typeStatement) {
                    return findMemberOfStatement(phrase, typeStatement);
                } else {
                    return PStatement();
                }
            } else if (STLMaps.contains(parentScope->fullName)) {
                QString typeName=doFindTemplateParamOf(fileName,ownerEvalStatement->templateParams,1,parentScope);
            //                        qDebug()<<"typeName"<<typeName<<lastResult->baseStatement->type<<lastResult->baseStatement->command;
                PStatement typeStatement=doFindTypeDefinitionOf(fileName, typeName,parentScope);
                if (typeStatement) {
                    return findMemberOfStatement(phrase, typeStatement);
                } else {
                    return PStatement();
                }
            }
        }
        return findMemberOfStatement(phrase, ownerEvalStatement->effectiveTypeStatement);
    }

}

PStatement CppParser::findStatementOf(const QString &fileName, const QStringList &expression, int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PStatement();
    return doFindStatementOf(fileName,expression,line);
}

PStatement CppParser::doFindStatementOf(const QString &fileName, const QStringList &expression, int line) const
{
    PStatement statement = doFindStatementOf(fileName,expression,doFindScopeStatement(fileName,line));
    if (statement && statement->line != line
            && statement->definitionLine != line) {
        PStatement parentStatement = statement->parentScope.lock();
        if (parentStatement &&
                (parentStatement->line == line && parentStatement->fileName == fileName)) {
            return parentStatement;
        }
    }
    return statement;
}

PStatement CppParser::findAliasedStatement(const PStatement &statement) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PStatement();
    return doFindAliasedStatement(statement);
}

QList<PStatement> CppParser::listTypeStatements(const QString &fileName, int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return QList<PStatement>();
    return doListTypeStatements(fileName,line);
}

PStatement CppParser::doFindAliasedStatement(const PStatement &statement) const {
    QSet<Statement *> foundSet;
    return doFindAliasedStatement(statement,foundSet);
}

PStatement CppParser::doFindNoTemplateSpecializationClass(const PStatement &statement) const
{
    Q_ASSERT(statement!=nullptr);
    Q_ASSERT(statement->kind == StatementKind::Class);
    if (statement->templateSpecializationParams.isEmpty())
        return statement;
    PStatement parent = statement->parentScope.lock();
    const StatementMap & statementMap = mStatementList.childrenStatements(parent);
    QList<PStatement> list = statementMap.values(statement->command);
    foreach(const PStatement &child, list) {
        if (child->kind == StatementKind::Class
                && child->templateSpecializationParams.isEmpty())
            return child;
    }
    return statement;
}

PStatement CppParser::doFindAliasedStatement(const PStatement &statement, QSet<Statement *> foundSet) const
{
    if (!statement)
        return PStatement();
    QString alias = statement->type;
    int pos = statement->type.lastIndexOf("::");
    if (pos<0)
        return PStatement();
    QString nsName=statement->type.mid(0,pos);
    QString name = statement->type.mid(pos+2);
    PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(statement->fileName);
    if (!fileInfo)
        return PStatement();
    foundSet.insert(statement.get());
    PStatement result;
    if (nsName.isEmpty()) {
        QList<PStatement> resultList = findMembersOfStatement(name,PStatement());
        foreach(const PStatement& resultStatement,resultList) {
            if (fileInfo->including(resultStatement->fileName)) {
                result = resultStatement;
                break;
            }
        }
    } else {
        PStatementList namespaceStatements = doFindNamespace(nsName);
        if (!namespaceStatements)
            return PStatement();
        foreach (const PStatement& namespaceStatement, *namespaceStatements) {
            QList<PStatement> resultList = findMembersOfStatement(name,namespaceStatement);

            foreach(const PStatement& resultStatement,resultList) {
                if (fileInfo->including(resultStatement->fileName)) {
                    result = resultStatement;
                    break;
                }
            }
            if (result)
                break;
        }
    }
    if (!result)
        return PStatement();
    if (foundSet.contains(result.get()))
        return PStatement();
    if (result->kind == StatementKind::Alias)
        result = doFindAliasedStatement(result, foundSet);
    return result;
}

QList<PStatement> CppParser::doListTypeStatements(const QString &fileName, int line) const
{
    QList<PStatement> result;
    QSet<QString> usedNamespaces;
    PStatement scopeStatement = doFindScopeStatement(fileName,line);
    while (true) {
        const StatementMap& statementMap = mStatementList.childrenStatements(scopeStatement);
        foreach (const PStatement statement, statementMap.values()) {
            if (isTypeStatement(statement->kind))
                result.append(statement);
        }
        if (!scopeStatement)
            break;
        usedNamespaces = usedNamespaces.unite(scopeStatement->usingList);
        scopeStatement=scopeStatement->parentScope.lock();
    }
    usedNamespaces = usedNamespaces.unite(internalGetFileUsings(fileName));
    foreach(const QString& ns, usedNamespaces) {
        PStatementList namespaceStatementsList=doFindNamespace(ns);
        foreach (const PStatement& namespaceStatement,*namespaceStatementsList) {
            const StatementMap& statementMap = mStatementList.childrenStatements(namespaceStatement);
            foreach (const PStatement statement, statementMap.values()) {
                if (isTypeStatement(statement->kind))
                    result.append(statement);
            }
        }
    }
    return result;
}

PStatement CppParser::findStatementStartingFrom(const QString &fileName, const QString &phrase, const PStatement& startScope) const
{
    PStatement scopeStatement = startScope;

    // repeat until reach global
    PStatement result;
    while (scopeStatement) {
        //search members of current scope
        result = findStatementInScope(phrase, scopeStatement);
        if (result)
            return result;
        // not found
        // search members of all usings (in current scope )
        foreach (const QString& namespaceName, scopeStatement->usingList) {
            result = findStatementInNamespace(phrase,namespaceName);
            if (result)
                return result;
        }
        scopeStatement = scopeStatement->parentScope.lock();
    }

    // Search all global members
    result = findMemberOfStatement(fileName, phrase,PStatement());
    if (result)
        return result;

    //Find in all global usings
    const QSet<QString>& fileUsings = internalGetFileUsings(fileName);
    // add members of all fusings
    for (const QString& namespaceName:fileUsings) {
        result = findStatementInNamespace(phrase,namespaceName);
        if (result)
            return result;
    }
    return PStatement();
}

PStatement CppParser::findTypeDefinitionOf(const QString &fileName, const QString &aType, const PStatement& currentClass) const
{
    QMutexLocker locker(&mMutex);

    if (mParsing)
        return PStatement();

    return doFindTypeDefinitionOf(fileName,aType,currentClass);
}

PStatement CppParser::findTypeDef(const PStatement &statement, const QString &fileName) const
{
    QMutexLocker locker(&mMutex);

    if (mParsing)
        return PStatement();
    return getTypeDef(statement, fileName, "");
}

bool CppParser::freeze()
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return false;
    mLockCount++;
    return true;
}

bool CppParser::freeze(const QString &serialId)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return false;
    if (mSerialId!=serialId)
        return false;
    mLockCount++;
    return true;
}

QStringList CppParser::getClassesList() const
{
    QMutexLocker locker(&mMutex);

    QStringList list;
    return list;
    // fills List with a list of all the known classes
    QQueue<PStatement> queue;
    queue.enqueue(PStatement());
    while (!queue.isEmpty()) {
        PStatement statement = queue.dequeue();
        StatementMap statementMap = mStatementList.childrenStatements(statement);
        for (PStatement& child:statementMap) {
            if (child->kind == StatementKind::Class)
                list.append(child->command);
            if (!child->children.isEmpty())
                queue.enqueue(child);
        }
    }
    return list;
}

QStringList CppParser::getFileDirectIncludes(const QString &filename) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return QStringList();
    if (filename.isEmpty())
        return QStringList();
    PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(filename);

    if (fileInfo) {
        return fileInfo->directIncludes();
    }
    return QStringList();

}

QSet<QString> CppParser::internalGetIncludedFiles(const QString &filename) const {
    QSet<QString> list;
    if (mParsing)
        return list;
    if (filename.isEmpty())
        return list;
    list.insert(filename);
    PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(filename);

    if (fileInfo) {
        foreach (const QString& file, fileInfo->includes()) {
            list.insert(file);
        }
    }
    return list;
}

QSet<QString> CppParser::getIncludedFiles(const QString &filename) const
{
    QMutexLocker locker(&mMutex);
    return internalGetIncludedFiles(filename);
}

QSet<QString> CppParser::getFileUsings(const QString &filename) const
{
    QMutexLocker locker(&mMutex);
    return internalGetFileUsings(filename);
}

QSet<QString> CppParser::internalGetFileUsings(const QString &filename) const
{
    QSet<QString> result;
    if (filename.isEmpty())
        return result;
//    if (mParsing)
//        return result;
    PParsedFileInfo fileInfo= mPreprocessor.findFileInfo(filename);
    if (fileInfo) {
        foreach (const QString& usingName, fileInfo->usings()) {
            result.insert(usingName);
        }
        foreach (const QString& subFile,fileInfo->includes()){
            PParsedFileInfo subIncludes = mPreprocessor.findFileInfo(subFile);
            if (subIncludes) {
                foreach (const QString& usingName, subIncludes->usings()) {
                    result.insert(usingName);
                }
            }
        }
    }
    return result;
}

QString CppParser::getHeaderFileName(const QString &relativeTo, const QString &headerName, bool fromNext) const
{
    QMutexLocker locker(&mMutex);
    QString currentDir = extractFileDir(relativeTo);
    QStringList includes;
    QStringList projectIncludes;
    bool found=false;
    if (fromNext && mPreprocessor.includePaths().contains(currentDir)) {
        foreach(const QString& s, mPreprocessor.includePathList()) {
            if (found) {
                includes.append(s);
                continue;
            } else if (s == currentDir)
                found = true;
        }
        projectIncludes = mPreprocessor.projectIncludePathList();
    } else if (fromNext && mPreprocessor.projectIncludePaths().contains(currentDir)) {
        includes = mPreprocessor.includePathList();
        foreach(const QString& s, mPreprocessor.projectIncludePathList()) {
            if (found) {
                includes.append(s);
                continue;
            } else if (s == currentDir)
                found = true;
        }
    } else {
        includes = mPreprocessor.includePathList();
        projectIncludes = mPreprocessor.projectIncludePathList();
    }
    return ::getHeaderFilename(relativeTo, headerName, includes,
                               projectIncludes);
}

bool CppParser::isLineVisible(const QString &fileName, int line) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing) {
        return true;
    }
    PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(fileName);
    if (!fileInfo)
        return true;
    return fileInfo->isLineVisible(line);
}

void CppParser::invalidateFile(const QString &fileName)
{
    if (!mEnabled)
        return;
    {
        QMutexLocker locker(&mMutex);
        if (mParsing || mLockCount>0)
            return;
        updateSerialId();
        mParsing = true;
    }
    QSet<QString> files = calculateFilesToBeReparsed(fileName);
    internalInvalidateFiles(files);
    mParsing = false;
}

bool CppParser::isIncludeLine(const QString &line) const
{
    QString trimmedLine = line.trimmed();
    if ((trimmedLine.length() > 0)
            && trimmedLine.startsWith('#')) { // it's a preprocessor line
        if (trimmedLine.mid(1).trimmed().startsWith("include"))
            return true;
    }
    return false;
}

bool CppParser::isIncludeNextLine(const QString &line) const
{
    QString trimmedLine = line.trimmed();
    if ((trimmedLine.length() > 0)
            && trimmedLine.startsWith('#')) { // it's a preprocessor line
        if (trimmedLine.mid(1).trimmed().startsWith("include_next"))
            return true;
    }
    return false;
}

bool CppParser::isProjectHeaderFile(const QString &fileName) const
{
    QMutexLocker locker(&mMutex);
    return ::isSystemHeaderFile(fileName,mPreprocessor.projectIncludePaths());
}

bool CppParser::isSystemHeaderFile(const QString &fileName) const
{
    QMutexLocker locker(&mMutex);
    return ::isSystemHeaderFile(fileName,mPreprocessor.includePaths());
}

void CppParser::parseFile(const QString &fileName, bool inProject, bool onlyIfNotParsed, bool updateView, std::shared_ptr<CppParser> parserPtr)
{
    if (!mEnabled)
        return;
    {
        QMutexLocker locker(&mMutex);
        if (mParsing) {
            mLastParseFileCommand = std::make_unique<ParseFileCommand>();
            mLastParseFileCommand->fileName = fileName;
            mLastParseFileCommand->inProject = inProject;
            mLastParseFileCommand->onlyIfNotParsed = onlyIfNotParsed;
            mLastParseFileCommand->updateView = updateView;
            mLastParseFileCommand->parserPtr = parserPtr;
            return;
        }
        if (mLockCount>0)
            return;
        mParsing = true;
        updateSerialId();
        if (updateView)
            emit onBusy();
        emit onStartParsing();
    }
    {
        auto action = finally([&,this]{
            QMutexLocker locker(&mMutex);
            if (updateView)
                emit onEndParsing(mFilesScannedCount,1);
            else
                emit onEndParsing(mFilesScannedCount,0);

            if (mLastParseFileCommand) {
                ::parseFile(mLastParseFileCommand->parserPtr,
                            mLastParseFileCommand->fileName,
                            mLastParseFileCommand->inProject,
                            mLastParseFileCommand->onlyIfNotParsed,
                            mLastParseFileCommand->updateView);
                mLastParseFileCommand = nullptr;
            }
            mParsing = false;
        });
        QString fName = fileName;
        if (onlyIfNotParsed && mPreprocessor.fileScanned(fName))
            return;

        if (inProject) {
            QSet<QString> filesToReparsed = calculateFilesToBeReparsed(fileName);
            QStringList files = sortFilesByIncludeRelations(filesToReparsed);
            internalInvalidateFiles(filesToReparsed);

            mFilesToScanCount = files.count();
            mFilesScannedCount = 0;

            foreach (const QString& file,files) {
                mFilesScannedCount++;
                emit onProgress(file,mFilesToScanCount,mFilesScannedCount);
                if (!mPreprocessor.fileScanned(file)) {
                    internalParse(file);
                }
            }
        } else {
            internalInvalidateFile(fileName);
            mFilesToScanCount = 1;
            mFilesScannedCount = 0;

            mFilesScannedCount++;
            emit onProgress(fileName,mFilesToScanCount,mFilesScannedCount);
            internalParse(fileName);
        }
    }
}

void CppParser::parseFileList(bool updateView)
{
    if (!mEnabled)
        return;
    {
        QMutexLocker locker(&mMutex);
        if (mParsing || mLockCount>0)
            return;
        updateSerialId();
        mParsing = true;
        if (updateView)
            emit onBusy();
        emit onStartParsing();
    }
    {
        auto action = finally([&,this]{
            mParsing = false;
            if (updateView)
                emit onEndParsing(mFilesScannedCount,1);
            else
                emit onEndParsing(mFilesScannedCount,0);
        });
        // Support stopping of parsing when files closes unexpectedly
        mFilesScannedCount = 0;
        mFilesToScanCount = mFilesToScan.count();

        QStringList files = sortFilesByIncludeRelations(mFilesToScan);
        // parse header files in the first parse
        foreach (const QString& file, files) {
            mFilesScannedCount++;
            emit onProgress(mCurrentFile,mFilesToScanCount,mFilesScannedCount);
            if (!mPreprocessor.fileScanned(file)) {
                internalParse(file);
            }
        }
        mFilesToScan.clear();
    }
}

void CppParser::parseHardDefines()
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return;
    int oldIsSystemHeader = mIsSystemHeader;
    mIsSystemHeader = true;
    mParsing=true;
    {
        auto action = finally([&,this]{
            mParsing = false;
            mIsSystemHeader=oldIsSystemHeader;
        });
        for (const PDefine& define:mPreprocessor.hardDefines()) {
            addStatement(
                        PStatement(), // defines don't belong to any scope
                        "",
                        "", // define has no type
                        define->name,
                        define->args,
                        "",
                        define->value,
                        -1,
                        StatementKind::Preprocessor,
                        StatementScope::Global,
                        StatementAccessibility::None,
                        StatementProperty::HasDefinition);
        }
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
            if (!mParsing && mLockCount ==0) {
                mParsing = true;
                break;
            }
        }
        QThread::msleep(50);
        QCoreApplication* app = QApplication::instance();
        app->processEvents();
    }
    {
        auto action = finally([this]{
            mParsing = false;
        });
        emit  onBusy();
        mUniqId = 0;

        mParseLocalHeaders = true;
        mParseGlobalHeaders = true;
        mIsSystemHeader = false;
        mIsHeader = false;
        mIsProjectFile = false;
        mFilesScannedCount=0;
        mFilesToScanCount = 0;

        mCurrentScope.clear();
        mMemberAccessibilities.clear();
        mStatementList.clear();

        mProjectFiles.clear();
//        mBlockBeginSkips.clear(); //list of for/catch block begin token index;
//        mBlockEndSkips.clear(); //list of for/catch block end token index;
        mInlineNamespaceEndSkips.clear(); // list for inline namespace end token index;
        mFilesToScan.clear(); // list of base files to scan
        mNamespaces.clear();  // namespace and the statements in its scope
        mInlineNamespaces.clear();
        mClassInheritances.clear();
        mPreprocessor.clear();
        mTokenizer.clear();
    }
}

void CppParser::unFreeze()
{
    QMutexLocker locker(&mMutex);
    mLockCount--;
}

bool CppParser::fileScanned(const QString &fileName) const
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return false;
    return mPreprocessor.fileScanned(fileName);
}

bool CppParser::isFileParsed(const QString &filename) const
{
    return mPreprocessor.fileScanned(filename);
}

QString CppParser::getScopePrefix(const PStatement& statement) const{
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

QString CppParser::prettyPrintStatement(const PStatement& statement, const QString& filename, int line) const
{
    QString result;
    switch(statement->kind) {
    case StatementKind::Preprocessor:
        if (statement->command == "__FILE__")
            result = '"'+filename+'"';
        else if (statement->command == "__LINE__")
            result = QString("\"%1\"").arg(line);
        else if (statement->command == "__DATE__")
            result = QString("\"%1\"").arg(QDate::currentDate().toString(Qt::ISODate));
        else if (statement->command == "__TIME__")
            result = QString("\"%1\"").arg(QTime::currentTime().toString(Qt::ISODate));
        else {
            QString hintText = "#define";
            if (statement->command != "")
                hintText += ' ' + statement->command;
            if (statement->args != "")
                hintText += ' ' + statement->args;
            if (statement->value != "")
                hintText += ' ' + statement->value;
            result = hintText;
        }
        break;
    case StatementKind::EnumClassType:
        result = "enum class "+statement->command;
        break;
    case StatementKind::EnumType:
        result = "enum "+statement->command;
        break;
    case StatementKind::Enum:
        if (!statement->type.isEmpty())
            result = statement->type + "::";
        else
            result = "";
        result += statement->command;
        if (!statement->value.isEmpty())
            result += "(" + statement->value + ")";
        break;
    case StatementKind::Typedef:
        result = "typedef "+statement->type+" "+statement->command;
        if (!statement->args.isEmpty())
            result += " "+statement->args;
        break;
    case StatementKind::Alias:
        result = "using "+statement->type;
        break;
    case StatementKind::Function:
    case StatementKind::Variable:
    case StatementKind::Parameter:
    case StatementKind::Class:
        if (statement->scope!= StatementScope::Local)
            result = getScopePrefix(statement)+ ' '; // public
        result += statement->type + ' '; // void
        result += statement->fullName; // A::B::C::Bar
        result += statement->args; // (int a)
        break;
    case StatementKind::Namespace:
        result = statement->fullName; // Bar
        break;
    case StatementKind::Constructor:
        result = getScopePrefix(statement); // public
        result += QObject::tr("constructor") + ' '; // constructor
        result += statement->type + ' '; // void
        result += statement->fullName; // A::B::C::Bar
        result += statement->args; // (int a)
        break;
    case StatementKind::Destructor:
        result = getScopePrefix(statement); // public
        result += QObject::tr("destructor") + ' '; // constructor
        result += statement->type + ' '; // void
        result += statement->fullName; // A::B::C::Bar
        result += statement->args; // (int a)
        break;
    default:
        break;
    }
    return result;
}

QString CppParser::getTemplateParam(const PStatement& statement,
                                    const QString& filename,
                                    const QString& phrase,
                                    int index,
                                    const PStatement& currentScope) const
{
    if (!statement)
        return "";
    if (statement->kind != StatementKind::Typedef)
        return "";
    if (statement->type == phrase) // prevent infinite loop
        return "";
    return doFindTemplateParamOf(filename,statement->type,index,currentScope);
}

int CppParser::getTemplateParamStart(const QString &s, int startAt, int index) const
{
    int i = startAt+1;
    int count=0;
    while (count<index) {
        i = getTemplateParamEnd(s,i)+1;
        count++;
    }
    return i;
}

int CppParser::getTemplateParamEnd(const QString &s, int startAt) const {
    int i = startAt;
    int level = 1; // pretend we start on top of '<'
    while (i < s.length()) {
        switch (s[i].unicode()) {
        case '<':
            level++;
            break;
        case ',':
            if (level == 1) {
                return i;
            }
            break;
        case '>':
            level--;
            if (level==0)
                return i;
        }
        i++;
    }
    return startAt;
}

void CppParser::addProjectFile(const QString &fileName, bool needScan)
{
    QMutexLocker locker(&mMutex);
    //value.replace('/','\\'); // only accept full file names

    // Update project listing
    mProjectFiles.insert(fileName);

    // Only parse given file
    if (needScan && !mPreprocessor.fileScanned(fileName)) {
        mFilesToScan.insert(fileName);
    }
}

PStatement CppParser::addInheritedStatement(const PStatement& derived, const PStatement& inherit, StatementAccessibility access)
{

    PStatement statement = addStatement(
                derived,
                inherit->fileName,
                inherit->type,
                inherit->command,
                inherit->args,
                inherit->noNameArgs,
                inherit->value,
                inherit->line,
                inherit->kind,
                inherit->scope,
                access,
                inherit->properties | StatementProperty::Inherited);
    return statement;
}

PStatement CppParser::addChildStatement(const PStatement& parent, const QString &fileName,
                                        const QString &aType,
                                        const QString &command, const QString &args,
                                        const QString& noNameArgs,
                                        const QString &value, int line, StatementKind kind,
                                        const StatementScope& scope, const StatementAccessibility& classScope,
                                        StatementProperties properties)
{
    return addStatement(
                parent,
                fileName,
                aType,
                command,
                args,
                noNameArgs,
                value,
                line,
                kind,
                scope,
                classScope,
                properties);
}

PStatement CppParser::addStatement(const PStatement& parent,
                                   const QString &fileName,
                                   const QString &aType,
                                   const QString &command,
                                   const QString &args,
                                   const QString &noNameArgs,
                                   const QString &value,
                                   int line, StatementKind kind,
                                   const StatementScope& scope,
                                   const StatementAccessibility& accessibility,
                                   StatementProperties properties)
{
    // Move '*', '&' to type rather than cmd (it's in the way for code-completion)
    QString newType = aType;
    QString newCommand = command;
    while (!newCommand.isEmpty() && (newCommand.front() == '*' || newCommand.front() == '&')) {
        newType += newCommand.front();
        newCommand.remove(0,1); // remove first
    }
    QString templateSpecializationParams;
    int pos = newCommand.indexOf("<");
    if (pos>0 && !newCommand.startsWith("operator<")) {
        templateSpecializationParams = newCommand.mid(pos);
        newCommand = newCommand.left(pos);
    }
    newCommand.squeeze();
//    if (newCommand.startsWith("::") && parent && kind!=StatementKind::skBlock ) {
//        qDebug()<<command<<fileName<<line<<kind<<parent->fullName;
//    }
    if (kind == StatementKind::Constructor
            || kind == StatementKind::Function
            || kind == StatementKind::Destructor
            || kind == StatementKind::Variable
            ) {
        //find
        if (properties.testFlag(StatementProperty::HasDefinition)) {
            PStatement oldStatement = findStatementInScope(newCommand,noNameArgs,kind,parent);
            if (oldStatement  && !oldStatement->hasDefinition()) {
                oldStatement->setHasDefinition(true);
                if (oldStatement->fileName!=fileName) {
                    PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(fileName);
                    if (fileInfo) {
                        fileInfo->addStatement(oldStatement);
                    }
                }
                oldStatement->definitionLine = line;
                oldStatement->definitionFileName = fileName;
                return oldStatement;
            }
        }
    }
    PStatement result = std::make_shared<Statement>();
    result->parentScope = parent;
    result->type = newType;
    if (!newCommand.isEmpty())
        result->command = newCommand;
    else {
        mUniqId++;
        result->command = QString("__STATEMENT__%1").arg(mUniqId);
    }
    result->args = args;
    result->noNameArgs = noNameArgs;
    result->value = value;
    result->templateSpecializationParams = templateSpecializationParams;
    result->kind = kind;
    result->scope = scope;
    result->accessibility = accessibility;
    result->properties = properties;
    result->line = line;
    result->definitionLine = line;
    result->fileName = fileName;
    result->definitionFileName = fileName;
    if (!fileName.isEmpty()) {
        result->setInProject(mIsProjectFile);
        result->setInSystemHeader(mIsSystemHeader);
    } else {
        result->setInProject(false);
        result->setInSystemHeader(true);
    }
    if (scope == StatementScope::Local)
        result->fullName =  newCommand;
    else
        result->fullName =  getFullStatementName(newCommand + templateSpecializationParams, parent);
    result->usageCount = -1;

    result->args.squeeze();
    result->noNameArgs.squeeze();
    result->value.squeeze();
    result->type.squeeze();
    mStatementList.add(result);
    if (result->kind == StatementKind::Namespace) {
        PStatementList namespaceList = doFindNamespace(result->fullName);
        if (!namespaceList) {
            namespaceList=std::make_shared<StatementList>();
            mNamespaces.insert(result->fullName,namespaceList);
        }
        namespaceList->append(result);
    }

    if (result->kind!= StatementKind::Block) {
        PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(fileName);
        if (fileInfo) {
            fileInfo->addStatement(result);
        }
    }
    return result;
}

PStatement CppParser::addStatement(const PStatement &parent,
                                   const QString &fileName,
                                   const QString &aType,
                                   const QString &command,
                                   int argStart, int argEnd,
                                   const QString &value, int line,
                                   StatementKind kind, const StatementScope &scope,
                                   const StatementAccessibility &classScope,
                                   StatementProperties properties)
{
    Q_ASSERT(mTokenizer[argStart]->text=='(');
    QString args;
    QString noNameArgs;

    int start=argStart+1;
    bool typeGetted = false;
    int braceLevel=0;
    QString word;
    for (int i=start;i<argEnd;i++) {
        QChar ch=mTokenizer[i]->text[0];
        if (this->isIdentChar(ch)) {
            QString spaces=(i>argStart)?" ":"";
            if (args.length()>0 && (isWordChar(args.back()) || args.back()=='>'))
                args+=spaces;
            word += mTokenizer[i]->text;
            if (!typeGetted) {
                if (noNameArgs.length()>0 && isWordChar(noNameArgs.back()))
                    noNameArgs+=spaces;
                noNameArgs+=word;
                if (mCppTypeKeywords.contains(word) || !isCppKeyword(word))
                    typeGetted = true;
            } else {
                if (isCppKeyword(word)) {
                    noNameArgs+=spaces+word;
                }
            }
            word="";
        } else if (this->isDigitChar(ch)) {
        } else if (mTokenizer[i]->text=="::") {
            if (braceLevel==0) {
                noNameArgs+= mTokenizer[i]->text;
                typeGetted = false;
            }
        } else {
            switch(ch.unicode()) {
            case ',':
                if (braceLevel==0) {
                    typeGetted=false;
                    noNameArgs+= mTokenizer[i]->text;
                }
                break;
            case '{':
            case '[':
            case '<':
            case '(':
                braceLevel++;
                break;
            case '}':
            case ']':
            case '>':
            case ')':
                braceLevel--;
                break;
                //todo: * and & processing
            case '*':
            case '&':
                if (braceLevel==0) {
                    noNameArgs+= mTokenizer[i]->text;
                }
                break;
            }
        }
        args+=mTokenizer[i]->text;
    }
    if (!word.isEmpty()) {
        noNameArgs.append(word);
    }
    args="("+args.trimmed()+")";
    noNameArgs="("+noNameArgs.trimmed()+")";
    return addStatement(
                parent,
                fileName,
                aType,
                command,
                args,
                noNameArgs,
                value,
                line,
                kind,
                scope,
                classScope,
                properties);
}

void CppParser::addMethodParameterStatement(QStringList words, int line, const PStatement &functionStatement)
{
    if (words.isEmpty())
        return;
    QString args,suffix;
    QString cmd=words.last();
    parseCommandTypeAndArgs(cmd,suffix,args);
    words.pop_back();
    if (!cmd.isEmpty()) {
        PStatement statement = doFindStatementOf(mCurrentFile,cmd,functionStatement);
        bool noCmd = (statement && isTypeStatement(statement->kind));
        if (!noCmd) {
            addStatement(
                        functionStatement,
                        mCurrentFile,
                        words.join(" ")+" "+suffix, // 'int*'
                        cmd, // a
                        args,
                        "",
                        "",
                        line,
                        StatementKind::Parameter,
                        StatementScope::Local,
                        StatementAccessibility::None,
                        StatementProperty::HasDefinition);
        }
    }
}

void CppParser::setInheritance(int index, const PStatement& classStatement, bool isStruct, int maxIndex)
{
    // Clear it. Assume it is assigned
    StatementAccessibility lastInheritScopeType = StatementAccessibility::None;
    // Assemble a list of statements in text form we inherit from
    while (true) {
        QString currentText = mTokenizer[index]->text;
        if (currentText=='(') {
            //skip to matching ')'
            index=mTokenizer[index]->matchIndex;
        } else if (currentText=="::"
                   || (isIdentChar(currentText[0]))) {
            KeywordType keywordType = mCppKeywords.value(currentText, KeywordType::None);
            if (keywordType!=KeywordType::None) {
                StatementAccessibility inheritScopeType = getClassMemberAccessibility(mTokenizer[index]->text);
                if (inheritScopeType != StatementAccessibility::None) {
                    lastInheritScopeType = inheritScopeType;
                }
            } else {
                QString basename = currentText;
                bool isGlobal = false;
                index++;
                if (basename=="::") {
                    if (index>=maxIndex || !isIdentChar(mTokenizer[index]->text[0])) {
                        return;
                    }
                    isGlobal=true;
                    basename=mTokenizer[index]->text;
                    index++;
                }

                //remove template staff
                if (basename.endsWith('>')) {
                    int pBegin = basename.indexOf('<');
                    if (pBegin>=0)
                        basename.truncate(pBegin);
                }

                while (index+1<maxIndex
                       && mTokenizer[index]->text=="::"
                       && isIdentChar(mTokenizer[index+1]->text[0])){
                    basename += "::" + mTokenizer[index+1]->text;
                    index+=2;
                    //remove template staff
                    if (basename.endsWith('>')) {
                        int pBegin = basename.indexOf('<');
                        if (pBegin>=0)
                            basename.truncate(pBegin);
                    }
                }

                PClassInheritanceInfo inheritanceInfo = std::make_shared<ClassInheritanceInfo>();

                inheritanceInfo->derivedClass = classStatement;
                inheritanceInfo->file = mCurrentFile;
                inheritanceInfo->parentClassName = basename;
                inheritanceInfo->isGlobal = isGlobal;
                inheritanceInfo->isStruct = isStruct;
                inheritanceInfo->visibility = lastInheritScopeType;
                inheritanceInfo->handled = false;

                mClassInheritances.append(inheritanceInfo);

                handleInheritance(classStatement, inheritanceInfo);
            }
        }

        index++;
        if (index >= maxIndex)
            break;
        if (mTokenizer[index]->text.front() == '{'
                || mTokenizer[index]->text.front() == ';')
            break;
    }
}

bool CppParser::isCurrentScope(const QString &command) const
{
    PStatement statement = getCurrentScope();
    if (!statement)
        return false;
    QString s = command;
    // remove template staff
    if (s.endsWith('>')) {
        int i= command.indexOf('<');
        if (i>=0) {
            s.truncate(i);
        }
    }
    QString s2 = statement->command;
    if (s2.endsWith('>')) {
        int i= s2.indexOf('<');
        if (i>=0) {
            s2.truncate(i);
        }
    }
    return (s2 == s);
}

void CppParser::addSoloScopeLevel(PStatement& statement, int line, bool shouldResetBlock)
{
    // Add class list

    PStatement parentScope;
    if (shouldResetBlock && statement && (statement->kind == StatementKind::Block)) {
        parentScope = statement->parentScope.lock();
        while (parentScope && (parentScope->kind == StatementKind::Block)) {
            parentScope = parentScope->parentScope.lock();
        }
        if (!parentScope)
            statement.reset();
    }

    if (mMemberAccessibilities.count()>0) {
        mMemberAccessibilities.back() = mCurrentMemberAccessibility;
    }

    mCurrentScope.append(statement);

    PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(mCurrentFile);

    if (fileInfo) {
        fileInfo->addScope(line,statement);
    }

    // Set new scope
    if (!statement)
        mCurrentMemberAccessibility = StatementAccessibility::None; // {}, namespace or class that doesn't exist
    else if (statement->kind == StatementKind::Namespace)
        mCurrentMemberAccessibility = StatementAccessibility::None;
    else if (statement->type == "class")
        mCurrentMemberAccessibility = StatementAccessibility::Private; // classes are private by default
    else
        mCurrentMemberAccessibility = StatementAccessibility::Public; // structs are public by default
    mMemberAccessibilities.push_back(mCurrentMemberAccessibility);
#ifdef QT_DEBUG
//    if (mCurrentClassScope.count()<=2)
//        qDebug()<<"++add scope"<<mCurrentFile<<line<<mCurrentClassScope.count();
#endif
}

void CppParser::removeScopeLevel(int line, int maxIndex)
{
    // Remove class list
    if (mCurrentScope.isEmpty())
        return; // TODO: should be an exception
#ifdef QT_DEBUG
//    if (mCurrentClassScope.count()<=2)
//        qDebug()<<"--remove scope"<<mCurrentFile<<line<<mCurrentClassScope.count();
#endif
    PStatement currentScope = getCurrentScope();
    PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(mCurrentFile);
    if (currentScope) {
        if (currentScope->kind == StatementKind::Block) {
            if (currentScope->children.isEmpty()) {
                // remove no children block
                if (fileInfo)
                    fileInfo->removeLastScope();
                mStatementList.deleteStatement(currentScope);
            } else {
                if (fileInfo)
                    fileInfo->addStatement(currentScope);
            }
        } else if (currentScope->kind == StatementKind::Class) {
            mIndex=indexOfNextSemicolon(mIndex, maxIndex);
        }
    }
    mCurrentScope.pop_back();
    mMemberAccessibilities.pop_back();

    // Set new scope
    currentScope = getCurrentScope();
    if (fileInfo && fileInfo->lastScope()!=currentScope) {
        fileInfo->addScope(line,currentScope);
    }

    if (!currentScope) {
        mCurrentMemberAccessibility = StatementAccessibility::None;
    } else {
        mCurrentMemberAccessibility = mMemberAccessibilities.back();
    }
}

void CppParser::internalClear()
{
    mCurrentScope.clear();
    mMemberAccessibilities.clear();
    mIndex = 0;
    mCurrentMemberAccessibility = StatementAccessibility::None;
//    mBlockBeginSkips.clear();
//    mBlockEndSkips.clear();
    mInlineNamespaceEndSkips.clear();
}

QStringList CppParser::sortFilesByIncludeRelations(const QSet<QString> &files)
{
    QStringList result;
    QSet<QString> saveScannedFiles{mPreprocessor.scannedFiles()};

    //rebuild file include relations
    foreach(const QString& file, files) {
        if (mPreprocessor.fileScanned(file))
            continue;
        //already removed in interalInvalidateFiles
        //mPreprocessor.removeScannedFile(file);
        //we only use local include relations
        mPreprocessor.setScanOptions(false, true);
        mPreprocessor.preprocess(file);
        mPreprocessor.clearTempResults();
    }

    QSet<QString> fileSet=files;
    while (!fileSet.isEmpty()) {
        bool found=false;
        foreach (const QString& file,fileSet) {
            PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(file);
            bool hasInclude=false;
            if (fileInfo) {
                foreach(const QString& inc,fileInfo->includes()) {
                    if (fileSet.contains(inc)) {
                        hasInclude=true;
                        break;
                    }
                }
            }
            if (!hasInclude) {
                result.push_front(file);
                fileSet.remove(file);
                found=true;
                break;
            }
        }
        if (!found) {
            foreach (const QString& file,fileSet) {
                result.push_front(file);
                fileSet.remove(file);
            }
        }
    }
    QSet<QString> newScannedFiles{mPreprocessor.scannedFiles()};
    foreach(const QString& file, newScannedFiles) {
        if (!saveScannedFiles.contains(file))
            mPreprocessor.removeScannedFile(file);
    }
    return result;
}

int CppParser::evaluateConstExpr(int endIndex, bool &ok)
{
    int result = 0;
    if (mIndex>=endIndex) {
        ok=false;
        return 0;
    }
    result = evaluateAdditionConstExpr(endIndex,ok);
    if (mIndex!=endIndex)
        ok = false;
    return result;
}

int CppParser::evaluateAdditionConstExpr(int endIndex, bool &ok)
{
    int result = 0;
    if (mIndex>=endIndex) {
        ok=false;
        return 0;
    }
    result = evaluateMultiplyConstExpr(endIndex,ok);
    if (!ok)
        return result;
    while (mIndex<endIndex) {
        if (mTokenizer[mIndex]->text=='+') {
            mIndex++;
            int temp = evaluateMultiplyConstExpr(endIndex,ok);
            if (!ok)
                return result;
            result+=temp;
        } else if (mTokenizer[mIndex]->text=='-') {
            mIndex++;
            int temp = evaluateMultiplyConstExpr(endIndex,ok);
            if (!ok)
                return result;
            result-=temp;
        } else
            break;
    }
    return result;
}

int CppParser::evaluateMultiplyConstExpr(int endIndex, bool &ok)
{
    int result = 0;
    if (mIndex>=endIndex) {
        ok=false;
        return 0;
    }
    result = evaluateConstExprTerm(endIndex,ok);
    if (!ok)
        return result;
    while (mIndex<endIndex) {
        if (mTokenizer[mIndex]->text=='*') {
            mIndex++;
            int temp = evaluateConstExprTerm(endIndex,ok);
            if (!ok)
                return result;
            result*=temp;
        } else if (mTokenizer[mIndex]->text=='/') {
            mIndex++;
            int temp = evaluateConstExprTerm(endIndex,ok);
            if (!ok)
                return result;
            result/=temp;
        } else if (mTokenizer[mIndex]->text=='%') {
            mIndex++;
            int temp = evaluateConstExprTerm(endIndex,ok);
            if (!ok)
                return result;
            result%=temp;
        } else
            break;
    }
    return result;
}

int CppParser::evaluateConstExprTerm(int endIndex, bool &ok)
{
    int result = 0;
    if (mIndex>=endIndex) {
        ok=false;
        return 0;
    }
    if (mTokenizer[mIndex]->text=="(") {
        mIndex++;
        result = evaluateConstExpr(endIndex, ok);
        if (mIndex>=endIndex || mTokenizer[mIndex]->text!=')')
            ok=false;
        mIndex++;
    } else if (isIdentChar(mTokenizer[mIndex]->text[0])
               || mTokenizer[mIndex]->text=="::") {
        QString s = mTokenizer[mIndex]->text;
        QSet<QString> searched;
        bool isGlobal = false;
        mIndex++;
        if (s=="::") {
            if (mIndex>=endIndex || !isIdentChar(mTokenizer[mIndex]->text[0])) {
                ok=false;
                return result;
            }
            isGlobal = true;
            s+=mTokenizer[mIndex]->text;
            mIndex++;
        }
        while (mIndex+1<endIndex
               && mTokenizer[mIndex]->text=="::"
               && isIdentChar(mTokenizer[mIndex+1]->text[0])){
            s += "::" + mTokenizer[mIndex+1]->text;
            mIndex+=2;
        }
        while (true){
            //prevent infinite loop
            if (searched.contains(s)) {
                ok=false;
                return result;
            }
            searched.insert(s);
            PStatement statement = doFindStatementOf(
                        mCurrentFile,s,
                        isGlobal?PStatement():getCurrentScope());

            if (!statement) {
                ok=false;
                return result;
            }
            if (statement->kind == StatementKind::Enum) {
                result = statement->value.toInt(&ok);
                break;
            } else if (statement->kind == StatementKind::Alias) {
                s = statement->value;
            } else  {
                ok=false;
                return result;
            }
        }
    } else {
        result = evaluateLiteralNumber(endIndex,ok);
        mIndex++;
    }
    return result;
}

int CppParser::evaluateLiteralNumber(int endIndex, bool &ok)
{
    int result = 0;
    if (mIndex>=endIndex) {
        ok=false;
        return 0;
    }
    if (mTokenizer[mIndex]->text.startsWith("0x")
          || mTokenizer[mIndex]->text.startsWith("0X"))
        result = mTokenizer[mIndex]->text.mid(2).toInt(&ok,16);
    else if (mTokenizer[mIndex]->text.startsWith("0b")
          || mTokenizer[mIndex]->text.startsWith("0B"))
        result = mTokenizer[mIndex]->text.mid(2).toInt(&ok,2);
    else if (mTokenizer[mIndex]->text.startsWith("0"))
        result = mTokenizer[mIndex]->text.toInt(&ok,8);
    else
        result = mTokenizer[mIndex]->text.toInt(&ok);
    return result;
}

bool CppParser::checkForKeyword(KeywordType& keywordType)
{
    keywordType = mCppKeywords.value(mTokenizer[mIndex]->text,KeywordType::NotKeyword);
    switch(keywordType) {
    case KeywordType::Catch:
    case KeywordType::For:
    case KeywordType::Public:
    case KeywordType::Private:
    case KeywordType::Protected:
    case KeywordType::Struct:
    case KeywordType::Enum:
    case KeywordType::Inline:
    case KeywordType::Namespace:
    case KeywordType::Typedef:
    case KeywordType::Using:
    case KeywordType::Friend:
    case KeywordType::None:
    case KeywordType::NotKeyword:
    case KeywordType::DeclType:
    case KeywordType::Operator:
    case KeywordType::Requires:
    case KeywordType::Concept:
    case KeywordType::Extern:
        return false;
    default:
        return true;
    }
}

bool CppParser::checkForNamespace(KeywordType keywordType, int maxIndex)
{
    return (keywordType==KeywordType::Namespace &&(mIndex < maxIndex -1))
            || (
                keywordType==KeywordType::Inline
                && (mIndex+1 < maxIndex-1)
                &&mTokenizer[mIndex+1]->text == "namespace"
            );
}

bool CppParser::checkForPreprocessor()
{
    return (mTokenizer[mIndex]->text.startsWith('#'));
}

bool CppParser::checkForAccessibilitySpecifiers(KeywordType keywordType)
{
    return (keywordType == KeywordType::Public || keywordType == KeywordType::Protected
              || keywordType == KeywordType::Private);
}

bool CppParser::checkForStructs(KeywordType keywordType, int maxIndex)
{
    int dis = 0;

//    int keyLen = calcKeyLenForStruct(word);
//    if (keyLen<0)
//        return false;
//    bool result = (word.length() == keyLen) || isSpaceChar(word[keyLen])
//            || (word[keyLen] == '[');
    bool result;
    if (keywordType == KeywordType::Friend
            || keywordType == KeywordType::Public
            || keywordType == KeywordType::Private) {
        dis = 1;
        if (mIndex+dis>=maxIndex) {
            mIndex++;
            return false;
        }
        result = (mCppKeywords.value(mTokenizer[mIndex+dis]->text,KeywordType::None)==KeywordType::Struct);
    } else {
        result = (keywordType==KeywordType::Struct);
    }

    if (result) {
        if (mIndex >= maxIndex - 2 - dis)
            return false;
        if (mTokenizer[mIndex + 2+dis]->text[0] != ';') { // not: class something;
            int i = mIndex+dis +1;
            // the check for ']' was added because of this example:
            // struct option long_options[] = {
            //		{"debug", 1, 0, 'D'},
            //		{"info", 0, 0, 'i'},
            //    ...
            // };
            while (i < maxIndex) {
                QChar ch = mTokenizer[i]->text.back();
                if (ch=='{' || ch == ':')
                    break;
                switch(ch.unicode()) {
                case ';':
                case '{':
                case '}':
                case ',':
                case '(':
                case ')':
                case '[':
                case ']':
                case '=':
                case '*':
                case '&':
                case '%':
                case '+':
                case '-':
                case '~':
                    return false;
                }
                i++;
            }
        }
    }
    return result;
}

bool CppParser::checkForTypedefEnum(int maxIndex)
{
    //we assume that typedef is the current index, so we check the next
    //should call CheckForTypedef first!!!
    return (mIndex+1 < maxIndex ) &&
            (mTokenizer[mIndex + 1]->text == "enum");
}

bool CppParser::checkForTypedefStruct(int maxIndex)
{
    //we assume that typedef is the current index, so we check the next
    //should call CheckForTypedef first!!!
    if (mIndex+1 >= maxIndex)
        return false;
    return (mCppKeywords.value(mTokenizer[mIndex+1]->text,KeywordType::None)==KeywordType::Struct);

}

bool CppParser::checkForUsing(KeywordType keywordType, int maxIndex)
{
    return keywordType==KeywordType::Using && (mIndex < maxIndex - 1);
}

void CppParser::checkAndHandleMethodOrVar(KeywordType keywordType, int maxIndex)
{
    if (mIndex+2>=maxIndex) {
        mIndex+=2; // let's finish;
        return;
    }
    QString currentText=mTokenizer[mIndex]->text;
    if (keywordType==KeywordType::DeclType) {
        if (mTokenizer[mIndex+1]->text=='(') {
            currentText="auto";
            mIndex=mTokenizer[mIndex+1]->matchIndex+1;

        } else {
            currentText=mTokenizer[mIndex+1]->text;
            mIndex+=2;
        }
    } else {
        if (keywordType == KeywordType::Operator) {
            handleOperatorOverloading("",
                                      mIndex,
                                      false,
                                      maxIndex);
            return;
        }
        mIndex++;
    }
    //next token must be */&/word/(/{
    if (mTokenizer[mIndex]->text=='(') {
        int indexAfterParentheis=mTokenizer[mIndex]->matchIndex+1;
        if (indexAfterParentheis>=maxIndex) {
            //error
            mIndex=indexAfterParentheis;
        } else if (mTokenizer[indexAfterParentheis]->text=='(') {
            // operator overloading like (operator int)
            if (mTokenizer[mIndex+1]->text=="operator") {
                mIndex=indexAfterParentheis;
                handleMethod(StatementKind::Function,"",
                             mergeArgs(mIndex+1,mTokenizer[mIndex]->matchIndex-1),
                             indexAfterParentheis,false,false,true, maxIndex);
            } else {
                handleVar(currentText,false,false, maxIndex);
            }
        } else {
            if (currentText=="operator") {
                // operator overloading
                handleOperatorOverloading(
                            "",
                            mIndex,
                            false, maxIndex);
                return;
            }
            //check for constructor like Foo::Foo()
            QString name;
            QString temp,temp2;
            QString parentName;
            if (splitLastMember(currentText,name,temp)) {
                //has '::'
                bool isDestructor=false;
                if (!splitLastMember(temp,parentName,temp2))
                    parentName=temp;
                if (name.startsWith('~'))
                    name=name.mid(1);
                if (removeTemplateParams(name)==removeTemplateParams(parentName)) {
                    handleMethod( (isDestructor?StatementKind::Destructor:StatementKind::Constructor),
                                 "",
                                 currentText,
                                 mIndex,false,false, false, maxIndex);
                    return;
                }
            }
            // check for constructor like:
            // class Foo {
            //   Foo();
            // };
            PStatement scope=getCurrentScope();
            if (scope && scope->kind==StatementKind::Class
                    && removeTemplateParams(scope->command) == removeTemplateParams(currentText)) {
                handleMethod(StatementKind::Constructor,"",
                             currentText,
                             mIndex,false,false, false, maxIndex);
                return;
            }

            // function call, skip it
            mIndex=moveToEndOfStatement(mIndex,true, maxIndex);
        }
    } else if (mTokenizer[mIndex]->text == "*"
               || mTokenizer[mIndex]->text == "&"
               || mTokenizer[mIndex]->text=="::"
               || tokenIsIdentifier(mTokenizer[mIndex]->text)
                   ) {
        // it should be function/var

        bool isStatic = false;
        bool isFriend = false;
        bool isExtern = false;

        QString sType; // should contain type "int"
        QString sName; // should contain function name "foo::function"
        if (mTokenizer[mIndex]->text=="::") {
            mIndex--;
        } else {
            if (currentText=="::") {
                sName = currentText;
            } else {
                if (currentText == "static")
                    isStatic = true;
                else if (currentText == "friend")
                    isFriend = true;
                else if (currentText == "extern")
                    isExtern = true;
                sType = currentText;
            }
        }

        // Gather data for the string parts
        while (mIndex+1 < maxIndex) {
            if (mTokenizer[mIndex]->text=="operator") {
                handleOperatorOverloading(sType,
                                      //sName,
                                      mIndex,
                                      isStatic, maxIndex);
                return;
            } else if (mTokenizer[mIndex + 1]->text == '(') {
#ifdef ENABLE_SDCC
                if (mLanguage==ParserLanguage::SDCC && mTokenizer[mIndex]->text=="__at") {
                    if (!sName.isEmpty()) {
                        sType = sType+" "+sName;
                        sName = "";
                    }
                    sType+=" __at";
                    mIndex++;
                    int idx= mTokenizer[mIndex]->matchIndex;
                    if (idx<maxIndex) {
                        for (int i=mIndex;i<=idx;i++) {
                            sType+=mTokenizer[i]->text;
                        }
                    }
                    mIndex=idx+1;
                    continue;
                }
#endif
                if (mIndex+2<maxIndex && mTokenizer[mIndex+2]->text == '*') {
                    //foo(*blabla), it's a function pointer var
                    handleVar(sType+" "+sName,isExtern,isStatic, maxIndex);
                    return;
                }

                int indexAfter=mTokenizer[mIndex + 1]->matchIndex+1;
                if (indexAfter>=maxIndex) {
                    //error
                    mIndex=indexAfter;
                    return;
                }
                //if it's like: foo(...)(...)
                if (mTokenizer[indexAfter]->text=='(') {
                    //foo(...)(...), it's a function pointer var
                    handleVar(sType+" "+sName,isExtern,isStatic, maxIndex);
                    //Won't implement: ignore function decl like int (text)(int x) { };
                    return;
                }

                //it's not a function define
                if (mTokenizer[indexAfter]->text == ',') {
                    // var decl with init
                    handleVar(sType+" "+sName,isExtern,isStatic, maxIndex);
                    return;
                }
                if (sType!="void") {
                    //function can only be defined in global/namespaces/classes
                    PStatement currentScope=getCurrentScope();
                    if (currentScope) {
                        //in namespace, it might be function or object initilization
                        if (currentScope->kind == StatementKind::Namespace) {
                            if (isNotFuncArgs(mIndex + 1)) {
                                // var decl with init
                                handleVar(sType+" "+sName,isExtern,isStatic, maxIndex);
                                return;
                            }
                        } else if (currentScope->kind != StatementKind::Class) {
                            //not in class, it can't be a valid function definition
                            // var decl with init
                            handleVar(sType+" "+sName,isExtern,isStatic, maxIndex);
                            return;
                        }
                        //variable can't be initialized in class definition, it must be a function
                    } else if (isNotFuncArgs(mIndex + 1)){
                        // var decl with init
                        handleVar(sType+" "+sName,isExtern,isStatic, maxIndex);
                        return;
                    }
                }
                bool isDestructor = false;
                if (!sName.isEmpty()) {
                    if (sName.endsWith("::"))
                        sName+=mTokenizer[mIndex]->text;
                    else if (sName.endsWith("~")) {
                        isDestructor=true;
                        sName+=mTokenizer[mIndex]->text;
                    } else {
                        sType += " "+sName;
                        sName = mTokenizer[mIndex]->text;
                    }
                } else
                    sName = mTokenizer[mIndex]->text;
                mIndex++;

                if (isDestructor)
                    handleMethod(StatementKind::Destructor,sType,
                                 sName,mIndex,false,isFriend, false, maxIndex);
                else {
                    sType=sType.trimmed();
                    if (sType.isEmpty())
                        handleMethod(StatementKind::Constructor,sType,
                                 sName,mIndex,false,isFriend, false, maxIndex);
                    else
                        handleMethod(StatementKind::Function,sType,
                             sName,mIndex,isStatic,isFriend, false, maxIndex);
                }

                return;
            } else if (
                       mTokenizer[mIndex + 1]->text == ','
                       ||mTokenizer[mIndex + 1]->text == ';'
                       ||mTokenizer[mIndex + 1]->text == ':'
                       ||mTokenizer[mIndex + 1]->text == '{'
                       || mTokenizer[mIndex + 1]->text == '=') {
                if (mTokenizer[mIndex]->text.startsWith("[")
                        && AutoTypes.contains(sType)) {
                    handleStructredBinding(sType,maxIndex);
                    return;
                }
                handleVar(sType+" "+sName,isExtern,isStatic, maxIndex);
                return;
            } else if ( mTokenizer[mIndex + 1]->text == "::") {
                sName = sName + mTokenizer[mIndex]->text+ "::";
                mIndex+=2;
            } else if (mTokenizer[mIndex]->text == "~") {
                sName = sName + "~";
                mIndex++;
            } else {
                QString s = mTokenizer[mIndex]->text;
                if (!isWordChar(s.front())) {
                    mIndex = indexOfNextPeriodOrSemicolon(mIndex, maxIndex);
                    return;
                }
                if (sName.endsWith("::")) {
                    sName+=s;
                } else {
                    if (!sName.isEmpty()) {
                        sType = sType+" "+sName;
                        sName = "";
                    }
                    if (s == "static")
                        isStatic = true;
                    else if (s == "friend")
                        isFriend = true;
                    else if (s == "extern")
                        isExtern = true;
                    if (!s.isEmpty() && !(s=="extern")) {
                        sType = sType + ' '+ s;
                    }
                }
                mIndex++;
            }
        }
    }
}

PStatement CppParser::doFindTypeDefinitionOf(const QString &fileName, const QString &aType, const PStatement &currentClass) const
{
    if (aType.isEmpty())
        return PStatement();
    // Remove pointer stuff from type
    QString s = aType; // 'Type' is a keyword
    int position = s.length()-1;
    while ((position >= 0) && (s[position] == '*'
                               || s[position] == ' '
                               || s[position] == '&'))
        position--;
    if (position != s.length()-1)
        s.truncate(position+1);

    // Strip template stuff
    position = s.indexOf('<');
    if (position >= 0) {
        int endPos = getBracketEnd(s,position);
        s.remove(position,endPos-position+1);
    }

    // Use last word only (strip 'const', 'static', etc)
    position = s.lastIndexOf(' ');
    if (position >= 0)
        s = s.mid(position+1);

    PStatement scopeStatement = currentClass;

    PStatement statement = doFindStatementOf(fileName,s,currentClass);
    return getTypeDef(statement,fileName,aType);
}

QString CppParser::doFindFirstTemplateParamOf(const QString &fileName, const QString &phrase, const PStatement &currentScope) const
{
    return doFindTemplateParamOf(fileName,phrase,0,currentScope);
}

QString CppParser::doFindTemplateParamOf(const QString &fileName, const QString &phrase, int index, const PStatement &currentScope) const
{
    if (phrase.isEmpty())
        return QString();
    // Remove pointer stuff from type
    QString s = phrase; // 'Type' is a keyword
    int i = s.indexOf('<');
    if (i>=0) {
        i=getTemplateParamStart(s,i,index);
        int t=getTemplateParamEnd(s,i);
        //qDebug()<<index<<s<<s.mid(i,t-i)<<i<<t;
        return s.mid(i,t-i).replace(QRegularExpression("\\s+"),"");
    }
    int position = s.length()-1;
    while ((position >= 0) && (s[position] == '*'
                               || s[position] == ' '
                               || s[position] == '&'))
        position--;
    if (position != s.length()-1)
        s.truncate(position+1);

    PStatement scopeStatement = currentScope;

    PStatement statement = doFindStatementOf(fileName,s,currentScope);
    return getTemplateParam(statement,fileName, phrase,index, currentScope);
}

int CppParser::getCurrentInlineNamespaceEndSkip(int endIndex) const
{
    if (mInlineNamespaceEndSkips.isEmpty())
        return endIndex;
    return mInlineNamespaceEndSkips.back();
}

PStatement CppParser::getCurrentScope() const
{
    if (mCurrentScope.isEmpty()) {
        return PStatement();
    }
    return mCurrentScope.back();
}

void CppParser::getFullNamespace(const QString &phrase, QString &sNamespace, QString &member) const
{
    sNamespace = "";
    member = phrase;
    int strLen = phrase.length();
    if (strLen==0)
        return;
    int lastI =-1;
    int i=0;
    while (i<strLen) {
        if ((i+1<strLen) && (phrase[i]==':') && (phrase[i+1]==':') ) {
            if (!mNamespaces.contains(sNamespace)) {
                break;
            } else {
                lastI = i;
            }
        }
        sNamespace += phrase[i];
        i++;
    }
    if (i>=strLen) {
        if (mNamespaces.contains(sNamespace)) {
            sNamespace = phrase;
            member = "";
            return;
        }
    }
    if (lastI >= 0) {
        sNamespace = phrase.mid(0,lastI);
        member = phrase.mid(lastI+2);
    } else {
        sNamespace = "";
        member = phrase;
    }
}

QString CppParser::getFullStatementName(const QString &command, const PStatement& parent) const
{
    PStatement scopeStatement=parent;
    while (scopeStatement && !isNamedScope(scopeStatement->kind))
        scopeStatement = scopeStatement->parentScope.lock();
    if (scopeStatement) {
        return calcFullname(scopeStatement->fullName, command);
    } else
        return command;
}

PStatement CppParser::getIncompleteClass(const QString &command, const PStatement& parentScope)
{
    QString s=command;
    //remove template parameter
    int p = s.indexOf('<');
    if (p>=0) {
        s.truncate(p);
    }
    PStatement result = doFindStatementOf(mCurrentFile,s,parentScope);
    if (result && result->kind!=StatementKind::Class)
        return PStatement();
    return result;
}

StatementScope CppParser::getScope()
{
    // Don't blindly trust levels. Namespaces and externs can have levels too
    PStatement currentScope = getCurrentScope();

    // Invalid class or namespace/extern
    if (!currentScope || (currentScope->kind == StatementKind::Namespace))
        return StatementScope::Global;
    else if (currentScope->kind == StatementKind::Class)
        return StatementScope::ClassLocal;
    else
        return StatementScope::Local;
}

QString CppParser::getStatementKey(const QString &sName, const QString &sType, const QString &sNoNameArgs) const
{
    return sName + "--" + sType + "--" + sNoNameArgs;
}

PStatement CppParser::getTypeDef(const PStatement& statement,
                                 const QString& fileName, const QString& aType) const
{
    if (!statement) {
        return PStatement();
    }
    if (statement->kind == StatementKind::Class
            || statement->kind == StatementKind::EnumType
            || statement->kind == StatementKind::EnumClassType) {
        return statement;
    } else if (statement->kind == StatementKind::Constructor) {
        return statement->parentScope.lock();
    } else if (statement->kind == StatementKind::Typedef) {
        if (statement->type == aType) // prevent infinite loop
            return statement;
        PStatement result = doFindTypeDefinitionOf(fileName,statement->type, statement->parentScope.lock());
        if (!result) // found end of typedef trail, return result
            return statement;
        return result;
    } else if (statement->kind == StatementKind::Alias) {
        PStatement result = doFindAliasedStatement(statement);
        if (!result) // found end of typedef trail, return result
            return statement;
        return result;
    } else
        return PStatement();
}

void CppParser::doAddVar(const QString &name, const QString &type, bool isConst, const QString& suffix)
{
    if (!isIdentifier(name))
        return;
    if (type.isEmpty())
        return;
    QString realType = type;
    if (isConst)
        realType = "const "+realType;
    if (!suffix.isEmpty())
        realType += " "+suffix;
    addChildStatement(
                getCurrentScope(),
                mCurrentFile,
                realType,
                name,
                "",
                "",
                "",
                mTokenizer[mIndex]->line,
                StatementKind::Variable,
                getScope(),
                mCurrentMemberAccessibility,
                StatementProperty::HasDefinition);
}

void CppParser::handleConcept(int maxIndex)
{
    mIndex++; // skip 'concept';
    // just skip it;
    mIndex = indexOfNextSemicolonOrLeftBrace(mIndex, maxIndex);
    if (mIndex<maxIndex) {
        if (mTokenizer[mIndex]->text=='{')
            mIndex = mTokenizer[mIndex]->matchIndex+1; // skip '}'
        else
            mIndex++; // skip ;
    }
}

void CppParser::handleEnum(bool isTypedef, int maxIndex)
{
    QString enumName = "";
    bool isEnumClass = false;
    int startLine = mTokenizer[mIndex]->line;
    mIndex++; //skip 'enum'

    if (mIndex < maxIndex &&
            (mTokenizer[mIndex]->text == "class"
             || mTokenizer[mIndex]->text == "struct")) {
        //enum class
        isEnumClass = true;
        mIndex++; //skip class
    }
    bool isAdhocVar=false;
    bool isNonameEnum=false;
    int definitionEndIndex=-1;
    if ((mIndex< maxIndex) && mTokenizer[mIndex]->text.startsWith('{')) { // enum {...} NAME
        // Skip to the closing brace
        int i = indexOfMatchingBrace(mIndex);
        // Have we found the name?
        if (i + 1 < maxIndex) {
            enumName = mTokenizer[i + 1]->text.trimmed();
            if (!isIdentifierOrPointer(enumName)) {
                if (isTypedef || isEnumClass) {
                    //not a valid enum, skip to j
                    mIndex=indexOfNextSemicolon(i+1, maxIndex)+1;
                    return;
                } else
                    isNonameEnum = true;
            }
            if (!isTypedef) {
                //it's an ad-hoc enum var define;
                if (isEnumClass) {
                    //Enum class can't add hoc, just skip to ;
                    mIndex=indexOfNextSemicolon(i+1, maxIndex)+1;
                    return;
                }
                enumName = "___enum___"+enumName+"__";
                isAdhocVar=true;
            }
        }
        definitionEndIndex=i+1;
    } else if (mIndex+1< maxIndex && mTokenizer[mIndex+1]->text.startsWith('{')){ // enum NAME {...};
        enumName = mTokenizer[mIndex]->text;
        mIndex++;
    } else if (mIndex+1< maxIndex && mTokenizer[mIndex+1]->text.startsWith(':')){ // enum NAME:int {...};
        enumName = mTokenizer[mIndex]->text;
        //skip :
        mIndex = indexOfNextLeftBrace(mIndex, maxIndex);
        if (mIndex>maxIndex)
            return;
    } else {
        // enum NAME blahblah
        // it's an old c-style enum variable definition
        return;
    }

    // Add statement for enum name too
    PStatement enumStatement;
    if (!isNonameEnum) {
        if (isEnumClass) {
            enumStatement=addStatement(
                        getCurrentScope(),
                        mCurrentFile,
                        "enum class",
                        enumName,
                        "",
                        "",
                        "",
                        startLine,
                        StatementKind::EnumClassType,
                        getScope(),
                        mCurrentMemberAccessibility,
                        StatementProperty::HasDefinition);
        } else {
            enumStatement=addStatement(
                        getCurrentScope(),
                        mCurrentFile,
                        "enum",
                        enumName,
                        "",
                        "",
                        "",
                        startLine,
                        StatementKind::EnumType,
                        getScope(),
                        mCurrentMemberAccessibility,
                        isAdhocVar?(StatementProperty::HasDefinition|StatementProperty::DummyStatement)
                            :StatementProperty::HasDefinition );
        }
    }
    if (isAdhocVar) {
        //Ad-hoc var definition
        // Skip to the closing brace
        int i = indexOfMatchingBrace(mIndex)+1;
        QString typeSuffix="";
        while (i<maxIndex) {
            QString name=mTokenizer[i]->text;
            if (isIdentifierOrPointer(name)) {
                QString suffix;
                QString args;
                parseCommandTypeAndArgs(name,suffix,args);
                if (!name.isEmpty()) {
                    addStatement(
                                getCurrentScope(),
                                mCurrentFile,
                                enumName+suffix,
                                mTokenizer[i]->text,
                                args,
                                "",
                                "",
                                mTokenizer[i]->line,
                                StatementKind::Variable,
                                getScope(),
                                mCurrentMemberAccessibility,
                                StatementProperty::HasDefinition);
                }
            } else if (name!=',') {
                break;
            }
            i++;
        }
        definitionEndIndex=indexOfNextSemicolon(i, maxIndex);
    }


    // Skip opening brace
    mIndex++;

    // Call every member "enum NAME ITEMNAME"
    QString lastType;
    if (enumStatement && !isAdhocVar)
        lastType = "enum " + enumName;
    QString cmd;
    QString args;
    int value=0;
    bool canCalcValue=true;
    while ((mIndex < maxIndex) &&
                     mTokenizer[mIndex]->text!='}') {
        if (tokenIsIdentifier(mTokenizer[mIndex]->text)) {
            cmd = mTokenizer[mIndex]->text;
            args = "";
            if (mIndex+1<maxIndex &&
                    mTokenizer[mIndex+1]->text=="=") {
                mIndex+=2;
                if (mIndex<maxIndex) {
                    int tempIndex = indexOfNextPeriodOrSemicolon(mIndex, maxIndex);
                    value = evaluateConstExpr(tempIndex,canCalcValue);
                    mIndex = tempIndex - 1;
                }
            }
            if (isEnumClass) {
                if (enumStatement) {
                    addStatement(
                      enumStatement,
                      mCurrentFile,
                      lastType,
                      cmd,
                      args,
                      "",
                      canCalcValue?QString("%1").arg(value):"",
                      mTokenizer[mIndex]->line,
                      StatementKind::Enum,
                      getScope(),
                      mCurrentMemberAccessibility,
                      StatementProperty::HasDefinition);
                }
            } else {
                QString strValue=canCalcValue?QString("%1").arg(value):"";
                if (enumStatement) {
                    addStatement(
                      enumStatement,
                      mCurrentFile,
                      lastType,
                      cmd,
                      args,
                      "",
                      strValue,
                      mTokenizer[mIndex]->line,
                      StatementKind::Enum,
                      getScope(),
                      mCurrentMemberAccessibility,
                      StatementProperty::HasDefinition);
                }
                addStatement(
                            getCurrentScope(),
                            mCurrentFile,
                            lastType,
                            cmd,
                            "",
                            "",
                            strValue,
                            mTokenizer[mIndex]->line,
                            StatementKind::Enum,
                            getScope(),
                            mCurrentMemberAccessibility,
                            StatementProperty::HasDefinition);
            }
            value++;
        }
        mIndex ++ ;
    }
    if (mIndex<definitionEndIndex)
        mIndex=definitionEndIndex;
    mIndex = indexOfNextSemicolon(mIndex, maxIndex)+1;
}

void CppParser::handleForBlock(int maxIndex)
{
    mIndex++; // skip for/catch;
    if (mIndex >= maxIndex)
        return;
    if (mTokenizer[mIndex]->text!='(')
        return;
    int i=mTokenizer[mIndex]->matchIndex; //")"
    int i2 = i+1;
    if (i2>=maxIndex)
        return;
    if (mTokenizer[i2]->text=='{') {
        mTokenizer[mIndex]->text="{";
        mTokenizer[mIndex]->matchIndex = mTokenizer[i2]->matchIndex;
        mTokenizer[mTokenizer[mIndex]->matchIndex]->matchIndex = mIndex;
        mTokenizer[i]->text=";";
        mTokenizer[i2]->text=";";
    } else {
        mTokenizer[mIndex]->text=";";
        mTokenizer[i]->text=";";
        mIndex++; //skip ';'
    }
}

void CppParser::handleKeyword(KeywordType skipType, int maxIndex)
{
    // Skip
    switch (skipType) {
    case KeywordType::SkipItself:
        // skip it;
        mIndex++;
        break;
    case KeywordType::SkipNextSemicolon:
        // Skip to ; and over it
        skipNextSemicolon(mIndex, maxIndex);
        break;
    case KeywordType::SkipNextColon:
        // Skip to : and over it
        mIndex = indexOfNextColon(mIndex, maxIndex)+1;
        break;
    case KeywordType::SkipNextParenthesis:
        // skip pass ()
        skipParenthesis(mIndex, maxIndex);
        break;
    case KeywordType::MoveToLeftBrace:
        // Skip to {
        mIndex = indexOfNextLeftBrace(mIndex, maxIndex);
        break;
//    case KeywordType::MoveToRightBrace:
//        // Skip pass {}
//        mIndex = indexPassBraces(mIndex);
//        break;
    default:
        break;
    }
}

void CppParser::handleLambda(int index, int maxIndex)
{
    Q_ASSERT(mTokenizer[index]->text.startsWith('['));
    QSet<QString> captures = parseLambdaCaptures(index);
    int startLine=mTokenizer[index]->line;
    int argStart=index+1;
    int argEnd, bodyStart;
    if (mTokenizer[argStart]->text == '(' ) {
        argEnd = mTokenizer[argStart]->matchIndex;
        bodyStart=indexOfNextLeftBrace(argEnd+1, maxIndex);
        if (bodyStart>=maxIndex) {
            return;
        }
    } else if (mTokenizer[argStart]->text == '{') {
        argEnd = argStart;
        bodyStart = argStart;
    } else
        return;
    int bodyEnd = mTokenizer[bodyStart]->matchIndex;
    if (bodyEnd>maxIndex) {
        return;
    }
    PStatement lambdaBlock = addStatement(
                getCurrentScope(),
                mCurrentFile,
                "",
                "",
                "",
                "",
                "",
                startLine,
                StatementKind::Lambda,
                StatementScope::Local,
                StatementAccessibility::None,
                StatementProperty::HasDefinition);
    lambdaBlock->lambdaCaptures = captures;
    if (argEnd > argStart)
        scanMethodArgs(lambdaBlock,argStart);
    addSoloScopeLevel(lambdaBlock,mTokenizer[bodyStart]->line);
    int oldIndex = mIndex;
    mIndex = bodyStart+1;
    while (handleStatement(bodyEnd));
    Q_ASSERT(mIndex == bodyEnd);
    mIndex = oldIndex;
    removeScopeLevel(mTokenizer[bodyEnd]->line, maxIndex);
}

void CppParser::handleOperatorOverloading(const QString &sType,
                                          int operatorTokenIndex, bool isStatic, int maxIndex)
{
    //operatorTokenIndex is the token index of "operator"
    int index=operatorTokenIndex+1;
    QString op="";
    if (index>=maxIndex) {
        mIndex=index;
        return;
    }
    if (mTokenizer[index]->text=="(") {
        op="()";
        index=mTokenizer[index]->matchIndex+1;
    } else if (mTokenizer[index]->text=="new"
               || mTokenizer[index]->text=="delete") {
            op=mTokenizer[index]->text;
            index++;
            if (index<maxIndex
                    && mTokenizer[index]->text=="[]") {
                op+="[]";
                index++;
            }
    } else {
        op=mTokenizer[index]->text;
        index++;
        while (index<maxIndex
               && mTokenizer[index]->text != "(")
            index++;
    }
    while (index<maxIndex
           && mTokenizer[index]->text == ")")
        index++;
    if (index>=maxIndex
            || mTokenizer[index]->text!="(") {
        mIndex=index;
        return;
    }
    Q_ASSERT(!op.isEmpty());
    if (isIdentChar(op.front())) {
        handleMethod(StatementKind::Function,
                     sType+" "+op,
                     "operator("+op+")",
                     index,
                     isStatic,
                     false,
                     true,
                     maxIndex);
    } else {
        handleMethod(StatementKind::Function,
                 sType,
                 "operator"+op,
                 index,
                 isStatic,
                 false,
                 true,
                 maxIndex);
    }
}

void CppParser::handleMethod(StatementKind functionKind,const QString &sType, const QString &sName, int argStart, bool isStatic, bool isFriend,bool isOperatorOverload, int maxIndex)
{
    bool isValid = true;
    bool isDeclaration = false; // assume it's not a prototype
    int startLine = mTokenizer[mIndex]->line;
    int argEnd = mTokenizer[argStart]->matchIndex;

    if (mIndex >= maxIndex) // not finished define, just skip it;
        return;

    PStatement scopeStatement = getCurrentScope();

    //find start of the function body;
    bool foundColon=false;
    mIndex=argEnd+1;
    while ((mIndex < maxIndex) && !isblockChar(mTokenizer[mIndex]->text.front())) {
        if (mTokenizer[mIndex]->text=='(') {
            mIndex=mTokenizer[mIndex]->matchIndex+1;
        }else if (mTokenizer[mIndex]->text==':') {
            foundColon=true;
            break;
        } else
            mIndex++;
    }
    if (foundColon) {
        mIndex++;
        while ((mIndex < maxIndex) && !isblockChar(mTokenizer[mIndex]->text.front())) {
            if (isWordChar(mTokenizer[mIndex]->text[0])
                    && mIndex+1< maxIndex
                    && mTokenizer[mIndex+1]->text=='{') {
                //skip parent {}intializer
                mIndex=mTokenizer[mIndex+1]->matchIndex+1;
            } else if (mTokenizer[mIndex]->text=='(') {
                mIndex=mTokenizer[mIndex]->matchIndex+1;
            } else
                mIndex++;
        }
    }

    if (mIndex>=maxIndex)
        return;

    // Check if this is a prototype
    if (mTokenizer[mIndex]->text.startsWith(';')
            || mTokenizer[mIndex]->text.startsWith('}')) {// prototype
        isDeclaration = true;
    }

    QString scopelessName;
    PStatement functionStatement;
    if (isFriend && isDeclaration && scopeStatement) {
        int delimPos = sName.indexOf("::");
        if (delimPos >= 0) {
            scopelessName = sName.mid(delimPos+2);
        } else
            scopelessName = sName;
        //TODO : we should check namespace
        scopeStatement->friends.insert(scopelessName);
    } else if (isValid) {
        // Use the class the function belongs to as the parent ID if the function is declared outside of the class body
        QString scopelessName;
        QString parentClassName;
        if (!isOperatorOverload && splitLastMember(sName,scopelessName,parentClassName)) {
            // Provide Bar instead of Foo::Bar
            scopeStatement = getIncompleteClass(parentClassName,getCurrentScope());

            //parent not found
            if (!parentClassName.isEmpty() && !scopeStatement)
                scopelessName=sName;
        } else
            scopelessName = sName;

        // For function definitions, the parent class is given. Only use that as a parent
        if (!isDeclaration) {
            functionStatement=addStatement(
                        scopeStatement,
                        mCurrentFile,
                        sType,
                        scopelessName,
                        argStart,
                        argEnd,
                        "",
                        //mTokenizer[mIndex - 1]^.Line,
                        startLine,
                        functionKind,
                        getScope(),
                        mCurrentMemberAccessibility,
                        StatementProperty::HasDefinition
                        | (isStatic?StatementProperty::Static:StatementProperty::None)
                        | (isOperatorOverload?StatementProperty::OperatorOverloading:StatementProperty::None));
            scanMethodArgs(functionStatement, argStart);
            // add variable this to the class function
            if (scopeStatement && scopeStatement->kind == StatementKind::Class &&
                    !isStatic) {
                //add this to non-static class member function
                addStatement(
                            functionStatement,
                            mCurrentFile,
                            scopeStatement->command+"*",
                            "this",
                            "",
                            "",
                            "",
                            startLine,
                            StatementKind::Variable,
                            StatementScope::Local,
                            StatementAccessibility::None,
                            StatementProperty::HasDefinition
                            | (isOperatorOverload?StatementProperty::OperatorOverloading:StatementProperty::None));
            }

            // add "__func__ variable"
            addStatement(
                        functionStatement,
                        mCurrentFile,
                        "static const char ",
                        "__func__",
                        "[]",
                        "",
                        "\""+scopelessName+"\"",
                        startLine+1,
                        StatementKind::Variable,
                        StatementScope::Local,
                        StatementAccessibility::None,
                        StatementProperty::HasDefinition
                        | (isOperatorOverload?StatementProperty::OperatorOverloading:StatementProperty::None));

        } else {
            functionStatement = addStatement(
                        scopeStatement,
                        mCurrentFile,
                        sType,
                        scopelessName,
                        argStart,
                        argEnd,
                        "",
                        //mTokenizer[mIndex - 1]^.Line,
                        startLine,
                        functionKind,
                        getScope(),
                        mCurrentMemberAccessibility,
                        (isStatic?StatementProperty::Static:StatementProperty::None)
                        | (isOperatorOverload?StatementProperty::OperatorOverloading:StatementProperty::None));
        }

    }

    if ((mIndex < maxIndex) && mTokenizer[mIndex]->text.startsWith('{')) {
        addSoloScopeLevel(functionStatement,startLine);
        mIndex++; //skip '{'
    } else if ((mIndex < maxIndex) && mTokenizer[mIndex]->text.startsWith(';')) {
        addSoloScopeLevel(functionStatement,startLine);
        if (mTokenizer[mIndex]->line != startLine)
            removeScopeLevel(mTokenizer[mIndex]->line+1, maxIndex);
        else
            removeScopeLevel(startLine+1, maxIndex);
        mIndex++;
    }
}

void CppParser::handleNamespace(KeywordType skipType, int maxIndex)
{
    bool isInline=false;
    int startLine = mTokenizer[mIndex]->line;

    if (skipType==KeywordType::Inline) {
        isInline = true;
        mIndex++; //skip 'inline'
    }

    mIndex++; //skip 'namespace'

//    if (!tokenIsIdentifier(mTokenizer[mIndex]->text))
//        //wrong namespace define, stop handling
//        return;
    QString command = mTokenizer[mIndex]->text;

    QString fullName = getFullStatementName(command,getCurrentScope());
    if (isInline) {
        mInlineNamespaces.insert(fullName);
    } else if (mInlineNamespaces.contains(fullName)) {
        isInline = true;
    }
//    if (command.startsWith("__")) // hack for inline namespaces
//      isInline = true;
    mIndex++;
    if (mIndex>=maxIndex)
        return;
    QString aliasName;
    if ((mIndex+2<maxIndex) && (mTokenizer[mIndex]->text == '=')) {
        aliasName=mTokenizer[mIndex+1]->text;
        mIndex+=2;
        if (aliasName == "::" && mIndex<maxIndex) {
            aliasName += mTokenizer[mIndex]->text;
            mIndex++;
        }
        while(mIndex+1<maxIndex && mTokenizer[mIndex]->text == "::") {
            aliasName+="::";
            aliasName+=mTokenizer[mIndex+1]->text;
            mIndex+=2;
        }
        //qDebug()<<command<<aliasName;
        //namespace alias
        if (aliasName != command
                && aliasName != getFullStatementName(command, getCurrentScope())) {
            addStatement(
                getCurrentScope(),
                mCurrentFile,
                aliasName, // name of the alias namespace
                command, // command
                "", // args
                "", // noname args
                "", // values
                //mTokenizer[mIndex]^.Line,
                startLine,
                StatementKind::NamespaceAlias,
                getScope(),
                mCurrentMemberAccessibility,
                StatementProperty::HasDefinition);
        }
        mIndex++; // skip ;
        return;
    } else if (isInline) {
        //inline namespace , just skip it
        // Skip to '{'
        while ((mIndex<maxIndex) && (mTokenizer[mIndex]->text != '{'))
            mIndex++;
        int i =indexOfMatchingBrace(mIndex); //skip '}'
        if (i==mIndex)
            mInlineNamespaceEndSkips.append(maxIndex);
        else
            mInlineNamespaceEndSkips.append(i);
        if (mIndex<maxIndex)
            mIndex++; //skip '{'
    } else {
        PStatement namespaceStatement = addStatement(
                    getCurrentScope(),
                    mCurrentFile,
                    "", // type
                    command, // command
                    "", // args
                    "", // noname args
                    "", // values
                    startLine,
                    StatementKind::Namespace,
                    getScope(),
                    mCurrentMemberAccessibility,
                    StatementProperty::HasDefinition);

        // find next '{' or ';'
        mIndex = indexOfNextSemicolonOrLeftBrace(mIndex, maxIndex);
        if (mIndex<maxIndex && mTokenizer[mIndex]->text=='{')
            addSoloScopeLevel(namespaceStatement,startLine);
        //skip it
        mIndex++;
    }
}

void CppParser::handleOtherTypedefs(int maxIndex)
{
    int startLine = mTokenizer[mIndex]->line;
    // Skip typedef word
    mIndex++;

    if (mIndex>=maxIndex)
        return;

    if (mTokenizer[mIndex]->text == '('
            || mTokenizer[mIndex]->text == ','
            || mTokenizer[mIndex]->text == ';') { // error typedef
        //skip over next ;
        mIndex=indexOfNextSemicolon(mIndex, maxIndex)+1;
        return;
    }
    if ((mIndex+1<maxIndex)
            && (mTokenizer[mIndex+1]->text == ';')) {
        //no old type, not valid
        mIndex+=2; //skip ;
        return;
    }

    QString oldType;
    QString tempType;
    // Walk up to first new word (before first comma or ;)
    while(true) {
        if (oldType.endsWith("::"))
            oldType += mTokenizer[mIndex]->text;
        else if (mTokenizer[mIndex]->text=="::")
            oldType += "::";
        else if (mTokenizer[mIndex]->text=="*"
                 || mTokenizer[mIndex]->text=="&")
            tempType += mTokenizer[mIndex]->text;
        else {
            oldType += tempType + ' ' + mTokenizer[mIndex]->text;
            tempType="";
        }
        mIndex++;
        if (mIndex+1>=maxIndex) {
            //not valid, just exit
            return;
        }
        if  (mTokenizer[mIndex]->text=='(') {
            break;
        }
        if (mTokenizer[mIndex + 1]->text.front() == ','
                  || mTokenizer[mIndex + 1]->text == ';')
            break;
        //typedef function pointer

    }
    oldType = oldType.trimmed();
    if (oldType.isEmpty()) {
        //skip over next ;
        mIndex=indexOfNextSemicolon(mIndex, maxIndex)+1;
        return;
    }
    QString newType;
    while(mIndex+1<maxIndex) {
        if (mTokenizer[mIndex]->text == ',' ) {
            mIndex++;
        } else if (mTokenizer[mIndex]->text == ';' ) {
            break;
        } else if (mTokenizer[mIndex]->text == '(') {
            int paramStart=mTokenizer[mIndex]->matchIndex+1;
            if (paramStart>=maxIndex
                    || mTokenizer[paramStart]->text!='(') {
                //not valid function pointer (no args)
                //skip over next ;
                mIndex=indexOfNextSemicolon(paramStart, maxIndex)+1;
                return;
            }
            QString newType = findFunctionPointerName(mIndex);
            if (!newType.isEmpty()) {
                addStatement(
                        getCurrentScope(),
                        mCurrentFile,
                        oldType+tempType,
                        newType,
                        mergeArgs(paramStart,mTokenizer[paramStart]->matchIndex),
                        "",
                        "",
                        startLine,
                        StatementKind::Typedef,
                        getScope(),
                        mCurrentMemberAccessibility,
                        StatementProperty::HasDefinition);
                tempType="";
            }
            mIndex = mTokenizer[paramStart]->matchIndex+1;
        } else if (mTokenizer[mIndex+1]->text.front() ==','
                       || mTokenizer[mIndex+1]->text.front() ==';') {
                newType += mTokenizer[mIndex]->text;
                QString suffix;
                QString args;
                parseCommandTypeAndArgs(newType,suffix,args);

                addStatement(
                            getCurrentScope(),
                            mCurrentFile,
                            oldType+tempType+suffix,
                            newType,
                            args,
                            "",
                            "",
                            startLine,
                            StatementKind::Typedef,
                            getScope(),
                            mCurrentMemberAccessibility,
                            StatementProperty::HasDefinition);
                tempType="";
                newType = "";
                mIndex++;
        } else {
            newType += mTokenizer[mIndex]->text;
            mIndex++;
        }
    }

    // Step over semicolon (saves one HandleStatement loop)
    mIndex++;
}

void CppParser::handlePreprocessor()
{
    QString text = mTokenizer[mIndex]->text.mid(1).trimmed();
    if (text.startsWith("include")) { // start of new file
        // format: #include fullfilename:line
        // Strip keyword
        QString s = text.mid(QString("include").length());
        if (!s.startsWith(" ") && !s.startsWith("\t"))
            goto handlePreprocessorEnd;
        int delimPos = s.lastIndexOf(':');
        if (delimPos>=0) {
//            qDebug()<<mCurrentScope.size()<<mCurrentFile<<mTokenizer[mIndex]->line<<s.mid(0,delimPos).trimmed();
            mCurrentFile = s.mid(0,delimPos).trimmed();
            PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(mCurrentFile);
            if (fileInfo) {
                mCurrentFile = fileInfo->fileName();
            } else {
                mCurrentFile.squeeze();
            }
            mIsSystemHeader = isSystemHeaderFile(mCurrentFile) || isProjectHeaderFile(mCurrentFile);
            mIsProjectFile = mProjectFiles.contains(mCurrentFile);
            mIsHeader = isHFile(mCurrentFile);

            // Mention progress to user if we enter a NEW file
            bool ok;
            int line = QStringView(s.begin() + delimPos + 1, s.end()).toInt(&ok);
            if (line == 1) {
                mFilesScannedCount++;
                mFilesToScanCount++;
                emit onProgress(mCurrentFile,mFilesToScanCount,mFilesScannedCount);
            }
        }
    } else if (text.startsWith("define")) {

      // format: #define A B, remove define keyword
      QString s = text.mid(QString("define").length());
      if (!s.startsWith(" ") && !s.startsWith("\t"))
          goto handlePreprocessorEnd;
      s = s.trimmed();
      // Ask the preprocessor to cut parts up
      QString name,args,value;
      mPreprocessor.getDefineParts(s,name,args,value);

      addStatement(
                  nullptr, // defines don't belong to any scope
                  mCurrentFile,
                  "", // define has no type
                  name,
                  args,
                  "",// noname args
                  value,
                  mTokenizer[mIndex]->line,
                  StatementKind::Preprocessor,
                  StatementScope::Global,
                  StatementAccessibility::None,
                  StatementProperty::HasDefinition);
    } // TODO: undef ( define has limited scope)
handlePreprocessorEnd:
    mIndex++;
}

StatementAccessibility CppParser::getClassMemberAccessibility(const QString& text) const {
    KeywordType type = mCppKeywords.value(text,KeywordType::None);
    return getClassMemberAccessibility(type);
}

StatementAccessibility CppParser::getClassMemberAccessibility(KeywordType keywordType) const
{
    switch(keywordType) {
    case KeywordType::Public:
        return StatementAccessibility::Public;
    case KeywordType::Private:
        return StatementAccessibility::Private;
    case KeywordType::Protected:
        return StatementAccessibility::Protected;
    default:
        return StatementAccessibility::None;
    }
}

void CppParser::handleAccessibilitySpecifiers(KeywordType keywordType, int maxIndex)
{
    mCurrentMemberAccessibility = getClassMemberAccessibility(keywordType);
    mIndex++;

    if (mIndex < maxIndex
            && mTokenizer[mIndex]->text == ':')
        mIndex++;   // skip ':'
}

bool CppParser::handleStatement(int maxIndex)
{
    QString funcType,funcName;
//    int idx=getCurrentBlockEndSkip();
//    int idx2=getCurrentBlockBeginSkip();
    int idx3=getCurrentInlineNamespaceEndSkip(maxIndex);
    KeywordType keywordType;
#ifdef QT_DEBUG
//    qDebug()<<lastIndex<<mIndex;
    Q_ASSERT(mIndex>=mLastIndex);
    mLastIndex=mIndex;
#endif

    if (mIndex >= idx3) {
        //skip (previous handled) inline name space end
        mInlineNamespaceEndSkips.pop_back();
        if (mIndex == idx3)
            mIndex++;
    } else if (mTokenizer[mIndex]->text.startsWith('{')) {
        PStatement block = addStatement(
                    getCurrentScope(),
                    mCurrentFile,
                    "",
                    "",
                    "",
                    "",
                    "",
                    //mTokenizer[mIndex]^.Line,
                    mTokenizer[mIndex]->line,
                    StatementKind::Block,
                    getScope(),
                    mCurrentMemberAccessibility,
                    StatementProperty::HasDefinition);
        addSoloScopeLevel(block,mTokenizer[mIndex]->line,true);
        mIndex++;
    } else if (mTokenizer[mIndex]->text[0] == '}') {
        removeScopeLevel(mTokenizer[mIndex]->line, maxIndex);
        mIndex++;
    } else if (checkForPreprocessor()) {
        handlePreprocessor();
//    } else if (checkForLambda()) { // is lambda
//        handleLambda();
    } else if (mTokenizer[mIndex]->text=='(') {
        if (mIndex+1<maxIndex &&
                mTokenizer[mIndex+1]->text=="operator") {
            // things like (operator int)
            mIndex++; //just skip '('
        } else
            skipParenthesis(mIndex, maxIndex);
    } else if (mTokenizer[mIndex]->text==')') {
        mIndex++;
    } else if (mTokenizer[mIndex]->text.startsWith('~')) {
        //it should be a destructor
        if (mIndex+2<maxIndex
                && isIdentChar(mTokenizer[mIndex+1]->text[0])
                && mTokenizer[mIndex+2]->text=='(') {
            //dont further check to speed up
            handleMethod(StatementKind::Destructor, "", '~'+mTokenizer[mIndex+1]->text, mIndex+2, false, false, false, maxIndex);
        } else {
            //error
            mIndex=moveToEndOfStatement(mIndex,false, maxIndex);
        }
    } else if (mTokenizer[mIndex]->text=="::") {
        checkAndHandleMethodOrVar(KeywordType::None, maxIndex);
    } else if (!isIdentChar(mTokenizer[mIndex]->text[0])) {
        mIndex=moveToEndOfStatement(mIndex,true, maxIndex);
    } else if (checkForKeyword(keywordType)) { // includes template now
        handleKeyword(keywordType, maxIndex);
    } else if (keywordType==KeywordType::Concept) {
        handleConcept(maxIndex);
    } else if (keywordType==KeywordType::Requires) {
        skipRequires(maxIndex);
    } else if (keywordType==KeywordType::For || keywordType==KeywordType::Catch) { // (for/catch)
        handleForBlock(maxIndex);
    } else if (checkForAccessibilitySpecifiers(keywordType)) { // public /private/proteced
        handleAccessibilitySpecifiers(keywordType, maxIndex);
    } else if (keywordType==KeywordType::Enum) {
        handleEnum(false, maxIndex);
    } else if (keywordType==KeywordType::Typedef) {
        if (mIndex+1 < maxIndex) {
            if (checkForTypedefStruct(maxIndex)) { // typedef struct something
                mIndex++; // skip 'typedef'
                handleStructs(true, maxIndex);
            } else if (checkForTypedefEnum(maxIndex)) { // typedef enum something
                mIndex++; // skip 'typedef'
                handleEnum(true, maxIndex);
            } else
                handleOtherTypedefs(maxIndex); // typedef Foo Bar
        } else
            mIndex++;
    } else if (checkForNamespace(keywordType, maxIndex)) {
        handleNamespace(keywordType, maxIndex);
    } else if (checkForUsing(keywordType, maxIndex)) {
        handleUsing(maxIndex);
    } else if (checkForStructs(keywordType, maxIndex)) {
        handleStructs(false, maxIndex);
    } else if (keywordType == KeywordType::Inline) {
        mIndex++;
    }else {
        if (keywordType == KeywordType::Extern) {
            if (mIndex+1<maxIndex) {
                if (mTokenizer[mIndex+1]->text=="template") {
                    //extern template, skit to ;
                    //see https://en.cppreference.com/w/cpp/language/class_template#Class_template_instantiation
                    skipNextSemicolon(mIndex, maxIndex);
                    goto _exit;
                }
            }
            keywordType = KeywordType::None;
        }
        // it should be method/constructor/var
        checkAndHandleMethodOrVar(keywordType, maxIndex);
    }
    //Q_ASSERT(mIndex<999999);
_exit:
    return mIndex < maxIndex;
}

void CppParser::handleStructs(bool isTypedef, int maxIndex)
{
    bool isFriend = false;
    QString prefix = mTokenizer[mIndex]->text;
    if (prefix == "friend") {
        isFriend = true;
        mIndex++;
    }
    // Check if were dealing with a struct or union
    prefix = mTokenizer[mIndex]->text;
    bool isStruct = ("class" != prefix); //struct/union
    int startLine = mTokenizer[mIndex]->line;

    mIndex++; //skip struct/class/union

    if (mIndex>=maxIndex)
        return;

    // Do not modifiy index
    int i=indexOfNextSemicolonOrLeftBrace(mIndex, maxIndex);
    if (i >= maxIndex) {
        //error
        mIndex=i;
        return;
    }
    // Forward class/struct decl *or* typedef, e.g. typedef struct some_struct synonym1, synonym2;
    if (mTokenizer[i]->text == ";") {
        // typdef struct Foo Bar
        if (isTypedef) {
            QString structTypeName = mTokenizer[mIndex]->text;
            QString tempType = "";
            mIndex++; // skip struct/class name
            while(mIndex+1 < maxIndex) {
                // Add definition statement for the synonym
                if ( (mTokenizer[mIndex + 1]->text==","
                            || mTokenizer[mIndex + 1]->text==";")) {
                    QString newType = mTokenizer[mIndex]->text;
                    QString suffix,tempArgs;
                    parseCommandTypeAndArgs(newType,suffix,tempArgs);
                    addStatement(
                                getCurrentScope(),
                                mCurrentFile,
                                structTypeName + " "+ tempType + " " + suffix,
                                newType,
                                tempArgs, // args
                                "", // noname args
                                "", // values
                                mTokenizer[mIndex]->line,
                                StatementKind::Typedef,
                                getScope(),
                                mCurrentMemberAccessibility,
                                StatementProperty::HasDefinition);
                    tempType="";
                    mIndex++; //skip , ;
                    if (mTokenizer[mIndex]->text.front() == ';')
                        break;
                } else
                    tempType+= mTokenizer[mIndex]->text;
                mIndex++;
            }
        } else {
            if (isFriend) { // friend class
                PStatement parentStatement = getCurrentScope();
                if (parentStatement) {
                    parentStatement->friends.insert(mTokenizer[mIndex]->text);
                }
            } else {
            // todo: Forward declaration, struct Foo. Don't mention in class browser
            }
            i++; // step over ;
            mIndex = i;
        }

        // normal class/struct decl
    } else {
        PStatement firstSynonym;
        // Add class/struct name BEFORE opening brace
        if (mTokenizer[mIndex]->text != "{") {
            while(mIndex < maxIndex) {
                if (mTokenizer[mIndex]->text == ":"
                        || mTokenizer[mIndex]->text == "{"
                        || mTokenizer[mIndex]->text == ";") {
                    break;
                } else if ((mIndex + 1 < maxIndex)
                  && (mTokenizer[mIndex + 1]->text == ","
                      || mTokenizer[mIndex + 1]->text == ";"
                      || mTokenizer[mIndex + 1]->text == "{"
                      || mTokenizer[mIndex + 1]->text == ":")) {
                    QString command = mTokenizer[mIndex]->text;

                    PStatement scopeStatement=getCurrentScope();
                    QString scopelessName;
                    QString parentName;

                    if (splitLastMember(command,scopelessName,parentName)) {
                        scopeStatement = getIncompleteClass(parentName,getCurrentScope());
                    } else {
                        scopelessName=command;
                    }
                    if (!command.isEmpty()) {
                        firstSynonym = addStatement(
                                    scopeStatement,
                                    mCurrentFile,
                                    prefix, // type
                                    scopelessName, // command
                                    "", // args
                                    "", // no name args,
                                    "", // values
                                    mTokenizer[mIndex]->line,
                                    //startLine,
                                    StatementKind::Class,
                                    getScope(),
                                    mCurrentMemberAccessibility,
                                    StatementProperty::HasDefinition);
                        command = "";
                    }
                    mIndex++;
                    break;
                } else if ((mIndex + 2 < maxIndex)
                           && (mTokenizer[mIndex + 1]->text == "final")
                           && (mTokenizer[mIndex + 2]->text==","
                               || mTokenizer[mIndex + 2]->text==":"
                               || isblockChar(mTokenizer[mIndex + 2]->text.front()))) {
                    QString command = mTokenizer[mIndex]->text;
                    if (!command.isEmpty()) {
                        firstSynonym = addStatement(
                                    getCurrentScope(),
                                    mCurrentFile,
                                    prefix, // type
                                    command, // command
                                    "", // args
                                    "", // no name args
                                    "", // values
                                    mTokenizer[mIndex]->line,
                                    //startLine,
                                    StatementKind::Class,
                                    getScope(),
                                    mCurrentMemberAccessibility,
                                    StatementProperty::HasDefinition);
                        command="";
                    }
                    mIndex+=2;
                    break;
                } else
                    mIndex++;
            }
        }

        // Walk to opening brace if we encountered inheritance statements
        if ((mIndex < maxIndex) && (mTokenizer[mIndex]->text == ":")) {
            if (firstSynonym)
                setInheritance(mIndex, firstSynonym, isStruct, maxIndex); // set the _InheritanceList value
            mIndex=indexOfNextLeftBrace(mIndex, maxIndex);
        }

        // Check for struct/class synonyms after close brace

        // Walk to closing brace
        i = indexOfMatchingBrace(mIndex); // step onto closing brace

        if ((i + 1 < maxIndex) && !(
                    mTokenizer[i + 1]->text.front() == ';'
                    || mTokenizer[i + 1]->text.front() ==  '}')) {
            // When encountering names again after struct body scanning, skip it
            QString command = "";
            QString args = "";

            // Add synonym before opening brace
            while(true) {
                i++;
                if (mTokenizer[i]->text=='('
                        || mTokenizer[i]->text==')') {
                    //skip
                } else if (!(mTokenizer[i]->text == '{'
                      || mTokenizer[i]->text == ','
                      || mTokenizer[i]->text == ';')) {
                    if (mTokenizer[i]->text.endsWith(']')) { // cut-off array brackets
                        int pos = mTokenizer[i]->text.indexOf('[');
                        command += mTokenizer[i]->text.mid(0,pos) + ' ';
                        args =  mTokenizer[i]->text.mid(pos);
                    } else if (mTokenizer[i]->text == "*"
                               || mTokenizer[i]->text == "&") { // do not add spaces after pointer operator
                        command += mTokenizer[i]->text;
                    } else {
                        command += mTokenizer[i]->text + ' ';
                    }
                } else {
                    command = command.trimmed();
                    QString suffix,tempArgs;
                    parseCommandTypeAndArgs(command,suffix,tempArgs);
                    if (!command.isEmpty() &&
                            ( !firstSynonym
                              || command!=firstSynonym->command )) {
                        //not define the struct yet, we define a unamed struct
                        if (!firstSynonym) {
                            firstSynonym = addStatement(
                                        getCurrentScope(),
                                        mCurrentFile,
                                        prefix,
                                        "___dummy___"+command,
                                        "",
                                        "",
                                        "",
                                        mTokenizer[i]->line,
                                        //startLine,
                                        StatementKind::Class,
                                        getScope(),
                                        mCurrentMemberAccessibility,
                                        StatementProperty::HasDefinition | StatementProperty::DummyStatement);
                        }
                        if (isTypedef) {
                            //typedef
                            addStatement(
                                        getCurrentScope(),
                                        mCurrentFile,
                                        firstSynonym->command+suffix,
                                        command,
                                        args+tempArgs,
                                        "",
                                        "",
                                        mTokenizer[mIndex]->line,
                                        StatementKind::Typedef,
                                        getScope(),
                                        mCurrentMemberAccessibility,
                                        StatementProperty::HasDefinition); // typedef
                        } else {
                            //variable define
                            addStatement(
                              getCurrentScope(),
                              mCurrentFile,
                              firstSynonym->command+suffix,
                              command,
                              args+tempArgs,
                              "",
                              "",
                              mTokenizer[i]->line,
                              StatementKind::Variable,
                              getScope(),
                              mCurrentMemberAccessibility,
                              StatementProperty::HasDefinition); // TODO: not supported to pass list
                        }
                    }
                    command = "";
                }
                if (i >= maxIndex - 1)
                    break;
                if (mTokenizer[i]->text=='{'
                      || mTokenizer[i]->text== ';')
                    break;
            }

          // Nothing worth mentioning after closing brace
          // Proceed to set first synonym as current class
        }
        if (!firstSynonym) {
            PStatement scope = getCurrentScope();
            if (scope && scope->kind == StatementKind::Class
                    && mIndex<maxIndex && mTokenizer[mIndex]->text=="{") {
                //C11 anonymous union/struct
                addSoloScopeLevel(scope, mTokenizer[mIndex]->line);
                //skip {
                mIndex++;
                return;
            } else {
                //anonymous union/struct/class, add as a block
                firstSynonym=addStatement(
                          getCurrentScope(),
                          mCurrentFile,
                          "",
                          "",
                          "",
                          "",
                          "",
                          mTokenizer[mIndex]->line,
                          StatementKind::Block,
                          getScope(),
                          mCurrentMemberAccessibility,
                          StatementProperty::HasDefinition);
            }
        }
        if (mIndex < maxIndex)
            addSoloScopeLevel(firstSynonym,mTokenizer[mIndex]->line);
        else
            addSoloScopeLevel(firstSynonym,startLine);

        // Step over {
        if ((mIndex < maxIndex) && (mTokenizer[mIndex]->text == "{"))
            mIndex++;
    }
}

void CppParser::handleStructredBinding(const QString &sType, int maxIndex)
{
    if (mIndex+1 < maxIndex
            && ((mTokenizer[mIndex+1]->text == ":")
                || (mTokenizer[mIndex+1]->text == "="))) {
        QString typeName;
        QString templateParams;
        int endIndex = indexOfNextSemicolon(mIndex+2, maxIndex);
        QString expressionText;
        for (int i=mIndex+2;i<endIndex;i++) {
            expressionText+=mTokenizer[i]->text+" ";
        }
        QStringList phraseExpression = splitExpression(expressionText);
        int pos = 0;
        PEvalStatement aliasStatement = doEvalExpression(mCurrentFile,
                                phraseExpression,
                                pos,
                                getCurrentScope(),
                                PEvalStatement(),
                                true,false);
        if(aliasStatement && aliasStatement->effectiveTypeStatement) {
            if ( mTokenizer[mIndex+1]->text == ":" ) {
                if (STLMaps.contains(aliasStatement->effectiveTypeStatement->fullName)) {
                    typeName = "std::pair";
                    templateParams = aliasStatement->templateParams;
                }
            }
            if (typeName == "std::pair" && !templateParams.isEmpty()) {
                QString firstType = doFindFirstTemplateParamOf(mCurrentFile,aliasStatement->templateParams,
                                                                                  getCurrentScope());
                QString secondType = doFindTemplateParamOf(mCurrentFile,aliasStatement->templateParams,1,
                                                                                  getCurrentScope());
                QString s = mTokenizer[mIndex]->text;
                s = s.mid(1,s.length()-2);
                QStringList lst = s.split(",");
                if (lst.length()==2) {
                    QString firstVar = lst[0].trimmed();
                    QString secondVar = lst[1].trimmed();
                    bool isConst = sType.startsWith("const");
                    QString suffix;
                    if (sType.endsWith("&&")) suffix = "&&";
                    else if (sType.endsWith("&")) suffix = "&";
                    doAddVar(firstVar, firstType, isConst, suffix);
                    doAddVar(secondVar, secondType, isConst, suffix);
                }
            }
        }
    }
    mIndex = indexOfNextPeriodOrSemicolon(mIndex+1, maxIndex);
}

void CppParser::handleUsing(int maxIndex)
{
    int startLine = mTokenizer[mIndex]->line;
    if (mCurrentFile.isEmpty()) {
        //skip pass next ;
        mIndex=indexOfNextSemicolon(mIndex, maxIndex)+1;
        return;
    }

    mIndex++; //skip 'using'

    //handle things like 'using vec = std::vector; '
    if (mIndex+1 < maxIndex
            && mTokenizer[mIndex+1]->text == "=") {
        QString fullName = mTokenizer[mIndex]->text;
        QString aliasName;
        mIndex+=2;
        while (mIndex<maxIndex &&
               mTokenizer[mIndex]->text!=';') {
            aliasName += mTokenizer[mIndex]->text;
            mIndex++;
        }
        addStatement(
                    getCurrentScope(),
                    mCurrentFile,
                    aliasName, // name of the alias (type)
                    fullName, // command
                    "", // args
                    "", // noname args
                    "", // values
                    startLine,
                    StatementKind::Typedef,
                    getScope(),
                    mCurrentMemberAccessibility,
                    StatementProperty::HasDefinition);
        // skip ;
        mIndex++;
        return;
    }
    //handle things like 'using std::vector;'
    if ((mIndex+2>=maxIndex)
            || (mTokenizer[mIndex]->text != "namespace")) {
        QString fullName;
        QString usingName;
        bool appendUsingName = false;
        while (mIndex<maxIndex &&
               mTokenizer[mIndex]->text!=';') {
            fullName += mTokenizer[mIndex]->text;
            if (!appendUsingName) {
                usingName = mTokenizer[mIndex]->text;
                if (usingName == "operator") {
                    appendUsingName=true;
                }
            } else {
                usingName += mTokenizer[mIndex]->text;
            }
            mIndex++;
        }
        if (fullName!=usingName) {
            addStatement(
                        getCurrentScope(),
                        mCurrentFile,
                        fullName, // name of the alias (type)
                        usingName, // command
                        "", // args
                        "", // noname args
                        "", // values
                        startLine,
                        StatementKind::Alias,
                        getScope(),
                        mCurrentMemberAccessibility,
                        StatementProperty::HasDefinition);
        }
        //skip ;
        mIndex++;
        return;
    }
    mIndex++;  // skip 'namespace'
    PStatement scopeStatement = getCurrentScope();

    QString usingName;
    while (mIndex<maxIndex &&
           mTokenizer[mIndex]->text!=';') {
        usingName += mTokenizer[mIndex]->text;
        mIndex++;
    }

    if (scopeStatement) {
        QString fullName = calcFullname(scopeStatement->fullName, usingName);

        if (!mNamespaces.contains(fullName)) {
            fullName = usingName;
        }
        if (mNamespaces.contains(fullName)) {
            scopeStatement->usingList.insert(fullName);
        }
    } else {
        PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(mCurrentFile);
        if (!fileInfo)
            return;
        if (mNamespaces.contains(usingName)) {
            fileInfo->addUsing(usingName);
        }
    }
    //skip ;
    mIndex++;
}

void CppParser::handleVar(const QString& typePrefix,bool isExtern,bool isStatic, int maxIndex)
{
    QString lastType;
    if (typePrefix=="extern") {
        isExtern=true;
    } else if (typePrefix=="static") {
        isStatic=true;
    } else {
        if (typePrefix.back()==':')
            return;
        lastType=typePrefix.trimmed();
    }

    PStatement addedVar;

    QString tempType;
    while (lastType.endsWith("*") || lastType.endsWith("&")) {
        tempType = (lastType.back()+tempType);
        lastType.truncate(lastType.length()-2);
    }

    while(mIndex<maxIndex) {
        switch(mTokenizer[mIndex]->text[0].unicode()) {
        case ':':
            if (mTokenizer[mIndex]->text.length()>1) {
                //handle '::'
                tempType+=mTokenizer[mIndex]->text;
                mIndex++;
            } else {
                // Skip bit identifiers,
                // e.g.:
                // handle
                // unsigned short bAppReturnCode:8,reserved:6,fBusy:1,fAck:1
                // as
                // unsigned short bAppReturnCode,reserved,fBusy,fAck
                if (mIndex+1<maxIndex
                        && isIdentChar(mTokenizer[mIndex+1]->text.front())
                        && (isIdentChar(mTokenizer[mIndex+1]->text.back()) || isDigitChar(mTokenizer[mIndex+1]->text.back()))
                        && addedVar
                        && !(addedVar->properties & StatementProperty::FunctionPointer)
                        && AutoTypes.contains(addedVar->type)) {
                    //handle e.g.: for(auto x:vec)
                    // for(auto x:vec ) is replaced by "for { auto x: vec ;"  in handleForAndCatch();
                    int endIndex = indexOfNextSemicolon(mIndex+1, maxIndex);
                    QString expressionText;
                    for (int i=mIndex+1;i<endIndex;i++) {
                        expressionText+=mTokenizer[i]->text+" ";
                    }
                    QStringList phraseExpression = splitExpression(expressionText);
                    int pos = 0;
                    PEvalStatement aliasStatement = doEvalExpression(mCurrentFile,
                                            phraseExpression,
                                            pos,
                                            getCurrentScope(),
                                            PEvalStatement(),
                                            true,false);
                    if(aliasStatement && aliasStatement->effectiveTypeStatement) {
                        if (STLMaps.contains(aliasStatement->effectiveTypeStatement->fullName)) {
                            addedVar->type = "std::pair"+aliasStatement->templateParams;
                        } else if (STLContainers.contains(aliasStatement->effectiveTypeStatement->fullName)){
                            QString type=doFindFirstTemplateParamOf(mCurrentFile,aliasStatement->templateParams,
                                                                  getCurrentScope());
                            if (!type.isEmpty())
                                addedVar->type = type;
                        }
                    }
                    mIndex=endIndex;
                }
                addedVar.reset();
                bool should_exit=false;
                while (mIndex < maxIndex) {
                    switch(mTokenizer[mIndex]->text[0].unicode()) {
                    case ',':
                    case ';':
                    case '=':
                        should_exit = true;
                        break;
                    case ')':
                        mIndex++;
                        return;
                    case '(':
                        mIndex=mTokenizer[mIndex]->matchIndex+1;
                        break;
                    default:
                        mIndex++;
                    }
                    if (should_exit)
                        break;
                }
            }
            break;
        case ';':
            mIndex++;
            return;
        case '=':
            if (mIndex+1<maxIndex
                    && mTokenizer[mIndex+1]->text!="{"
                    && addedVar
                    && !(addedVar->properties & StatementProperty::FunctionPointer)
                    && AutoTypes.contains(addedVar->type)) {
                //handle e.g.: auto x=blahblah;
                int pos = 0;

                int endIndex = skipAssignment(mIndex, maxIndex);
                QString expressionText;
                for (int i=mIndex+1;i<endIndex;i++) {
                    expressionText.append(mTokenizer[i]->text);
                    expressionText.append(" ");
                }
                QStringList phraseExpression = splitExpression(expressionText);
                PEvalStatement aliasStatement = doEvalExpression(mCurrentFile,
                                        phraseExpression,
                                        pos,
                                        getCurrentScope(),
                                        PEvalStatement(),
                                        true,false);
                if(aliasStatement) {
                    if (aliasStatement->typeStatement) {
                        addedVar->type = aliasStatement->typeStatement->fullName;
                        if (!aliasStatement->templateParams.isEmpty()) {
                            if (!addedVar->type.endsWith(">")) {
                                addedVar->type += aliasStatement->templateParams;
                            } else {
                                QString type = addedVar->type;
                                int pos = type.indexOf('<');
                                if (pos>=0) {
                                    type = type.left(pos);
                                    addedVar->type = type + aliasStatement->templateParams;
                                }
                            }
                        }
                        if (aliasStatement->typeStatement
                                && STLIterators.contains(aliasStatement->typeStatement->command)
                                && !aliasStatement->templateParams.isEmpty()) {
                            PStatement parentStatement = aliasStatement->typeStatement->parentScope.lock();
                            if (parentStatement
                                    && STLContainers.contains(parentStatement->fullName)) {
                                addedVar->type = parentStatement->fullName+aliasStatement->templateParams+"::"+aliasStatement->typeStatement->command;
                            } else if (parentStatement
                                    && STLMaps.contains(parentStatement->fullName)) {
                                addedVar->type = parentStatement->fullName+aliasStatement->templateParams+"::"+aliasStatement->typeStatement->command;
                            }
                        }
                    } else
                        addedVar->type = aliasStatement->baseType + aliasStatement->templateParams;
                    if (aliasStatement->pointerLevel>0)
                        addedVar->type += QString(aliasStatement->pointerLevel,'*');
                }
                mIndex = endIndex;
            } else
                mIndex = skipAssignment(mIndex, maxIndex);
            addedVar.reset();
            break;
        case '*':
        case '&':
            tempType+=mTokenizer[mIndex]->text;
            mIndex++;
            break;
        case ',':
            tempType="";
            mIndex++;
            break;
        case '(':
            if (mTokenizer[mIndex]->matchIndex+1<maxIndex
                    && mTokenizer[mTokenizer[mIndex]->matchIndex+1]->text=='(') {
                        //function pointer
                QString cmd = findFunctionPointerName(mIndex);
                int argStart=mTokenizer[mIndex]->matchIndex+1;
                int argEnd=mTokenizer[argStart]->matchIndex;

                if (!cmd.isEmpty()) {
                    QString type=lastType;
                    if(type.endsWith("::"))
                        type+=tempType.trimmed();
                    else
                        type+=" "+tempType.trimmed();

                    addChildStatement(
                                getCurrentScope(),
                                mCurrentFile,
                                type.trimmed(),
                                cmd,
                                mergeArgs(argStart,argEnd),
                                "",
                                "",
                                mTokenizer[mIndex]->line,
                                StatementKind::Variable,
                                getScope(),
                                mCurrentMemberAccessibility,
                                (isExtern?StatementProperty::None:StatementProperty::HasDefinition)
                                | (isStatic?StatementProperty::Static:StatementProperty::None)
                                | StatementProperty::FunctionPointer);
                }
                addedVar.reset();
                tempType="";
                mIndex=indexOfNextPeriodOrSemicolon(argEnd+1, maxIndex);
                break;
            }
            //not function pointer, fall through
            [[fallthrough]];
        case '{':
            tempType="";
            if (mIndex+1<maxIndex
                    && isIdentifier(mTokenizer[mIndex+1]->text)
                    && addedVar
                    && !(addedVar->properties & StatementProperty::FunctionPointer)
                    && AutoTypes.contains(addedVar->type)) {
                int pos = 0;
                int endIndex = mTokenizer[mIndex]->matchIndex;
                QString expressionText;
                for (int i=mIndex+1;i<endIndex;i++) {
                    expressionText.append(mTokenizer[i]->text);
                    expressionText.append(" ");
                }
                QStringList phraseExpression = splitExpression(expressionText);
                PEvalStatement aliasStatement = doEvalExpression(mCurrentFile,
                                        phraseExpression,
                                        pos,
                                        getCurrentScope(),
                                        PEvalStatement(),
                                        true,false);
                if(aliasStatement  && aliasStatement->effectiveTypeStatement) {
                    if (aliasStatement->typeStatement) {
                        addedVar->type = aliasStatement->typeStatement->fullName;
                        if (!addedVar->type.endsWith(">"))
                            addedVar->type += aliasStatement->templateParams;
                        if (aliasStatement->typeStatement
                                && STLIterators.contains(aliasStatement->typeStatement->command)) {
                            PStatement parentStatement = aliasStatement->typeStatement->parentScope.lock();
                            if (parentStatement
                                    && STLContainers.contains(parentStatement->fullName)) {
                                addedVar->type = parentStatement->fullName+aliasStatement->templateParams+"::"+aliasStatement->typeStatement->command;
                            } else if (parentStatement
                                    && STLMaps.contains(parentStatement->fullName)) {
                                addedVar->type = parentStatement->fullName+aliasStatement->templateParams+"::"+aliasStatement->typeStatement->command;
                            }
                        }
                    } else
                        addedVar->type = aliasStatement->baseType  + aliasStatement->templateParams;
                    if (aliasStatement->pointerLevel>0)
                        addedVar->type += QString(aliasStatement->pointerLevel,'*');
                }
            }
            mIndex=mTokenizer[mIndex]->matchIndex+1;
            addedVar.reset();
            //If there are multiple var define in the same line, the next token should be ','
            if (mIndex>=maxIndex || mTokenizer[mIndex]->text != ",")
                return;
            break;
        default:
            if (isIdentChar(mTokenizer[mIndex]->text[0])) {
                QString cmd=mTokenizer[mIndex]->text;
                //normal var
                if (cmd=="const") {
                    if (tempType.isEmpty()) {
                        tempType = cmd;
                    } else if (tempType.endsWith("*")) {
                        tempType+=cmd;
                    } else {
                        tempType+=" "+cmd;
                    }
                } else {
                    QString suffix;
                    QString args;
                    cmd=mTokenizer[mIndex]->text;
                    parseCommandTypeAndArgs(cmd,suffix,args);
                    if (!cmd.isEmpty() && !isKeyword(cmd)) {
                        addedVar = addChildStatement(
                                    getCurrentScope(),
                                    mCurrentFile,
                                    (lastType+' '+tempType+suffix).trimmed(),
                                    cmd,
                                    args,
                                    "",
                                    "",
                                    mTokenizer[mIndex]->line,
                                    StatementKind::Variable,
                                    getScope(),
                                    mCurrentMemberAccessibility,
                                    (isExtern?StatementProperty::None:StatementProperty::HasDefinition)
                                    | (isStatic?StatementProperty::Static:StatementProperty::None));
                        tempType="";
                    }
                }
                mIndex++;
            } else {
                mIndex = indexOfNextPeriodOrSemicolon(mIndex, maxIndex);
                return;
            }
        }
    }
    // Skip ;
    mIndex++;
}

void CppParser::handleInheritance(PStatement derivedStatement, PClassInheritanceInfo inheritanceInfo)
{
    if (inheritanceInfo->handled)
        return;
    PStatement statement = doFindStatementOf(
                inheritanceInfo->file,
                inheritanceInfo->parentClassName,
                inheritanceInfo->isGlobal?PStatement():derivedStatement->parentScope.lock());

    if (statement && statement->kind == StatementKind::Class) {
        inheritClassStatement(derivedStatement,
                              inheritanceInfo->isStruct,
                              statement,
                              inheritanceInfo->visibility);
//        inheritanceInfo->parentClassFilename = statement->fileName;
        inheritanceInfo->handled = true;
        PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(statement->fileName);
        Q_ASSERT(fileInfo!=nullptr);
        fileInfo->addHandledInheritances(inheritanceInfo);
    }

}

void CppParser::handleInheritances()
{
    for (int i=mClassInheritances.length()-1;i>=0;i--) {
        PClassInheritanceInfo inheritanceInfo = mClassInheritances[i];
        PStatement derivedStatement = inheritanceInfo->derivedClass.lock();
        if (!derivedStatement) {
            //the derived class is invalid, remove it
            if (i<mClassInheritances.length()-1) {
                mClassInheritances[i]=mClassInheritances.back();
            } else {
                mClassInheritances.pop_back();
            }
            continue;
        }
        handleInheritance(derivedStatement, inheritanceInfo);
    }
    //mClassInheritances.clear();
}

void CppParser::skipRequires(int maxIndex)
{
    mIndex++; //skip 'requires';

    while (mIndex < maxIndex) { // ||
        while (mIndex < maxIndex) { // &&
            if (mTokenizer[mIndex]->text=='(') {
                //skip parenthesized expression
                mIndex = mTokenizer[mIndex]->matchIndex+1;
            } else if (isIdentifier(mTokenizer[mIndex]->text)) {
                // skip foo<T> or foo::boo::ttt<T>
                while (mIndex < maxIndex) {
                    if (!isIdentifier(mTokenizer[mIndex]->text))
                        return;
                    mIndex++;
                    if (mIndex>=maxIndex)
                        return;
                    if (mTokenizer[mIndex]->text!="::")
                        break;
                    mIndex++; // skip '::';
                }
            }
            if (mIndex+1>=maxIndex)
                return;
            if (mTokenizer[mIndex]->text!="&" || mTokenizer[mIndex+1]->text!="&")
                break;
            mIndex+=2; // skip '&&';
        }
        if (mIndex+1>=maxIndex)
            return;
        if (mTokenizer[mIndex]->text!="|" || mTokenizer[mIndex+1]->text!="|")
            break;
        mIndex+=2; // skip '||';
    }
}

void CppParser::internalParse(const QString &fileName)
{
    // Perform some validation before we start
    if (!mEnabled)
        return;
//    if (!isCfile(fileName) && !isHfile(fileName))  // support only known C/C++ files
//        return;

    //QElapsedTimer timer;
    // Preprocess the file...
    auto action = finally([this]{
        mTokenizer.clear();
    });
    //timer.start();
    // Let the preprocessor augment the include records
    mPreprocessor.setScanOptions(mParseGlobalHeaders, mParseLocalHeaders);
    mPreprocessor.preprocess(fileName);

    QStringList preprocessResult = mPreprocessor.result();
#ifdef QT_DEBUG
       // stringsToFile(mPreprocessor.result(),QString("z:\\preprocess-%1.txt").arg(extractFileName(fileName)));
       // mPreprocessor.dumpDefinesTo("z:\\defines.txt");
       // mPreprocessor.dumpIncludesListTo("z:\\includes.txt");
#endif
    //qDebug()<<"preprocess"<<timer.elapsed();
    //reduce memory usage
    //timer.restart();
    mPreprocessor.clearTempResults();
    //qDebug()<<"preprocess clean"<<timer.elapsed();

    //timer.restart();
    // Tokenize the preprocessed buffer file
    mTokenizer.tokenize(preprocessResult);
    //reduce memory usage
    preprocessResult.clear();
    //qDebug()<<"tokenize"<<timer.elapsed();
    if (mTokenizer.tokenCount() == 0)
        return;
#ifdef QT_DEBUG
       // mTokenizer.dumpTokens(QString("z:\\tokens-%1.txt").arg(extractFileName(fileName)));
#endif
#ifdef QT_DEBUG
        mLastIndex = -1;
#endif
    //    timer.restart();
    // Process the token list
    int endIndex = mTokenizer.tokenCount();
    while(true) {
        if (!handleStatement(endIndex))
            break;
    }
#ifdef QT_DEBUG
       // mTokenizer.dumpTokens(QString("z:\\tokens-after-%1.txt").arg(extractFileName(fileName)));
#endif
    handleInheritances();
    //    qDebug()<<"parse"<<timer.elapsed();
#ifdef QT_DEBUG
       // mStatementList.dumpAll(QString("z:\\all-stats-%1.txt").arg(extractFileName(fileName)));
       // mStatementList.dump(QString("z:\\stats-%1.txt").arg(extractFileName(fileName)));
#endif
    //reduce memory usage
    internalClear();
}

void CppParser::inheritClassStatement(const PStatement& derived, bool isStruct,
                                      const PStatement& base, StatementAccessibility access)
{
    //differentiate class and struct
    if (access == StatementAccessibility::None) {
        if (isStruct)
            access = StatementAccessibility::Public;
        else
            access = StatementAccessibility::Private;
    }
    foreach (const PStatement& statement, base->children) {
        if (statement->accessibility == StatementAccessibility::Private
                || statement->kind == StatementKind::Constructor
                || statement->kind == StatementKind::Destructor)
            continue;
        if (derived->children.contains(statement->command)) {
            // Check if it's overwritten(hidden) by the derived
            QList<PStatement> children = derived->children.values(statement->command);
            bool overwritten = false;
            foreach(const PStatement& child, children) {
                if (!child->isInherited() && child->noNameArgs == statement->noNameArgs) {
                    overwritten = true;
                    break;
                }
            }
            if (overwritten)
                continue;
        }
        StatementAccessibility m_acc;
        switch(access) {
        case StatementAccessibility::Public:
            m_acc = statement->accessibility;
            break;
        case StatementAccessibility::Protected:
            m_acc = StatementAccessibility::Protected;
            break;
        case StatementAccessibility::Private:
            m_acc = StatementAccessibility::Private;
            break;
        default:
            m_acc = StatementAccessibility::Private;
        }
        //inherit
        addInheritedStatement(derived,statement,m_acc);
    }
}

QList<PStatement> CppParser::getListOfFunctions(const QString &fileName, int line, const PStatement &statement, const PStatement &scopeStatement) const
{
    QList<PStatement> result;
    StatementMap children = mStatementList.childrenStatements(scopeStatement);
    QSet<QString> includedFiles = internalGetIncludedFiles(fileName);
    for (const PStatement& child:children) {
        if (statement->command == child->command) {
            if (child->kind == StatementKind::Alias)
                continue;
            if (!includedFiles.contains(fileName))
                continue;
            if (line < child->line && (child->fileName == fileName))
                continue;
            result.append(child);
        }
    }
    return result;
}

PStatement CppParser::findMacro(const QString &phrase, const QString &fileName) const
{
    const StatementMap& statementMap =mStatementList.childrenStatements(nullptr);
    if (statementMap.isEmpty())
        return PStatement();
    StatementList statements = statementMap.values(phrase);
    PParsedFileInfo includes = mPreprocessor.findFileInfo(fileName);
    foreach (const PStatement& s, statements) {
        if (s->kind == StatementKind::Preprocessor) {
            if (includes && fileName != s->fileName && !includes->including(s->fileName))
                continue;
            return s;
        }
    }
    return PStatement();
}

PStatement CppParser::findMemberOfStatement(const QString &phrase,
                                            const PStatement& scopeStatement) const
{
    const StatementMap& statementMap =mStatementList.childrenStatements(scopeStatement);
    if (statementMap.isEmpty())
        return PStatement();

    QString s = phrase;
    //remove []
    int p = phrase.indexOf('[');
    if (p>=0)
        s.truncate(p);
    //remove ()
    p = phrase.indexOf('(');
    if (p>=0)
        s.truncate(p);

    //remove <>
    p =s.indexOf('<');
    if (p>=0)
        s.truncate(p);

    return statementMap.value(s,PStatement());
}


PStatement CppParser::findMemberOfStatement(const QString& filename,
                                            const QString &phrase,
                                            const PStatement& scopeStatement) const
{
    const StatementMap& statementMap =mStatementList.childrenStatements(scopeStatement);
    if (statementMap.isEmpty())
        return PStatement();

    QString s = phrase;
    //remove []
    int p = phrase.indexOf('[');
    if (p>=0)
        s.truncate(p);
    //remove ()
    p = phrase.indexOf('(');
    if (p>=0)
        s.truncate(p);

    //remove <>
    p =s.indexOf('<');
    if (p>=0)
        s.truncate(p);
    if (scopeStatement) {
        return statementMap.value(s,PStatement());
    } else {
        QList<PStatement> stats = statementMap.values(s);
        PParsedFileInfo fileInfo =  mPreprocessor.findFileInfo(filename);
        foreach(const PStatement &s,stats) {
            if (s->line==-1) {
                return s; // hard defines
            } if (s->fileName == filename || s->definitionFileName==filename) {
                return s;
            } else if (fileInfo && (fileInfo->including(s->fileName)
                    || fileInfo->including(s->definitionFileName))) {
                return s;
            }
        }
        return PStatement();
    }
}

QList<PStatement> CppParser::findMembersOfStatement(const QString &phrase, const PStatement &scopeStatement) const
{
    const StatementMap& statementMap =mStatementList.childrenStatements(scopeStatement);
    if (statementMap.isEmpty())
        return QList<PStatement>();

    QString s = phrase;
    //remove []
    int p = phrase.indexOf('[');
    if (p>=0)
        s.truncate(p);
    //remove ()
    p = phrase.indexOf('(');
    if (p>=0)
        s.truncate(p);

    //remove <>
    p =s.indexOf('<');
    if (p>=0)
        s.truncate(p);

    return statementMap.values(s);
}

PStatement CppParser::findStatementInScope(const QString &name, const QString &noNameArgs,
                                           StatementKind kind, const PStatement& scope) const
{
    if (scope && scope->kind == StatementKind::Namespace) {
        PStatementList namespaceStatementsList = doFindNamespace(scope->command);
        if (!namespaceStatementsList)
            return PStatement();
        foreach (const PStatement& namespaceStatement, *namespaceStatementsList) {
            PStatement result=doFindStatementInScope(name,noNameArgs,kind,namespaceStatement);
            if (result)
                return result;
        }
    } else {
        return doFindStatementInScope(name,noNameArgs,kind,scope);
    }
    return PStatement();
}

PStatement CppParser::findStatementInScope(const QString &name, const PStatement &scope) const
{
    if (!scope)
        return findMemberOfStatement(name,scope);
    if (scope->kind == StatementKind::Namespace) {
        return findStatementInNamespace(name, scope->fullName);
    } else {
        return findMemberOfStatement(name,scope);
    }
}

PStatement CppParser::findStatementInNamespace(const QString &name, const QString &namespaceName) const
{
    PStatementList namespaceStatementsList=doFindNamespace(namespaceName);
    if (!namespaceStatementsList)
        return PStatement();
    foreach (const PStatement& namespaceStatement,*namespaceStatementsList) {
        PStatement result = findMemberOfStatement(name,namespaceStatement);
        if (result)
            return result;
    }
    return PStatement();
}

PEvalStatement CppParser::doEvalExpression(const QString& fileName,
                                       QStringList phraseExpression,
                                       int &pos,
                                       const PStatement& scope,
                                       const PEvalStatement& previousResult,
                                       bool freeScoped,
                                       bool expandMacros) const
{
    if (expandMacros) {
        QList<QSet<QString>> usedMacros;
        usedMacros.reserve(phraseExpression.length());
        for(int i=0;i<phraseExpression.length();i++) {
            usedMacros.append(QSet<QString>());
        }
        int i=pos;
        while(i<phraseExpression.length()) {
            QString word = phraseExpression[i];
            if (isIdentifier(word)) {
                QSet<QString> used = usedMacros[i];
                if (!used.contains(word)) {
                    PStatement macro = findMacro(word, fileName);
                    if (macro) {
                        if(!expandMacro(phraseExpression, i,  macro, usedMacros))
                            return PEvalStatement();
                    }
                }
            }
            i++;
        }
    }
    if (pos>=phraseExpression.length())
        return PEvalStatement();
    if (phraseExpression[pos] == "new") {
        pos++;
        if (pos>=phraseExpression.length())
            return PEvalStatement();
        PEvalStatement result = doEvalExpression(
                    fileName,
                    phraseExpression,
                    pos,
                    scope,
                    previousResult,
                    freeScoped,
                    false);
        if (result) {
            if (pos < phraseExpression.length())
                result = PEvalStatement();
            else if (result->kind == EvalStatementKind::Variable) {
                if (result->pointerLevel==0)
                    result->pointerLevel = 1;
            } else if (result->kind == EvalStatementKind::Type) {
                result->kind = EvalStatementKind::Variable;
                if (result->pointerLevel==0)
                    result->pointerLevel = 1;
            } else
                result = PEvalStatement();
        }
        return result;
    } else
        return doEvalArithmeticOperation(
                    fileName,
                    phraseExpression,
                    pos,
                    scope,
                    previousResult,
                    freeScoped);
}

PEvalStatement CppParser::doEvalArithmeticOperation(const QString &fileName, const QStringList &phraseExpression, int &pos, const PStatement &scope, const PEvalStatement &previousResult, bool freeScoped) const
{
    if (pos>=phraseExpression.length())
        return PEvalStatement();
    //find the start scope statement
    PEvalStatement currentResult = doEvalPointerToMembers(
                fileName,
                phraseExpression,
                pos,
                scope,
                previousResult,
                freeScoped);
    while (pos < phraseExpression.length()) {
        if (!currentResult)
            break;
        if (currentResult &&
                (phraseExpression[pos]=="+"
                    || phraseExpression[pos]=="-")) {
            if (currentResult->kind == EvalStatementKind::Variable) {
                pos++;
                PEvalStatement op2=doEvalPointerToMembers(
                            fileName,
                            phraseExpression,
                            pos,
                            scope,
                            currentResult,
                            false);
                //todo operator+/- overload
            } else if (currentResult->kind == EvalStatementKind::Literal
                       && currentResult->baseType == "int") {
                pos++;
                PEvalStatement op2=doEvalPointerToMembers(
                            fileName,
                            phraseExpression,
                            pos,
                            scope,
                            currentResult,
                            false);
                currentResult = op2;
            } else
                break;
        } else
            break;
    }
//    qDebug()<<pos<<"pointer add member end";
    return currentResult;

}

PEvalStatement CppParser::doEvalPointerToMembers(
        const QString &fileName,
        const QStringList &phraseExpression,
        int &pos,
        const PStatement &scope,
        const PEvalStatement &previousResult,
        bool freeScoped) const
{
    if (pos>=phraseExpression.length())
        return PEvalStatement();
    //find the start scope statement
    PEvalStatement currentResult = doEvalTypeCast(
                fileName,
                phraseExpression,
                pos,
                scope,
                previousResult,
                freeScoped);
    while (pos < phraseExpression.length()) {
        if (!currentResult)
            break;
        if (currentResult &&
                (currentResult->kind == EvalStatementKind::Variable)
                && (phraseExpression[pos]==".*"
                    || phraseExpression[pos]=="->*")) {
            pos++;
            currentResult =
                    doEvalTypeCast(
                        fileName,
                        phraseExpression,
                        pos,
                        scope,
                        currentResult,
                        false);
            if (currentResult) {
                currentResult->pointerLevel++;
            }
        } else
            break;
    }
//    qDebug()<<pos<<"pointer member end";
    return currentResult;
}

PEvalStatement CppParser::doEvalTypeCast(const QString &fileName,
                                                   const QStringList &phraseExpression,
                                                   int &pos,
                                                   const PStatement& scope,
                                                   const PEvalStatement& previousResult,
                                                   bool freeScoped) const
{
    if (pos>=phraseExpression.length())
        return PEvalStatement();
    PEvalStatement result;
    if (phraseExpression[pos]=="*") {
        //deference
        pos++; //skip "*"
        result = doEvalTypeCast(
                    fileName,
                    phraseExpression,
                    pos,
                    scope,
                    previousResult,
                    freeScoped);
        if (result) {
            //todo: STL container;

            if (result->pointerLevel==0) {
                //STL smart pointers
                if (result->typeStatement
                        && STLIterators.contains(result->typeStatement->command)
                ) {
                    PStatement parentScope = result->typeStatement->parentScope.lock();
                    if (STLContainers.contains(parentScope->fullName)) {
                        QString typeName=doFindFirstTemplateParamOf(fileName,result->templateParams, parentScope);
    //                        qDebug()<<"typeName"<<typeName<<lastResult->baseStatement->type<<lastResult->baseStatement->command;
                        PStatement typeStatement=doFindTypeDefinitionOf(fileName, typeName,parentScope);
                        if (typeStatement) {
                            result = doCreateTypedEvalVar(fileName,typeName,parentScope, result->baseStatement);
                        } else {
                            result = PEvalStatement();
                        }
                    } else if (STLMaps.contains(parentScope->fullName)) {
                        QString typeName=doFindTemplateParamOf(fileName,result->templateParams,1,parentScope);
    //                        qDebug()<<"typeName"<<typeName<<lastResult->baseStatement->type<<lastResult->baseStatement->command;
                        PStatement typeStatement=doFindTypeDefinitionOf(fileName, typeName,parentScope);
                        if (typeStatement) {
                            result = doCreateTypedEvalVar(fileName,typeName,parentScope,result->baseStatement);
                        } else {
                            result = PEvalStatement();
                        }
                    }

                } else {
                    PStatement typeStatement = result->effectiveTypeStatement;
                    if ((typeStatement)
                        && STLPointers.contains(typeStatement->fullName)
                        && result->kind == EvalStatementKind::Variable
                        && result->baseStatement) {
                        PStatement parentScope = result->baseStatement->parentScope.lock();
                        QString typeName;
                        if (!previousResult || previousResult->definitionString.isEmpty())
                            typeName = doFindFirstTemplateParamOf(fileName,result->baseStatement->type, parentScope);
                        else
                            typeName = doFindFirstTemplateParamOf(fileName,previousResult->definitionString,parentScope);

    //                    qDebug()<<"typeName"<<typeName;
                        typeStatement=doFindTypeDefinitionOf(fileName, typeName,parentScope);
                        if (typeStatement) {
                            result = doCreateTypedEvalVar(fileName,typeName,parentScope,result->baseStatement);
                        } else {
                            result = PEvalStatement();
                        }
                    }
                }
            } else
                result->pointerLevel--;
        }
    } else if (phraseExpression[pos]=="&") {
        //Address-of
        pos++; //skip "&"
        result = doEvalTypeCast(
                    fileName,
                    phraseExpression,
                    pos,
                    scope,
                    previousResult,
                    freeScoped);
        if (result) {
            result->pointerLevel++;
        }
    } else if (phraseExpression[pos]=="++"
               || phraseExpression[pos]=="--") {
        // Prefix increment and decrement
        pos++; //skip "++" or "--"
        result = doEvalTypeCast(
                    fileName,
                    phraseExpression,
                    pos,
                    scope,
                    previousResult,
                    freeScoped);
    } else if (phraseExpression[pos]=="(") {
        //Type Cast
        //parse
        int startPos = pos;
        pos++;
//        qDebug()<<"parse type cast ()";
        PEvalStatement evalType = doEvalExpression(
                    fileName,
                    phraseExpression,
                    pos,
                    scope,
                    PEvalStatement(),
                    true,
                    false);
//        qDebug()<<pos;
        if (pos >= phraseExpression.length() || phraseExpression[pos]!=")") {
            return PEvalStatement();
        } else if (evalType &&
                   (evalType->kind == EvalStatementKind::Type)) {
            pos++; // skip ")"
//            qDebug()<<"parse type cast exp";
            //it's a type cast
            result = doEvalTypeCast(fileName,
                        phraseExpression,
                        pos,
                        scope,
                        previousResult,
                        freeScoped);
            if (result) {
//                qDebug()<<"type cast";
                result->assignType(evalType);
            }
        }   else //it's not a type cast
            result = doEvalMemberAccess(
                        fileName,
                        phraseExpression,
                        startPos, //we must reparse it
                        scope,
                        previousResult,
                        freeScoped);
    } else
        result = doEvalMemberAccess(
                fileName,
                phraseExpression,
                pos,
                scope,
                previousResult,
                freeScoped);
    return result;
}

PEvalStatement CppParser::doEvalMemberAccess(const QString &fileName,
                                                   const QStringList &phraseExpression,
                                                   int &pos,
                                                   const PStatement& scope,
                                                   const PEvalStatement& previousResult,
                                                   bool freeScoped) const
{
//    qDebug()<<"eval member access "<<pos<<phraseExpression;
    PEvalStatement result;
    if (pos>=phraseExpression.length())
        return result;
    PEvalStatement lastResult = previousResult;
    result = doEvalScopeResolution(
                fileName,
                phraseExpression,
                pos,
                scope,
                previousResult,
                freeScoped);
    if (!result)
        return PEvalStatement();
    while (pos<phraseExpression.length()) {
        if (!result)
            break;
        if (phraseExpression[pos]=="++" || phraseExpression[pos]=="--") {
            //Suffix/postfix increment and decrement
            pos++; //just skip it
        } else if (phraseExpression[pos] == "(") {
            // Function call
            if (result->kind == EvalStatementKind::Type) {
                doSkipInExpression(phraseExpression,pos,"(",")");
                result->kind = EvalStatementKind::Variable;
            } else if (result->kind == EvalStatementKind::Function) {
                doSkipInExpression(phraseExpression,pos,"(",")");
//                qDebug()<<"????"<<(result->baseStatement!=nullptr)<<(lastResult!=nullptr);
                if (result->baseStatement && lastResult) {
                    PStatement parentScope = result->baseStatement->parentScope.lock();
                    if (parentScope
                        && STLElementMethods.contains(result->baseStatement->command)
                    ) {
                        if (STLContainers.contains(parentScope->fullName)) {
                            //stl container methods
                            PStatement typeStatement = result->effectiveTypeStatement;
                            QString typeName;
                            if (!lastResult->definitionString.isEmpty())
                                typeName = doFindFirstTemplateParamOf(fileName,lastResult->definitionString,parentScope);
                            else if (lastResult->baseStatement)
                                typeName = doFindFirstTemplateParamOf(fileName,lastResult->baseStatement->type, parentScope);
    //                        qDebug()<<"typeName"<<typeName<<lastResult->baseStatement->type<<lastResult->baseStatement->command;
                            if (!typeName.isEmpty())
                                typeStatement=doFindTypeDefinitionOf(fileName, typeName,parentScope);
                            if (typeStatement) {
                                result = doCreateTypedEvalVar(fileName,typeName,parentScope,result->baseStatement);
                                lastResult = result;
                            } else {
                                return PEvalStatement();
                            }
                        } else if (STLMaps.contains(parentScope->fullName)) {
                            //stl map methods
                            PStatement typeStatement = result->effectiveTypeStatement;
                            QString typeName;
                            if (!lastResult->definitionString.isEmpty())
                                typeName = doFindTemplateParamOf(fileName,lastResult->definitionString,1,parentScope);
                            else if (lastResult->baseStatement)
                                typeName = doFindTemplateParamOf(fileName,lastResult->baseStatement->type,1, parentScope);
    //                        qDebug()<<"typeName"<<typeName<<lastResult->baseStatement->type<<lastResult->baseStatement->command;
                            if (!typeName.isEmpty())
                                typeStatement=doFindTypeDefinitionOf(fileName, typeName,parentScope);
                            if (typeStatement) {
                                result = doCreateTypedEvalVar(fileName,typeName,parentScope,result->baseStatement);
                                lastResult = result;
                            } else {
                                return PEvalStatement();
                            }
                        }
                    }
                }
//                qDebug()<<"baseType:"<<result->baseType;
//                if (result->baseStatement)
//                    qDebug()<<"baseStatement"<<result->baseStatement->fullName;
//                if (result->typeStatement)
//                    qDebug()<<"typeStatement"<<result->typeStatement->fullName;
//                if (result->effectiveTypeStatement)
//                    qDebug()<<"typeStatement"<<result->effectiveTypeStatement->fullName;
                result->kind = EvalStatementKind::Variable;
            } else if(result->kind == EvalStatementKind::Variable
                      && result->effectiveTypeStatement
                      && result->effectiveTypeStatement->kind == StatementKind::Class) {
                //overload of operator ()
                const StatementMap& statementMap = mStatementList.childrenStatements(result->effectiveTypeStatement);
                if (statementMap.isEmpty())
                    result = PEvalStatement();
                else {
                    PStatement operStatement = statementMap.value("operator()");
                    if (operStatement) {
                        doSkipInExpression(phraseExpression,pos,"(",")");
                        PEvalStatement temp = doCreateEvalFunction(fileName,operStatement);
                        result->effectiveTypeStatement = temp->effectiveTypeStatement;
                        result->kind = EvalStatementKind::Variable;
                    } else {
                        result = PEvalStatement();
                    }
                }
            } else {
                result = PEvalStatement();
            }
        } else if (phraseExpression[pos] == "{") {
            // Varaible Initialization
            if (result->kind == EvalStatementKind::Type) {
                doSkipInExpression(phraseExpression,pos,"{","}");
                result->kind = EvalStatementKind::Variable;
            }
        } else if (phraseExpression[pos] == "[") {
            //Array subscripting
            //skip to "]"
            doSkipInExpression(phraseExpression,pos,"[","]");
            if (result->kind == EvalStatementKind::Type) {
                // Array defintion
                result->pointerLevel++;
            } else if (result->kind == EvalStatementKind::Variable){
                if (result->pointerLevel>0)
                    result->pointerLevel--;
                else {
                    PStatement typeStatement = result->effectiveTypeStatement;
                    if (typeStatement && result->baseStatement) {
                        if (STLContainers.contains(typeStatement->fullName)) {
                            PStatement parentScope = result->baseStatement->parentScope.lock();
                            QString typeName;
                            if (!lastResult || lastResult->definitionString.isEmpty())
                                typeName = doFindFirstTemplateParamOf(fileName,result->baseStatement->type, parentScope);
                            else
                                typeName = doFindFirstTemplateParamOf(fileName,lastResult->definitionString,parentScope);
                            typeStatement = doFindTypeDefinitionOf(fileName, typeName,
                                                             parentScope);
                            if (typeStatement) {
                                result = doCreateTypedEvalVar(fileName,typeName,parentScope,result->baseStatement);
                                lastResult = result;
                            } else {
                                return PEvalStatement();
                            }
                        } else if (STLMaps.contains(typeStatement->fullName)) {
                            PStatement parentScope = result->baseStatement->parentScope.lock();
                            QString typeName;
                            if (!lastResult || lastResult->definitionString.isEmpty())
                                typeName = doFindTemplateParamOf(fileName,result->baseStatement->type, 1,parentScope);
                            else
                                typeName = doFindTemplateParamOf(fileName,lastResult->definitionString,1,parentScope);
                            typeStatement = doFindTypeDefinitionOf(fileName, typeName,
                                                             parentScope);
                            if (typeStatement) {
                                result = doCreateTypedEvalVar(fileName,typeName,parentScope,result->baseStatement);
                                lastResult = result;
                            } else {
                                return PEvalStatement();
                            }
                        }

                    } else {
                        return PEvalStatement();
                    }
                }
            }
        } else if (phraseExpression[pos] == ".") {
            //Structure and union member access
            pos++;
            lastResult = result;
            result = doEvalScopeResolution(
                        fileName,
                        phraseExpression,
                        pos,
                        scope,
                        result,
                        false);
        } else if (phraseExpression[pos] == "->") {
            // Structure and union member access through pointer
            pos++;
            if (result->pointerLevel==0) {
                // iterator
                if (result->typeStatement
                        && STLIterators.contains(result->typeStatement->command)
                ) {
                    PStatement parentScope = result->typeStatement->parentScope.lock();
                    if (STLContainers.contains(parentScope->fullName)) {
                        QString typeName=doFindFirstTemplateParamOf(fileName,result->templateParams, parentScope);
//                        qDebug()<<"typeName"<<typeName<<lastResult->baseStatement->type<<lastResult->baseStatement->command;
                        PStatement typeStatement=doFindTypeDefinitionOf(fileName, typeName,parentScope);
                        if (typeStatement) {
                            result = doCreateTypedEvalVar(fileName,typeName,parentScope,result->baseStatement);
                        } else {
                            result = PEvalStatement();
                        }
                    } else if (STLMaps.contains(parentScope->fullName)) {
                        QString typeName=doFindTemplateParamOf(fileName,result->templateParams,1,parentScope);
    //                        qDebug()<<"typeName"<<typeName<<lastResult->baseStatement->type<<lastResult->baseStatement->command;
                        PStatement typeStatement=doFindTypeDefinitionOf(fileName, typeName,parentScope);
                        if (typeStatement) {
                            result = doCreateTypedEvalVar(fileName,typeName,parentScope,result->baseStatement);
                        } else {
                            result = PEvalStatement();
                        }
                    }
                } else {
                    //smart pointer
                    PStatement typeStatement = result->effectiveTypeStatement;
                    if ((typeStatement)
                            && STLPointers.contains(typeStatement->fullName)
                            && result->kind == EvalStatementKind::Variable
                            && result->baseStatement) {
                        PStatement parentScope = result->baseStatement->parentScope.lock();
                        QString typeName;
                        if (!previousResult || previousResult->definitionString.isEmpty())
                            typeName = doFindFirstTemplateParamOf(fileName,result->baseStatement->type, parentScope);
                        else
                            typeName = doFindFirstTemplateParamOf(fileName,previousResult->definitionString,parentScope);
    //                    qDebug()<<"typeName"<<typeName;
                        typeStatement=doFindTypeDefinitionOf(fileName, typeName,parentScope);
                        if (typeStatement) {
                            result = doCreateTypedEvalVar(fileName,typeName,parentScope,result->baseStatement);
                        } else {
                            return PEvalStatement();
                        }
                    }
                }
            } else {
                result->pointerLevel--;
            }
            lastResult = result;
            result = doEvalScopeResolution(
                        fileName,
                        phraseExpression,
                        pos,
                        scope,
                        result,
                        false);
        } else
            break;
    }
    return result;
}

PEvalStatement CppParser::doEvalScopeResolution(const QString &fileName,
                                                   const QStringList &phraseExpression,
                                                   int &pos,
                                                   const PStatement& scope,
                                                   const PEvalStatement& previousResult,
                                                   bool freeScoped) const
{
//    qDebug()<<"eval scope res "<<pos<<phraseExpression;
    PEvalStatement result;
    if (pos>=phraseExpression.length())
        return result;
    result = doEvalTerm(
                fileName,
                phraseExpression,
                pos,
                scope,
                previousResult,
                freeScoped);
    while (pos<phraseExpression.length()) {
        if (phraseExpression[pos]=="::" ) {
            pos++;
            if (!result) {
                //global
                result = doEvalTerm(fileName,
                                    phraseExpression,
                                    pos,
                                    PStatement(),
                                    PEvalStatement(),
                                    false);
            } else if (result->kind == EvalStatementKind::Type) {
                //class static member
                result = doEvalTerm(fileName,
                                    phraseExpression,
                                    pos,
                                    scope,
                                    result,
                                    false);
            } else if (result->kind == EvalStatementKind::Namespace) {
                //namespace
                result = doEvalTerm(fileName,
                                    phraseExpression,
                                    pos,
                                    scope,
                                    result,
                                    false);
            }
            if (!result)
                break;
        } else
            break;
    }
//    qDebug()<<pos<<"scope end";
    return result;
}

PEvalStatement CppParser::doEvalTerm(const QString &fileName,
                                                   const QStringList &phraseExpression,
                                                   int &pos,
                                                   const PStatement& scope,
                                                   const PEvalStatement& previousResult,
                                                   bool freeScoped) const
{
//    if (previousResult) {
//        qDebug()<<"eval term "<<pos<<phraseExpression<<previousResult->baseType<<freeScoped;
//    } else {
//        qDebug()<<"eval term "<<pos<<phraseExpression<<"no type"<<freeScoped;
//    }
    PEvalStatement result;
    if (pos>=phraseExpression.length())
        return result;
    if (phraseExpression[pos]=="(") {
        pos++;
        result = doEvalExpression(fileName,phraseExpression,pos,scope,PEvalStatement(),freeScoped,false);
        if (pos >= phraseExpression.length() || phraseExpression[pos]!=")")
            return PEvalStatement();
        else {
            pos++; // skip ")";
            return result;
        }
    } else {
        int pointerLevel = 0;
        //skip "struct", "const", "static", etc
        while(pos < phraseExpression.length()) {
            QString token = phraseExpression[pos];
            if (token=="*") // for expression like (const * char)?
                pointerLevel++;
            else if (mCppTypeKeywords.contains(token)
                    || !mCppKeywords.contains(token))
                break;
            pos++;
        }
        if (pos>=phraseExpression.length() || phraseExpression[pos]==")")
            return result;
        if (mCppTypeKeywords.contains(phraseExpression[pos])) {
            result = doCreateEvalType(phraseExpression[pos]);
            pos++;
        } else if (isIdentifier(phraseExpression[pos])) {
            PStatement statement;
            if (freeScoped) {
                if (!previousResult) {
                    statement = findStatementStartingFrom(
                                fileName,
                                phraseExpression[pos],
                                scope);
                } else {
                    statement = findStatementStartingFrom(
                                fileName,
                                phraseExpression[pos],
                                previousResult->effectiveTypeStatement);
                }
            } else {
                if (!previousResult) {
                    statement = findStatementInScope(phraseExpression[pos],PStatement());
                } else {
                    statement = findStatementInScope(phraseExpression[pos],previousResult->effectiveTypeStatement);
                }
//                if (!statement) {
//                    statement = findMacro(
//                                phraseExpression[pos],
//                                fileName);
//                }
            }
            pos++;
            if (statement && statement->kind == StatementKind::Constructor) {
                statement = statement->parentScope.lock();
            }
            while (statement && statement->kind == StatementKind::Alias) {
                statement = doFindAliasedStatement(statement);
            }
            if (statement) {
                switch (statement->kind) {
                case StatementKind::Namespace:
                    result = doCreateEvalNamespace(statement);
                    break;
                case StatementKind::NamespaceAlias:
                    result = doFindAliasedNamespace(statement);
                    break;
                case StatementKind::Variable:
                case StatementKind::Parameter:
                    result = doCreateEvalVariable(fileName,statement, previousResult?previousResult->templateParams:"",scope);
                    break;
                case StatementKind::Class:
                    statement = doFindNoTemplateSpecializationClass(statement);
                    [[fallthrough]];
                case StatementKind::EnumType:
                case StatementKind::EnumClassType:
                case StatementKind::Typedef:
                    result = doCreateEvalType(fileName,statement);
                    break;
                case StatementKind::Function: {
                    if (statement->type=="auto") {
                        PStatement scopeStatement = statement->parentScope.lock();
                        if (scopeStatement) {
                            StatementMap children = mStatementList.childrenStatements(scopeStatement);
                            QList<PStatement> lstFuncs = children.values(statement->command);
                            for (const PStatement& func:lstFuncs) {
                                if (func->type!="auto")
                                    statement=func;
                            }
                        }
                    }
                    result = doCreateEvalFunction(fileName,statement);
                }
                    break;
                default:
                    result = PEvalStatement();
                }
                if (result
                        && result->typeStatement
                        && STLIterators.contains(result->typeStatement->command)
                        && previousResult) {
                    PStatement parentStatement = result->typeStatement->parentScope.lock();
                    if (parentStatement
                            && STLContainers.contains(parentStatement->fullName)) {
                        result->templateParams = previousResult->templateParams;
                    } else if (parentStatement
                            && STLMaps.contains(parentStatement->fullName)) {
                        result->templateParams = previousResult->templateParams;
                    }
                }
            }
        } else if (isIntegerLiteral(phraseExpression[pos])) {
            result = doCreateEvalLiteral("int");
            pos++;
        } else if (isFloatLiteral(phraseExpression[pos])) {
            result = doCreateEvalLiteral("double");
            pos++;
        } else if (isStringLiteral(phraseExpression[pos])) {
            result = doCreateEvalLiteral("char");
            result->pointerLevel = 1;
            pos++;
        } else if (isCharLiteral(phraseExpression[pos])) {
            result = doCreateEvalLiteral("char");
            pos++;
        } else
            return result;
//        if (result) {
//            qDebug()<<"term kind:"<<(int)result->kind;
//        }
        if (result && result->kind == EvalStatementKind::Type) {
            if (result->templateParams.isEmpty()) {
                if (pos<phraseExpression.length() && phraseExpression[pos]=='<') {
                    int oldPos = pos;
                    doSkipInExpression(phraseExpression,pos,"<",">");
                    for(int i=oldPos;i<pos;i++) {
                        result->templateParams+=phraseExpression[i];
                    }
                }
            }
            //skip "struct", "const", "static", etc
            while(pos < phraseExpression.length()) {
                QString token = phraseExpression[pos];
                if (token=="*") // for expression like (const * char)?
                    pointerLevel++;
                else if (mCppTypeKeywords.contains(token)
                        || !mCppKeywords.contains(token))
                    break;
                pos++;
            }
            result->pointerLevel = pointerLevel;
        } else if (result && result->kind == EvalStatementKind::Function
                   && pos<phraseExpression.length()
                   && phraseExpression[pos]=='<') {
            result->templateParams = "";
            int oldPos = pos;
            doSkipInExpression(phraseExpression,pos,"<",">");
            for(int i=oldPos;i<pos;i++) {
                result->templateParams+=phraseExpression[i];
            }
        }
    }
    return result;
}

bool CppParser::expandMacro(QStringList &phraseExpression, int pos, PStatement macro, QList< QSet<QString> > &usedMacros) const
{
    QString s;
    QSet<QString> used = usedMacros[pos];
    if (macro->args.isEmpty()) {
        s=macro->value;
        phraseExpression.removeAt(pos);
        usedMacros.removeAt(pos);
    } else {
        //don't expand
        if (pos+1 >= phraseExpression.length() || phraseExpression[pos+1]!="(")
            return true;
        QString args=macro->args.mid(1,macro->args.length()-2).trimmed(); // remove '(' ')'

        if(args=="")
            return false;
        QStringList argList = args.split(',');
        QList<bool> argUsed;
        for (int i=0;i<argList.size();i++) {
            argList[i]=argList[i].trimmed();
            argUsed.append(false);
        }
        QList<PDefineArgToken> tokens = mPreprocessor.tokenizeValue(macro->value);

        QString formatStr = "";
        DefineArgTokenType lastTokenType=DefineArgTokenType::Other;
        int index;
        foreach (const PDefineArgToken& token, tokens) {
            switch(token->type) {
            case DefineArgTokenType::Identifier:
                index = argList.indexOf(token->value);
                if (index>=0) {
                    argUsed[index] = true;
                    if (lastTokenType == DefineArgTokenType::Sharp) {
                        formatStr+= "\"%"+QString("%1").arg(index+1)+"\"";
                        break;
                    } else {
                        formatStr+= "%"+QString("%1").arg(index+1);
                        break;
                    }
                }
                formatStr += token->value;
                break;
            case DefineArgTokenType::DSharp:
            case DefineArgTokenType::Sharp:
                break;
            case DefineArgTokenType::Space:
            case DefineArgTokenType::Symbol:
                formatStr+=token->value;
                break;
            default:
                break;
            }
            lastTokenType = token->type;
        }
        QStringList values;
        int i;
        int level=1;
        QString current;
        for (i=pos+2;i<phraseExpression.length();i++) {
            if (phraseExpression[i]==')') {
                level--;
                if (level==0) {
                    values.append(current);
                    break;
                }
            } else if (phraseExpression[i]=='(') {
                level++;
            } else {
                if (level==1 && phraseExpression[i]==',') {
                     values.append(current);
                     current.clear();
                     continue;
                }
            }
            current+=phraseExpression[i];
        }
        if (level!=0)
            return false;
        if (values.length()!=argList.length())
            return false;
        s = formatStr;
        for (int i=0;i<values.length();i++) {
            if (argUsed[i]) {
                QString argValue = values[i];
                s=s.arg(argValue.trimmed());
            }
        }
        for (int j=pos;j<=i;j++) {
            phraseExpression.removeAt(pos);
            usedMacros.removeAt(pos);
        }
    }

    used.insert(macro->command);
    QSynedit::CppSyntaxer syntaxer;
    syntaxer.resetState();
    syntaxer.setLine(s,0);
    while (!syntaxer.eol()) {
        QString token=syntaxer.getToken();
        QSynedit::PTokenAttribute attr = syntaxer.getTokenAttribute();
        switch(attr->tokenType()) {
        case QSynedit::TokenType::Space:
        case QSynedit::TokenType::Comment:
            break;
        case QSynedit::TokenType::Identifier:
            phraseExpression.insert(pos,token);
            usedMacros.insert(pos,used);
            pos++;
            break;
        default:
            phraseExpression.insert(pos,token);
            usedMacros.insert(pos,used);
            pos++;
        }
        syntaxer.next();
    }
    return true;
}

PEvalStatement CppParser::doCreateEvalNamespace(const PStatement &namespaceStatement) const
{
    if (!namespaceStatement)
        return PEvalStatement();
    return std::make_shared<EvalStatement>(
                namespaceStatement->fullName,
                EvalStatementKind::Namespace,
                PStatement(),
                namespaceStatement,
                namespaceStatement);
}

PEvalStatement CppParser::doFindAliasedNamespace(const PStatement &namespaceAlias) const
{
    QStringList expList;
    QString s = namespaceAlias->type;
    int pos = s.indexOf("::");
    while (pos>=0) {
        expList.append(s.left(pos));
        expList.append("::");
        s = s.mid(pos+2);
        pos = s.indexOf("::");
    }
    expList.append(s);
    pos=0;
    return doEvalExpression(
                namespaceAlias->fileName,
                expList,
                pos,
                namespaceAlias->parentScope.lock(),
                PEvalStatement(),
                true,
                false
                );
}

PEvalStatement CppParser::doCreateEvalType(const QString &fileName, const QString &typeName, const PStatement& parentScope) const
{
    QString baseType;
    PStatement typeStatement;
    int pointerLevel=0;
    QString templateParams;
    PStatement effectiveTypeStatement =  doParseEvalTypeInfo(
                fileName,
                parentScope,
                typeName,
                baseType,
                typeStatement,
                pointerLevel,
                templateParams);
    return std::make_shared<EvalStatement>(
                baseType,
                EvalStatementKind::Type,
                PStatement(),
                typeStatement,
                effectiveTypeStatement,
                pointerLevel,
                templateParams
                );
}

PEvalStatement CppParser::doCreateEvalType(const QString& fileName,const PStatement &typeStatement) const
{
    if (!typeStatement)
        return PEvalStatement();
    if (typeStatement->kind == StatementKind::Typedef) {
        QString baseType;
        int pointerLevel=0;
        QString templateParams;
        PStatement tempStatement;
        PStatement effectiveTypeStatement  = doParseEvalTypeInfo(
                    fileName,
                    typeStatement->parentScope.lock(),
                    typeStatement->type + typeStatement->args,
                    baseType,
                    tempStatement,
                    pointerLevel,
                    templateParams);
        if (effectiveTypeStatement && effectiveTypeStatement->kind == StatementKind::Class)
            effectiveTypeStatement = doFindNoTemplateSpecializationClass(effectiveTypeStatement);
        return std::make_shared<EvalStatement>(
                    baseType,
                    EvalStatementKind::Type,
                    PStatement(),
                    typeStatement,
                    effectiveTypeStatement,
                    pointerLevel,
                    templateParams
                    );
    } else {
        return std::make_shared<EvalStatement>(
                typeStatement->fullName,
                EvalStatementKind::Type,
                typeStatement,
                typeStatement,
                typeStatement);
    }
}

PEvalStatement CppParser::doCreateEvalType(const QString &primitiveType) const
{
    return std::make_shared<EvalStatement>(
                primitiveType,
                EvalStatementKind::Type,
                PStatement(),
                PStatement(),
                PStatement());
}

PEvalStatement CppParser::doCreateTypedEvalVar(const QString &fileName, const QString &typeName, const PStatement &parentScope, const PStatement &baseStatement) const
{
    PEvalStatement result = doCreateEvalType(fileName,typeName,parentScope);
    result->definitionString=typeName;
    result->kind = EvalStatementKind::Variable;
    result->baseStatement = baseStatement;
    return result;
}

PEvalStatement CppParser::doCreateEvalVariable(
        const QString &fileName,
        const PStatement& varStatement,
        const QString& baseTemplateParams,
        const PStatement& scope) const
{
    if (!varStatement)
        return PEvalStatement();
    QString baseType;
    int pointerLevel=0;
    QString templateParams;
    PStatement typeStatement;
    PStatement effectiveTypeStatement;
    //todo: ugly implementation for std::pair
    if (varStatement->fullName == "std::pair::first") {
        effectiveTypeStatement = doParseEvalTypeInfo(
                fileName,
                scope,
                doFindFirstTemplateParamOf(fileName,baseTemplateParams,scope),
                baseType,
                typeStatement,
                pointerLevel,
                templateParams);
    } else if (varStatement->fullName == "std::pair::second") {
        effectiveTypeStatement = doParseEvalTypeInfo(
                fileName,
                scope,
                doFindTemplateParamOf(fileName,baseTemplateParams,1,scope),
                baseType,
                typeStatement,
                pointerLevel,
                templateParams);

    } else {
        effectiveTypeStatement = doParseEvalTypeInfo(
                fileName,
                varStatement->parentScope.lock(),
                varStatement->type+varStatement->args,
                baseType,
                typeStatement,
                pointerLevel,
                templateParams);
    }
//    if (!varStatement->args.isEmpty()) {
//        int j = 0;
//        while ((j = varStatement->args.indexOf("[", j)) != -1) {
//            ++j;
//            pointerLevel++;
//        }
//    }
//    qDebug()<<"parse ..."<<baseType<<pointerLevel;
    return std::make_shared<EvalStatement>(
                baseType,
                EvalStatementKind::Variable,
                varStatement,
                typeStatement,
                effectiveTypeStatement,
                pointerLevel,
                templateParams
                );
}

PEvalStatement CppParser::doCreateEvalFunction(
        const QString &fileName,
        const PStatement& funcStatement) const
{
    if (!funcStatement)
        return PEvalStatement();
    QString baseType;
    int pointerLevel=0;
    QString templateParams;
    PStatement typeStatement;
    QString type = funcStatement->type;
    if (funcStatement->fullName == "std::make_unique")
        type = "unique_ptr";
    PStatement effetiveTypeStatement = doParseEvalTypeInfo(
                fileName,
                funcStatement->parentScope.lock(),
                type,
                baseType,
                typeStatement,
                pointerLevel,
                templateParams);
    return std::make_shared<EvalStatement>(
                baseType,
                EvalStatementKind::Function,
                funcStatement,
                typeStatement,
                effetiveTypeStatement,
                pointerLevel,
                templateParams
                );
}

PEvalStatement CppParser::doCreateEvalLiteral(const QString &type) const
{
    return std::make_shared<EvalStatement>(
                type,
                EvalStatementKind::Literal,
                PStatement(),
                PStatement(),
                PStatement());
}

void CppParser::doSkipInExpression(const QStringList &expression, int &pos, const QString &startSymbol, const QString &endSymbol) const
{
    int level = 0;
    while (pos<expression.length()) {
        QString token = expression[pos];
        if (token == startSymbol) {
            level++;
        } else if (token == endSymbol) {
            level--;
            if (level==0) {
                pos++;
                return;
            }
        }
        pos++;
    }
}

QString CppParser::findFunctionPointerName(int startIdx)
{
    Q_ASSERT(mTokenizer[startIdx]->text=="(");
    int i=startIdx+1;
    int endIdx = mTokenizer[startIdx]->matchIndex;
    while (i<endIdx) {
        if (isIdentChar(mTokenizer[i]->text[0])) {
            return mTokenizer[i]->text;
        }
        i++;
    }
    return QString();
}

PStatement CppParser::doParseEvalTypeInfo(
        const QString &fileName,
        const PStatement &scope,
        const QString &type,
        QString &baseType,
        PStatement& typeStatement,
        int &pointerLevel,
        QString& templateParams) const
{
    // Remove pointer stuff from type
    QString s = type;
//    qDebug()<<"eval type info"<<type;
    int position = s.length()-1;
    QSynedit::CppSyntaxer syntaxer;
    syntaxer.resetState();
    syntaxer.setLine(type,0);
    int bracketLevel = 0;
    int templateLevel  = 0;
    while(!syntaxer.eol()) {
        QString token = syntaxer.getToken();
        if (bracketLevel == 0 && templateLevel ==0) {
            if (token == "*")
                pointerLevel++;
            else if (syntaxer.getTokenAttribute()->tokenType() == QSynedit::TokenType::Identifier) {
                baseType += token;
            } else if (token == "[") {
                pointerLevel++;
                bracketLevel++;
            } else if (token == "<") {
                templateLevel++;
                templateParams += token;
            } else if (token == "::") {
                baseType += token;
            }
        } else if (bracketLevel > 0) {
            if (token == "[") {
                bracketLevel++;
            } else if (token == "]") {
                bracketLevel--;
            }
        } else if (templateLevel > 0) {
            if (token == "<") {
                templateLevel++;
            } else if (token == ">") {
                templateLevel--;
            } else if (token == ">>") {
                templateLevel-=2;
            }
            templateParams += token;
        }
        syntaxer.next();
    }
    while ((position >= 0) && (s[position] == '*'
                               || s[position] == ' '
                               || s[position] == '&')) {
        if (s[position]=='*') {

        }
        position--;
    }
    QStringList expression = splitExpression(baseType);
    typeStatement = doFindStatementOf(fileName,expression,scope);
    PStatement effectiveTypeStatement = typeStatement;
    int level=0;
    while (effectiveTypeStatement && (effectiveTypeStatement->kind == StatementKind::Typedef
                                      || effectiveTypeStatement->kind == StatementKind::Alias
                                      || effectiveTypeStatement->kind == StatementKind::Preprocessor)) {
        if (level >20) // prevent infinite loop
            break;
        level++;
        baseType="";
        syntaxer.resetState();
        syntaxer.setLine(effectiveTypeStatement->type+effectiveTypeStatement->args,0);
        int bracketLevel = 0;
        int templateLevel  = 0;
        while(!syntaxer.eol()) {
            QString token = syntaxer.getToken();
            if (bracketLevel == 0 && templateLevel ==0) {
                if (token == "*")
                    pointerLevel++;
                else if (syntaxer.getTokenAttribute()->tokenType() == QSynedit::TokenType::Identifier) {
                    baseType += token;
                } else if (token == "[") {
                    pointerLevel++;
                    bracketLevel++;
                } else if (token == "<") {
                    templateLevel++;
                    templateParams += token;
                } else if (token == "::") {
                    baseType += token;
                }
            } else if (bracketLevel > 0) {
                if (token == "[") {
                    bracketLevel++;
                } else if (token == "]") {
                    bracketLevel--;
                }
            } else if (templateLevel > 0) {
                if (token == "<") {
                    templateLevel++;
                } else if (token == ">") {
                    templateLevel--;
                }
                templateParams += token;
            }
            syntaxer.next();
        }
        effectiveTypeStatement = doFindStatementOf(fileName,baseType, effectiveTypeStatement->parentScope.lock());
    }
    effectiveTypeStatement = getTypeDef(effectiveTypeStatement,fileName,baseType);
    return effectiveTypeStatement;
}

int CppParser::getBracketEnd(const QString &s, int startAt) const
{
    int i = startAt;
    int level = 0; // assume we start on top of [
    while (i < s.length()) {
        switch(s[i].unicode()) {
        case '<':
            level++;
            break;
        case '>':
            level--;
            if (level == 0)
                return i;
        }
        i++;
    }
    return startAt;
}

PStatement CppParser::doFindStatementInScope(const QString &name,
                                             const QString &noNameArgs,
                                             StatementKind kind,
                                             const PStatement& scope) const
{
    const StatementMap& statementMap = mStatementList.childrenStatements(scope);

    foreach (const PStatement& statement, statementMap.values(name)) {
        if (statement->kind == kind && statement->noNameArgs == noNameArgs) {
            return statement;
        }
    }
    return PStatement();
}

void CppParser::internalInvalidateFile(const QString &fileName)
{
    if (fileName.isEmpty())
        return;

    // remove its include files list
    PParsedFileInfo p = mPreprocessor.findFileInfo(fileName);
    if (p) {
        //invalidDefinesInFile(FileName); //we don't need this, since we reset defines after each parse
        for(PStatement statement:p->statements()) {
            if (statement->fileName==fileName) {
                mStatementList.deleteStatement(statement);
            } else {
                statement->setHasDefinition(false);
                statement->definitionFileName = statement->fileName;
                statement->definitionLine = statement->line;
            }
        }

        //invalidate all handledInheritances
        for (std::weak_ptr<ClassInheritanceInfo> pWeakInfo: p->handledInheritances()) {
            PClassInheritanceInfo info = pWeakInfo.lock();
            if (info) {
                info->handled = false;
            }
        }

        mPreprocessor.removeFileInfo(fileName);
    }

    //remove all statements from namespace cache
    for (auto it=mNamespaces.begin();it!=mNamespaces.end();) {
        PStatementList statements = it.value();
        for (int i=statements->size()-1;i>=0;i--) {
            PStatement statement = statements->at(i);
            if (statement->fileName == fileName) {
                statements->removeAt(i);
            }
        }
        if (statements->isEmpty()) {
            it = mNamespaces.erase(it);
        } else {
            ++it;
        }
    }
    // class inheritance
    // invalid class inheritance infos (derived class is not valid) whould be auto removed in handleInheritances()
    // foreach (const PClassInheritanceInfo& info, mClassInheritances) {
    //     if (info->handled && info->parentClassFilename == fileName)
    //         info->handled = false;
    // }
    // delete it from scannedfiles
    mPreprocessor.removeScannedFile(fileName);
}

void CppParser::internalInvalidateFiles(const QSet<QString> &files)
{
    for (const QString& file:files)
        internalInvalidateFile(file);
}

QSet<QString> CppParser::calculateFilesToBeReparsed(const QString &fileName)
{
    if (fileName.isEmpty())
        return QSet<QString>();
    QSet<QString> result;
    result.insert(fileName);
    foreach (const QString& file, mPreprocessor.scannedFiles()) {
        PParsedFileInfo fileInfo = mPreprocessor.findFileInfo(file);
        if (fileInfo && fileInfo->including(fileName)) {
            result.insert(file);
        }
    }
    return result;
}

//int CppParser::calcKeyLenForStruct(const QString &word)
//{
//    if (word.startsWith("struct"))
//        return 6;
//    else if (word.startsWith("class")
//             || word.startsWith("union"))
//        return 5;
//    return -1;
//}

void CppParser::scanMethodArgs(const PStatement& functionStatement, int argStart)
{
    Q_ASSERT(mTokenizer[argStart]->text=='(');
    int argEnd=mTokenizer[argStart]->matchIndex;
    int paramStart = argStart+1;
    int i = paramStart ; // assume it starts with ( and ends with )
    // Keep going and stop on top of the variable name
    QStringList words;
    while (i < argEnd) {
        if (mTokenizer[i]->text=='('
                && mTokenizer[i]->matchIndex+1<argEnd
                && mTokenizer[mTokenizer[i]->matchIndex+1]->text=='(') {
            //function pointer
            int argStart=mTokenizer[i]->matchIndex+1;
            int argEnd=mTokenizer[argStart]->matchIndex;
            QString cmd=findFunctionPointerName(i);
            QString args=mergeArgs(argStart,argEnd);
            if (!cmd.isEmpty()) {
                addStatement(
                            functionStatement,
                            mCurrentFile,
                            words.join(" "), // 'int*'
                            cmd, // a
                            args,
                            "",
                            "",
                            mTokenizer[i+1]->line,
                            StatementKind::Parameter,
                            StatementScope::Local,
                            StatementAccessibility::None,
                            StatementProperty::HasDefinition);
            }
            i=argEnd+1;
            words.clear();
        } else if (mTokenizer[i]->text=='{') {
            i=mTokenizer[i]->matchIndex+1;
        } else if (mTokenizer[i]->text.endsWith('=')) {
            addMethodParameterStatement(words,mTokenizer[i]->line,functionStatement);
            i=skipAssignment(i,argEnd);
        } else if (mTokenizer[i]->text=="::") {
            int lastIdx=words.count()-1;
            if (lastIdx>=0 && words[lastIdx]!="const") {
                words[lastIdx]=words[lastIdx]+mTokenizer[i]->text;
            } else
                words.append(mTokenizer[i]->text);
            i++;
        } else if (mTokenizer[i]->text==',') {
           addMethodParameterStatement(words,mTokenizer[i]->line,functionStatement);
           i++;
           words.clear();
        } else if (isIdentChar(mTokenizer[i]->text[0])) {
            // identifier
            int lastIdx=words.count()-1;
            if (lastIdx>=0 && words[lastIdx].endsWith("::")) {
                words[lastIdx]=words[lastIdx]+mTokenizer[i]->text;
            } else
                words.append(mTokenizer[i]->text);
            i++;
        } else if (isWordChar(mTokenizer[i]->text[0])) {
            // * &
            words.append(mTokenizer[i]->text);
            i++;
        } else if (mTokenizer[i]->text.startsWith("[")) {
            if (!words.isEmpty()) {
                int lastIdx=words.count()-1;
                words[lastIdx]=words[lastIdx]+mTokenizer[i]->text;
            }
            i++;
        } else {
            i++;
        }
    }
    addMethodParameterStatement(words,mTokenizer[i-1]->line,functionStatement);
}

QSet<QString> CppParser::parseLambdaCaptures(int index)
{
    QString s = mTokenizer[index]->text;
    QString word;
    QSet<QString> result;
    //skip '[' ']'
    for (int i=1;i<s.length()-1;i++) {
        switch(s[i].unicode()){
        case ',':
            if (!word.isEmpty()) {
                result.insert(word);
                word.clear();
            }
            break;
        case ' ':
        case '\t':
        case '*':
            break;
        default:
            if (word=="&")
                word.clear();
            word+=s[i];
        }
    }
    if (!word.isEmpty())
        result.insert(word);
    return result;
}

QString CppParser::splitPhrase(const QString &phrase, QString &sClazz,
                               QString &sOperator, QString &sMember) const
{
    sClazz="";
    sMember="";
    sOperator="";
    QString result="";
    int bracketLevel = 0;
    int phraseLength = phrase.length();
    // Obtain stuff before first operator
    int firstOpStart = phraseLength + 1;
    int firstOpEnd = phraseLength + 1;
    for (int i = 0; i<phraseLength;i++) {
        if ((i+1<phrase.length()) && (phrase[i] == '-') && (phrase[i + 1] == '>') && (bracketLevel==0)) {
            firstOpStart = i;
            firstOpEnd = i+2;
            sOperator = "->";
            break;
        } else if ((i+1<phrase.length()) && (phrase[i] == ':') && (phrase[i + 1] == ':') && (bracketLevel==0)) {
            firstOpStart = i;
            firstOpEnd = i+2;
            sOperator = "::";
            break;
        } else if ((phrase[i] == '.') && (bracketLevel==0)) {
            firstOpStart = i;
            firstOpEnd = i+1;
            sOperator = ".";
            break;
        } else if (phrase[i] == '[') {
            bracketLevel++;
        } else if (phrase[i] == ']') {
            bracketLevel--;
        }
    }
    sClazz = phrase.mid(0, firstOpStart);
    if (firstOpStart == 0) {
        sMember = "";
        return "";
    }

    result = phrase.mid(firstOpEnd);

    // ... and before second op, if there is one
    int secondOp = 0;
    bracketLevel = 0;
    for (int i = firstOpEnd; i<phraseLength;i++) {
        if ((i+1<phraseLength) && (phrase[i] == '-') && (phrase[i + 1] == '>') && (bracketLevel=0)) {
            secondOp = i;
            break;
        } else if ((i+1<phraseLength) && (phrase[i] == ':') && (phrase[i + 1] == ':') && (bracketLevel=0)) {
            secondOp = i;
            break;
        } else if ((phrase[i] == '.') && (bracketLevel=0)) {
            secondOp = i;
            break;
        } else if (phrase[i] == '[') {
            bracketLevel++;
        } else if (phrase[i] == ']') {
            bracketLevel--;
        }
    }
    if (secondOp == 0) {
        sMember = phrase.mid(firstOpEnd);
    } else {
        sMember = phrase.mid(firstOpEnd,secondOp-firstOpEnd);
    }
    return result;
}

QString CppParser::removeTemplateParams(const QString &phrase) const
{
    int pos = phrase.indexOf('<');
    if (pos>=0) {
        return phrase.left(pos);
    }
    return phrase;
}

bool CppParser::splitLastMember(const QString &token, QString &lastMember, QString &remaining)
{
    int pos = token.length()-1;
    int level=0;
    bool found=false;
    while (pos>=0 && !found) {
        switch(token[pos].unicode()) {
        case ']':
        case '>':
        case ')':
            level++;
            break;
        case '[':
        case '<':
        case '(':
            level--;
            break;
        case ':':
            if (level==0 && pos>0 && token[pos-1]==':') {
                found=true;
                break;
            }
        }
        pos--;
    }
    if (pos<0)
        return false;
    lastMember=token.mid(pos+2);
    remaining=token.left(pos);
    return true;
}

//static void appendArgWord(QString& args, const QString& word) {
//    QString s=word.trimmed();
//    if (s.isEmpty())
//        return;
//    if (args.isEmpty())
//        args.append(s);
//    else if (isIdentChar(args.back()) && isIdentChar(word.front()) ) {
//        args+=" ";
//        args+=s;
//    } else {
//        args+=s;
//    }
//}

bool CppParser::isNotFuncArgs(int startIndex)
{
    Q_ASSERT(mTokenizer[startIndex]->text=='(');
    int endIndex=mTokenizer[startIndex]->matchIndex;
    //no args, it must be a function
    if (endIndex-startIndex==1)
        return false;
    int i=startIndex+1; //skip '('
    int endPos = endIndex;
    QString word = "";
    while (i<endPos) {
        QChar ch=mTokenizer[i]->text[0];
        switch(ch.unicode()) {
        // args contains a string/char, can't be a func define
        case '"':
        case '\'':
        case '+':
        case '-':
        case '/':
        case '|':
        case '!':
        case '{':
            return true;
        case '[': // function args like int f[10]
            i=mTokenizer[i]->matchIndex+1;
            if (i<endPos &&
                    (mTokenizer[i]->text=='('
                     || mTokenizer[i]->text=='{')) //lambda
                return true;
            continue;
        }
        if (isDigitChar(ch))
            return true;
        if (isIdentChar(ch)) {
            QString currentText=mTokenizer[i]->text;
//            if (mTokenizer[i]->text.endsWith('.'))
//                return true;
//            if (mTokenizer[i]->text.endsWith("->"))
//                return true;
            if (!mCppTypeKeywords.contains(currentText)) {
                if (currentText=="true" || currentText=="false" || currentText=="nullptr" ||
                        currentText=="this")
                    return true;
                if (currentText=="const")
                    return false;

                if (isCppKeyword(currentText))
                    return false;

                PStatement statement = doFindStatementOf(mCurrentFile,word,getCurrentScope());
                //template arguments
                if (!statement)
                    return false;
                if (statement && isTypeStatement(statement->kind))
                    return false;
            } else {
                return false;
            }

        }
        i++;
    }
    //function with no args

    return false;
}

bool CppParser::isNamedScope(StatementKind kind) const
{
    switch(kind) {
    case StatementKind::Class:
    case StatementKind::Namespace:
    case StatementKind::Function:
        return true;
    default:
        return false;
    }
}

bool CppParser::isTypeStatement(StatementKind kind) const
{
    switch(kind) {
    case StatementKind::Class:
    case StatementKind::Typedef:
    case StatementKind::EnumClassType:
    case StatementKind::EnumType:
        return true;
    default:
        return false;
    }
}

void CppParser::updateSerialId()
{
    mSerialId = QString("%1 %2").arg(mParserId).arg(mSerialCount);
}

int CppParser::indexOfNextSemicolon(int index, int maxIndex)
{
    while (index<maxIndex) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case ';':
            return index;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

int CppParser::indexOfNextPeriodOrSemicolon(int index, int maxIndex)
{
    while (index<maxIndex) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case ';':
        case ',':
        case '}':
        case ')':
            return index;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

int CppParser::indexOfNextSemicolonOrLeftBrace(int index, int maxIndex)
{
    while (index<maxIndex) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case ';':
        case '{':
            return index;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

int CppParser::indexOfNextColon(int index, int maxIndex)
{
    while (index<maxIndex) {
        QString s =mTokenizer[index]->text;
        switch(s[0].unicode()) {
        case ':':
            if (s.length()==1)
                return index;
            else
                index++;
            break;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

int CppParser::indexOfNextLeftBrace(int index, int maxIndex)
{
    while (index<maxIndex) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case '{':
            return index;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

int CppParser::indexPassParenthesis(int index, int maxIndex)
{
    while (index<maxIndex) {
        if (mTokenizer[index]->text=='(') {
            return mTokenizer[index]->matchIndex+1;
        }
        index++;
    }
    return index;
}

int CppParser::indexOfNextRightParenthesis(int index, int maxIndex)
{
    while (index<maxIndex) {
        QString s =mTokenizer[index]->text;
        switch(s[0].unicode()) {
        case ')':
            return index;
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    return index;
}

void CppParser::skipNextSemicolon(int index, int endIndex)
{
    mIndex=index;
    while (mIndex<endIndex) {
        switch(mTokenizer[mIndex]->text[0].unicode()) {
        case ';':
            mIndex++;
            return;
        case '{':
            mIndex = mTokenizer[mIndex]->matchIndex+1;
            break;
        case '(':
            mIndex = mTokenizer[mIndex]->matchIndex+1;
            break;
        default:
            mIndex++;
        }
    }
}

int CppParser::moveToEndOfStatement(int index, bool checkLambda, int maxIndex)
{
    int startIndex=index;
    if (index>=maxIndex)
        return index;
    index--; // compensate for the first loop

    bool skip=true;
    do {
        index++;
        bool stop = false;
        while (index<maxIndex && !stop) {
            switch(mTokenizer[index]->text[0].unicode()) {
            case ';':
                stop=true;
                break;
            case '=':
                stop=true;
                break;
            case '}':
                stop=true;
                skip=false; //don't skip the orphan '}' that we found
                break;
            case '{':
                //move to '}'
                index=mTokenizer[index]->matchIndex;
                stop=true;
                break;
            case '(':
                index = mTokenizer[index]->matchIndex+1;
                break;
            default:
                index++;
            }
        }
    } while (index<maxIndex && mTokenizer[index]->text=='=');
    if (index<maxIndex && checkLambda) {
        while (mTokenizer.lambdasCount()>0 && mTokenizer.indexOfFirstLambda()<index) {
            int i=mTokenizer.indexOfFirstLambda();
            mTokenizer.removeFirstLambda();
            if (i>=startIndex) {
                handleLambda(i,index);
            }
        }
    }
    if (skip)
        index++;
    return index;
}

void CppParser::skipParenthesis(int index, int maxIndex)
{
    mIndex=index;
    while (mIndex<maxIndex) {
        if (mTokenizer[mIndex]->text=='(') {
            mIndex=mTokenizer[mIndex]->matchIndex+1;
            return;
        }
        mIndex++;
    }
}

int CppParser::skipAssignment(int index, int maxIndex)
{
    int startIndex=index;
    bool stop=false;
    while (index<maxIndex && !stop) {
        switch(mTokenizer[index]->text[0].unicode()) {
        case ';':
        case ',':
        case '}':
        case ')':
            stop=true;
            break;
        case '{':
        case '(':
            index = mTokenizer[index]->matchIndex+1;
            break;
        default:
            index++;
        }
    }
    if (stop) {
        while (mTokenizer.lambdasCount()>0 && mTokenizer.indexOfFirstLambda()<index) {
            int i=mTokenizer.indexOfFirstLambda();
            mTokenizer.removeFirstLambda();
            if (i>=startIndex) {
                handleLambda(i,index);
            }
        }
    }
    return (index<maxIndex)?index:maxIndex;
}

QString CppParser::mergeArgs(int startIndex, int endIndex)
{
    QString result;
    for (int i=startIndex;i<=endIndex;i++) {
        if (i>startIndex)
            result+=' ';
        result+=mTokenizer[i]->text;
    }
    return result;
}

void CppParser::parseCommandTypeAndArgs(QString &command, QString &typeSuffix, QString &args) const
{
    int prefix=0;
    while (prefix<command.length() && (command[prefix]=='*' || command[prefix]=='&')) {
        prefix++;
    }
    if (prefix>0) {
        typeSuffix=command.left(prefix);
        command=command.mid(prefix);
    } else {
        typeSuffix="";
    }
    int pos=command.indexOf('[');
    if (pos>=0) {
        args=command.mid(pos);
        command=command.left(pos);
    } else {
        args="";
    }

}

QString CppParser::expandMacro(const QString &text) const
{
    QSet<QString> usedMacros;
    return mPreprocessor.expandMacros(text, usedMacros);
}

QStringList CppParser::splitExpression(const QString &expr)
{
    QStringList result;
    QSynedit::CppSyntaxer syntaxer;
    syntaxer.resetState();
    QStringList lines = textToLines(expr);
    for(int i=0;i<lines.length();i++) {
        syntaxer.setLine(lines[i],i+1);
        while(!syntaxer.eol()) {
            QSynedit::TokenType tokenType = syntaxer.getTokenAttribute()->tokenType();
            QString token = syntaxer.getToken();
            if (tokenType == QSynedit::TokenType::Operator) {
                if ( token == ">>" ) {
                    result.append(">");
                    result.append(">");
                } else {
                    result.append(token);
                }
            } else if (tokenType!=QSynedit::TokenType::Comment
                    && tokenType!=QSynedit::TokenType::Space)
                result.append(token);
            syntaxer.next();
        }
    }
    return result;
}

const QSet<QString> &CppParser::projectFiles() const
{
    return mProjectFiles;
}

QList<QString> CppParser::namespaces()
{
    QMutexLocker locker(&mMutex);
    return mNamespaces.keys();
}

ParserLanguage CppParser::language() const
{
    return mLanguage;
}

void CppParser::setLanguage(ParserLanguage newLanguage)
{
    if (mLanguage != newLanguage) {
        mLanguage = newLanguage;
        mCppKeywords = CppKeywords;
        mCppTypeKeywords = CppTypeKeywords;
#ifdef ENABLE_SDCC
        if (mLanguage == ParserLanguage::SDCC) {
            mCppKeywords.insert(SDCCKeywords);
            mCppTypeKeywords.unite(SDCCTypeKeywords);
        }
#endif
    }
}



const StatementModel &CppParser::statementList() const
{
    return mStatementList;
}

bool CppParser::parseGlobalHeaders() const
{
    return mParseGlobalHeaders;
}

void CppParser::setParseGlobalHeaders(bool newParseGlobalHeaders)
{
    mParseGlobalHeaders = newParseGlobalHeaders;
}

const QSet<QString> &CppParser::includePaths()
{
    return mPreprocessor.includePaths();
}

const QSet<QString> &CppParser::projectIncludePaths()
{
    return mPreprocessor.projectIncludePaths();
}

bool CppParser::parseLocalHeaders() const
{
    return mParseLocalHeaders;
}

void CppParser::setParseLocalHeaders(bool newParseLocalHeaders)
{
    mParseLocalHeaders = newParseLocalHeaders;
}

const QString &CppParser::serialId() const
{
    return mSerialId;
}

int CppParser::parserId() const
{
    return mParserId;
}

void CppParser::setOnGetFileStream(const GetFileStreamCallBack &newOnGetFileStream)
{
    mPreprocessor.setOnGetFileStream(newOnGetFileStream);
}

const QSet<QString> &CppParser::filesToScan() const
{
    return mFilesToScan;
}

void CppParser::setFilesToScan(const QSet<QString> &newFilesToScan)
{
    mFilesToScan = newFilesToScan;
}

bool CppParser::enabled() const
{
    return mEnabled;
}

void CppParser::setEnabled(bool newEnabled)
{
    if (mEnabled!=newEnabled) {
        mEnabled = newEnabled;
        if (!mEnabled) {
            resetParser();
        }
    }
}

CppFileParserThread::CppFileParserThread(
        PCppParser parser,
        QString fileName,
        bool inProject,
        bool onlyIfNotParsed,
        bool updateView,
        QObject *parent):QThread(parent),
    mParser(parser),
    mFileName(fileName),
    mInProject(inProject),
    mOnlyIfNotParsed(onlyIfNotParsed),
    mUpdateView(updateView)
{
    connect(this,&QThread::finished,
            this,&QObject::deleteLater);
}

void CppFileParserThread::run()
{
    if (mParser) {
        mParser->parseFile(mFileName,mInProject,mOnlyIfNotParsed,mUpdateView,mParser);
    }
}

CppFileListParserThread::CppFileListParserThread(PCppParser parser,
                                                 bool updateView, QObject *parent):
    QThread(parent),
    mParser(parser),
    mUpdateView(updateView)
{
    connect(this,&QThread::finished,
            this,&QObject::deleteLater);
}

void CppFileListParserThread::run()
{
    if (mParser && !mParser->parsing()) {
        mParser->parseFileList(mUpdateView);
    }
}

void parseFile(PCppParser parser, const QString& fileName, bool inProject, bool onlyIfNotParsed, bool updateView)
{
    if (!parser)
        return;
    if (!parser->enabled())
        return;
//    qDebug()<<"parsing "<<fileName;
    //delete when finished
    CppFileParserThread* thread = new CppFileParserThread(parser,fileName,inProject,onlyIfNotParsed,updateView);
    thread->connect(thread,
                    &QThread::finished,
                    thread,
                    &QThread::deleteLater);
    thread->start();
}

void parseFileList(PCppParser parser, bool updateView)
{
    if (!parser)
        return;
    if (!parser->enabled())
        return;
    //delete when finished
    CppFileListParserThread *thread = new CppFileListParserThread(parser,updateView);
    thread->connect(thread,
                    &QThread::finished,
                    thread,
                    &QThread::deleteLater);
    thread->start();
}
