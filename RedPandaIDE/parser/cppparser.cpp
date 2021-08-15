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
    return  mIndex < mTokenizer.tokenCount() &&
            mTokenizer[mIndex]->text == "catch";
}

bool CppParser::checkForEnum()
{
    return  mIndex < mTokenizer.tokenCount() &&
            mTokenizer[mIndex]->text == "enum";
}

bool CppParser::checkForForBlock()
{
    return  mIndex < mTokenizer.tokenCount() &&
            mTokenizer[mIndex]->text == "for";
}

bool CppParser::checkForKeyword()
{
    SkipType st = CppKeywords.value(mTokenizer[mIndex]->text,SkipType::skNone);
    return st!=SkipType::skNone;
}

bool CppParser::checkForMethod(QString &sType, QString &sName, QString &sArgs, bool &isStatic, bool &isFriend)
{

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
            if (isLetterChar(args[i]))
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
