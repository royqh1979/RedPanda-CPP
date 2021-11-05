#include "cppparser.h"
#include "parserutils.h"
#include "../utils.h"

#include <QApplication>
#include <QDate>
#include <QHash>
#include <QQueue>
#include <QThread>
#include <QTime>

static QAtomicInt cppParserCount(0);
CppParser::CppParser(QObject *parent) : QObject(parent)
{
    mParserId = cppParserCount.fetchAndAddRelaxed(1);
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
    //mNamespaces;
    //mBlockBeginSkips;
    //mBlockEndSkips;
    //mInlineNamespaceEndSkips;
}

CppParser::~CppParser()
{
    while (true) {
        //wait for all methods finishes running
        {
            QMutexLocker locker(&mMutex);
            if (!mParsing && (mLockCount == 0)) {
              mParsing = true;
              break;
            }
        }
        QThread::msleep(50);
        QCoreApplication* app = QApplication::instance();
        app->processEvents();
    }
}

void CppParser::addHardDefineByLine(const QString &line)
{
    QMutexLocker  locker(&mMutex);
    if (line.startsWith('#')) {
        mPreprocessor.addDefineByLine(line.mid(1).trimmed(), true);
    } else {
        mPreprocessor.addDefineByLine(line, true);
    }
}

void CppParser::addIncludePath(const QString &value)
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.addIncludePath(includeTrailingPathDelimiter(value));
}

void CppParser::addProjectIncludePath(const QString &value)
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.addProjectIncludePath(includeTrailingPathDelimiter(value));
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

QList<PStatement> CppParser::getListOfFunctions(const QString &fileName, const QString &phrase, int line)
{
    QMutexLocker locker(&mMutex);
    QList<PStatement> result;
    if (mParsing)
        return result;

    PStatement statement = findStatementOf(fileName,phrase, line);
    if (!statement)
        return result;
    PStatement parentScope = statement->parentScope.lock();
    if (parentScope && parentScope->kind == StatementKind::skNamespace) {
        PStatementList namespaceStatementsList = findNamespace(parentScope->command);
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

PStatement CppParser::findAndScanBlockAt(const QString &filename, int line)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return PStatement();
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(filename);
    if (!fileIncludes)
        return PStatement();

    PStatement statement = fileIncludes->scopes.findScopeAtLine(line);
    return statement;
}

PFileIncludes CppParser::findFileIncludes(const QString &filename, bool deleteIt)
{
    QMutexLocker locker(&mMutex);
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(filename,PFileIncludes());
    if (deleteIt && fileIncludes)
        mPreprocessor.includesList().remove(filename);
    return fileIncludes;
}

QString CppParser::findFirstTemplateParamOf(const QString &fileName, const QString &phrase, const PStatement& currentScope)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return "";
    // Remove pointer stuff from type
    QString s = phrase; // 'Type' is a keyword
    int i = s.indexOf('<');
    if (i>=0) {
        int t=getFirstTemplateParamEnd(s,i);
        return s.mid(i+1,t-i-1);
    }
    int position = s.length()-1;
    while ((position >= 0) && (s[position] == '*'
                               || s[position] == ' '
                               || s[position] == '&'))
        position--;
    if (position != s.length()-1)
        s.truncate(position+1);

    PStatement scopeStatement = currentScope;

    PStatement statement = findStatementOf(fileName,s,currentScope);
    return getFirstTemplateParam(statement,fileName, phrase, currentScope);
}

PStatement CppParser::findFunctionAt(const QString &fileName, int line)
{
    QMutexLocker locker(&mMutex);
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(fileName);
    if (!fileIncludes)
        return PStatement();
    for (PStatement& statement : fileIncludes->statements) {
        if (statement->kind != StatementKind::skFunction
                && statement->kind != StatementKind::skConstructor
                && statement->kind != StatementKind::skDestructor)
            continue;
        if (statement->line == line || statement->definitionLine == line)
            return statement;
    }
    return PStatement();
}

int CppParser::findLastOperator(const QString &phrase) const
{
    int i = phrase.length()-1;

    // Obtain stuff after first operator
    while (i>=0) {
        if ((i+1<phrase.length()) &&
                (phrase[i + 1] == '>') && (phrase[i] == '-'))
            return i;
        else if ((i+1<phrase.length()) &&
                 (phrase[i + 1] == ':') && (phrase[i] == ':'))
            return i;
        else if (phrase[i] == '.')
            return i;
        i--;
    }
    return -1;
}

PStatementList CppParser::findNamespace(const QString &name)
{
    QMutexLocker locker(&mMutex);
    return mNamespaces.value(name,PStatementList());
}

