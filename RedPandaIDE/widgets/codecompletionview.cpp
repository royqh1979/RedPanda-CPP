#include "codecompletionview.h"
#include "../utils.h"

#include <QKeyEvent>
#include <QVBoxLayout>

CodeCompletionView::CodeCompletionView(QWidget *parent) :
    QWidget(parent)
{
    setWindowFlags(Qt::Popup);
    mListView = new CodeCompletionListView(this);
    mModel=new CodeCompletionListModel(&mCompletionStatementList);
    mListView->setModel(mModel);
    setLayout(new QVBoxLayout());
    layout()->addWidget(mListView);
    layout()->setMargin(0);

    mShowKeywords=true;
    mUseCppKeyword=true;

    mOnlyGlobals = false;
    mShowCount = 1000;
    mShowCodeIns = true;

    mIgnoreCase = false;

}

CodeCompletionView::~CodeCompletionView()
{
    delete mListView;
    delete mModel;
}

void CodeCompletionView::setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback)
{
    mListView->setKeypressedCallback(newKeypressedCallback);
}

void CodeCompletionView::prepareSearch(const QString &phrase, const QString &filename, int line)
{
    QMutexLocker locker(&mMutex);
    if (!isEnabled())
        return;
    mPhrase = phrase;
    //Screen.Cursor := crHourglass;
    QCursor oldCursor = cursor();
    setCursor(Qt::CursorShape::WaitCursor);

    mIncludedFiles = mParser->getFileIncludes(filename);
    getCompletionFor(filename,phrase,line);

    //todo: notify model
//CodeComplForm.lbCompletion.Font.Size := FontSize;
//CodeComplForm.lbCompletion.ItemHeight := CodeComplForm.lbCompletion.Canvas.TextHeight('F')+6;
// Round(2 * FontSize);
//CodeComplForm.Update;
    setCursor(oldCursor);
}

bool CodeCompletionView::search(const QString &phrase, bool autoHideOnSingleResult)
{
    QMutexLocker locker(&mMutex);

    mPhrase = phrase;

    if (phrase.isEmpty()) {
        hide();
        return false;
    }
    if (!isEnabled()) {
        hide();
        return false;
    }

    QCursor oldCursor = cursor();
    setCursor(Qt::CursorShape::WaitCursor);

    // Sort here by member
    int i = mParser->findLastOperator(phrase);
    while ((i>=0) && (i<phrase.length()) && (
               phrase[i] == '.'
               || phrase[i] == ':'
               || phrase[i] == '-'
               || phrase[i] == '>'))
        i++;

    QString symbol = phrase.mid(i);
    // filter fFullCompletionStatementList to fCompletionStatementList
    filterList(symbol);
    mModel->notifyUpdated();
    setCursor(oldCursor);

    if (!mCompletionStatementList.isEmpty()) {
        // if only one suggestion, and is exactly the symbol to search, hide the frame (the search is over)
        // if only one suggestion and auto hide , don't show the frame
        if(mCompletionStatementList.count() == 1)
            if (autoHideOnSingleResult
                    || (symbol == mCompletionStatementList.front()->command)) {
            return true;
        }
    } else {
        hide();
    }
    return false;
}

PStatement CodeCompletionView::selectedStatement()
{
    if (isEnabled()) {
        int index = mListView->currentIndex().row();
        if (mListView->currentIndex().isValid()
                && (index<mCompletionStatementList.count()) ) {
            return mCompletionStatementList[index];
        } else {
            if (!mCompletionStatementList.isEmpty())
                return mCompletionStatementList.front();
            else
                return PStatement();
        }
    } else
        return PStatement();
}

void CodeCompletionView::addChildren(PStatement scopeStatement, const QString &fileName, int line)
{
    if (scopeStatement && !isIncluded(scopeStatement->fileName)
      && !isIncluded(scopeStatement->definitionFileName))
        return;
    const StatementMap& children = mParser->statementList().childrenStatements(scopeStatement);
    if (children.isEmpty())
        return;

    if (!scopeStatement) { //Global scope
        for (PStatement childStatement: children) {
            if (childStatement->fileName.isEmpty()) {
                // hard defines
                addStatement(childStatement,fileName,-1);
            } else if (!( childStatement->kind == StatementKind::skConstructor
                          || childStatement->kind == StatementKind::skDestructor
                          || childStatement->kind == StatementKind::skBlock)
                       && (!mAddedStatements.contains(childStatement->command))
                       && (
                           isIncluded(childStatement->fileName)
                           || isIncluded(childStatement->definitionFileName)
                           )
                       ) {
                //we must check if the statement is included by the file
                addStatement(childStatement,fileName,line);
            }
        }
    } else {
        for (PStatement childStatement: children) {
            if (!( childStatement->kind == StatementKind::skConstructor
                                      || childStatement->kind == StatementKind::skDestructor
                                      || childStatement->kind == StatementKind::skBlock)
                                   && (!mAddedStatements.contains(childStatement->command)))
                addStatement(childStatement,fileName,line);
        }
    }
}

