#include "cppparser.h"
#include "parserutils.h"
#include "../utils.h"

#include <QApplication>
#include <QHash>
#include <QQueue>
#include <QThread>

static QAtomicInt cppParserCount(0);
CppParser::CppParser(QObject *parent) : QObject(parent)
{
    mParserId = cppParserCount.fetchAndAddRelaxed(1);
    mSerialCount = 0;
    updateSerialId();
    mUniqId = 0;
    mParsing = false;
    //mStatementList ; // owns the objects
    mIncludesList = std::make_shared<QHash<QString,PFileIncludes>>();
    //mFilesToScan;
    mScannedFiles = std::make_shared<QSet<QString>>();
    //mIncludePaths;
    //mProjectIncludePaths;
    //mProjectFiles;
    // mCurrentScope;
    //mCurrentClassScope;
    //mSkipList;
    mParseLocalHeaders = true;
    mParseGlobalHeaders = true;
    mLockCount = 0;
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
    mPreprocessor.includePaths().insert(value);
}

void CppParser::addProjectIncludePath(const QString &value)
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.projectIncludePaths().insert(value);
}

void CppParser::clearIncludePaths()
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.includePaths().clear();
}

void CppParser::clearProjectIncludePaths()
{
    QMutexLocker  locker(&mMutex);
    mPreprocessor.projectIncludePaths().clear();
}

void CppParser::clearProjectFiles()
{
    QMutexLocker  locker(&mMutex);
    mProjectFiles.clear();
}

void CppParser::fillListOfFunctions(const QString &fileName, const QString &phrase, int line, QStringList &list)
{
    QMutexLocker locker(&mMutex);
    list.clear();
    PStatement statement = findStatementOf(fileName,phrase, line);
    if (!statement)
        return;
    PStatement parentScope = statement->parentScope.lock();
    if (parentScope && parentScope->kind == StatementKind::skNamespace) {
        PStatementList namespaceStatementsList = findNamespace(parentScope->command);
        if (namespaceStatementsList) {
            for (PStatement namespaceStatement  : *namespaceStatementsList) {
                fillListOfFunctions(fileName,line,statement,namespaceStatement,list);
            }
        }
    } else
        fillListOfFunctions(fileName,line,statement,parentScope,list);
}

void CppParser::addFileToScan(QString value, bool inProject)
{
    QMutexLocker locker(&mMutex);
    //value.replace('/','\\'); // only accept full file names

    // Update project listing
    if (inProject)
        mProjectFiles.insert(value);

    // Only parse given file
    if (!mScannedFiles->contains(value)) {
        mFilesToScan.insert(value);
    }

}

PStatement CppParser::addInheritedStatement(PStatement derived, PStatement inherit, StatementClassScope access)
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

PStatement CppParser::addChildStatement(PStatement parent, const QString &fileName, const QString &hintText, const QString &aType, const QString &command, const QString &args, const QString &value, int line, StatementKind kind, StatementScope scope, StatementClassScope classScope, bool isDefinition, bool isStatic)
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

PStatement CppParser::addStatement(PStatement parent, const QString &fileName, const QString &hintText, const QString &aType, const QString &command, const QString &args, const QString &value, int line, StatementKind kind, StatementScope scope, StatementClassScope classScope, bool isDefinition, bool isStatic)
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
                PFileIncludes fileIncludes1=findFileIncludes(fileName);
                if (fileIncludes1) {
                    fileIncludes1->statements.insert(oldStatement->fullName,
                                                     oldStatement);
                    fileIncludes1->dependingFiles.insert(oldStatement->fileName);
                    PFileIncludes fileIncludes2=findFileIncludes(oldStatement->fileName);
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
    result->usageCount = 0;
    result->freqTop = 0;
    if (result->kind == StatementKind::skNamespace) {
        PStatementList namespaceList = mNamespaces.value(result->fullName,PStatementList());
        if (!namespaceList) {
            namespaceList=std::make_shared<StatementList>();
            mNamespaces.insert(result->fullName,namespaceList);
        }
        namespaceList->append(result);
    }

    if (result->kind!= StatementKind::skBlock) {
        PFileIncludes fileIncludes = findFileIncludes(fileName);
        if (fileIncludes) {
            fileIncludes->statements.insert(result->fullName,result);
            fileIncludes->declaredStatements.insert(result->fullName,result);
        }
    }
    return result;
}

void CppParser::setInheritance(int index, PStatement classStatement, bool isStruct)
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

