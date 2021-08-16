#include "cppparser.h"
#include "parserutils.h"

#include <QHash>
#include <QQueue>

CppParser::CppParser(QObject *parent) : QObject(parent)
{

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
      inherit->inheritanceList,
      inherit->isStatic);
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
                    QList<std::weak_ptr<Statement>>(),
                isStatic);
}

PStatement CppParser::addStatement(PStatement parent, const QString &fileName, const QString &hintText, const QString &aType, const QString &command, const QString &args, const QString &value, int line, StatementKind kind, StatementScope scope, StatementClassScope classScope, bool isDefinition, const QList<std::weak_ptr<Statement> > &inheritanceList, bool isStatic)
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
                    fileIncludes1->statements.insert(oldStatement->command,
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
    result->inheritanceList = inheritanceList;
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
    return result;
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
                bTypeOK = isInCurrentScopeLevel(sType); // constructor/destructor
            }
            break;
        } else {
            //if IsValidIdentifier(fTokenizer[fIndex]^.Text) then
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

bool CppParser::CheckForStructs()
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

QString CppParser::expandMacroType(const QString &name)
{
    //its done in the preprocessor
    return name;
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

bool CppParser::isLineChar(const QChar &ch)
{
    return ch=='\n' || ch=='\r';
}

bool CppParser::isNotFuncArgs(const QString &args)
{
    bool result = true;
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
            !(statement->kind == StatementKind::skClass
              || statement->kind == StatementKind::skTypedef
              || statement->kind == StatementKind::skEnum
              || statement->kind == StatementKind::skEnumType))
        return true;
    return false;
}