void CodeCompletionView::addStatement(PStatement statement, const QString &fileName, int line)
{
    if (mAddedStatements.contains(statement->command))
        return;
    if ((line!=-1)
            && (line < statement->line)
            && (fileName == statement->fileName))
        return;
    mAddedStatements.insert(statement->command);
    mFullCompletionStatementList.append(statement);
}

static bool defaultComparator(PStatement statement1,PStatement statement2) {
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeIn) {
        if (statement2->kind != StatementKind::skUserCodeIn)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeIn) {
        return false;
        // show keywords first
    } else if ((statement1->kind == StatementKind::skKeyword)
               && (statement2->kind != StatementKind::skKeyword)) {
        return true;
    } else if ((statement1->kind != StatementKind::skKeyword)
               && (statement2->kind == StatementKind::skKeyword)) {
        return false;
    } else
        return statement1->command < statement2->command;
}

static bool sortByScopeComparator(PStatement statement1,PStatement statement2){
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeIn) {
        if (statement2->kind != StatementKind::skUserCodeIn)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeIn) {
        return false;
        // show keywords first
    } else if (statement1->kind == StatementKind::skKeyword) {
        if (statement2->kind != StatementKind::skKeyword)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skKeyword) {
        return false;
        // Show stuff from local headers first
    } else if (statement1->inSystemHeader && ! statement2->inSystemHeader) {
        return true;
    } else if (!statement1->inSystemHeader && statement2->inSystemHeader) {
        return false;
        // Show local statements first
    } else if (statement1->scope != StatementScope::ssGlobal
               && statement1->scope == StatementScope::ssGlobal ) {
        return true;
    } else if (statement1->scope == StatementScope::ssGlobal
               && statement1->scope != StatementScope::ssGlobal ) {
        return false;
        // otherwise, sort by name
    } else
        return statement1->command < statement2->command;
}

static bool sortWithUsageComparator(PStatement statement1,PStatement statement2) {
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeIn) {
        if (statement2->kind != StatementKind::skUserCodeIn)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeIn) {
        return false;
        //show most freq first
    } else if (statement1->freqTop > statement2->freqTop) {
        return true;
    } else if (statement1->freqTop < statement2->freqTop) {
        return false;
        // show keywords first
    } else if ((statement1->kind == StatementKind::skKeyword)
               && (statement2->kind != StatementKind::skKeyword)) {
        return true;
    } else if ((statement1->kind != StatementKind::skKeyword)
               && (statement2->kind == StatementKind::skKeyword)) {
        return false;
    } else
        return statement1->command < statement2->command;
}

static bool sortByScopeWithUsageComparator(PStatement statement1,PStatement statement2){
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeIn) {
        if (statement2->kind != StatementKind::skUserCodeIn)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeIn) {
        return false;
        //show most freq first
    } else if (statement1->freqTop > statement2->freqTop) {
        return true;
    } else if (statement1->freqTop < statement2->freqTop) {
        return false;
        // show keywords first
    } else if (statement1->kind == StatementKind::skKeyword) {
        if (statement2->kind != StatementKind::skKeyword)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skKeyword) {
        return false;
        // Show stuff from local headers first
    } else if (statement1->inSystemHeader && ! statement2->inSystemHeader) {
        return true;
    } else if (!statement1->inSystemHeader && statement2->inSystemHeader) {
        return false;
        // Show local statements first
    } else if (statement1->scope != StatementScope::ssGlobal
               && statement1->scope == StatementScope::ssGlobal ) {
        return true;
    } else if (statement1->scope == StatementScope::ssGlobal
               && statement1->scope != StatementScope::ssGlobal ) {
        return false;
        // otherwise, sort by name
    } else
        return statement1->command < statement2->command;
}