void CppParser::addSoloScopeLevel(PStatement statement, int line)
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

    PFileIncludes fileIncludes = findFileIncludes(mCurrentFile);

    if (fileIncludes) {
        fileIncludes->scopes.insert(line,statement);
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
    PFileIncludes fileIncludes = findFileIncludes(mCurrentFile);
    if (currentScope && (currentScope->kind == StatementKind::skBlock)) {
        if (currentScope->children.isEmpty()) {
            // remove no children block
            if (fileIncludes && !fileIncludes->scopes.isEmpty()) {
                fileIncludes->scopes.remove(currentScope->line);
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
    if (fileIncludes && !fileIncludes->scopes.isEmpty()
            && fileIncludes->scopes.value(fileIncludes->scopes.keys().back())!=currentScope) {
        fileIncludes->scopes.insert(line,currentScope);
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
    SkipType st = CppKeywords.value(mTokenizer[mIndex]->text,SkipType::skNone);
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
    return ("#" == mTokenizer[mIndex]->text);
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

void CppParser::getFullNameSpace(const QString &phrase, QString &sNamespace, QString &member)
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

QString CppParser::getFullStatementName(const QString &command, PStatement parent)
{
    PStatement scopeStatement=parent;
    while (scopeStatement && !isNamedScope(scopeStatement->kind))
        scopeStatement = scopeStatement->parentScope.lock();
    if (scopeStatement)
        return scopeStatement->fullName + "::" + command;
    else
        return command;
}

PStatement CppParser::getIncompleteClass(const QString &command, PStatement parentScope)
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
    if (mTokenizer[mIndex]->text.startsWith('{')) { // enum {...} NAME
        // Skip to the closing brace
        int i = skipBraces(mIndex);
        if ((i + 1 < mTokenizer.tokenCount()) && mTokenizer[i]->text == "class") {
            //enum class {...} NAME
            isEnumClass = true;
            i++;
        }
        // Have we found the name?
        if ((i + 1 < mTokenizer.tokenCount()) && !mTokenizer[i]->text.startsWith('}')
            && !mTokenizer[i + 1]->text.startsWith(';'))
            enumName = mTokenizer[i + 1]->text.trimmed();
    } else { // enum NAME {...};
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
                if (!isEnumClass) {
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
    SkipType skipType = CppKeywords.value(mTokenizer[mIndex]->text,SkipType::skNone);
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
        while (mIndex < mTokenizer.tokenCount() && !mTokenizer[mIndex]->text.startsWith(')'))
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
    while ((mIndex < mTokenizer.tokenCount()) and ! (
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
        PStatement functionClass;
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
            scanMethodArgs(functionStatement, functionStatement->args);
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
                        getCurrentScope(),
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
        if ((mIndex< mTokenizer.tokenCount()) && !mTokenizer[mIndex]->text.startsWith(';'))
            mIndex++;
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
            if ((mIndex>= mTokenizer.tokenCount()) || (mTokenizer[mIndex]->text[1] == ';'))
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
            int line = s.mid(delimPos+1).toInt(&ok);
            if (line == 1) {
                mFilesScannedCount++;
                mFilesToScanCount++;
                onProgress(mCurrentFile,mFilesToScanCount,mFilesScannedCount);
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
    } else if (mTokenizer[mIndex]->text[1] == '}') {
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
                        if ((mTokenizer[i]->text.front() == '_')
                            && (mTokenizer[i]->text.back() == '_')) {
                            // skip possible gcc attributes
                            // start and end with 2 underscores (i.e. __attribute__)
                            // so, to avoid slow checks of strings, we just check the first and last letter of the token
                            // if both are underscores, we split
                            break;
                        } else {
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
                        }
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
        PFileIncludes fileInfo = findFileIncludes(mCurrentFile);
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
                        || isblockChar(';')
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
                  not isExtern,
                  isStatic); // TODO: not supported to pass list
                varAdded = true;
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

    QStringList tempStream;
    if (mOnGetFileStream) {
        mOnGetFileStream(fileName,tempStream);
    }

    // Preprocess the file...
    {
        auto action = finally([this]{
            mPreprocessor.reset();
            mTokenizer.reset();
        });
        // Let the preprocessor augment the include records
//        mPreprocessor.setIncludesList(mIncludesList);
//        mPreprocessor.setIncludePaths(mIncludePaths);
//        mPreprocessor.setProjectIncludePaths(mProjectIncludePaths);
//        mPreprocessor.setScannedFileList(mScannedFiles);
        mPreprocessor.setScanOptions(mParseGlobalHeaders, mParseLocalHeaders);
        mPreprocessor.preprocess(fileName, tempStream);

//    with TStringList.Create do try
//      Text:=fPreprocessor.Result;
//      SaveToFile('f:\\Preprocess.txt');
//    finally
//      Free;
//    end;
        //fPreprocessor.DumpIncludesListTo('f:\\includes.txt');
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
        //mTokenizer.DumpTokens('f:\tokens.txt');
        //Statements.DumpTo('f:\stats.txt');
        //Statements.DumpWithScope('f:\\statements.txt');
        //fPreprocessor.DumpDefinesTo('f:\defines.txt');
        //fPreprocessor.DumpIncludesListTo('f:\\includes.txt');
    }
}

void CppParser::inheritClassStatement(PStatement derived, bool isStruct, PStatement base, StatementClassScope access)
{
    PFileIncludes fileIncludes1=findFileIncludes(derived->fileName);
    PFileIncludes fileIncludes2=findFileIncludes(base->fileName);
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
    for (PStatement statement : base->children) {
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

void CppParser::fillListOfFunctions(const QString& fileName, int line,PStatement statement, PStatement scopeStatement, QStringList &list)
{
    StatementMap children = mStatementList.childrenStatements(scopeStatement);
    for (PStatement child:children) {
        if ((statement->command == child->command)
#ifdef Q_OS_WIN
                || (statement->command +'A' == child->command)
                || (statement->command +'W' == child->command)
#endif
                ) {
            if (line < child->line && (child->fileName == fileName))
                continue;
            list.append(prettyPrintStatement(child));
        }
    }
}

PStatement CppParser::findMemberOfStatement(const QString &phrase, PStatement scopeStatement)
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

PStatement CppParser::findStatementInScope(const QString &name, const QString &noNameArgs, StatementKind kind, PStatement scope)
{
    if (scope && scope->kind == StatementKind::skNamespace) {
        PStatementList namespaceStatementsList = findNamespace(scope->command);
        if (!namespaceStatementsList)
            return PStatement();
        for (PStatement namespaceStatement: *namespaceStatementsList) {
            PStatement result=doFindStatementInScope(name,noNameArgs,kind,namespaceStatement);
            if (result)
                return result;
        }
    } else {
        return doFindStatementInScope(name,noNameArgs,kind,scope);
    }
    return PStatement();
}

PStatement CppParser::doFindStatementInScope(const QString &name, const QString &noNameArgs, StatementKind kind, PStatement scope)
{
    const StatementMap& statementMap =mStatementList.childrenStatements(scope);
    if (statementMap.isEmpty())
        return PStatement();

    QList<PStatement> statementList = statementMap.values(name);

    for (PStatement statement: statementList) {
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
    QList<QString> keys=mNamespaces.keys();
    for (QString key:keys) {
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
    mScannedFiles->remove(fileName);

    // remove its include files list
    PFileIncludes p = findFileIncludes(fileName, true);
    if (p) {
        //fPreprocessor.InvalidDefinesInFile(FileName); //we don't need this, since we reset defines after each parse
        //p->includeFiles.clear();
        //p->usings.clear();
        for (PStatement statement:p->statements) {
            if ((statement->kind == StatementKind::skFunction
                 || statement->kind == StatementKind::skConstructor
                 || statement->kind == StatementKind::skDestructor
                 || statement->kind == StatementKind::skVariable)
                    && (fileName != statement->fileName)) {
                statement->hasDefinition = false;
            }
        }

        for (PStatement statement:p->declaredStatements) {
            mStatementList.deleteStatement(statement);
        }

        //p->declaredStatements.clear();
        //p->statements.clear();
        //p->scopes.clear();
        //p->dependedFiles.clear();
        //p->dependingFiles.clear();
    }
}

void CppParser::internalInvalidateFiles(const QStringList &files)
{
    for (QString file:files)
        internalInvalidateFile(file);
}

void CppParser::calculateFilesToBeReparsed(const QString &fileName, QStringList &files)
{
    if (fileName.isEmpty())
        return;
    files.clear();
    QQueue<QString> queue;
    QSet<QString> processed;
    queue.enqueue(fileName);
    while (!queue.isEmpty()) {
        QString name = queue.dequeue();
        files.append(name);
        processed.insert(name);
        PFileIncludes p=findFileIncludes(name);
        if (!p)
          continue;
        for (QString s:p->dependedFiles) {
            if (!processed.contains(s)) {
                queue.enqueue(s);
            }
        }
    }
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

void CppParser::scanMethodArgs(PStatement functionStatement, const QString &argStr)
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
        if ((i+1<phrase.length()) && (phrase[i] == '-') && (phrase[i + 1] == '>') && (bracketLevel=0)) {
            firstOpStart = i;
            firstOpEnd = i+2;
            sOperator = "->";
            break;
        } else if ((i+1<phrase.length()) && (phrase[i] == ':') && (phrase[i + 1] == ':') && (bracketLevel=0)) {
            firstOpStart = i;
            firstOpEnd = i+2;
            sOperator = "::";
            break;
        } else if ((phrase[i] == '.') && (bracketLevel=0)) {
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
                    if (CppTypeKeywords.contains(word) || !isKeyword(word))
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
    return (ch>= 'A' && ch<='Z')
            || (ch>='a' && ch<='z')
            || ch == '_'
            || ch == '*'
            || ch == '&';
}

bool CppParser::isLetterChar(const QChar &ch)
{
    return (ch>= 'A' && ch<='Z')
            || (ch>='a' && ch<='z')
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