PStatement CppParser::findStatement(const QString &fullname)
{
    QMutexLocker locker(&mMutex);
    if (fullname.isEmpty())
        return PStatement();
    QStringList phrases = fullname.split("::");
    if (phrases.isEmpty())
        return PStatement();
    PStatement parentStatement;
    PStatement statement;
    foreach (const QString& phrase, phrases) {
        if (parentStatement && parentStatement->kind == StatementKind::skNamespace) {
            PStatementList lst = findNamespace(parentStatement->fullName);
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

PStatement CppParser::findStatementOf(const QString &fileName, const QString &phrase, int line)
{
    QMutexLocker locker(&mMutex);
    return findStatementOf(fileName,phrase,findAndScanBlockAt(fileName,line));
}

PStatement CppParser::findStatementOf(const QString &fileName, const QString &phrase, const PStatement& currentScope, PStatement &parentScopeType, bool force)
{
    QMutexLocker locker(&mMutex);
    PStatement result;
    parentScopeType = currentScope;
    if (mParsing && !force)
        return PStatement();

    //find the start scope statement
    QString namespaceName, remainder;
    QString nextScopeWord,operatorToken,memberName;
    PStatement statement;
    getFullNamespace(phrase, namespaceName, remainder);
    if (!namespaceName.isEmpty()) {  // (namespace )qualified Name
        PStatementList namespaceList = mNamespaces.value(namespaceName);

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
    } else if ((phrase.length()>2) &&
               (phrase[0]==':') && (phrase[1]==':')) {
        //global
        remainder= phrase.mid(2);
        remainder= splitPhrase(remainder,nextScopeWord,operatorToken,memberName);
        statement= findMemberOfStatement(nextScopeWord,PStatement());
        if (!statement)
            return PStatement();
    } else {
        //unqualified name
        parentScopeType = currentScope;
        remainder = splitPhrase(remainder,nextScopeWord,operatorToken,memberName);
        if (currentScope && (currentScope->kind == StatementKind::skNamespace)) {
            PStatementList namespaceList = mNamespaces.value(currentScope->fullName);
            if (!namespaceList || namespaceList->isEmpty())
                return PStatement();
            for (PStatement& currentNamespace:*namespaceList){
                statement = findMemberOfStatement(nextScopeWord,currentNamespace);
                if (statement)
                    break;
            }
            if (!statement)
                statement = findStatementStartingFrom(fileName,nextScopeWord,currentScope->parentScope.lock(),force);
        } else {
            statement = findStatementStartingFrom(fileName,nextScopeWord,parentScopeType,force);
        }
        if (!statement)
            return PStatement();
    }
    parentScopeType = currentScope;

    if (!memberName.isEmpty() && (statement->kind == StatementKind::skTypedef)) {
        PStatement typeStatement = findTypeDefinitionOf(fileName,statement->type, parentScopeType);
        if (typeStatement)
            statement = typeStatement;
    }

    //using alias like 'using std::vector;'
    if ((statement->kind ==  StatementKind::skAlias) &&
            (phrase!=statement->type)) {
        statement = findStatementOf(fileName, statement->type,
                                    currentScope, parentScopeType, force);
        if (!statement)
            return PStatement();
    }

    if (statement->kind == StatementKind::skConstructor) {
        // we need the class, not the construtor
        statement = statement->parentScope.lock();
        if (!statement)
            return PStatement();
    }

    PStatement lastScopeStatement;
    QString typeName;
    PStatement typeStatement;
    while (!memberName.isEmpty()) {
        if (statement->kind == StatementKind::skVariable
                || statement->kind ==  StatementKind::skParameter
                || statement->kind ==  StatementKind::skFunction) {

            bool isSTLContainerFunctions = false;

            if (statement->kind == StatementKind::skFunction){
                PStatement parentScope = statement->parentScope.lock();
                if (parentScope
                    && STLContainers.contains(parentScope->fullName)
                    && STLElementMethods.contains(statement->command)
                    && lastScopeStatement) {
                    isSTLContainerFunctions = true;
                    PStatement lastScopeParent = lastScopeStatement->parentScope.lock();
                    typeName=findFirstTemplateParamOf(fileName,lastScopeStatement->type,
                                                      lastScopeParent );
                    typeStatement=findTypeDefinitionOf(fileName, typeName,
                                                       lastScopeParent );
                }
            }
            if (!isSTLContainerFunctions)
                typeStatement = findTypeDefinitionOf(fileName,statement->type, parentScopeType);

            //it's stl smart pointer
            if ((typeStatement)
                    && STLPointers.contains(typeStatement->fullName)
                    && (operatorToken == "->")) {
                PStatement parentScope = statement->parentScope.lock();
                typeName=findFirstTemplateParamOf(fileName,statement->type, parentScope);
                typeStatement=findTypeDefinitionOf(fileName, typeName,parentScope);
            } else if ((typeStatement)
                       && STLContainers.contains(typeStatement->fullName)
                       && nextScopeWord.endsWith(']')) {
                //it's a std container
                PStatement parentScope = statement->parentScope.lock();
                typeName = findFirstTemplateParamOf(fileName,statement->type,
                                                    parentScope);
                typeStatement = findTypeDefinitionOf(fileName, typeName,
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
        if (!memberName.isEmpty() && (statement->kind == StatementKind::skTypedef)) {
            PStatement typeStatement = findTypeDefinitionOf(fileName,statement->type, parentScopeType);
            if (typeStatement)
                statement = typeStatement;
        }
    }
    return statement;
}

PStatement CppParser::findStatementOf(const QString &fileName, const QString &phrase, const PStatement& currentClass, bool force)
{
    PStatement statementParentType;
    return findStatementOf(fileName,phrase,currentClass,statementParentType,force);
}

PStatement CppParser::findStatementStartingFrom(const QString &fileName, const QString &phrase, const PStatement& startScope, bool force)
{
    QMutexLocker locker(&mMutex);
    if (mParsing && !force)
        return PStatement();

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
    result = findMemberOfStatement(phrase,PStatement());
    if (result)
        return result;

    //Find in all global usings
    const QSet<QString>& fileUsings = getFileUsings(fileName);
    // add members of all fusings
    for (const QString& namespaceName:fileUsings) {
        result = findStatementInNamespace(phrase,namespaceName);
        if (result)
            return result;
    }
    return PStatement();
}

PStatement CppParser::findTypeDefinitionOf(const QString &fileName, const QString &aType, const PStatement& currentClass)
{
    QMutexLocker locker(&mMutex);

    if (mParsing)
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

    PStatement statement = findStatementOf(fileName,s,currentClass);
    return getTypeDef(statement,fileName,aType);
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

QStringList CppParser::getClassesList()
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
            if (child->kind == StatementKind::skClass)
                list.append(child->command);
            if (!child->children.isEmpty())
                queue.enqueue(child);
        }
    }
    return list;
}

QSet<QString> CppParser::getFileDirectIncludes(const QString &filename)
{
    QMutexLocker locker(&mMutex);
    QSet<QString> list;
    if (mParsing)
        return list;
    if (filename.isEmpty())
        return list;
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(filename,PFileIncludes());

    if (fileIncludes) {
        QMap<QString, bool>::const_iterator iter = fileIncludes->includeFiles.cbegin();
        while (iter != fileIncludes->includeFiles.cend()) {
            if (iter.value())
                list.insert(iter.key());
            iter++;
        }
    }
    return list;

}

QSet<QString> CppParser::getFileIncludes(const QString &filename)
{
    QMutexLocker locker(&mMutex);
    QSet<QString> list;
    if (mParsing)
        return list;
    if (filename.isEmpty())
        return list;
    list.insert(filename);
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(filename,PFileIncludes());

    if (fileIncludes) {
        foreach (const QString& file, fileIncludes->includeFiles.keys()) {
            list.insert(file);
        }
    }
    return list;
}

QSet<QString> CppParser::getFileUsings(const QString &filename)
{
    QMutexLocker locker(&mMutex);
    QSet<QString> result;
    if (filename.isEmpty())
        return result;
    if (mParsing)
        return result;
    PFileIncludes fileIncludes= mPreprocessor.includesList().value(filename,PFileIncludes());
    if (fileIncludes) {
        foreach (const QString& usingName, fileIncludes->usings) {
            result.insert(usingName);
        }
        foreach (const QString& subFile,fileIncludes->includeFiles.keys()){
            PFileIncludes subIncludes = mPreprocessor.includesList().value(subFile,PFileIncludes());
            if (subIncludes) {
                foreach (const QString& usingName, subIncludes->usings) {
                    result.insert(usingName);
                }
            }
        }
    }
    return result;
}

QString CppParser::getHeaderFileName(const QString &relativeTo, const QString &line)
{
    QMutexLocker locker(&mMutex);
    return ::getHeaderFilename(relativeTo, line, mPreprocessor.includePathList(),
                             mPreprocessor.projectIncludePathList());
}

StatementKind CppParser::getKindOfStatement(const PStatement& statement)
{
    if (!statement)
        return StatementKind::skUnknown;
    if (statement->kind == StatementKind::skVariable) {
        if (!statement->parentScope.lock()) {
            return StatementKind::skGlobalVariable;
        }  else if (statement->scope == StatementScope::ssLocal) {
            return StatementKind::skLocalVariable;
        } else {
            return StatementKind::skVariable;
        }
    }
    return statement->kind;
}

void CppParser::invalidateFile(const QString &fileName)
{
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

bool CppParser::isIncludeLine(const QString &line)
{
    QString trimmedLine = line.trimmed();
    if ((trimmedLine.length() > 0)
            && trimmedLine.startsWith('#')) { // it's a preprocessor line
        if (trimmedLine.mid(1).trimmed().startsWith("include"))
            return true;
    }
    return false;
}

bool CppParser::isProjectHeaderFile(const QString &fileName)
{
    QMutexLocker locker(&mMutex);
    return ::isSystemHeaderFile(fileName,mPreprocessor.projectIncludePaths());
}

bool CppParser::isSystemHeaderFile(const QString &fileName)
{
    QMutexLocker locker(&mMutex);
    return ::isSystemHeaderFile(fileName,mPreprocessor.includePaths());
}

void CppParser::parseFile(const QString &fileName, bool inProject, bool onlyIfNotParsed, bool updateView)
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
        QString fName = fileName;
        if (onlyIfNotParsed && mPreprocessor.scannedFiles().contains(fName))
            return;

        QSet<QString> files = calculateFilesToBeReparsed(fileName);
        internalInvalidateFiles(files);

        if (inProject)
            mProjectFiles.insert(fileName);
        else {
            mProjectFiles.remove(fileName);
        }

        // Parse from disk or stream
        mFilesToScanCount = files.count();
        mFilesScannedCount = 0;

        // parse header files in the first parse
        foreach (const QString& file,files) {
            if (isHfile(file)) {
                mFilesScannedCount++;
                emit onProgress(file,mFilesToScanCount,mFilesScannedCount);
                if (!mPreprocessor.scannedFiles().contains(file)) {
                    internalParse(file);
                }
            }
        }
        //we only parse CFile in the second parse
        foreach (const QString& file,files) {
            if (isCfile(file)) {
                mFilesScannedCount++;
                emit onProgress(file,mFilesToScanCount,mFilesScannedCount);
                if (!mPreprocessor.scannedFiles().contains(file)) {
                    internalParse(file);
                }
            }
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
        // parse header files in the first parse
        foreach (const QString& file, mFilesToScan) {
            if (isHfile(file)) {
                mFilesScannedCount++;
                emit onProgress(mCurrentFile,mFilesToScanCount,mFilesScannedCount);
                if (!mPreprocessor.scannedFiles().contains(file)) {
                    internalParse(file);
                }
            }
        }
        //we only parse CFile in the second parse
        foreach (const QString& file,mFilesToScan) {
            if (isCfile(file)) {
                mFilesScannedCount++;
                emit onProgress(mCurrentFile,mFilesToScanCount,mFilesScannedCount);
                if (!mPreprocessor.scannedFiles().contains(file)) {
                    internalParse(file);
                }
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
            QString hintText = "#define";
            if (define->name != "")
                hintText += ' ' + define->name;
            if (define->args != "")
                hintText += ' ' + define->args;
            if (define->value != "")
                hintText += ' ' + define->value;
            addStatement(
              PStatement(), // defines don't belong to any scope
              "",
              hintText, // override hint
              "", // define has no type
              define->name,
              define->value,
              define->args,
              -1,
              StatementKind::skPreprocessor,
              StatementScope::ssGlobal,
              StatementClassScope::scsNone,
              true,
              false);
        }
    }
}

bool CppParser::parsing() const
{
    return mParsing;
}

void CppParser::reset()
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
        mPreprocessor.clear();
        mUniqId = 0;
        mSkipList.clear();
        mBlockBeginSkips.clear();
        mBlockEndSkips.clear();
        mInlineNamespaceEndSkips.clear();
        mParseLocalHeaders = true;
        mParseGlobalHeaders = true;
        mIsSystemHeader = false;
        mIsHeader = false;
        mIsProjectFile = false;

        mCurrentScope.clear();
        mCurrentClassScope.clear();
        mProjectFiles.clear();
        mFilesToScan.clear();
        mTokenizer.reset();
        // Remove all statements
        mStatementList.clear();

        // We haven't scanned anything anymore
        mPreprocessor.scannedFiles().clear();

        // We don't include anything anymore
        mPreprocessor.includesList().clear();

        mNamespaces.clear();
        mInlineNamespaces.clear();

        mPreprocessor.clearProjectIncludePaths();
        mPreprocessor.clearIncludePaths();
        mProjectFiles.clear();
    }
}

void CppParser::unFreeze()
{
    QMutexLocker locker(&mMutex);
    mLockCount--;
}

QSet<QString> CppParser::scannedFiles()
{
    return mPreprocessor.scannedFiles();
}

QString CppParser::getScopePrefix(const PStatement& statement){
    switch (statement->classScope) {
    case StatementClassScope::scsPublic:
        return "public";
    case StatementClassScope::scsPrivate:
        return "private";
    case StatementClassScope::scsProtected:
        return "protected";
    default:
        return "";
    }
}

QString CppParser::prettyPrintStatement(const PStatement& statement, const QString& filename, int line)
{
    QString result;
    if (!statement->hintText.isEmpty()) {
      if (statement->kind != StatementKind::skPreprocessor)
          result = statement->hintText;
      else if (statement->command == "__FILE__")
          result = '"'+filename+'"';
      else if (statement->command == "__LINE__")
          result = QString("\"%1\"").arg(line);
      else if (statement->command == "__DATE__")
          result = QString("\"%1\"").arg(QDate::currentDate().toString(Qt::ISODate));
      else if (statement->command == "__TIME__")
          result = QString("\"%1\"").arg(QTime::currentTime().toString(Qt::ISODate));
      else
          result = statement->hintText;
    } else {
        switch(statement->kind) {
        case StatementKind::skFunction:
        case StatementKind::skVariable:
        case StatementKind::skParameter:
        case StatementKind::skClass:
            if (statement->scope!= StatementScope::ssLocal)
                result = getScopePrefix(statement)+ ' '; // public
            result += statement->type + ' '; // void
            result += statement->fullName; // A::B::C::Bar
            result += statement->args; // (int a)
            break;
        case StatementKind::skNamespace:
            result = statement->fullName; // Bar
            break;
        case StatementKind::skConstructor:
            result = getScopePrefix(statement); // public
            result += QObject::tr("constructor") + ' '; // constructor
            result += statement->type + ' '; // void
            result += statement->fullName; // A::B::C::Bar
            result += statement->args; // (int a)
            break;
        case StatementKind::skDestructor:
            result = getScopePrefix(statement); // public
            result += QObject::tr("destructor") + ' '; // constructor
            result += statement->type + ' '; // void
            result += statement->fullName; // A::B::C::Bar
            result += statement->args; // (int a)
            break;
        default:
            break;
        }
    }
    return result;
}

QString CppParser::getFirstTemplateParam(const PStatement& statement,
                                         const QString& filename,
                                         const QString& phrase,
                                         const PStatement& currentScope)
{
    if (!statement)
        return "";
    if (statement->kind != StatementKind::skTypedef)
        return "";
    if (statement->type == phrase) // prevent infinite loop
        return "";
    return findFirstTemplateParamOf(filename,statement->type, currentScope);
}

int CppParser::getFirstTemplateParamEnd(const QString &s, int startAt)
{
    int i = startAt;
    int level = 0; // assume we start on top of '<'
    while (i < s.length()) {
        switch (s[i].unicode()) {
        case '<':
            level++;
            break;
        case ',':
            if (level == 1)
                return i;
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

void CppParser::addFileToScan(const QString& value, bool inProject)
{
    QMutexLocker locker(&mMutex);
    //value.replace('/','\\'); // only accept full file names

    // Update project listing
    if (inProject)
        mProjectFiles.insert(value);

    // Only parse given file
    if (!mPreprocessor.scannedFiles().contains(value)) {
        mFilesToScan.insert(value);
    }

}

PStatement CppParser::addInheritedStatement(const PStatement& derived, const PStatement& inherit, StatementClassScope access)
{

    PStatement statement = addStatement(
      derived,
      inherit->fileName,
      inherit->hintText,
      inherit->type, // "Type" is already in use
      inherit->command,
      inherit->args,
      inherit->value,
      inherit->line,
      inherit->kind,
      inherit->scope,
      access,
      true,
      inherit->isStatic);
    statement->inheritanceList.append(inherit->inheritanceList),
    statement->isInherited = true;
    return statement;
}

PStatement CppParser::addChildStatement(const PStatement& parent, const QString &fileName,
                                        const QString &hintText, const QString &aType,
                                        const QString &command, const QString &args,
                                        const QString &value, int line, StatementKind kind,
                                        const StatementScope& scope, const StatementClassScope& classScope,
                                        bool isDefinition, bool isStatic)
{
    return addStatement(
                parent,
                fileName,
                hintText,
                aType,
                command,
                args,
                value,
                line,
                kind,
                scope,
                classScope,
                isDefinition,
                isStatic);
}

PStatement CppParser::addStatement(const PStatement& parent,
                                   const QString &fileName,
                                   const QString &hintText,
                                   const QString &aType,
                                   const QString &command,
                                   const QString &args,
                                   const QString &value,
                                   int line, StatementKind kind,
                                   const StatementScope& scope,
                                   const StatementClassScope& classScope, bool isDefinition, bool isStatic)
{
    // Move '*', '&' to type rather than cmd (it's in the way for code-completion)
    QString newType = aType;
    QString newCommand = command;
    while (!newCommand.isEmpty() && (newCommand.front() == '*' || newCommand.front() == '&')) {
        newType += newCommand.front();
        newCommand.remove(0,1); // remove first
    }

    QString noNameArgs = "";
    if (kind == StatementKind::skConstructor
            || kind == StatementKind::skFunction
            || kind == StatementKind::skDestructor
            || kind == StatementKind::skVariable) {
        noNameArgs = removeArgNames(args);
        //find
        PStatement oldStatement = findStatementInScope(command,noNameArgs,kind,parent);
        if (oldStatement && isDefinition && !oldStatement->hasDefinition) {
            oldStatement->hasDefinition = true;
            if (oldStatement->fileName!=fileName) {
                PFileIncludes fileIncludes1=mPreprocessor.includesList().value(fileName);
                if (fileIncludes1) {
                    fileIncludes1->statements.insert(oldStatement->fullName,
                                                     oldStatement);
                    fileIncludes1->dependingFiles.insert(oldStatement->fileName);
                    PFileIncludes fileIncludes2=mPreprocessor.includesList().value(oldStatement->fileName);
                    if (fileIncludes2) {
                        fileIncludes2->dependedFiles.insert(fileName);
                    }
                }
            }
            oldStatement->definitionLine = line;
            oldStatement->definitionFileName = fileName;
            return oldStatement;
        }
    }
    PStatement result = std::make_shared<Statement>();
    result->parentScope = parent;
    result->hintText = hintText;
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
    result->kind = kind;
    //result->inheritanceList;
    result->scope = scope;
    result->classScope = classScope;
    result->hasDefinition = isDefinition;
    result->line = line;
    result->definitionLine = line;
    result->fileName = fileName;
    result->definitionFileName = fileName;
    if (!fileName.isEmpty())
        result->inProject = mIsProjectFile;
    else
        result->inProject = false;
    result->inSystemHeader = mIsSystemHeader;
    //result->children;
    //result->friends;
    result->isStatic = isStatic;
    result->isInherited = false;
    if (scope == StatementScope::ssLocal)
        result->fullName =  newCommand;
    else
        result->fullName =  getFullStatementName(newCommand, parent);
    result->usageCount = -1;
    result->freqTop = 0;
    mStatementList.add(result);
    if (result->kind == StatementKind::skNamespace) {
        PStatementList namespaceList = mNamespaces.value(result->fullName,PStatementList());
        if (!namespaceList) {
            namespaceList=std::make_shared<StatementList>();
            mNamespaces.insert(result->fullName,namespaceList);
        }
        namespaceList->append(result);
    }

    if (result->kind!= StatementKind::skBlock) {
        PFileIncludes fileIncludes = mPreprocessor.includesList().value(fileName);
        if (fileIncludes) {
            fileIncludes->statements.insert(result->fullName,result);
            fileIncludes->declaredStatements.insert(result->fullName,result);
        }
    }
    return result;
}

void CppParser::setInheritance(int index, const PStatement& classStatement, bool isStruct)
{
    // Clear it. Assume it is assigned
    classStatement->inheritanceList.clear();
    StatementClassScope lastInheritScopeType = StatementClassScope::scsNone;
    // Assemble a list of statements in text form we inherit from
    while (true) {
        StatementClassScope inheritScopeType = getClassScope(index);
        if (inheritScopeType == StatementClassScope::scsNone) {
            if (mTokenizer[index]->text.front()!=','
                    && mTokenizer[index]->text.front()!=':'
                    && mTokenizer[index]->text.front()!='(') {
                QString basename = mTokenizer[index]->text;
                //remove template staff
                if (basename.endsWith('>')) {
                    int pBegin = basename.indexOf('<');
                    if (pBegin>=0)
                        basename.truncate(pBegin);
                }
                // Find the corresponding PStatement
                PStatement statement = findStatementOf(mCurrentFile,basename,
                                                       classStatement->parentScope.lock(),true);
                if (statement && statement->kind == StatementKind::skClass) {
                    classStatement->inheritanceList.append(statement);
                    inheritClassStatement(classStatement,isStruct,statement,lastInheritScopeType);
                }
            }
        }
        index++;
        lastInheritScopeType = inheritScopeType;
        if (index >= mTokenizer.tokenCount())
            break;
        if (mTokenizer[index]->text.front() == '{'
                || mTokenizer[index]->text.front() == ';')
            break;
    }
}

bool CppParser::isCurrentScope(const QString &command)
{
    PStatement statement = getCurrentScope();
    if (!statement)
        return false;
    QString s = command;
    // remove template staff
    int i= command.indexOf('<');
    if (i>=0) {
        s.truncate(i);
    }
    return (statement->command == s);
}

void CppParser::addSoloScopeLevel(PStatement& statement, int line)
{
    // Add class list

    PStatement parentScope;
    if (statement && (statement->kind == StatementKind::skBlock)) {
        parentScope = statement->parentScope.lock();
        while (parentScope && (parentScope->kind == StatementKind::skBlock)) {
            parentScope = parentScope->parentScope.lock();
        }
        if (!parentScope)
            statement.reset();
    }

    if (mCurrentClassScope.count()>0) {
        mCurrentClassScope.back() = mClassScope;
    }

    mCurrentScope.append(statement);

    PFileIncludes fileIncludes = mPreprocessor.includesList().value(mCurrentFile);

    if (fileIncludes) {
        fileIncludes->scopes.addScope(line,statement);
    }

    // Set new scope
    if (!statement)
        mClassScope = StatementClassScope::scsNone; // {}, namespace or class that doesn't exist
    else if (statement->kind == StatementKind::skNamespace)
        mClassScope = StatementClassScope::scsNone;
    else if (statement->type == "class")
        mClassScope = StatementClassScope::scsPrivate; // classes are private by default
    else
        mClassScope = StatementClassScope::scsPublic; // structs are public by default
    mCurrentClassScope.append(mClassScope);
}

void CppParser::removeScopeLevel(int line)
{
    // Remove class list
    if (mCurrentScope.isEmpty())
        return; // TODO: should be an exception
    PStatement currentScope = mCurrentScope.back();;
    PFileIncludes fileIncludes = mPreprocessor.includesList().value(mCurrentFile);
    if (currentScope && (currentScope->kind == StatementKind::skBlock)) {
        if (currentScope->children.isEmpty()) {
            // remove no children block
            if (fileIncludes) {
                fileIncludes->scopes.removeLastScope();
            }
            mStatementList.deleteStatement(currentScope);
        } else {
            fileIncludes->statements.insert(currentScope->fullName,currentScope);
            fileIncludes->declaredStatements.insert(currentScope->fullName,currentScope);
        }
    }
    mCurrentScope.pop_back();
    mCurrentClassScope.pop_back();

    // Set new scope
    currentScope = getCurrentScope();
  //  fileIncludes:=FindFileIncludes(fCurrentFile);
    if (fileIncludes && fileIncludes->scopes.lastScope()!=currentScope) {
        fileIncludes->scopes.addScope(line,currentScope);
    }

    if (!currentScope) {
        mClassScope = StatementClassScope::scsNone;
    } else {
        mClassScope = mCurrentClassScope.back();
    }
}

int CppParser::skipBraces(int startAt)
{
    int i = startAt;
    int level = 0; // assume we start on top of {
    while (i < mTokenizer.tokenCount()) {
        switch(mTokenizer[i]->text.front().unicode()) {
        case '{': level++;
            break;
        case '}':
            level--;
            if (level==0)
                return i;
        }
        i++;
    }
    return startAt;
}

int CppParser::skipBracket(int startAt)
{
    int i = startAt;
    int level = 0; // assume we start on top of {
    while (i < mTokenizer.tokenCount()) {
        switch(mTokenizer[i]->text.front().unicode()) {
        case '[': level++;
            break;
        case ']':
            level--;
            if (level==0)
                return i;
        }
        i++;
    }
    return startAt;
}

bool CppParser::checkForCatchBlock()
{
//    return  mIndex < mTokenizer.tokenCount() &&
//            mTokenizer[mIndex]->text == "catch";
    return  mTokenizer[mIndex]->text == "catch";
}

bool CppParser::checkForEnum()
{
//    return  mIndex < mTokenizer.tokenCount() &&
//            mTokenizer[mIndex]->text == "enum";
    return  mTokenizer[mIndex]->text == "enum";
}

bool CppParser::checkForForBlock()
{
//    return  mIndex < mTokenizer.tokenCount() &&
//            mTokenizer[mIndex]->text == "for";
    return  mTokenizer[mIndex]->text == "for";
}

bool CppParser::checkForKeyword()
{
    SkipType st = mCppKeywords.value(mTokenizer[mIndex]->text,SkipType::skNone);
    return st!=SkipType::skNone;
}

bool CppParser::checkForMethod(QString &sType, QString &sName, QString &sArgs, bool &isStatic, bool &isFriend)
{
    PStatement scope = getCurrentScope();

    if (scope && !(scope->kind == StatementKind::skNamespace
                   || scope->kind == StatementKind::skClass)) { //don't care function declaration in the function's
        return false;
    }

    // Function template:
    // compiler directives (>= 0 words), added to type
    // type (>= 1 words)
    // name (1 word)
    // (argument list)
    // ; or {

    isStatic = false;
    isFriend = false;

    sType = ""; // should contain type "int"
    sName = ""; // should contain function name "foo::function"
    sArgs = ""; // should contain argument list "(int a)"

    bool bTypeOK = false;
    bool bNameOK = false;
    bool bArgsOK = false;

    // Don't modify index
    int indexBackup = mIndex;

    // Gather data for the string parts
    while ((mIndex < mTokenizer.tokenCount()) && !isSeperator(mTokenizer[mIndex]->text[0])) {
        if ((mIndex + 1 < mTokenizer.tokenCount())
                && (mTokenizer[mIndex + 1]->text[0] == '(')) { // and start of a function
            //it's not a function define
            if ((mIndex+2 < mTokenizer.tokenCount()) && (mTokenizer[mIndex + 2]->text[0] == ','))
                break;

            if ((mIndex+2 < mTokenizer.tokenCount()) && (mTokenizer[mIndex + 2]->text[0] == ';')) {
                if (isNotFuncArgs(mTokenizer[mIndex + 1]->text))
                    break;
            }
            sName = mTokenizer[mIndex]->text;
            sArgs = mTokenizer[mIndex + 1]->text;
            bTypeOK = !sType.isEmpty();
            bNameOK = !sName.isEmpty();
            bArgsOK = !sArgs.isEmpty();

            // Allow constructor/destructor too
            if (!bTypeOK) {
                // Check for constructor/destructor outside class body
                int delimPos = sName.indexOf("::");
                if (delimPos >= 0) {
                    bTypeOK = true;
                    sType = sName.mid(0, delimPos);

                    // remove template staff
                    int pos1 = sType.indexOf('<');
                    if (pos1>=0) {
                        sType.truncate(pos1);
                        sName = sType+sName.mid(delimPos);
                    }
                }
            }

            // Are we inside a class body?
            if (!bTypeOK) {
                sType = mTokenizer[mIndex]->text;
                if (sType[0] == '~')
                    sType.remove(0,1);
                bTypeOK = isCurrentScope(sType); // constructor/destructor
            }
            break;
        } else {
            //if IsValidIdentifier(mTokenizer[mIndex]->text) then
            // Still walking through type
            QString s = expandMacroType(mTokenizer[mIndex]->text); //todo: do we really need expand macro? it should be done in preprocessor
            if (s == "static")
                isStatic = true;
            if (s == "friend")
                isFriend = true;
            if (!s.isEmpty() && !(s=="extern"))
                sType = sType + ' '+ s;
            bTypeOK = !sType.isEmpty();
        }
        mIndex++;
    }

    mIndex = indexBackup;

    // Correct function, don't jump over
    if (bTypeOK && bNameOK && bArgsOK) {
        sType = sType.trimmed(); // should contain type "int"
        sName = sName.trimmed(); // should contain function name "foo::function"
        sArgs = sArgs.trimmed(); // should contain argument list "(int a)"
        return true;
    } else
        return false;
}

bool CppParser::checkForNamespace()
{
    return ((mIndex < mTokenizer.tokenCount()-1)
            && (mTokenizer[mIndex]->text == "namespace"))
        || (
            (mIndex+1 < mTokenizer.tokenCount()-1)
                && (mTokenizer[mIndex]->text == "inline")
                && (mTokenizer[mIndex+1]->text == "namespace"));
}

bool CppParser::checkForPreprocessor()
{
//    return (mIndex < mTokenizer.tokenCount())
//            && ( "#" == mTokenizer[mIndex]->text);
    return (mTokenizer[mIndex]->text.startsWith('#'));
}

bool CppParser::checkForScope()
{
    return (mIndex < mTokenizer.tokenCount() - 1)
            && (mTokenizer[mIndex + 1]->text == ':')
            && (
                (mTokenizer[mIndex]->text == "public")
                || (mTokenizer[mIndex]->text == "protected")
                || (mTokenizer[mIndex]->text == "private")
                );
}

void CppParser::checkForSkipStatement()
{
    if ((mSkipList.count()>0) && (mIndex == mSkipList.back())) { // skip to next ';'
        do {
            mIndex++;
        } while ((mIndex < mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text[0] != ';'));
        mIndex++; //skip ';'
        mSkipList.pop_back();
    }
}

bool CppParser::checkForStructs()
{
    int dis = 0;
    if ((mTokenizer[mIndex]->text == "friend")
            || (mTokenizer[mIndex]->text == "public")
            || (mTokenizer[mIndex]->text == "private"))
        dis = 1;
    if (mIndex >= mTokenizer.tokenCount() - 2 - dis)
        return false;
    QString word = mTokenizer[mIndex+dis]->text;
    int keyLen = calcKeyLenForStruct(word);
    if (keyLen<0)
        return false;
    bool result = (word.length() == keyLen) || isSpaceChar(word[keyLen] == ' ')
            || (word[keyLen] == '[');

    if (result) {
        if (mTokenizer[mIndex + 2+dis]->text[0] != ';') { // not: class something;
            int i = mIndex+dis +1;
            // the check for ']' was added because of this example:
            // struct option long_options[] = {
            //		{"debug", 1, 0, 'D'},
            //		{"info", 0, 0, 'i'},
            //    ...
            // };
            while (i < mTokenizer.tokenCount()) {
                QChar ch = mTokenizer[i]->text.back();
                if (ch=='{' || ch == ':')
                    break;
                switch(ch.unicode()) {
                case ';':
                case '}':
                case ',':
                case ')':
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

bool CppParser::checkForTypedef()
{
    return mTokenizer[mIndex]->text == "typedef";
}

bool CppParser::checkForTypedefEnum()
{
    //we assume that typedef is the current index, so we check the next
    //should call CheckForTypedef first!!!
    return (mIndex < mTokenizer.tokenCount() - 1) &&
            (mTokenizer[mIndex + 1]->text == "enum");
}

bool CppParser::checkForTypedefStruct()
{
    //we assume that typedef is the current index, so we check the next
    //should call CheckForTypedef first!!!
    if (mIndex+1 >= mTokenizer.tokenCount())
        return false;
    QString word = mTokenizer[mIndex + 1]->text;
    int keyLen = calcKeyLenForStruct(word);
    if (keyLen<0)
        return false;
    return (word.length() == keyLen) || isSpaceChar(word[keyLen]) || word[keyLen]=='[';
}

bool CppParser::checkForUsing()
{
    return (mIndex < mTokenizer.tokenCount()-1) && mTokenizer[mIndex]->text == "using";

}

bool CppParser::checkForVar()
{
    // Be pessimistic
    bool result = false;

    // Store old index
    int indexBackup = mIndex;

    // Use mIndex so we can reuse checking functions
    if (mIndex + 1 < mTokenizer.tokenCount()) {
        // Check the current and the next token
        for (int i = 0; i<=1; i++) {
            if (checkForKeyword()
                    || isInvalidVarPrefixChar(mTokenizer[mIndex]->text.front())
                    || (mTokenizer[mIndex]->text.back() == '.')
                    || (
                        (mTokenizer[mIndex]->text.length() > 1) &&
                        (mTokenizer[mIndex]->text[mTokenizer[mIndex]->text.length() - 2] == '-') &&
                        (mTokenizer[mIndex]->text[mTokenizer[mIndex]->text.length() - 1] == '>'))
                    ) {
                    // Reset index and fail
                    mIndex = indexBackup;
                    return false;
            } // Could be a function pointer?
            else if (mTokenizer[mIndex]->text.front() == '(') {
                // Quick fix: there must be a pointer operator in the first tiken
                if ( (mIndex + 1 >= mTokenizer.tokenCount())
                     || (mTokenizer[mIndex + 1]->text.front() != '(')
                     || mTokenizer[mIndex]->text.indexOf('*')<0) {
                    // Reset index and fail
                    mIndex = indexBackup;
                    return false;
                }
            }
            mIndex++;
        }
    }

    // Revert to the point we started at
    mIndex = indexBackup;

    // Fail if we do not find a comma or a semicolon or a ( (inline constructor)
    while (mIndex < mTokenizer.tokenCount()) {
        if (mTokenizer[mIndex]->text.front() == '#'
                || mTokenizer[mIndex]->text.front() == '}'
                || checkForKeyword()) {
            break; // fail
//        } else if ((mTokenizer[mIndex]->text.length()>1) && (mTokenizer[mIndex]->text[0] == '(')
//                   && (mTokenizer[mIndex]->text[1] == '(')) { // TODO: is this used to remove __attribute stuff?
//            break;
        } else if (mTokenizer[mIndex]->text.front() == ','
                   || mTokenizer[mIndex]->text.front() == ';'
                   || mTokenizer[mIndex]->text.front() == '{') {
            result = true;
            break;
        }
        mIndex++;
    }

    // Revert to the point we started at
    mIndex = indexBackup;
    return result;
}

int CppParser::getCurrentBlockEndSkip()
{
    if (mBlockEndSkips.isEmpty())
        return mTokenizer.tokenCount()+1;
    return mBlockEndSkips.back();
}

int CppParser::getCurrentBlockBeginSkip()
{
    if (mBlockBeginSkips.isEmpty())
        return mTokenizer.tokenCount()+1;
    return mBlockBeginSkips.back();
}

int CppParser::getCurrentInlineNamespaceEndSkip()
{
    if (mInlineNamespaceEndSkips.isEmpty())
        return mTokenizer.tokenCount()+1;
    return mInlineNamespaceEndSkips.back();
}

PStatement CppParser::getCurrentScope()
{
    if (mCurrentScope.isEmpty()) {
        return PStatement();
    }
    return mCurrentScope.back();
}

void CppParser::getFullNamespace(const QString &phrase, QString &sNamespace, QString &member)
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

QString CppParser::getFullStatementName(const QString &command, const PStatement& parent)
{
    PStatement scopeStatement=parent;
    while (scopeStatement && !isNamedScope(scopeStatement->kind))
        scopeStatement = scopeStatement->parentScope.lock();
    if (scopeStatement)
        return scopeStatement->fullName + "::" + command;
    else
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
    PStatement result = findStatementOf(mCurrentFile,s,parentScope,true);
    if (result && result->kind!=StatementKind::skClass)
        return PStatement();
    return result;
}

StatementScope CppParser::getScope()
{
    // Don't blindly trust levels. Namespaces and externs can have levels too
    PStatement currentScope = getCurrentScope();

    // Invalid class or namespace/extern
    if (!currentScope || (currentScope->kind == StatementKind::skNamespace))
        return StatementScope::ssGlobal;
    else if (currentScope->kind == StatementKind::skClass)
        return StatementScope::ssClassLocal;
    else
        return StatementScope::ssLocal;
}

QString CppParser::getStatementKey(const QString &sName, const QString &sType, const QString &sNoNameArgs)
{
    return sName + "--" + sType + "--" + sNoNameArgs;
}

PStatement CppParser::getTypeDef(const PStatement& statement,
                                 const QString& fileName, const QString& aType)
{
    if (!statement) {
        return PStatement();
    }
    if (statement->kind == StatementKind::skClass) {
        return statement;
    } else if (statement->kind == StatementKind::skTypedef) {
        if (statement->type == aType) // prevent infinite loop
            return statement;
        PStatement result = findTypeDefinitionOf(fileName,statement->type, statement->parentScope.lock());
        if (!result) // found end of typedef trail, return result
            return statement;
        return result;
    } else
        return PStatement();
}

void CppParser::handleCatchBlock()
{
    int startLine= mTokenizer[mIndex]->line;
    mIndex++; // skip for/catch;
    if (!((mIndex < mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text.startsWith('('))))
        return;
    //skip params
    int i2=mIndex+1;
    if (i2>=mTokenizer.tokenCount())
        return;
    if (mTokenizer[i2]->text.startsWith('{')) {
        mBlockBeginSkips.append(i2);
        int i = skipBraces(i2);
        if (i==i2) {
            mBlockEndSkips.append(mTokenizer.tokenCount());
        } else {
            mBlockEndSkips.append(i);
        }
    } else {
        int i=i2;
        while ((i<mTokenizer.tokenCount()) && !mTokenizer[i]->text.startsWith(';'))
            i++;
        mBlockEndSkips.append(i);
    }
    // add a block
    PStatement block = addStatement(
      getCurrentScope(),
      mCurrentFile,
      "", // override hint
      "",
      "",
      "",
      "",
      startLine,
      StatementKind::skBlock,
      getScope(),
      mClassScope,
      true,
      false);
    addSoloScopeLevel(block,startLine);
    if (!mTokenizer[mIndex]->text.contains("..."))
        scanMethodArgs(block,mTokenizer[mIndex]->text);
}

void CppParser::handleEnum()
{
    //todo : handle enum class
    QString enumName = "";
    bool isEnumClass = false;
    int startLine = mTokenizer[mIndex]->line;
    mIndex++; //skip 'enum'

    if (mIndex < mTokenizer.tokenCount() && mTokenizer[mIndex]->text == "class") {
        //enum class
        isEnumClass = true;
        mIndex++; //skip class

    }
    if ((mIndex< mTokenizer.tokenCount()) && mTokenizer[mIndex]->text.startsWith('{')) { // enum {...} NAME
        // Skip to the closing brace
        int i = skipBraces(mIndex);
        // Have we found the name?
        if ((i + 1 < mTokenizer.tokenCount()) && mTokenizer[i]->text.startsWith('}')) {
            if (!mTokenizer[i + 1]->text.startsWith(';'))
                enumName = mTokenizer[i + 1]->text.trimmed();
        }
    } else if (mIndex+1< mTokenizer.tokenCount() && mTokenizer[mIndex+1]->text.startsWith('{')){ // enum NAME {...};
        if ( (mIndex< mTokenizer.tokenCount()) && mTokenizer[mIndex]->text == "class") {
            //enum class {...} NAME
            isEnumClass = true;
            mIndex++;
        }
        while ((mIndex < mTokenizer.tokenCount()) &&
               !(mTokenizer[mIndex]->text.startsWith('{')
                  || mTokenizer[mIndex]->text.startsWith(';'))) {
            enumName += mTokenizer[mIndex]->text + ' ';
            mIndex++;
        }
        enumName = enumName.trimmed();
        // An opening brace must be present after NAME
        if ((mIndex >= mTokenizer.tokenCount()) || !mTokenizer[mIndex]->text.startsWith('{'))
            return;
    } else {
        // enum NAME blahblah
        // it's an old c-style enum variable definition
        return;
    }

    // Add statement for enum name too
    PStatement enumStatement;
    if (!enumName.isEmpty()) {
        if (isEnumClass) {
            enumStatement=addStatement(
                        getCurrentScope(),
                        mCurrentFile,
                        "enum class "+enumName,
                        "enum class",
                        enumName,
                        "",
                        "",
                        startLine,
                        StatementKind::skEnumClassType,
                        getScope(),
                        mClassScope,
                        true,
                        false);
        } else {
            enumStatement=addStatement(
                        getCurrentScope(),
                        mCurrentFile,
                        "enum "+enumName,
                        "enum",
                        enumName,
                        "",
                        "",
                        startLine,
                        StatementKind::skEnumType,
                        getScope(),
                        mClassScope,
                        true,
                        false);
        }
    } else {
        enumStatement = getCurrentScope();
    }

    // Skip opening brace
    mIndex++;

    // Call every member "enum NAME ITEMNAME"
    QString lastType("enum");
    if (!enumName.isEmpty())
        lastType += ' ' + enumName;
    QString cmd;
    QString args;
    if (!mTokenizer[mIndex]->text.startsWith('}')) {
        while ((mIndex < mTokenizer.tokenCount()) &&
                         !isblockChar(mTokenizer[mIndex]->text[0])) {
            if (!mTokenizer[mIndex]->text.startsWith(',')) {
                if (mTokenizer[mIndex]->text.endsWith(']')) { //array; break args
                    int p = mTokenizer[mIndex]->text.indexOf('[');
                    cmd = mTokenizer[mIndex]->text.mid(0,p);
                    args = mTokenizer[mIndex]->text.mid(p);
                } else {
                    cmd = mTokenizer[mIndex]->text;
                    args = "";
                }
                if (isEnumClass) {
                    if (enumStatement) {
                        addStatement(
                          enumStatement,
                          mCurrentFile,
                          lastType + "::" + mTokenizer[mIndex]->text, // override hint
                          lastType,
                          cmd,
                          args,
                          "",
                          //mTokenizer[mIndex]^.Line,
                          startLine,
                          StatementKind::skEnum,
                          getScope(),
                          mClassScope,
                          true,
                          false);
                    }
                } else {
                    if (enumStatement) {
                        addStatement(
                          enumStatement,
                          mCurrentFile,
                          lastType + "::" + mTokenizer[mIndex]->text, // override hint
                          lastType,
                          cmd,
                          args,
                          "",
                          //mTokenizer[mIndex]^.Line,
                          startLine,
                          StatementKind::skEnum,
                          getScope(),
                          mClassScope,
                          true,
                          false);
                    }
                    addStatement(
                      getCurrentScope(),
                      mCurrentFile,
                      lastType + "::" + mTokenizer[mIndex]->text, // override hint
                      lastType,
                      cmd,
                      args,
                      "",
                      //mTokenizer[mIndex]^.Line,
                      startLine,
                      StatementKind::skEnum,
                      getScope(),
                      mClassScope,
                      true,
                      false);
                }
            }
            mIndex ++ ;
        }
    }
    // Step over closing brace
    if ((mIndex < mTokenizer.tokenCount()) && mTokenizer[mIndex]->text.startsWith('}'))
        mIndex++;
}

void CppParser::handleForBlock()
{
    int startLine = mTokenizer[mIndex]->line;
    mIndex++; // skip for/catch;
    if (!(mIndex < mTokenizer.tokenCount()))
        return;
    int i=mIndex;
    while ((i<mTokenizer.tokenCount()) && !mTokenizer[i]->text.startsWith(';'))
        i++;
    if (i>=mTokenizer.tokenCount())
        return;
    int i2 = i+1; //skip over ';' (tokenizer have change for(;;) to for(;)
    if (i2>=mTokenizer.tokenCount())
        return;
    if (mTokenizer[i2]->text.startsWith('{')) {
        mBlockBeginSkips.append(i2);
        i=skipBraces(i2);
        if (i==i2)
            mBlockEndSkips.append(mTokenizer.tokenCount());
        else
            mBlockEndSkips.append(i);
    } else {
        i=i2;
        while ((i<mTokenizer.tokenCount()) && !mTokenizer[i]->text.startsWith(';'))
            i++;
        mBlockEndSkips.append(i);
    }
    // add a block
    PStatement block = addStatement(
                getCurrentScope(),
                mCurrentFile,
                "", // override hint
                "",
                "",
                "",
                "",
                startLine,
                StatementKind::skBlock,
                getScope(),
                mClassScope,
                true,
                false);

    addSoloScopeLevel(block,startLine);
}

void CppParser::handleKeyword()
{
    // Skip
    SkipType skipType = mCppKeywords.value(mTokenizer[mIndex]->text,SkipType::skNone);
    switch (skipType) {
    case SkipType::skItself:
        // skip it;
        mIndex++;
        break;
    case SkipType::skToSemicolon:
        // Skip to ;
        while (mIndex < mTokenizer.tokenCount() && !mTokenizer[mIndex]->text.startsWith(';'))
            mIndex++;
        mIndex++;// step over
        break;
    case SkipType::skToColon:
        // Skip to :
        while (mIndex < mTokenizer.tokenCount() && !mTokenizer[mIndex]->text.startsWith(':'))
            mIndex++;
        break;
    case SkipType::skToRightParenthesis:
        // skip to )
        while (mIndex < mTokenizer.tokenCount() && !mTokenizer[mIndex]->text.endsWith(')'))
            mIndex++;
        mIndex++; // step over
        break;
    case SkipType::skToLeftBrace:
        // Skip to {
        while (mIndex < mTokenizer.tokenCount() && !mTokenizer[mIndex]->text.startsWith('{'))
            mIndex++;
        break;
    case SkipType::skToRightBrace:
        // Skip to }
        while (mIndex < mTokenizer.tokenCount() && !mTokenizer[mIndex]->text.startsWith('}'))
            mIndex++;
        mIndex++; // step over
        break;
    default:
        break;
    }
}

void CppParser::handleMethod(const QString &sType, const QString &sName, const QString &sArgs, bool isStatic, bool isFriend)
{
    bool isValid = true;
    bool isDeclaration = false; // assume it's not a prototype
    int i = mIndex;
    int startLine = mTokenizer[mIndex]->line;

    // Skip over argument list
    while ((mIndex < mTokenizer.tokenCount()) && ! (
               isblockChar(mTokenizer[mIndex]->text.front())
                           || mTokenizer[mIndex]->text.startsWith(':')))
        mIndex++;

    if (mIndex >= mTokenizer.tokenCount()) // not finished define, just skip it;
        return;

    PStatement functionClass = getCurrentScope();
    // Check if this is a prototype
    if (mTokenizer[mIndex]->text.startsWith(';')
            || mTokenizer[mIndex]->text.startsWith('}')) {// prototype
        isDeclaration = true;
    } else {
        // Find the function body start after the inherited constructor
        if ((mIndex < mTokenizer.tokenCount()) && mTokenizer[mIndex]->text.startsWith(':')) {
            while ((mIndex < mTokenizer.tokenCount()) && !isblockChar(mTokenizer[mIndex]->text.front()))
                mIndex++;
        }

        // Still a prototype
        if ((mIndex < mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text.startsWith(';')
                || mTokenizer[mIndex]->text.startsWith('}'))) {// prototype
              isDeclaration = true;
        }
    }

    QString scopelessName;
    PStatement functionStatement;
    if (isFriend && isDeclaration && functionClass) {
        int delimPos = sName.indexOf("::");
        if (delimPos >= 0) {
            scopelessName = sName.mid(delimPos+2);
        } else
            scopelessName = sName;
        //TODO : we should check namespace
        functionClass->friends.insert(scopelessName);
    } else if (isValid) {
        // Use the class the function belongs to as the parent ID if the function is declared outside of the class body
        int delimPos = sName.indexOf("::");
        QString scopelessName;
        QString parentClassName;
        if (delimPos >= 0) {
            // Provide Bar instead of Foo::Bar
            scopelessName = sName.mid(delimPos);

            // Check what class this function belongs to
            parentClassName = sName.mid(0, delimPos);
            functionClass = getIncompleteClass(parentClassName,getCurrentScope());
        } else
            scopelessName = sName;

        StatementKind functionKind;
        // Determine function type
        if (scopelessName == sType) {
            functionKind = StatementKind::skConstructor;
        } else if (scopelessName == '~' + sType) {
            functionKind = StatementKind::skDestructor;
        } else {
            functionKind = StatementKind::skFunction;
        }

        // For function definitions, the parent class is given. Only use that as a parent
        if (!isDeclaration) {
            functionStatement=addStatement(
                        functionClass,
                        mCurrentFile,
                        "", // do not override hint
                        sType,
                        scopelessName,
                        sArgs,
                        "",
                        //mTokenizer[mIndex - 1]^.Line,
                        startLine,
                        functionKind,
                        getScope(),
                        mClassScope,
                        true,
                        isStatic);
            scanMethodArgs(functionStatement, sArgs);
            // add variable this to the class function
            if (functionClass && functionClass->kind == StatementKind::skClass &&
                    !isStatic) {
                //add this to non-static class member function
                addStatement(
                            functionStatement,
                            mCurrentFile,
                            "", // do not override hint
                            functionClass->command,
                            "this",
                            "",
                            "",
                            startLine,
                            StatementKind::skVariable,
                            StatementScope::ssLocal,
                            StatementClassScope::scsNone,
                            true,
                            false);
            }
        } else {
            functionStatement = addStatement(
                        functionClass,
                        mCurrentFile,
                        "", // do not override hint
                        sType,
                        scopelessName,
                        sArgs,
                        "",
                        //mTokenizer[mIndex - 1]^.Line,
                        startLine,
                        functionKind,
                        getScope(),
                        mClassScope,
                        false,
                        isStatic);
        }

    }


    if ((mIndex < mTokenizer.tokenCount()) && mTokenizer[mIndex]->text.startsWith('{')) {
        addSoloScopeLevel(functionStatement,startLine);
        mIndex++; //skip '{'
    } else if ((mIndex < mTokenizer.tokenCount()) && mTokenizer[mIndex]->text.startsWith(';')) {
        addSoloScopeLevel(functionStatement,startLine);
        if (mTokenizer[mIndex]->line != startLine)
            removeScopeLevel(mTokenizer[mIndex]->line+1);
        else
            removeScopeLevel(startLine+1);
        mIndex++;
    }

    if (i == mIndex) { // if not moved ahead, something is wrong but don't get stuck ;)
        if ( (mIndex < mTokenizer.tokenCount()) &&
             ! isBraceChar(mTokenizer[mIndex]->text.front())) {
            mIndex++;
        }
    }
}

void CppParser::handleNamespace()
{
    bool isInline=false;
    if (mTokenizer[mIndex]->text == "inline") {
        isInline = true;
        mIndex++; //skip 'inline'
    }

    int startLine = mTokenizer[mIndex]->line;
    mIndex++; //skip 'namespace'

    if (!isLetterChar(mTokenizer[mIndex]->text.front()))
        //wrong namespace define, stop handling
        return;
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
    if (mIndex>=mTokenizer.tokenCount())
        return;
    QString aliasName;
    if ((mIndex+2<mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text.front() == '=')) {
        aliasName=mTokenizer[mIndex+1]->text;
        //namespace alias
        addStatement(
            getCurrentScope(),
            mCurrentFile,
            "", // do not override hint
            aliasName, // name of the alias namespace
            command, // command
            "", // args
            "", // values
            //mTokenizer[mIndex]^.Line,
            startLine,
            StatementKind::skNamespaceAlias,
            getScope(),
            mClassScope,
            true,
            false);
        mIndex+=2; //skip ;
        return;
    } else if (isInline) {
        //inline namespace , just skip it
        // Skip to '{'
        while ((mIndex<mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text.front() != '{'))
            mIndex++;
        int i =skipBraces(mIndex); //skip '}'
        if (i==mIndex)
            mInlineNamespaceEndSkips.append(mTokenizer.tokenCount());
        else
            mInlineNamespaceEndSkips.append(i);
        if (mIndex<mTokenizer.tokenCount())
            mIndex++; //skip '{'
    } else {
        PStatement namespaceStatement = addStatement(
            getCurrentScope(),
            mCurrentFile,
            "", // do not override hint
            "", // type
            command, // command
            "", // args
            "", // values
            startLine,
            StatementKind::skNamespace,
            getScope(),
            mClassScope,
            true,
            false);
        addSoloScopeLevel(namespaceStatement,startLine);

        // Skip to '{'
        while ((mIndex<mTokenizer.tokenCount()) && !mTokenizer[mIndex]->text.startsWith('{'))
            mIndex++;
        if (mIndex<mTokenizer.tokenCount())
            mIndex++; //skip '{'
    }
}

void CppParser::handleOtherTypedefs()
{
    int startLine = mTokenizer[mIndex]->line;
    // Skip typedef word
    mIndex++;

    if (mIndex>=mTokenizer.tokenCount())
        return;

    if (mTokenizer[mIndex]->text.front() == '('
            || mTokenizer[mIndex]->text.front() == ','
            || mTokenizer[mIndex]->text.front() == ';') { // error typedef
        //skip to ;
        while ((mIndex< mTokenizer.tokenCount()) && !mTokenizer[mIndex]->text.startsWith(';'))
            mIndex++;
        //skip ;
        if ((mIndex< mTokenizer.tokenCount()) && mTokenizer[mIndex]->text.startsWith(';'))
            mIndex++;
        return;
    }
    if ((mIndex+1<mTokenizer.tokenCount())
            && (mTokenizer[mIndex+1]->text == ';')) {
        //no old type
        QString newType = mTokenizer[mIndex]->text.trimmed();
        addStatement(
                    getCurrentScope(),
                    mCurrentFile,
                    "typedef " + newType, // override hint
                    "",
                    newType,
                    "",
                    "",
                    startLine,
                    StatementKind::skTypedef,
                    getScope(),
                    mClassScope,
                    true,
                    false);
        mIndex+=2; //skip ;
        return;
    }
    QString oldType;

    // Walk up to first new word (before first comma or ;)
    while(true) {
        oldType += mTokenizer[mIndex]->text + ' ';
        mIndex++;
        if (mIndex+1>=mTokenizer.tokenCount())
            break;
        if (mTokenizer[mIndex + 1]->text.front() == ','
                  || mTokenizer[mIndex + 1]->text.front() == ';')
            break;
        if  ((mIndex + 2 < mTokenizer.tokenCount())
             && (mTokenizer[mIndex + 2]->text.front() == ','
                 || mTokenizer[mIndex + 2]->text.front() == ';')
             && (mTokenizer[mIndex + 1]->text.front() == '('))
            break;
    }
    oldType = oldType.trimmed();

    // Add synonyms for old
    if ((mIndex+1 < mTokenizer.tokenCount()) && !oldType.isEmpty()) {
        QString newType;
        while(true) {
            // Support multiword typedefs
            if ((mIndex + 2 < mTokenizer.tokenCount())
                    && (mTokenizer[mIndex + 2]->text.front() == ','
                        || mTokenizer[mIndex + 2]->text.front() == ';')
                    && (mTokenizer[mIndex + 1]->text.front() == '(')) {
                //valid function define
                newType = mTokenizer[mIndex]->text.trimmed();
                newType = newType.mid(1,newType.length()-2); //remove '(' and ')';
                newType = newType.trimmed();
                int p = newType.lastIndexOf(' ');
                if (p>=0)
                    newType.truncate(p+1);
                addStatement(
                        getCurrentScope(),
                        mCurrentFile,
                        "typedef " + oldType + " " + mTokenizer[mIndex]->text + " " +
                                mTokenizer[mIndex + 1]->text, // do not override hint
                        oldType,
                        newType,
                        mTokenizer[mIndex + 1]->text,
                        "",
                        startLine,
                        StatementKind::skTypedef,
                        getScope(),
                        mClassScope,
                        true,
                        false);
                newType = "";
                //skip to ',' or ';'
                mIndex+=2;
            } else if (mTokenizer[mIndex+1]->text.front() ==','
                       || mTokenizer[mIndex+1]->text.front() ==';'
                       || mTokenizer[mIndex+1]->text.front() =='(') {
                newType += mTokenizer[mIndex]->text;
                newType = newType.trimmed();
                addStatement(
                            getCurrentScope(),
                            mCurrentFile,
                            "typedef " + oldType + " " + newType, // override hint
                            oldType,
                            newType,
                            "",
                            "",
                            startLine,
                            StatementKind::skTypedef,
                            getScope(),
                            mClassScope,
                            true,
                            false);
                newType = "";
                mIndex++;
            } else {
                newType += mTokenizer[mIndex]->text + ' ';
                mIndex++;
            }
            if ((mIndex>= mTokenizer.tokenCount()) || (mTokenizer[mIndex]->text[0] == ';'))
                break;
            else if (mTokenizer[mIndex]->text.front() == ',')
                mIndex++;
            if (mIndex+1 >= mTokenizer.tokenCount())
                break;
        }
    }

    // Step over semicolon (saves one HandleStatement loop)
    mIndex++;
}

void CppParser::handlePreprocessor()
{
    if (mTokenizer[mIndex]->text.startsWith("#include ")) { // start of new file
        // format: #include fullfilename:line
        // Strip keyword
        QString s = mTokenizer[mIndex]->text.mid(QString("#include ").length());
        int delimPos = s.lastIndexOf(':');
        if (delimPos>=0) {
            mCurrentFile = s.mid(0,delimPos);
            mIsSystemHeader = isSystemHeaderFile(mCurrentFile) || isProjectHeaderFile(mCurrentFile);
            mIsProjectFile = mProjectFiles.contains(mCurrentFile);             mIsHeader = isHfile(mCurrentFile);

            // Mention progress to user if we enter a NEW file
            bool ok;
            int line = s.midRef(delimPos+1).toInt(&ok);
            if (line == 1) {
                mFilesScannedCount++;
                mFilesToScanCount++;
                emit onProgress(mCurrentFile,mFilesToScanCount,mFilesScannedCount);
            }
        }
    } else if (mTokenizer[mIndex]->text.startsWith("#define ")) {

      // format: #define A B, remove define keyword
      QString s = mTokenizer[mIndex]->text.mid(QString("#define ").length());

      // Ask the preprocessor to cut parts up
      QString name,args,value;
      mPreprocessor.getDefineParts(s,name,args,value);

      // Generate custom hint
      QString hintText = "#define";
      if (!name.isEmpty())
          hintText += ' ' + name;
      if (!args.isEmpty())
          hintText += ' ' + args;
      if (!value.isEmpty())
          hintText += ' ' + value;

      addStatement(
        nullptr, // defines don't belong to any scope
        mCurrentFile,
        hintText, // override hint
        "", // define has no type
        name,
        args,
        value,
        mTokenizer[mIndex]->line,
        StatementKind::skPreprocessor,
        StatementScope::ssGlobal,
        StatementClassScope::scsNone,
        true,
        false);
    } // TODO: undef ( define has limited scope)
    mIndex++;
}

StatementClassScope CppParser::getClassScope(int index) {
    if (mTokenizer[index]->text=="public")
        return StatementClassScope::scsPublic;
    else if (mTokenizer[index]->text=="private")
        return StatementClassScope::scsPrivate;
    else if (mTokenizer[index]->text=="protected")
        return StatementClassScope::scsProtected;
    else
        return StatementClassScope::scsNone;
}

void CppParser::handleScope()
{
    mClassScope = getClassScope(mIndex);
    mIndex+=2; // the scope is followed by a ':'
}

bool CppParser::handleStatement()
{
    QString S1,S2,S3;
    bool isStatic, isFriend;
    int idx=getCurrentBlockEndSkip();
    int idx2=getCurrentBlockBeginSkip();
    int idx3=getCurrentInlineNamespaceEndSkip();
    if (mIndex >= idx2) {
        //skip (previous handled) block begin
        mBlockBeginSkips.pop_back();
        if (mIndex == idx2)
            mIndex++;
        else if (mIndex<mTokenizer.tokenCount())  //error happens, but we must remove an (error) added scope
            removeScopeLevel(mTokenizer[mIndex]->line);
    } else if (mIndex >= idx) {
        //skip (previous handled) block end
        mBlockEndSkips.pop_back();
        if (idx+1 < mTokenizer.tokenCount())
            removeScopeLevel(mTokenizer[idx+1]->line);
        if (mIndex == idx)
            mIndex++;
    } else if (mIndex >= idx3) {
        //skip (previous handled) inline name space end
        mInlineNamespaceEndSkips.pop_back();
        if (mIndex == idx3)
            mIndex++;
    } else if (mTokenizer[mIndex]->text.startsWith('{')) {
        PStatement block = addStatement(
            getCurrentScope(),
            mCurrentFile,
            "", // override hint
            "",
            "",
            "",
            "",
            //mTokenizer[mIndex]^.Line,
            mTokenizer[mIndex]->line,
            StatementKind::skBlock,
            getScope(),
            mClassScope,
            true,
            false);
        addSoloScopeLevel(block,mTokenizer[mIndex]->line);
        mIndex++;
    } else if (mTokenizer[mIndex]->text[0] == '}') {
        removeScopeLevel(mTokenizer[mIndex]->line);
        mIndex++;
    } else if (checkForPreprocessor()) {
        handlePreprocessor();
    } else if (checkForKeyword()) { // includes template now
        handleKeyword();
    } else if (checkForForBlock()) { // (for/catch)
        handleForBlock();
    } else if (checkForCatchBlock()) { // (for/catch)
        handleCatchBlock();
    } else if (checkForScope()) { // public /private/proteced
        handleScope();
    } else if (checkForEnum()) {
        handleEnum();
    } else if (checkForTypedef()) {
        if (mIndex+1 < mTokenizer.tokenCount()) {
            if (checkForTypedefStruct()) { // typedef struct something
                mIndex++; // skip 'typedef'
                handleStructs(true);
            } else if (checkForTypedefEnum()) { // typedef enum something
                mIndex++; // skip 'typedef'
                handleEnum();
            } else
                handleOtherTypedefs(); // typedef Foo Bar
        } else
            mIndex++;
    } else if (checkForNamespace()) {
        handleNamespace();
    } else if (checkForUsing()) {
        handleUsing();
    } else if (checkForStructs()) {
        handleStructs(false);
    } else if (checkForMethod(S1, S2, S3, isStatic, isFriend)) {
        handleMethod(S1, S2, S3, isStatic, isFriend); // don't recalculate parts
    } else if (checkForVar()) {
        handleVar();
    } else
        mIndex++;

    checkForSkipStatement();

    return mIndex < mTokenizer.tokenCount();

}

void CppParser::handleStructs(bool isTypedef)
{
    bool isFriend = false;
    QString prefix = mTokenizer[mIndex]->text;
    if (prefix == "friend") {
        isFriend = true;
        mIndex++;
    }
    // Check if were dealing with a struct or union
    prefix = mTokenizer[mIndex]->text;
    bool isStruct = ("struct" == prefix) || ("union"==prefix);
    int startLine = mTokenizer[mIndex]->line;

    mIndex++; //skip struct/class/union

    if (mIndex>=mTokenizer.tokenCount())
        return;

    // Do not modifiy index initially
    int i = mIndex;

    // Skip until the struct body starts
    while ((i < mTokenizer.tokenCount()) && ! (
               mTokenizer[i]->text.front() ==';'
               || mTokenizer[i]->text.front() =='{'))
        i++;

    // Forward class/struct decl *or* typedef, e.g. typedef struct some_struct synonym1, synonym2;
    if ((i < mTokenizer.tokenCount()) && (mTokenizer[i]->text.front() == ';')) {
        // typdef struct Foo Bar
        if (isTypedef) {
            QString oldType = mTokenizer[mIndex]->text;
            while(true) {
                // Add definition statement for the synonym
                if ((mIndex + 1 < mTokenizer.tokenCount())
                        && (mTokenizer[mIndex + 1]->text.front()==','
                            || mTokenizer[mIndex + 1]->text.front()==';')) {
                    QString newType = mTokenizer[mIndex]->text;
                    addStatement(
                                getCurrentScope(),
                                mCurrentFile,
                                "typedef " + prefix + " " + oldType + ' ' + newType, // override hint
                                oldType,
                                newType,
                                "",
                                "",
                                startLine,
                                StatementKind::skTypedef,
                                getScope(),
                                mClassScope,
                                true,
                                false);
                }
                mIndex++;
                if (mIndex >= mTokenizer.tokenCount())
                    break;
                if (mTokenizer[mIndex]->text.front() == ';')
                    break;
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
        if (mTokenizer[mIndex]->text.front() != '{') {
            while(true) {
                if ((mIndex + 1 < mTokenizer.tokenCount())
                  && (mTokenizer[mIndex + 1]->text.front() == ','
                      || mTokenizer[mIndex + 1]->text.front() == ';'
                      || mTokenizer[mIndex + 1]->text.front() == '{'
                      || mTokenizer[mIndex + 1]->text.front() == ':')) {
                    QString command = mTokenizer[mIndex]->text;
                    if (!command.isEmpty()) {
                        firstSynonym = addStatement(
                                    getCurrentScope(),
                                    mCurrentFile,
                                    "", // do not override hint
                                    prefix, // type
                                    command, // command
                                    "", // args
                                    "", // values
                                    startLine,
                                    StatementKind::skClass,
                                    getScope(),
                                    mClassScope,
                                    true,
                                    false);
                        command = "";
                    }
                    mIndex++;
                } else if ((mIndex + 2 < mTokenizer.tokenCount())
                           && (mTokenizer[mIndex + 1]->text == "final")
                           && (mTokenizer[mIndex + 2]->text.front()==','
                               || isblockChar(mTokenizer[mIndex + 2]->text.front()))) {
                    QString command = mTokenizer[mIndex]->text;
                    if (!command.isEmpty()) {
                        firstSynonym = addStatement(
                                    getCurrentScope(),
                                    mCurrentFile,
                                    "", // do not override hint
                                    prefix, // type
                                    command, // command
                                    "", // args
                                    "", // values
                                    startLine,
                                    StatementKind::skClass,
                                    getScope(),
                                    mClassScope,
                                    true,
                                    false);
                        command="";
                    }
                    mIndex+=2;
                } else
                    mIndex++;
                if (mIndex >= mTokenizer.tokenCount())
                    break;
                if (mTokenizer[mIndex]->text.front() == ':'
                        || mTokenizer[mIndex]->text.front() == '{'
                        || mTokenizer[mIndex]->text.front() == ';')
                    break;
            }
        }

        // Walk to opening brace if we encountered inheritance statements
        if ((mIndex < mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text.front() == ':')) {
            if (firstSynonym)
                setInheritance(mIndex, firstSynonym, isStruct); // set the _InheritanceList value
            while ((mIndex < mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text.front() != '{'))
                mIndex++; // skip decl after ':'
        }

        // Check for struct synonyms after close brace
        if (isStruct) {

            // Walk to closing brace
            i = skipBraces(mIndex); // step onto closing brace

            if ((i + 1 < mTokenizer.tokenCount()) && !(
                        mTokenizer[i + 1]->text.front() == ';'
                        || mTokenizer[i + 1]->text.front() ==  '}')) {
                // When encountering names again after struct body scanning, skip it
                mSkipList.append(i+1); // add first name to skip statement so that we can skip it until the next ;
                QString command = "";
                QString args = "";

                // Add synonym before opening brace
                while(true) {
                    i++;

                    if (!(mTokenizer[i]->text.front() == '{'
                          || mTokenizer[i]->text.front() == ','
                          || mTokenizer[i]->text.front() == ';')) {
//                        if ((mTokenizer[i]->text.front() == '_')
//                            && (mTokenizer[i]->text.back() == '_')) {
//                            // skip possible gcc attributes
//                            // start and end with 2 underscores (i.e. __attribute__)
//                            // so, to avoid slow checks of strings, we just check the first and last letter of the token
//                            // if both are underscores, we split
//                            break;
//                        } else {
                            if (mTokenizer[i]->text.endsWith(']')) { // cut-off array brackets
                                int pos = mTokenizer[i]->text.indexOf('[');
                                command += mTokenizer[i]->text.mid(0,pos) + ' ';
                                args =  mTokenizer[i]->text.mid(pos);
                            } else if (mTokenizer[i]->text.front() == '*'
                                       || mTokenizer[i]->text.front() == '&') { // do not add spaces after pointer operator
                                command += mTokenizer[i]->text;
                            } else {
                                command += mTokenizer[i]->text + ' ';
                            }
//                        }
                    } else {
                        command = command.trimmed();
                        if (!command.isEmpty() &&
                                ( !firstSynonym
                                  || command!=firstSynonym->command )) {
                            //not define the struct yet, we define a unamed struct
                            if (!firstSynonym) {
                                firstSynonym = addStatement(
                                            getCurrentScope(),
                                            mCurrentFile,
                                            "", // do not override hint
                                            prefix,
                                            "__"+command,
                                            "",
                                            "",
                                            startLine,
                                            StatementKind::skClass,
                                            getScope(),
                                            mClassScope,
                                            true,
                                            false);
                            }
                            if (isTypedef) {
                                //typedef
                                addStatement(
                                  getCurrentScope(),
                                  mCurrentFile,
                                  "typedef " + firstSynonym->command + ' ' + command, // override hint
                                  firstSynonym->command,
                                  command,
                                  "",
                                  "",
                                  mTokenizer[mIndex]->line,
                                  StatementKind::skTypedef,
                                  getScope(),
                                  mClassScope,
                                  true,
                                  false); // typedef
                            } else {
                                //variable define
                                addStatement(
                                  getCurrentScope(),
                                  mCurrentFile,
                                  "", // do not override hint
                                  firstSynonym->command,
                                  command,
                                  args,
                                  "",
                                  mTokenizer[i]->line,
                                  StatementKind::skVariable,
                                  getScope(),
                                  mClassScope,
                                  true,
                                  false); // TODO: not supported to pass list
                            }
                        }
                        command = "";
                    }
                    if (i >= mTokenizer.tokenCount() - 1)
                        break;
                    if (mTokenizer[i]->text.front()=='{'
                          || mTokenizer[i]->text.front()== ';')
                        break;
                }

              // Nothing worth mentioning after closing brace
              // Proceed to set first synonym as current class
            }
        }
        if (!firstSynonym) {
            //anonymous union/struct/class, add ast a block
            firstSynonym=addStatement(
                      getCurrentScope(),
                      mCurrentFile,
                      "", // override hint
                      "",
                      "",
                      "",
                      "",
                      startLine,
                      StatementKind::skBlock,
                      getScope(),
                      mClassScope,
                      true,
                      false);
        }
        addSoloScopeLevel(firstSynonym,startLine);

        // Step over {
        if ((mIndex < mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text.front() == '{'))
            mIndex++;
    }
}

void CppParser::handleUsing()
{
    int startLine = mTokenizer[mIndex]->line;
    if (mCurrentFile.isEmpty()) {
        //skip to ;
        while ((mIndex < mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text!=';'))
            mIndex++;
        mIndex++; //skip ;
        return;
    }

    mIndex++; //skip 'using'

    //handle things like 'using vec = std::vector; '
    if (mIndex+1 < mTokenizer.tokenCount()
            && mTokenizer[mIndex+1]->text == "=") {
        QString fullName = mTokenizer[mIndex]->text;
        QString aliasName;
        mIndex+=2;
        while (mIndex<mTokenizer.tokenCount() &&
               mTokenizer[mIndex]->text!=';') {
            aliasName += mTokenizer[mIndex]->text;
            mIndex++;
        }
        addStatement(
                    getCurrentScope(),
                    mCurrentFile,
                    "using "+fullName+" = " + aliasName, //hint text
                    aliasName, // name of the alias (type)
                    fullName, // command
                    "", // args
                    "", // values
                    startLine,
                    StatementKind::skTypedef,
                    getScope(),
                    mClassScope,
                    true,
                    false);
        // skip ;
        mIndex++;
        return;
    }
    //handle things like 'using std::vector;'
    if ((mIndex+2>=mTokenizer.tokenCount())
            || (mTokenizer[mIndex]->text != "namespace")) {
        int i= mTokenizer[mIndex]->text.lastIndexOf("::");
        if (i>=0) {
            QString fullName = mTokenizer[mIndex]->text;
            QString usingName = fullName.mid(i+2);
            addStatement(
                        getCurrentScope(),
                        mCurrentFile,
                        "using "+fullName, //hint text
                        fullName, // name of the alias (type)
                        usingName, // command
                        "", // args
                        "", // values
                        startLine,
                        StatementKind::skAlias,
                        getScope(),
                        mClassScope,
                        true,
                        false);
        }
        //skip to ;
        while ((mIndex<mTokenizer.tokenCount()) &&
                (mTokenizer[mIndex]->text!=";"))
            mIndex++;
        mIndex++; //and skip it
        return;
    }
    mIndex++;  // skip 'namespace'
    PStatement scopeStatement = getCurrentScope();

    QString usingName = mTokenizer[mIndex]->text;
    mIndex++;

    if (scopeStatement) {
        QString fullName = scopeStatement->fullName + "::" + usingName;
        if (!mNamespaces.contains(fullName)) {
            fullName = usingName;
        }
        if (mNamespaces.contains(fullName)) {
            scopeStatement->usingList.insert(fullName);
        }
    } else {
        PFileIncludes fileInfo = mPreprocessor.includesList().value(mCurrentFile);
        if (!fileInfo)
            return;
        if (mNamespaces.contains(usingName)) {
            fileInfo->usings.insert(usingName);
        }
    }
}

void CppParser::handleVar()
{
    // Keep going and stop on top of the variable name
    QString lastType = "";
    bool isFunctionPointer = false;
    bool isExtern = false;
    bool isStatic = false;
    bool varAdded = false;
    while (true) {
        if ((mIndex + 2 < mTokenizer.tokenCount())
                && (mTokenizer[mIndex + 1]->text.front() == '(')
                && (mTokenizer[mIndex + 2]->text.front() == '(')) {
            isFunctionPointer = mTokenizer[mIndex + 1]->text.indexOf('*') >= 0;
            if (!isFunctionPointer)
                break; // inline constructor
        } else if ((mIndex + 1 < mTokenizer.tokenCount())
                   && (mTokenizer[mIndex + 1]->text.front()=='('
                       || mTokenizer[mIndex + 1]->text.front()==','
                       || mTokenizer[mIndex + 1]->text.front()==';'
                       || mTokenizer[mIndex + 1]->text.front()==':'
                       || mTokenizer[mIndex + 1]->text.front()=='}'
                       || mTokenizer[mIndex + 1]->text.front()=='#'
                       || mTokenizer[mIndex + 1]->text.front()=='{')) {
            break;
        }


        // we've made a mistake, this is a typedef , not a variable definition.
        if (mTokenizer[mIndex]->text == "typedef")
            return;

        // struct/class/union is part of the type signature
        // but we dont store it in the type cache, so must trim it to find the type info
        if (mTokenizer[mIndex]->text!="struct"
                && mTokenizer[mIndex]->text!="class"
                && mTokenizer[mIndex]->text!="union") {
            if (mTokenizer[mIndex]->text == ':') {
                lastType += ':';
            } else {
                QString s=expandMacroType(mTokenizer[mIndex]->text);
                if (s == "extern") {
                    isExtern = true;
                } else {
                    if (!s.isEmpty())
                        lastType += ' '+s;
                    if (s == "static")
                        isStatic = true;
                }
            }
        }
        mIndex++;
        if(mIndex >= mTokenizer.tokenCount())
            break;
        if (isFunctionPointer)
            break;
    }
    lastType = lastType.trimmed();

    // Don't bother entering the scanning loop when we have failed
    if (mIndex >= mTokenizer.tokenCount())
        return;

    // Find the variable name
    while (true) {

        // Skip bit identifiers,
        // e.g.:
        // handle
        // unsigned short bAppReturnCode:8,reserved:6,fBusy:1,fAck:1
        // as
        // unsigned short bAppReturnCode,reserved,fBusy,fAck
        if ( (mIndex < mTokenizer.tokenCount()) && (mTokenizer[mIndex]->text.front() == ':')) {
            while ( (mIndex < mTokenizer.tokenCount())
                    && !(
                        mTokenizer[mIndex]->text.front() == ','
                        || isblockChar(';')
                        ))
                mIndex++;
        }

        // Skip inline constructors,
        // e.g.:
        // handle
        // int a(3)
        // as
        // int a
        if (!isFunctionPointer &&
                mIndex < mTokenizer.tokenCount() &&
                mTokenizer[mIndex]->text.front() == '(') {
            while ((mIndex < mTokenizer.tokenCount())
                    && !(
                        mTokenizer[mIndex]->text.front() == ','
                        || isblockChar(mTokenizer[mIndex]->text.front())
                        ))
                mIndex++;
        }

        // Did we stop on top of the variable name?
        if (mIndex < mTokenizer.tokenCount()) {
            if (mTokenizer[mIndex]->text.front()!=','
                    && mTokenizer[mIndex]->text.front()!=';') {
                QString cmd;
                QString args;
                if (isFunctionPointer && (mIndex + 1 < mTokenizer.tokenCount())) {
                        QString s = mTokenizer[mIndex]->text;
                        cmd = s.mid(2,s.length()-3).trimmed(); // (*foo) -> foo
                        args = mTokenizer[mIndex + 1]->text; // (int a,int b)
                        lastType += "(*)" + args; // void(int a,int b)
                        mIndex++;
                } else if (mTokenizer[mIndex]->text.back() == ']') { //array; break args
                    int pos = mTokenizer[mIndex]->text.indexOf('[');
                    cmd = mTokenizer[mIndex]->text.mid(0,pos);
                    args = mTokenizer[mIndex]->text.mid(pos);
                } else {
                    cmd = mTokenizer[mIndex]->text;
                    args = "";
                }

                // Add a statement for every struct we are in
                if (!lastType.isEmpty()) {
                    addChildStatement(
                      getCurrentScope(),
                      mCurrentFile,
                      "", // do not override hint
                      lastType,
                      cmd,
                      args,
                      "",
                      mTokenizer[mIndex]->line,
                      StatementKind::skVariable,
                      getScope(),
                      mClassScope,
                      //True,
                      !isExtern,
                      isStatic); // TODO: not supported to pass list
                    varAdded = true;
                }
            }

            // Step over the variable name
            if (isblockChar(mTokenizer[mIndex]->text.front())) {
                break;
            }
            mIndex++;
        }
        if (mIndex >= mTokenizer.tokenCount())
            break;
        if (isblockChar(mTokenizer[mIndex]->text.front()))
            break;
    }
    if (varAdded && (mIndex < mTokenizer.tokenCount())
            && (mTokenizer[mIndex]->text == '{')) {
        // skip { } like A x {new A};
        int i=skipBraces(mIndex);
        if (i!=mIndex)
            mIndex = i+1;
    }
    // Skip ; and ,
    if ( (mIndex < mTokenizer.tokenCount()) &&
         (mTokenizer[mIndex]->text.front() == ';'
          || mTokenizer[mIndex]->text.front() == ','))
        mIndex++;
}

void CppParser::internalParse(const QString &fileName)
{
    // Perform some validation before we start
    if (!mEnabled)
        return;
    if (!isCfile(fileName) && !isHfile(fileName))  // support only known C/C++ files
        return;

    QStringList buffer;
    if (mOnGetFileStream) {
        mOnGetFileStream(fileName,buffer);
    }

    // Preprocess the file...
    {
        auto action = finally([this]{
            mPreprocessor.reset();
            mTokenizer.reset();
        });
        // Let the preprocessor augment the include records
//        mPreprocessor.setIncludesList(mIncludesList);
//        mPreprocessor.setScannedFileList(mScannedFiles);
//        mPreprocessor.setIncludePaths(mIncludePaths);
//        mPreprocessor.setProjectIncludePaths(mProjectIncludePaths);
        mPreprocessor.setScanOptions(mParseGlobalHeaders, mParseLocalHeaders);
        mPreprocessor.preprocess(fileName, buffer);


        // Tokenize the preprocessed buffer file
        mTokenizer.tokenize(mPreprocessor.result());
        if (mTokenizer.tokenCount() == 0)
            return;

        // Process the token list
        mCurrentScope.clear();
        mCurrentClassScope.clear();
        mIndex = 0;
        mClassScope = StatementClassScope::scsNone;
        mSkipList.clear();
        mBlockBeginSkips.clear();
        mBlockEndSkips.clear();
        mInlineNamespaceEndSkips.clear();
        while(true) {
            if (!handleStatement())
                break;
        }
#ifdef QT_DEBUG
//        StringsToFile(mPreprocessor.result(),"f:\\preprocess.txt");
//        mPreprocessor.dumpDefinesTo("f:\\defines.txt");
//        mPreprocessor.dumpIncludesListTo("f:\\includes.txt");
//        mStatementList.dump("f:\\stats.txt");
//        mTokenizer.dumpTokens("f:\\tokens.txt");
//        mStatementList.dumpAll("f:\\all-stats.txt");
#endif
    }
}

void CppParser::inheritClassStatement(const PStatement& derived, bool isStruct,
                                      const PStatement& base, StatementClassScope access)
{
    PFileIncludes fileIncludes1=mPreprocessor.includesList().value(derived->fileName);
    PFileIncludes fileIncludes2=mPreprocessor.includesList().value(base->fileName);
    if (fileIncludes1 && fileIncludes2) {
        //derived class depeneds on base class
        fileIncludes1->dependingFiles.insert(base->fileName);
        fileIncludes2->dependedFiles.insert(derived->fileName);
    }
    //differentiate class and struct
    if (access == StatementClassScope::scsNone) {
        if (isStruct)
            access = StatementClassScope::scsPublic;
        else
            access = StatementClassScope::scsPrivate;
    }
    foreach (const PStatement& statement, base->children) {
        if (statement->classScope == StatementClassScope::scsPrivate
                || statement->kind == StatementKind::skConstructor
                || statement->kind == StatementKind::skDestructor)
            continue;
        StatementClassScope m_acc;
        switch(access) {
        case StatementClassScope::scsPublic:
            m_acc = statement->classScope;
            break;
        case StatementClassScope::scsProtected:
            m_acc = StatementClassScope::scsProtected;
            break;
        case StatementClassScope::scsPrivate:
            m_acc = StatementClassScope::scsPrivate;
            break;
        default:
            m_acc = StatementClassScope::scsPrivate;
        }
        //inherit
        addInheritedStatement(derived,statement,m_acc);
    }
}

QString CppParser::expandMacroType(const QString &name)
{
    //its done in the preprocessor
    return name;
}

void CppParser::fillListOfFunctions(const QString& fileName, int line,
                                    const PStatement& statement,
                                    const PStatement& scopeStatement, QStringList &list)
{
    StatementMap children = mStatementList.childrenStatements(scopeStatement);
    for (const PStatement& child:children) {
        if ((statement->command == child->command)
#ifdef Q_OS_WIN
                || (statement->command +'A' == child->command)
                || (statement->command +'W' == child->command)
#endif
                ) {
            if (line < child->line && (child->fileName == fileName))
                continue;
            list.append(prettyPrintStatement(child,child->fileName,child->line));
        }
    }
}

QList<PStatement> CppParser::getListOfFunctions(const QString &fileName, int line, const PStatement &statement, const PStatement &scopeStatement)
{
    QList<PStatement> result;
    StatementMap children = mStatementList.childrenStatements(scopeStatement);
    for (const PStatement& child:children) {
        if ((statement->command == child->command)
#ifdef Q_OS_WIN
                || (statement->command +'A' == child->command)
                || (statement->command +'W' == child->command)
#endif
                ) {
            if (line < child->line && (child->fileName == fileName))
                continue;
            result.append(child);
        }
    }
    return result;
}

PStatement CppParser::findMemberOfStatement(const QString &phrase,
                                            const PStatement& scopeStatement)
{
    const StatementMap& statementMap =mStatementList.childrenStatements(scopeStatement);
    if (statementMap.isEmpty())
        return PStatement();

    QString s = phrase;
    //remove []
    int p = phrase.indexOf('[');
    if (p>=0)
        s.truncate(p);

    //remove <>
    p =s.indexOf('<');
    if (p>=0)
        s.truncate(p);

    return statementMap.value(s,PStatement());
}

PStatement CppParser::findStatementInScope(const QString &name, const QString &noNameArgs,
                                           StatementKind kind, const PStatement& scope)
{
    if (scope && scope->kind == StatementKind::skNamespace) {
        PStatementList namespaceStatementsList = findNamespace(scope->command);
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

PStatement CppParser::findStatementInScope(const QString &name, const PStatement& scope)
{
    if (!scope)
        return findMemberOfStatement(name,scope);
    if (scope->kind == StatementKind::skNamespace) {
        return findStatementInNamespace(name, scope->fullName);
    } else {
        return findMemberOfStatement(name,scope);
    }
}

PStatement CppParser::findStatementInNamespace(const QString &name, const QString &namespaceName)
{
    PStatementList namespaceStatementsList=findNamespace(namespaceName);
    if (!namespaceStatementsList)
        return PStatement();
    foreach (const PStatement& namespaceStatement,*namespaceStatementsList) {
        PStatement result = findMemberOfStatement(name,namespaceStatement);
        if (result)
            return result;
    }
    return PStatement();
}

int CppParser::getBracketEnd(const QString &s, int startAt)
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
                                             const PStatement& scope)
{
    const StatementMap& statementMap =mStatementList.childrenStatements(scope);

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

    //remove all statements in the file
    const QList<QString>& keys=mNamespaces.keys();
    for (const QString& key:keys) {
        PStatementList statements = mNamespaces.value(key);
        for (int i=statements->size()-1;i>=0;i--) {
            PStatement statement = statements->at(i);
            if (statement->fileName == fileName
                    || statement->definitionFileName == fileName) {
                statements->removeAt(i);
            }
        }
        if (statements->isEmpty()) {
            mNamespaces.remove(key);
        }
    }
    // delete it from scannedfiles
    mPreprocessor.scannedFiles().remove(fileName);

    // remove its include files list
    PFileIncludes p = findFileIncludes(fileName, true);
    if (p) {
        //fPreprocessor.InvalidDefinesInFile(FileName); //we don't need this, since we reset defines after each parse
        //p->includeFiles.clear();
        //p->usings.clear();
        for (PStatement& statement:p->statements) {
            if ((statement->kind == StatementKind::skFunction
                 || statement->kind == StatementKind::skConstructor
                 || statement->kind == StatementKind::skDestructor
                 || statement->kind == StatementKind::skVariable)
                    && (fileName != statement->fileName)) {
                statement->hasDefinition = false;
            }
        }

        for (PStatement& statement:p->declaredStatements) {
            mStatementList.deleteStatement(statement);
        }

        //p->declaredStatements.clear();
        //p->statements.clear();
        //p->scopes.clear();
        //p->dependedFiles.clear();
        //p->dependingFiles.clear();
    }
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
    QQueue<QString> queue;
    QSet<QString> processed;
    queue.enqueue(fileName);
    while (!queue.isEmpty()) {
        QString name = queue.dequeue();
        processed.insert(name);
        PFileIncludes p=mPreprocessor.includesList().value(name);
        if (!p)
          continue;
        foreach (const QString& s,p->dependedFiles) {
            if (!processed.contains(s)) {
                queue.enqueue(s);
            }
        }
    }
    return processed;
}

int CppParser::calcKeyLenForStruct(const QString &word)
{
    if (word.startsWith("struct"))
        return 6;
    else if (word.startsWith("class")
             || word.startsWith("union"))
        return 5;
    return -1;
}

void CppParser::scanMethodArgs(const PStatement& functionStatement, const QString &argStr)
{
    // Split up argument string by ,
    int i = 1; // assume it starts with ( and ends with )
    int paramStart = i;

    QString args;
    while (i < argStr.length()) {
        if ((argStr[i] == ',') ||
                ((i == argStr.length()-1) && (argStr[i] == ')'))) {
            // We've found "int* a" for example
            QString s = argStr.mid(paramStart,i-paramStart);

            //remove default value
            int assignPos = s.indexOf('=');
            if (assignPos >= 0) {
                s.truncate(assignPos);
                s = s.trimmed();
            }
            // we don't support function pointer parameters now, till we can tokenize function parameters
//        {
//        // Can be a function pointer. If so, scan after last )
//        BracePos := LastPos(')', S);
//        if (BracePos > 0) then // it's a function pointer... begin
//          SpacePos := LastPos(' ', Copy(S, BracePos, MaxInt)) // start search at brace
//        end else begin
//        }
            int spacePos = s.lastIndexOf(' '); // Cut up at last space
            if (spacePos >= 0) {
                args = "";
                int bracketPos = s.indexOf('[');
                if (bracketPos >= 0) {
                    args = s.mid(bracketPos);
                    s.truncate(bracketPos);
                }
                addStatement(
                            functionStatement,
                            mCurrentFile,
                            "", // do not override hint
                            s.mid(0,spacePos), // 'int*'
                            s.mid(spacePos+1), // a
                            args,
                            "",
                            functionStatement->definitionLine,
                            StatementKind::skParameter,
                            StatementScope::ssLocal,
                            StatementClassScope::scsNone,
                            true,
                            false);
            }
            paramStart = i + 1; // step over ,
        }
        i++;
    }
}

QString CppParser::splitPhrase(const QString &phrase, QString &sClazz, QString &sMember, QString &sOperator)
{
    sClazz="";
    sMember="";
    sOperator="";
    QString result="";
    int bracketLevel = 0;
    // Obtain stuff before first operator
    int firstOpStart = phrase.length() + 1;
    int firstOpEnd = phrase.length() + 1;
    for (int i = 0; i<phrase.length();i++) {
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
    for (int i = firstOpEnd; i<phrase.length();i++) {
        if ((i+1<phrase.length()) && (phrase[i] == '-') && (phrase[i + 1] == '>') && (bracketLevel=0)) {
            secondOp = i;
            break;
        } else if ((i+1<phrase.length()) && (phrase[i] == ':') && (phrase[i + 1] == ':') && (bracketLevel=0)) {
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

QString CppParser::removeArgNames(const QString &args)
{
    QString result = "";
    int argsLen = args.length();
    if (argsLen < 2)
        return "";
    int i=1;   // skip start '('
    QString currentArg;
    QString word;
    int brackLevel = 0;
    bool typeGetted = false;
    while (i<argsLen-1) { //skip end ')'
        switch(args[i].unicode()) {
        case ',':
            if (brackLevel >0) {
                word+=args[i];
            } else {
                if (!typeGetted) {
                    currentArg += ' ' + word;
                } else {
                    if (isKeyword(word)) {
                        currentArg += ' ' + word;
                    }
                }
                word = "";
                result += currentArg.trimmed() + ',';
                currentArg = "";
                typeGetted = false;
            }
            break;
        case '<':
        case '[':
        case '(':
            brackLevel++;
            word+=args[i];
            break;
        case '>':
        case ']':
        case ')':
            brackLevel--;
            word+=args[i];
            break;
        case ' ':
        case '\t':
            if ((brackLevel >0) && !isSpaceChar(args[i-1])) {
                word+=args[i];
            } else if (!word.trimmed().isEmpty()) {
                if (!typeGetted) {
                    currentArg += ' ' + word;
                    if (mCppTypeKeywords.contains(word) || !isKeyword(word))
                        typeGetted = true;
                } else {
                    if (isKeyword(word))
                        currentArg += ' ' + word;
                }
                word = "";
            }
            break;
        default:
            if (isWordChar(args[i]) || isDigitChar(args[i]))
                word+=args[i];
        }
        i++;
    }
    if (!typeGetted) {
        currentArg += ' ' + word;
    } else {
        if (isKeyword(word)) {
            currentArg += ' ' + word;
        }
    }
    result += currentArg.trimmed();
    return result;
}

bool CppParser::isSpaceChar(const QChar &ch)
{
    return ch==' ' || ch =='\t';
}

bool CppParser::isWordChar(const QChar &ch)
{
//    return (ch>= 'A' && ch<='Z')
//            || (ch>='a' && ch<='z')
    return ch.isLetter()
            || ch == '_'
            || ch == '*'
            || ch == '&';
}

bool CppParser::isLetterChar(const QChar &ch)
{
//    return (ch>= 'A' && ch<='Z')
//            || (ch>='a' && ch<='z')
    return ch.isLetter()
            || ch == '_';
}

bool CppParser::isDigitChar(const QChar &ch)
{
    return (ch>='0' && ch<='9');
}

bool CppParser::isSeperator(const QChar &ch)  {
    switch(ch.unicode()){
    case '(':
    case ';':
    case ':':
    case '{':
    case '}':
    case '#':
        return true;
    default:
        return false;
    }
}

bool CppParser::isblockChar(const QChar &ch)
{
    switch(ch.unicode()){
    case ';':
    case '{':
    case '}':
        return true;
    default:
        return false;
    }
}

bool CppParser::isInvalidVarPrefixChar(const QChar &ch)
{
    switch (ch.unicode()) {
    case '#':
    case ',':
    case ';':
    case ':':
    case '{':
    case '}':
    case '!':
    case '/':
    case '+':
    case '-':
    case '<':
    case '>':
        return true;
    default:
        return false;
    }
}

bool CppParser::isBraceChar(const QChar &ch)
{
    return ch == '{' || ch =='}';
}

bool CppParser::isLineChar(const QChar &ch)
{
    return ch=='\n' || ch=='\r';
}

bool CppParser::isNotFuncArgs(const QString &args)
{
    int i=1; //skip '('
    int endPos = args.length()-1;//skip ')'
    bool lastCharIsId=false;
    QString word = "";
    while (i<endPos) {
        if (args[i] == '"' || args[i]=='\'') {
            // args contains a string/char, can't be a func define
            return true;
        } else if ( isLetterChar(args[i])) {
            word += args[i];
            lastCharIsId = true;
            i++;
        } else if ((args[i] == ':') && (args[i+1] == ':')) {
            lastCharIsId = false;
            word += "::";
            i+=2;
        } else if (isDigitChar(args[i])) {
            if (!lastCharIsId)
                return true;
            word+=args[i];
            i++;
        } else if (isSpaceChar(args[i]) || isLineChar(args[i])) {
            if (!word.isEmpty())
                break;
            i++;
        } else if (word.isEmpty()) {
            return true;
        } else
            break;
    }
    //function with no args
    if (i>endPos && word.isEmpty()) {
        return false;
    }

    if (isKeyword(word)) {
        return word == "true" || word == "false" || word == "nullptr";
    }
    PStatement statement =findStatementOf(mCurrentFile,word,getCurrentScope(),true);
    if (statement &&
            !isTypeStatement(statement->kind))
        return true;
    return false;
}

bool CppParser::isNamedScope(StatementKind kind)
{
    switch(kind) {
    case StatementKind::skClass:
    case StatementKind::skNamespace:
    case StatementKind::skFunction:
        return true;
    default:
        return false;
    }
}

bool CppParser::isTypeStatement(StatementKind kind)
{
    switch(kind) {
    case StatementKind::skClass:
    case StatementKind::skTypedef:
    case StatementKind::skEnumClassType:
    case StatementKind::skEnumType:
        return true;
    default:
        return false;
    }
}

void CppParser::updateSerialId()
{
    mSerialId = QString("%1 %2").arg(mParserId).arg(mSerialCount);
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
    mOnGetFileStream = newOnGetFileStream;
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
    mEnabled = newEnabled;
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

}

void CppFileParserThread::run()
{
    if (mParser && !mParser->parsing()) {
        mParser->parseFile(mFileName,mInProject,mOnlyIfNotParsed,mUpdateView);
    }
}

CppFileListParserThread::CppFileListParserThread(PCppParser parser,
                                                 bool updateView, QObject *parent):
    QThread(parent),
    mParser(parser),
    mUpdateView(updateView)
{

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
    CppFileListParserThread *thread = new CppFileListParserThread(parser,updateView);
    thread->connect(thread,
                    &QThread::finished,
                    thread,
                    &QThread::deleteLater);
    thread->start();
}