void CodeCompletionView::filterList(const QString &member)
{
    QMutexLocker locker(&mMutex);
    mCompletionStatementList.clear();
    if (!mParser)
        return;
    if (!mParser->enabled())
        return;
    //we don't need to freeze here since we use smart pointers
    //  and data have been retrieved from the parser
//    if (!mParser->freeze())
//        return;
//    {
//        auto action = finally([this]{
//            mParser->unFreeze();
//        });
//        if (mParserSerialId!=mParser->serialId()) {
//            return;
//        }

    mCompletionStatementList.clear();
    if (!member.isEmpty()) { // filter
        mCompletionStatementList.reserve(mFullCompletionStatementList.size());
        for (PStatement statement:mFullCompletionStatementList) {
            Qt::CaseSensitivity cs = (mIgnoreCase?
                                          Qt::CaseInsensitive:
                                          Qt::CaseSensitive);
            if (statement->command.startsWith(member, cs))
                mCompletionStatementList.append(statement);
        }
    } else
        mCompletionStatementList.append(mFullCompletionStatementList);
    if (mRecordUsage) {
        int topCount = 0;
        int secondCount = 0;
        int thirdCount = 0;
        int usageCount;
        for (PStatement statement:mCompletionStatementList) {
            if (statement->usageCount == 0) {
                usageCount = mSymbolUsage.value(statement->fullName,0);
                if (usageCount == 0)
                    continue;
                statement->usageCount = usageCount;
            } else
                usageCount = statement->usageCount;
            if (usageCount>topCount) {
                thirdCount = secondCount;
                secondCount = topCount;
                topCount = usageCount;
            } else if (usageCount == topCount) {
                continue;
            } else if (usageCount > secondCount) {
                thirdCount = secondCount;
                secondCount = usageCount;
            } else if (usageCount == secondCount) {
                continue;
            } else if (usageCount>thirdCount) {
                thirdCount = usageCount;
            }
        }
        for (PStatement statement:mCompletionStatementList) {
            if (statement->usageCount == 0) {
                statement->freqTop = 0;
            } else if  (statement->usageCount == topCount) {
                statement->freqTop = 30;
            } else if  (statement->usageCount == secondCount) {
                statement->freqTop = 20;
            } else if  (statement->usageCount == thirdCount) {
                statement->freqTop = 10;
            }
        }
        if (mSortByScope) {
            std::sort(mCompletionStatementList.begin(),
                      mCompletionStatementList.end(),
                      sortByScopeWithUsageComparator);
        } else {
            std::sort(mCompletionStatementList.begin(),
                      mCompletionStatementList.end(),
                      sortWithUsageComparator);
        }
    } else if (mSortByScope) {
        std::sort(mCompletionStatementList.begin(),
                  mCompletionStatementList.end(),
                  sortByScopeComparator);
    } else {
        std::sort(mCompletionStatementList.begin(),
                  mCompletionStatementList.end(),
                  defaultComparator);
    }
    //    }
}

void CodeCompletionView::getCompletionFor(const QString &fileName, const QString &phrase, int line)
{
    if(!mParser)
        return;
    if (!mParser->enabled())
        return;

    if (!mParser->freeze())
        return;
    {
        auto action = finally([this]{
            mParser->unFreeze();
        });

        //C++ preprocessor directives
        if (phrase.startsWith('#')) {
            if (mShowKeywords) {
                for (QString keyword:CppDirectives) {
                    PStatement statement = std::make_shared<Statement>();
                    statement->command = keyword;
                    statement->kind = StatementKind::skPreprocessor;
                    statement->fullName = keyword;
                    mFullCompletionStatementList.append(statement);
                }
            }
            return;
        }

        //docstring tags (javadoc style)
        if (phrase.startsWith('@')) {
            if (mShowKeywords) {
                for (QString keyword:JavadocTags) {
                    PStatement statement = std::make_shared<Statement>();
                    statement->command = keyword;
                    statement->kind = StatementKind::skKeyword;
                    statement->fullName = keyword;
                    mFullCompletionStatementList.append(statement);
                }
            }
            return;
        }


        // Pulling off the same trick as in TCppParser.FindStatementOf, but ignore everything after last operator
        int i = mParser->findLastOperator(phrase);
        if (i < 0 ) { // don't have any scope prefix

            if (mShowCodeIns) {
                //add custom code templates
                for (PCodeIns codeIn:mCodeInsList) {
                    PStatement statement = std::make_shared<Statement>();
                    statement->command = codeIn->prefix;
                    statement->kind = StatementKind::skUserCodeIn;
                    statement->fullName = codeIn->prefix;
                    mFullCompletionStatementList.append(statement);
                }
            }

            if (mShowKeywords) {
                //add keywords
                if (mUseCppKeyword) {
                    for (QString keyword:CppKeywords.keys()) {
                        PStatement statement = std::make_shared<Statement>();
                        statement->command = keyword;
                        statement->kind = StatementKind::skKeyword;
                        statement->fullName = keyword;
                        mFullCompletionStatementList.append(statement);
                    }
                } else {
                    for (QString keyword:CKeywords) {
                        PStatement statement = std::make_shared<Statement>();
                        statement->command = keyword;
                        statement->kind = StatementKind::skKeyword;
                        statement->fullName = keyword;
                        mFullCompletionStatementList.append(statement);
                    }
                }
            }

            PStatement scopeStatement = mCurrentStatement;
            // repeat until reach global
            while (scopeStatement) {
                //add members of current scope that not added before
                if (scopeStatement->kind == StatementKind::skClass) {
                    addChildren(scopeStatement, fileName, -1);
                } else {
                    addChildren(scopeStatement, fileName, line);
                }

                // add members of all usings (in current scope ) and not added before
                for (QString namespaceName:scopeStatement->usingList) {
                    PStatementList namespaceStatementsList =
                            mParser->findNamespace(namespaceName);
                    if (!namespaceStatementsList)
                        continue;
                    for (PStatement namespaceStatement:*namespaceStatementsList) {
                        addChildren(namespaceStatement, fileName, line);
                    }
                }
                scopeStatement=scopeStatement->parentScope.lock();
            }

            // add all global members and not added before
            addChildren(nullptr, fileName, line);

            // add members of all fusings
            mUsings = mParser->getFileUsings(fileName);
            for (QString namespaceName:mUsings) {
                PStatementList namespaceStatementsList =
                        mParser->findNamespace(namespaceName);
                if (!namespaceStatementsList)
                    continue;
                for (PStatement namespaceStatement:*namespaceStatementsList) {
                    addChildren(namespaceStatement, fileName, line);
                }
            }
        } else { //we are in some statement's scope
            MemberOperatorType opType=getOperatorType(phrase,i);
            QString scopeName = phrase.mid(0,i);
            if (opType == MemberOperatorType::otDColon) {
                if (scopeName.isEmpty()) {
                    // start with '::', we only find in global
                    // add all global members and not added before
                    addChildren(nullptr, fileName, line);
                    return;
                } else {
                    //assume the scope its a namespace
                    PStatementList namespaceStatementsList =
                            mParser->findNamespace(scopeName);
                    if (namespaceStatementsList) {
                        for (PStatement namespaceStatement:*namespaceStatementsList) {
                            addChildren(namespaceStatement, fileName, line);
                        }
                        return;
                    }
                    //namespace not found let's go on
                }
            }
            PStatement parentTypeStatement;
            PStatement statement = mParser->findStatementOf(
                        fileName, scopeName,mCurrentStatement,parentTypeStatement);
            if (!statement)
                return;
            // find the most inner scope statement that has a name (not a block)
            PStatement scopeTypeStatement = mCurrentStatement;
            while (scopeTypeStatement && !isScopeTypeKind(scopeTypeStatement->kind)) {
                scopeTypeStatement = scopeTypeStatement->parentScope.lock();
            }
            if (
                    (opType == MemberOperatorType::otArrow
                     ||  opType == MemberOperatorType::otDot)
                    && (
                        statement->kind == StatementKind::skVariable
                        || statement->kind == StatementKind::skParameter
                        || statement->kind == StatementKind::skFunction)
                    ) {
                // Get type statement  of current (scope) statement
                PStatement classTypeStatement;
                PStatement parentScope = statement->parentScope.lock();
                if ((statement->kind == StatementKind::skFunction)
                        && parentScope
                        && STLContainers.contains(parentScope->fullName)
                        && STLElementMethods.contains(statement->command)){
                    // it's an element method of STL container
                    // we must find the type in the template parameter

                    // get the function's owner variable's definition
                    int lastI = mParser->findLastOperator(scopeName);
                    QString lastScopeName = scopeName.mid(0,lastI);
                    PStatement lastScopeStatement =
                            mParser->findStatementOf(
                                fileName, lastScopeName,
                                mCurrentStatement,parentTypeStatement);
                    if (!lastScopeStatement)
                        return;


                    QString typeName =
                            mParser->findFirstTemplateParamOf(
                                fileName,lastScopeStatement->type,
                                lastScopeStatement->parentScope.lock());
                    classTypeStatement = mParser->findTypeDefinitionOf(
                                fileName, typeName,
                                lastScopeStatement->parentScope.lock());
                } else
                    classTypeStatement=mParser->findTypeDefinitionOf(
                                fileName, statement->type,parentTypeStatement);

                if (!classTypeStatement)
                    return;
                //is a smart pointer
                if (STLPointers.contains(classTypeStatement->fullName)
                   && (opType == MemberOperatorType::otArrow)) {
                    QString typeName= mParser->findFirstTemplateParamOf(
                                fileName,
                                statement->type,
                                parentScope);
                    classTypeStatement = mParser->findTypeDefinitionOf(
                                fileName,
                                typeName,
                                parentScope);
                    if (!classTypeStatement)
                        return;
                }
                //is a stl container operator[]
                if (STLContainers.contains(classTypeStatement->fullName)
                        && scopeName.endsWith(']')) {
                    QString typeName= mParser->findFirstTemplateParamOf(
                                fileName,
                                statement->type,
                                parentScope);
                    classTypeStatement = mParser->findTypeDefinitionOf(
                                fileName,
                                typeName,
                                parentScope);
                    if (!classTypeStatement)
                        return;
                }
                if (!isIncluded(classTypeStatement->fileName) &&
                    !isIncluded(classTypeStatement->definitionFileName))
                    return;
                if ((classTypeStatement == scopeTypeStatement) || (statement->command == "this")) {
                    //we can use all members
                    addChildren(classTypeStatement,fileName,-1);
                } else { // we can only use public members
                    const StatementMap& children = mParser->statementList().childrenStatements(classTypeStatement);
                    if (children.isEmpty())
                        return;
                    for (PStatement childStatement:children) {
                        if ((childStatement->classScope==StatementClassScope::scsPublic)
                                && !(
                                    childStatement->kind == StatementKind::skConstructor
                                    || childStatement->kind == StatementKind::skDestructor)
                                && !mAddedStatements.contains(childStatement->command)) {
                            addStatement(childStatement,fileName,-1);
                        }
                    }
                }
            //todo friend
            } else if ((opType == MemberOperatorType::otDColon)
                       && (statement->kind == StatementKind::skEnumType)
                       && (statement->kind == StatementKind::skEnumClassType)) {
                //we can add all child enum definess
                PStatement classTypeStatement = statement;
                if (!isIncluded(classTypeStatement->fileName) &&
                    !isIncluded(classTypeStatement->definitionFileName))
                    return;
                const StatementMap& children =
                        mParser->statementList().childrenStatements(classTypeStatement);
                for (PStatement child:children) {
                    addStatement(child,fileName,line);
                }
            } else if ((opType == MemberOperatorType::otDColon)
                       && (statement->kind == StatementKind::skClass)) {
                PStatement classTypeStatement = statement;
                if (!isIncluded(classTypeStatement->fileName) &&
                    !isIncluded(classTypeStatement->definitionFileName))
                    return;
                if (classTypeStatement == scopeTypeStatement) {
                    //we can use all static members
                    const StatementMap& children =
                            mParser->statementList().childrenStatements(classTypeStatement);
                    for (PStatement childStatement: children) {
                        if (
                          (childStatement->isStatic)
                           || (childStatement->kind == StatementKind::skTypedef
                            || childStatement->kind == StatementKind::skClass
                            || childStatement->kind == StatementKind::skEnum
                            || childStatement->kind == StatementKind::skEnumClassType
                            || childStatement->kind == StatementKind::skEnumType
                               )) {
                            addStatement(childStatement,fileName,-1);
                        }
                    }
                } else {
                    // we can only use public static members
                    const StatementMap& children =
                            mParser->statementList().childrenStatements(classTypeStatement);
                    for (PStatement childStatement: children) {
                        if (
                          (childStatement->isStatic)
                           || (childStatement->kind == StatementKind::skTypedef
                            || childStatement->kind == StatementKind::skClass
                            || childStatement->kind == StatementKind::skEnum
                            || childStatement->kind == StatementKind::skEnumClassType
                            || childStatement->kind == StatementKind::skEnumType
                               )) {
                            if (childStatement->classScope == StatementClassScope::scsPublic)
                                addStatement(childStatement,fileName,-1);
                        }
                    }
                }
              //todo friend
            }
        }
    }
}

bool CodeCompletionView::isIncluded(const QString &fileName)
{
    return mIncludedFiles.contains(fileName);
}

void CodeCompletionView::showEvent(QShowEvent *)
{
    mListView->setFocus();
}

const PStatement &CodeCompletionView::currentStatement() const
{
    return mCurrentStatement;
}

void CodeCompletionView::setCurrentStatement(const PStatement &newCurrentStatement)
{
    mCurrentStatement = newCurrentStatement;
}

bool CodeCompletionView::useCppKeyword() const
{
    return mUseCppKeyword;
}

void CodeCompletionView::setUseCppKeyword(bool newUseCppKeyword)
{
    mUseCppKeyword = newUseCppKeyword;
}

bool CodeCompletionView::sortByScope() const
{
    return mSortByScope;
}

void CodeCompletionView::setSortByScope(bool newSortByScope)
{
    mSortByScope = newSortByScope;
}

bool CodeCompletionView::ignoreCase() const
{
    return mIgnoreCase;
}

void CodeCompletionView::setIgnoreCase(bool newIgnoreCase)
{
    mIgnoreCase = newIgnoreCase;
}

bool CodeCompletionView::showCodeIns() const
{
    return mShowCodeIns;
}

void CodeCompletionView::setShowCodeIns(bool newShowCodeIns)
{
    mShowCodeIns = newShowCodeIns;
}

bool CodeCompletionView::showKeywords() const
{
    return mShowKeywords;
}

void CodeCompletionView::setShowKeywords(bool newShowKeywords)
{
    mShowKeywords = newShowKeywords;
}

bool CodeCompletionView::recordUsage() const
{
    return mRecordUsage;
}

void CodeCompletionView::setRecordUsage(bool newRecordUsage)
{
    mRecordUsage = newRecordUsage;
}

bool CodeCompletionView::onlyGlobals() const
{
    return mOnlyGlobals;
}

void CodeCompletionView::setOnlyGlobals(bool newOnlyGlobals)
{
    mOnlyGlobals = newOnlyGlobals;
}

int CodeCompletionView::showCount() const
{
    return mShowCount;
}

void CodeCompletionView::setShowCount(int newShowCount)
{
    mShowCount = newShowCount;
}

const PCppParser &CodeCompletionView::parser() const
{
    return mParser;
}

void CodeCompletionView::setParser(const PCppParser &newParser)
{
    mParser = newParser;
}

void CodeCompletionView::hideEvent(QHideEvent *event)
{
    QMutexLocker locker(&mMutex);
    mListView->setKeypressedCallback(nullptr);
    mCompletionStatementList.clear();
    mFullCompletionStatementList.clear();
    mIncludedFiles.clear();
    mUsings.clear();
    mAddedStatements.clear();
    QWidget::hideEvent(event);
}

CodeCompletionListView::CodeCompletionListView(QWidget *parent) : QListView(parent)
{

}

void CodeCompletionListView::keyPressEvent(QKeyEvent *event)
{
    if (!mKeypressedCallback || !mKeypressedCallback(event)) {
        QListView::keyPressEvent(event);
    }
}

const KeyPressedCallback &CodeCompletionListView::keypressedCallback() const
{
    return mKeypressedCallback;
}

void CodeCompletionListView::setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback)
{
    mKeypressedCallback = newKeypressedCallback;
}

CodeCompletionListModel::CodeCompletionListModel(StatementList *statements, QObject *parent):
    QAbstractListModel(parent),
    mStatements(statements)
{

}

int CodeCompletionListModel::rowCount(const QModelIndex &) const
{
    return mStatements->count();
}

QVariant CodeCompletionListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()>=mStatements->count())
        return QVariant();

    if (role == Qt::DisplayRole) {
        PStatement statement = mStatements->at(index.row());
        return statement->command;
    }
    return QVariant();
}

void CodeCompletionListModel::notifyUpdated()
{
    beginResetModel();
    endResetModel();
}
